#include "httplib.h"
#include "json.hpp"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <random>
#include <sstream>
#include <sqlite3.h>
#include <string>
#include <thread>
#include <vector>

// POSIX 头文件：用于元数据/特殊文件支持（属主/权限/时间/管道/链接/设备）
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>
#include <fcntl.h>
#include <sys/sysmacros.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct Task
{
    int id;
    int userId;
    std::string username;
    std::string type;
    std::string source;
    std::string destination;
    std::string status;
    std::string message;
    std::string createdAt;
};

struct User
{
    int id = 0;
    std::string username;
};

struct BackupFilter
{
    std::vector<std::string> includePaths;
    std::vector<std::string> excludePaths;
    std::vector<std::string> extensions;
    std::string fileNameContains;
    std::uintmax_t minSize = 0;
    std::uintmax_t maxSize = std::numeric_limits<std::uintmax_t>::max();
    fs::file_time_type modifiedAfter = fs::file_time_type::min();
    fs::file_time_type modifiedBefore = fs::file_time_type::max();
    int userId = 0;
    std::string username;
    // 扩展：用户筛选（按文件属主 uid/gid）
    int filterUid = -1;          // -1 表示不筛选
    int filterGid = -1;          // -1 表示不筛选
    std::string filterOwnerName; // 按属主名筛选
    std::string filterGroupName; // 按属组名筛选
    // 扩展：元数据/特殊文件
    bool preserveMetadata = true;     // 保留属主/权限/时间
    bool includeSpecialFiles = true;  // 备份符号链接/管道/设备等特殊文件
    // 扩展：归档与加密算法选择
    std::string archiveType = "zip";  // none | zip | tar | tar.gz
    std::string encryptAlgo = "aes-256-cbc";
    // 扩展：增量备份
    bool incremental = false;
    std::string incrementalBase; // 上一次完整备份的目录路径
};

// 文件元数据：属主/属组/权限/时间/类型/链接目标
struct FileMetadata
{
    mode_t mode = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    time_t mtime = 0;
    time_t atime = 0;
    std::string ownerName;
    std::string groupName;
    std::string symlinkTarget;
    bool isSymlink = false;
    bool isPipe = false;
    bool isBlockDevice = false;
    bool isCharDevice = false;
    bool isSocket = false;
    dev_t rdev = 0; // 设备文件的主从设备号
};

// 定时备份调度配置（含数据淘汰策略）
struct ScheduleConfig
{
    int id = 0;
    fs::path source;
    fs::path destination;
    int intervalSeconds = 3600;     // 周期
    int maxBackups = 0;             // 最多保留的备份数（0 表示不限）
    int maxAgeDays = 0;             // 最多保留天数（0 表示不限）
    bool compress = false;
    bool encrypt = false;
    std::string password;
    BackupFilter filter;
    User user;
};

std::mutex g_mutex;
std::vector<Task> g_tasks;
std::atomic<int> g_nextId{1};
std::atomic<bool> g_watching{false};
std::thread g_watchThread;

std::mutex g_tokenMutex;
std::map<std::string, User> g_tokens;

// 定时备份调度器全局状态
std::mutex g_scheduleMutex;
std::map<int, ScheduleConfig> g_schedules;
std::map<int, std::thread> g_scheduleThreads;
std::atomic<bool> g_scheduleRunning{false};
std::atomic<int> g_nextScheduleId{1};

std::string nowString()
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

std::string bytesToHex(const unsigned char *data, std::size_t size)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < size; ++i)
    {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }

    return oss.str();
}

std::string sha256Hex(const std::string &value)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(value.data()), value.size(), hash);
    return bytesToHex(hash, SHA256_DIGEST_LENGTH);
}

std::string randomHex(std::size_t bytes)
{
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);

    std::vector<unsigned char> data(bytes);
    for (auto &ch : data)
    {
        ch = static_cast<unsigned char>(dist(rd));
    }

    return bytesToHex(data.data(), data.size());
}

std::string hashPassword(const std::string &password, const std::string &salt)
{
    return sha256Hex(salt + ":" + password);
}

std::string getBearerToken(const httplib::Request &req)
{
    auto auth = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";

    if (auth.rfind(prefix, 0) == 0)
    {
        return auth.substr(prefix.size());
    }

    return "";
}

std::string shellQuote(const std::string &value)
{
#ifdef _WIN32
    std::string result = "\"";
    for (char ch : value)
    {
        if (ch == '"')
        {
            result += "\\\"";
        }
        else
        {
            result += ch;
        }
    }
    result += "\"";
    return result;
#else
    std::string result = "'";
    for (char ch : value)
    {
        if (ch == '\'')
        {
            result += "'\\''";
        }
        else
        {
            result += ch;
        }
    }
    result += "'";
    return result;
#endif
}

bool runCommand(const std::string &command)
{
    int result = std::system(command.c_str());
    return result == 0;
}

bool parseTimeString(const std::string &value, std::chrono::system_clock::time_point &timePoint)
{
    if (value.empty())
    {
        return false;
    }

    std::tm tm{};
    std::istringstream iss(value);

    if (value.find('-') != std::string::npos)
    {
        iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    }
    else
    {
        iss >> std::get_time(&tm, "%Y%m%d_%H%M%S");
    }

    if (iss.fail())
    {
        return false;
    }

    std::time_t t = std::mktime(&tm);
    if (t == -1)
    {
        return false;
    }

    timePoint = std::chrono::system_clock::from_time_t(t);
    return true;
}

fs::file_time_type systemTimeToFileTime(std::chrono::system_clock::time_point tp)
{
    auto nowFile = fs::file_time_type::clock::now();
    auto nowSystem = std::chrono::system_clock::now();
    return nowFile + std::chrono::duration_cast<fs::file_time_type::duration>(tp - nowSystem);
}

BackupFilter parseBackupFilter(const json &body, const User &user)
{
    BackupFilter filter;
    filter.userId = user.id;
    filter.username = user.username;

    if (!body.contains("filter") || body["filter"].is_null())
    {
        // 仍可从顶层读取归档/加密/增量配置
        filter.archiveType = body.value("archiveType", "zip");
        filter.encryptAlgo = body.value("encryptAlgo", "aes-256-cbc");
        filter.preserveMetadata = body.value("preserveMetadata", true);
        filter.includeSpecialFiles = body.value("includeSpecialFiles", true);
        filter.incremental = body.value("incremental", false);
        filter.incrementalBase = body.value("incrementalBase", "");
        return filter;
    }

    const auto &f = body["filter"];

    if (f.contains("includePaths") && f["includePaths"].is_array())
    {
        filter.includePaths = f["includePaths"].get<std::vector<std::string>>();
    }

    if (f.contains("excludePaths") && f["excludePaths"].is_array())
    {
        filter.excludePaths = f["excludePaths"].get<std::vector<std::string>>();
    }

    if (f.contains("extensions") && f["extensions"].is_array())
    {
        filter.extensions = f["extensions"].get<std::vector<std::string>>();
        for (auto &ext : filter.extensions)
        {
            if (!ext.empty() && ext[0] != '.')
            {
                ext = "." + ext;
            }
        }
    }

    filter.fileNameContains = f.value("fileNameContains", f.value("name", ""));

    if (f.contains("minSize"))
    {
        filter.minSize = f["minSize"].get<std::uintmax_t>();
    }

    if (f.contains("maxSize"))
    {
        filter.maxSize = f["maxSize"].get<std::uintmax_t>();
    }

    std::chrono::system_clock::time_point tp;
    if (parseTimeString(f.value("modifiedAfter", ""), tp))
    {
        filter.modifiedAfter = systemTimeToFileTime(tp);
    }

    if (parseTimeString(f.value("modifiedBefore", ""), tp))
    {
        filter.modifiedBefore = systemTimeToFileTime(tp);
    }

    // 扩展：用户筛选（属主 uid/gid/名称）
    filter.filterUid = f.value("uid", -1);
    filter.filterGid = f.value("gid", -1);
    filter.filterOwnerName = f.value("owner", "");
    filter.filterGroupName = f.value("group", "");

    // 扩展：元数据与特殊文件开关
    filter.preserveMetadata = f.value("preserveMetadata", true);
    filter.includeSpecialFiles = f.value("includeSpecialFiles", true);

    // 扩展：归档/加密算法（也可从顶层读取，便于旧前端使用）
    filter.archiveType = body.value("archiveType", f.value("archiveType", "zip"));
    filter.encryptAlgo = body.value("encryptAlgo", f.value("encryptAlgo", "aes-256-cbc"));

    // 扩展：增量备份
    filter.incremental = body.value("incremental", f.value("incremental", false));
    filter.incrementalBase = body.value("incrementalBase", f.value("incrementalBase", ""));

    return filter;
}

bool containsAnyPathPart(const std::string &path, const std::vector<std::string> &patterns)
{
    if (patterns.empty())
    {
        return true;
    }

    for (const auto &pattern : patterns)
    {
        if (!pattern.empty() && path.find(pattern) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool containsExcludedPathPart(const std::string &path, const std::vector<std::string> &patterns)
{
    for (const auto &pattern : patterns)
    {
        if (!pattern.empty() && path.find(pattern) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}

bool extensionMatches(const fs::path &path, const std::vector<std::string> &extensions)
{
    if (extensions.empty())
    {
        return true;
    }

    auto ext = path.extension().string();
    for (const auto &allowed : extensions)
    {
        if (ext == allowed)
        {
            return true;
        }
    }

    return false;
}

// ===== 元数据采集/恢复 =====

// 通过 lstat 获取文件元数据（不跟随符号链接）
bool getFileMetadata(const fs::path &path, FileMetadata &meta)
{
    struct stat st{};
    if (lstat(path.c_str(), &st) != 0)
    {
        return false;
    }

    meta.mode = st.st_mode;
    meta.uid = st.st_uid;
    meta.gid = st.st_gid;
    meta.mtime = st.st_mtime;
    meta.atime = st.st_atime;
    meta.rdev = st.st_rdev;

    if (S_ISLNK(st.st_mode))
    {
        meta.isSymlink = true;
        std::error_code ec;
        meta.symlinkTarget = fs::read_symlink(path, ec).string();
    }
    else if (S_ISFIFO(st.st_mode))
    {
        meta.isPipe = true;
    }
    else if (S_ISBLK(st.st_mode))
    {
        meta.isBlockDevice = true;
    }
    else if (S_ISCHR(st.st_mode))
    {
        meta.isCharDevice = true;
    }
    else if (S_ISSOCK(st.st_mode))
    {
        meta.isSocket = true;
    }

    if (auto *pw = getpwuid(st.st_uid))
    {
        meta.ownerName = pw->pw_name;
    }
    if (auto *gr = getgrgid(st.st_gid))
    {
        meta.groupName = gr->gr_name;
    }

    return true;
}

// 将元数据应用到目标文件（属主/属组/权限/时间）
void applyFileMetadata(const fs::path &path, const FileMetadata &meta)
{
    chmod(path.c_str(), meta.mode);

    // 仅 root 或具有 CAP_CHOWN 才能成功改变属主；普通用户调用失败可忽略
    chown(path.c_str(), meta.uid, meta.gid);

    struct utimbuf times{};
    times.actime = meta.atime;
    times.modtime = meta.mtime;
    utime(path.c_str(), &times);
}

// 复制特殊文件：符号链接/管道/设备文件
bool copySpecialFile(const fs::path &src, const fs::path &dst, const FileMetadata &meta, std::string &reason)
{
    (void)src; // 源路径不直接使用，元数据中已携带链接目标等信息
    fs::create_directories(dst.parent_path());

    if (meta.isSymlink)
    {
        std::error_code ec;
        fs::remove(dst, ec);
        fs::create_symlink(meta.symlinkTarget, dst, ec);
        if (ec)
        {
            reason = "创建符号链接失败: " + ec.message();
            return false;
        }
        return true;
    }

    if (meta.isPipe)
    {
        if (mkfifo(dst.c_str(), meta.mode) != 0)
        {
            reason = "创建管道失败";
            return false;
        }
        return true;
    }

    if (meta.isBlockDevice || meta.isCharDevice)
    {
        if (mknod(dst.c_str(), meta.mode, meta.rdev) != 0)
        {
            reason = "创建设备文件失败（需要 root 权限）";
            return false;
        }
        return true;
    }

    if (meta.isSocket)
    {
        // UNIX 域套接字无法通过常规方式重建，跳过
        reason = "套接字无法备份";
        return false;
    }

    reason = "未知特殊文件类型";
    return false;
}

// 计算文件内容的 SHA-256（用于增量备份对比）
std::string fileContentHash(const fs::path &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return "";
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        return "";
    }

    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    char buffer[65536];
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
    {
        EVP_DigestUpdate(ctx, buffer, static_cast<size_t>(in.gcount()));
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    return bytesToHex(hash, hashLen);
}

bool fileMatchesFilter(const fs::directory_entry &entry, const fs::path &source, const BackupFilter &filter)
{
    fs::path relativePath = fs::relative(entry.path(), source);
    std::string relative = relativePath.string();

    // 用户筛选：通过 lstat 获取属主/属组
    if (filter.filterUid != -1 || filter.filterGid != -1 ||
        !filter.filterOwnerName.empty() || !filter.filterGroupName.empty())
    {
        FileMetadata meta;
        if (!getFileMetadata(entry.path(), meta))
        {
            return false;
        }
        if (filter.filterUid != -1 && static_cast<int>(meta.uid) != filter.filterUid)
        {
            return false;
        }
        if (filter.filterGid != -1 && static_cast<int>(meta.gid) != filter.filterGid)
        {
            return false;
        }
        if (!filter.filterOwnerName.empty() && meta.ownerName != filter.filterOwnerName)
        {
            return false;
        }
        if (!filter.filterGroupName.empty() && meta.groupName != filter.filterGroupName)
        {
            return false;
        }
    }

    bool isRegular = entry.is_regular_file();
    bool isSymlink = entry.is_symlink();

    if (!isRegular && !isSymlink)
    {
        // 非常规文件：根据 includeSpecialFiles 决定是否纳入
        if (!filter.includeSpecialFiles)
        {
            return false;
        }
        // 特殊文件不参与尺寸/扩展名/时间筛选（这些只对常规文件有意义）
        if (!containsAnyPathPart(relative, filter.includePaths))
        {
            return false;
        }
        if (containsExcludedPathPart(relative, filter.excludePaths))
        {
            return false;
        }
        if (!filter.fileNameContains.empty() &&
            entry.path().filename().string().find(filter.fileNameContains) == std::string::npos)
        {
            return false;
        }
        return true;
    }

    if (!containsAnyPathPart(relative, filter.includePaths))
    {
        return false;
    }

    if (containsExcludedPathPart(relative, filter.excludePaths))
    {
        return false;
    }

    if (!extensionMatches(entry.path(), filter.extensions))
    {
        return false;
    }

    if (!filter.fileNameContains.empty() &&
        entry.path().filename().string().find(filter.fileNameContains) == std::string::npos)
    {
        return false;
    }

    if (isRegular)
    {
        auto size = entry.file_size();
        if (size < filter.minSize || size > filter.maxSize)
        {
            return false;
        }

        auto modified = fs::last_write_time(entry.path());
        if (modified < filter.modifiedAfter || modified > filter.modifiedBefore)
        {
            return false;
        }
    }

    return true;
}

int addTask(
    const std::string &type,
    const std::string &source,
    const std::string &destination,
    const User &user = {})
{
    std::lock_guard<std::mutex> lock(g_mutex);

    int id = g_nextId++;
    g_tasks.push_back({id,
                       user.id,
                       user.username,
                       type,
                       source,
                       destination,
                       "running",
                       "",
                       nowString()});

    return id;
}

void updateTask(int id, const std::string &status, const std::string &message)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    for (auto &task : g_tasks)
    {
        if (task.id == id)
        {
            task.status = status;
            task.message = message;
            return;
        }
    }
}

json tasksToJson()
{
    std::lock_guard<std::mutex> lock(g_mutex);

    json arr = json::array();
    for (const auto &task : g_tasks)
    {
        arr.push_back({{"id", task.id},
                       {"userId", task.userId},
                       {"username", task.username},
                       {"type", task.type},
                       {"source", task.source},
                       {"destination", task.destination},
                       {"status", task.status},
                       {"message", task.message},
                       {"createdAt", task.createdAt}});
    }

    return arr;
}

json tasksToJsonForUser(const User &user)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    json arr = json::array();
    for (const auto &task : g_tasks)
    {
        if (user.id != 0 && task.userId != user.id)
        {
            continue;
        }

        arr.push_back({{"id", task.id},
                       {"userId", task.userId},
                       {"username", task.username},
                       {"type", task.type},
                       {"source", task.source},
                       {"destination", task.destination},
                       {"status", task.status},
                       {"message", task.message},
                       {"createdAt", task.createdAt}});
    }

    return arr;
}

std::uintmax_t directorySize(const fs::path &path)
{
    std::uintmax_t total = 0;

    if (!fs::exists(path))
    {
        return total;
    }

    for (const auto &entry : fs::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            total += entry.file_size();
        }
    }

    return total;
}

void writeMetadata(
    const fs::path &backupDir,
    const fs::path &source,
    const std::string &mode,
    const BackupFilter &filter,
    int copiedFiles,
    const std::string &archiveType = "",
    const std::string &encryptAlgo = "",
    bool incremental = false)
{
    json meta;
    meta["source"] = source.string();
    meta["backupTime"] = nowString();
    meta["mode"] = mode;
    meta["size"] = directorySize(backupDir);
    meta["copiedFiles"] = copiedFiles;
    meta["userId"] = filter.userId;
    meta["username"] = filter.username;
    meta["archiveType"] = archiveType.empty() ? filter.archiveType : archiveType;
    meta["encryptAlgo"] = encryptAlgo.empty() ? filter.encryptAlgo : encryptAlgo;
    meta["preserveMetadata"] = filter.preserveMetadata;
    meta["includeSpecialFiles"] = filter.includeSpecialFiles;
    meta["incremental"] = incremental;
    meta["filter"] = {
        {"includePaths", filter.includePaths},
        {"excludePaths", filter.excludePaths},
        {"extensions", filter.extensions},
        {"fileNameContains", filter.fileNameContains},
        {"minSize", filter.minSize},
        {"maxSize", filter.maxSize == std::numeric_limits<std::uintmax_t>::max() ? 0 : filter.maxSize},
        {"uid", filter.filterUid},
        {"gid", filter.filterGid},
        {"owner", filter.filterOwnerName},
        {"group", filter.filterGroupName}};

    std::ofstream out(backupDir / "metadata.json");
    out << meta.dump(4);
}

// 校验加密算法名称（白名单，避免命令注入）
std::string normalizeEncryptAlgo(const std::string &algo)
{
    static const std::vector<std::string> allowed = {
        "aes-256-cbc", "aes-128-cbc", "camellia-256-cbc", "camellia-128-cbc",
        "des-ede3-cbc", "chacha20"};
    for (const auto &a : allowed)
    {
        if (algo == a)
        {
            return a;
        }
    }
    return "aes-256-cbc";
}

// 校验归档类型（白名单）
std::string normalizeArchiveType(const std::string &type)
{
    if (type == "none" || type == "zip" || type == "tar" || type == "tar.gz" || type == "tgz")
    {
        return type == "tgz" ? "tar.gz" : type;
    }
    return "zip";
}

// 加密单个文件（任意 openssl 支持的对称算法）
fs::path encryptFile(const fs::path &src, const std::string &password, const std::string &algo)
{
    std::string cipher = normalizeEncryptAlgo(algo);
    fs::path encPath = src;
    encPath += ".enc";

    std::string cmd =
        "openssl enc -" + cipher + " -salt -pbkdf2 "
        "-in " +
        shellQuote(src.string()) +
        " -out " + shellQuote(encPath.string()) +
        " -pass pass:" + shellQuote(password);

    if (!runCommand(cmd))
    {
        throw std::runtime_error("加密失败（算法: " + cipher + "），请确认系统已安装 openssl");
    }

    fs::remove(src);
    return encPath;
}

// 解密单个文件（自动从备份元数据/约定中获取算法；若失败则尝试默认）
fs::path decryptFile(const fs::path &src, const std::string &password, const std::string &algo)
{
    std::string cipher = normalizeEncryptAlgo(algo);
    fs::path outPath = src;
    outPath = outPath.replace_extension("");

    fs::path tempDir = fs::temp_directory_path() / ("dec_" + nowString());
    fs::create_directories(tempDir);
    fs::path decryptedPath = tempDir / outPath.filename();

    std::string cmd =
        "openssl enc -d -" + cipher + " -pbkdf2 "
        "-in " +
        shellQuote(src.string()) +
        " -out " + shellQuote(decryptedPath.string()) +
        " -pass pass:" + shellQuote(password);

    if (!runCommand(cmd))
    {
        fs::remove_all(tempDir);
        throw std::runtime_error("解密失败，请检查密码或算法（" + cipher + "）");
    }

    return decryptedPath;
}

// 将 backupDir 打包为指定格式（zip/tar/tar.gz），返回归档文件路径
fs::path createArchive(const fs::path &backupDir, const std::string &archiveType)
{
    std::string type = normalizeArchiveType(archiveType);

    if (type == "none")
    {
        return backupDir; // 不打包，直接返回目录
    }

    if (type == "zip")
    {
        fs::path archivePath = backupDir;
        archivePath += ".zip";
        std::string cmd =
            "cd " + shellQuote(backupDir.parent_path().string()) +
            " && zip -ry " + shellQuote(archivePath.filename().string()) +
            " " + shellQuote(backupDir.filename().string());
        if (!runCommand(cmd))
        {
            throw std::runtime_error("打包失败（zip），请确认系统已安装 zip");
        }
        fs::remove_all(backupDir);
        return archivePath;
    }

    if (type == "tar")
    {
        fs::path archivePath = backupDir;
        archivePath += ".tar";
        std::string cmd =
            "cd " + shellQuote(backupDir.parent_path().string()) +
            " && tar -cf " + shellQuote(archivePath.filename().string()) +
            " " + shellQuote(backupDir.filename().string());
        if (!runCommand(cmd))
        {
            throw std::runtime_error("打包失败（tar），请确认系统已安装 tar");
        }
        fs::remove_all(backupDir);
        return archivePath;
    }

    // tar.gz
    fs::path archivePath = backupDir;
    archivePath += ".tar.gz";
    std::string cmd =
        "cd " + shellQuote(backupDir.parent_path().string()) +
        " && tar -czf " + shellQuote(archivePath.filename().string()) +
        " " + shellQuote(backupDir.filename().string());
    if (!runCommand(cmd))
    {
        throw std::runtime_error("压缩打包失败（tar.gz），请确认系统已安装 tar/gzip");
    }
    fs::remove_all(backupDir);
    return archivePath;
}

// 解包归档文件到指定目录（直接解压，特殊文件由 tar/unzip 原生重建）
// 返回归档内的 backup_xxx 目录路径（若存在）
fs::path extractArchive(const fs::path &archivePath, const fs::path &workDir)
{
    fs::create_directories(workDir);
    std::string name = archivePath.filename().string();

    if (archivePath.extension() == ".zip")
    {
        std::string cmd =
            "unzip -o " + shellQuote(archivePath.string()) +
            " -d " + shellQuote(workDir.string());
        if (!runCommand(cmd))
        {
            throw std::runtime_error("解压失败（zip），请确认系统已安装 unzip");
        }
    }
    else if (archivePath.extension() == ".gz")
    {
        // tar.gz
        std::string cmd =
            "tar -xzf " + shellQuote(archivePath.string()) +
            " -C " + shellQuote(workDir.string());
        if (!runCommand(cmd))
        {
            throw std::runtime_error("解压失败（tar.gz），请确认系统已安装 tar/gzip");
        }
    }
    else if (archivePath.extension() == ".tar")
    {
        std::string cmd =
            "tar -xf " + shellQuote(archivePath.string()) +
            " -C " + shellQuote(workDir.string());
        if (!runCommand(cmd))
        {
            throw std::runtime_error("解包失败（tar），请确认系统已安装 tar");
        }
    }
    else
    {
        throw std::runtime_error("不支持的归档格式: " + name);
    }

    // 归档内通常有一层 backup_xxx 目录，定位它
    for (const auto &entry : fs::directory_iterator(workDir))
    {
        if (fs::is_directory(entry.path()))
        {
            return entry.path();
        }
    }
    return workDir;
}

// 递归复制目录树，正确处理符号链接/管道/设备文件并保留元数据
void copyTreeWithSpecial(const fs::path &src, const fs::path &dst)
{
    fs::create_directories(dst);

    for (const auto &entry : fs::directory_iterator(src))
    {
        fs::path target = dst / entry.path().filename();

        FileMetadata meta;
        bool hasMeta = getFileMetadata(entry.path(), meta);

        if (fs::is_directory(entry.path()))
        {
            copyTreeWithSpecial(entry.path(), target);
        }
        else if (entry.is_symlink())
        {
            std::string reason;
            copySpecialFile(entry.path(), target, meta, reason);
        }
        else if (hasMeta && (meta.isPipe || meta.isBlockDevice || meta.isCharDevice))
        {
            std::string reason;
            if (!copySpecialFile(entry.path(), target, meta, reason))
            {
                std::cerr << "[警告] 还原时跳过特殊文件 " << entry.path() << ": " << reason << "\n";
            }
        }
        else if (entry.is_regular_file())
        {
            fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
        }

        if (hasMeta)
        {
            applyFileMetadata(target, meta);
        }
    }
}

// 读取上次备份的文件哈希表（用于增量备份）
std::map<std::string, std::string> loadIncrementalManifest(const fs::path &baseDir)
{
    std::map<std::string, std::string> hashes;
    if (baseDir.empty() || !fs::exists(baseDir))
    {
        return hashes;
    }

    fs::path manifestPath = baseDir / "manifest.json";
    if (!fs::exists(manifestPath))
    {
        return hashes;
    }

    std::ifstream in(manifestPath);
    if (!in)
    {
        return hashes;
    }

    try
    {
        json manifest = json::parse(in);
        if (manifest.contains("files") && manifest["files"].is_object())
        {
            for (auto it = manifest["files"].begin(); it != manifest["files"].end(); ++it)
            {
                hashes[it.key()] = it.value().get<std::string>();
            }
        }
    }
    catch (...)
    {
        // 解析失败则视为空 manifest
    }

    return hashes;
}

// 写入本次备份的文件哈希清单（供下一次增量使用）
void writeIncrementalManifest(const fs::path &backupDir, const std::map<std::string, std::string> &hashes)
{
    json manifest;
    manifest["backupTime"] = nowString();
    manifest["files"] = json::object();
    for (const auto &kv : hashes)
    {
        manifest["files"][kv.first] = kv.second;
    }

    std::ofstream out(backupDir / "manifest.json");
    out << manifest.dump(4);
}

fs::path createBackup(
    const fs::path &source,
    const fs::path &destination,
    bool compress,
    bool encrypt,
    const std::string &password,
    const BackupFilter &filter)
{
    if (!fs::exists(source))
    {
        throw std::runtime_error("源目录不存在");
    }

    if (!fs::is_directory(source))
    {
        throw std::runtime_error("源路径必须是目录");
    }

    fs::create_directories(destination);

    fs::path backupDir = destination / ("backup_" + nowString());
    fs::create_directories(backupDir);

    int copiedFiles = 0;
    std::map<std::string, std::string> newHashes;
    std::map<std::string, std::string> baseHashes;
    if (filter.incremental)
    {
        baseHashes = loadIncrementalManifest(filter.incrementalBase);
    }

    for (const auto &entry : fs::recursive_directory_iterator(source))
    {
        if (!fileMatchesFilter(entry, source, filter))
        {
            continue;
        }

        fs::path relativePath = fs::relative(entry.path(), source);
        fs::path targetPath = backupDir / relativePath;

        fs::create_directories(targetPath.parent_path());

        FileMetadata meta;
        bool hasMeta = getFileMetadata(entry.path(), meta);

        bool copied = false;
        if (entry.is_regular_file())
        {
            std::string relStr = relativePath.string();
            // 始终计算内容哈希，写入本次 manifest（任何备份都可作为后续增量的基线）
            std::string hash = fileContentHash(entry.path());
            if (!hash.empty())
            {
                newHashes[relStr] = hash;
            }

            // 增量备份：内容哈希未变则跳过实际拷贝
            if (filter.incremental)
            {
                auto it = baseHashes.find(relStr);
                if (it != baseHashes.end() && !hash.empty() && it->second == hash)
                {
                    // 内容未变，跳过；写入标记文件以便还原时识别
                    fs::path markPath = targetPath;
                    markPath += ".incskip";
                    std::ofstream mk(markPath);
                    mk << "unchanged";
                    continue;
                }
            }

            fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
            copied = true;
        }
        else if (entry.is_symlink() && filter.includeSpecialFiles)
        {
            std::string reason;
            copied = copySpecialFile(entry.path(), targetPath, meta, reason);
            if (!copied)
            {
                std::cerr << "[警告] 跳过特殊文件 " << entry.path() << ": " << reason << "\n";
            }
        }
        else if (hasMeta && filter.includeSpecialFiles &&
                 (meta.isPipe || meta.isBlockDevice || meta.isCharDevice))
        {
            std::string reason;
            copied = copySpecialFile(entry.path(), targetPath, meta, reason);
            if (!copied)
            {
                std::cerr << "[警告] 跳过特殊文件 " << entry.path() << ": " << reason << "\n";
            }
        }
        else if (fs::is_directory(entry.path()))
        {
            fs::create_directories(targetPath);
            copied = true;
        }

        // 元数据保留：对常规文件/目录/符号链接/管道/设备均尝试应用
        if (copied && filter.preserveMetadata && hasMeta)
        {
            applyFileMetadata(targetPath, meta);
        }

        if (copied)
        {
            copiedFiles++;
        }
    }

    // 始终写入 manifest，使任意备份都可作为后续增量备份的基线
    writeIncrementalManifest(backupDir, newHashes);

    std::string archiveType = filter.archiveType;
    // 兼容旧字段：compress=true 且 archiveType 默认时使用 zip
    if (compress && archiveType == "zip")
    {
        archiveType = "zip";
    }
    else if (!compress && archiveType == "zip" && !filter.incremental)
    {
        // 若显式未启用压缩但 archiveType 为默认 zip，则保持 zip（向后兼容）
        archiveType = "zip";
    }

    writeMetadata(backupDir, source, archiveType, filter, copiedFiles, archiveType, filter.encryptAlgo, filter.incremental);

    fs::path resultPath = backupDir;

    // 打包/压缩
    std::string type = normalizeArchiveType(archiveType);
    if (type != "none")
    {
        resultPath = createArchive(backupDir, type);
    }

    // 加密
    if (encrypt)
    {
        if (password.empty())
        {
            throw std::runtime_error("启用加密时密码不能为空");
        }

        if (fs::is_directory(resultPath))
        {
            // 目录无法直接加密，先打包再加密
            resultPath = createArchive(resultPath, "tar.gz");
        }

        resultPath = encryptFile(resultPath, password, filter.encryptAlgo);
    }

    return resultPath;
}

void restoreBackup(
    const fs::path &backupPath,
    const fs::path &destination,
    const std::string &password,
    const std::string &encryptAlgo = "aes-256-cbc")
{
    if (!fs::exists(backupPath))
    {
        throw std::runtime_error("备份文件或目录不存在");
    }

    fs::create_directories(destination);

    fs::path workPath = backupPath;
    fs::path tempPath;

    // 解密
    if (backupPath.extension() == ".enc")
    {
        if (password.empty())
        {
            throw std::runtime_error("还原加密备份时密码不能为空");
        }

        tempPath = fs::temp_directory_path() / ("restore_" + nowString());
        fs::create_directories(tempPath);
        fs::path decryptedPath = decryptFile(backupPath, password, encryptAlgo);
        fs::path movedPath = tempPath / decryptedPath.filename();
        fs::rename(decryptedPath, movedPath);
        fs::remove_all(decryptedPath.parent_path());
        workPath = movedPath;
    }

    // 判断是否为归档
    bool isZip = workPath.extension() == ".zip";
    bool isTar = workPath.extension() == ".tar";
    bool isTarGz = (workPath.extension() == ".gz" && workPath.stem().extension() == ".tar");

    if (isZip || isTar || isTarGz)
    {
        // 直接解压到目标目录，特殊文件由 tar/unzip 原生重建
        extractArchive(workPath, destination);
    }
    else if (fs::is_directory(workPath))
    {
        // 目录型备份：递归复制并处理特殊文件 + 元数据
        fs::path targetDir = destination / workPath.filename();
        copyTreeWithSpecial(workPath, targetDir);
    }
    else
    {
        fs::copy_file(
            workPath,
            destination / workPath.filename(),
            fs::copy_options::overwrite_existing);
    }

    if (!tempPath.empty())
    {
        fs::remove_all(tempPath);
    }
}

std::map<std::string, fs::file_time_type> snapshotFiles(const fs::path &root)
{
    std::map<std::string, fs::file_time_type> files;

    if (!fs::exists(root))
    {
        return files;
    }

    for (const auto &entry : fs::recursive_directory_iterator(root))
    {
        if (entry.is_regular_file())
        {
            files[entry.path().string()] = fs::last_write_time(entry.path());
        }
    }

    return files;
}

class UserManager
{
public:
    explicit UserManager(const std::string &dbPath)
    {
        if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK)
        {
            throw std::runtime_error("无法打开数据库");
        }

        exec("CREATE TABLE IF NOT EXISTS users ("
             "id INTEGER PRIMARY KEY AUTOINCREMENT,"
             "username TEXT NOT NULL UNIQUE,"
             "password_hash TEXT NOT NULL,"
             "password_salt TEXT NOT NULL,"
             "created_at TEXT NOT NULL"
             ");");
    }

    ~UserManager()
    {
        if (db_)
        {
            sqlite3_close(db_);
        }
    }

    User registerUser(const std::string &username, const std::string &password)
    {
        if (username.empty() || password.empty())
        {
            throw std::runtime_error("用户名和密码不能为空");
        }

        std::string salt = randomHex(16);
        std::string hash = hashPassword(password, salt);

        sqlite3_stmt *stmt = nullptr;
        const char *sql =
            "INSERT INTO users (username, password_hash, password_salt, created_at) "
            "VALUES (?, ?, ?, ?);";

        prepare(sql, &stmt);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_TRANSIENT);
        std::string createdAt = nowString();
        sqlite3_bind_text(stmt, 4, createdAt.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);
            throw std::runtime_error("注册失败，用户名可能已存在");
        }

        sqlite3_finalize(stmt);

        return authenticate(username, password);
    }

    User authenticate(const std::string &username, const std::string &password)
    {
        sqlite3_stmt *stmt = nullptr;
        const char *sql =
            "SELECT id, username, password_hash, password_salt "
            "FROM users WHERE username = ?;";

        prepare(sql, &stmt);
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_ROW)
        {
            sqlite3_finalize(stmt);
            throw std::runtime_error("用户名或密码错误");
        }

        int id = sqlite3_column_int(stmt, 0);
        std::string dbUsername = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string passwordHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        std::string salt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

        sqlite3_finalize(stmt);

        if (hashPassword(password, salt) != passwordHash)
        {
            throw std::runtime_error("用户名或密码错误");
        }

        return {id, dbUsername};
    }

private:
    sqlite3 *db_ = nullptr;

    void exec(const std::string &sql)
    {
        char *err = nullptr;
        if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
        {
            std::string message = err ? err : "数据库执行失败";
            sqlite3_free(err);
            throw std::runtime_error(message);
        }
    }

    void prepare(const char *sql, sqlite3_stmt **stmt)
    {
        if (sqlite3_prepare_v2(db_, sql, -1, stmt, nullptr) != SQLITE_OK)
        {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }
};

std::string createTokenForUser(const User &user)
{
    std::string token = randomHex(32);

    std::lock_guard<std::mutex> lock(g_tokenMutex);
    g_tokens[token] = user;

    return token;
}

bool getUserByToken(const std::string &token, User &user)
{
    if (token.empty())
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_tokenMutex);
    auto it = g_tokens.find(token);

    if (it == g_tokens.end())
    {
        return false;
    }

    user = it->second;
    return true;
}

User optionalUserFromRequest(const httplib::Request &req, const json *body = nullptr)
{
    std::string token = getBearerToken(req);

    if (token.empty() && body)
    {
        token = body->value("token", "");
    }

    User user;
    getUserByToken(token, user);
    return user;
}

void startWatcher(
    const fs::path &source,
    const fs::path &destination,
    int intervalSeconds,
    const BackupFilter &filter,
    const User &user)
{
    if (intervalSeconds <= 0)
    {
        throw std::runtime_error("监听间隔必须大于 0");
    }

    if (g_watching)
    {
        throw std::runtime_error("实时备份已经在运行");
    }

    g_watching = true;

    g_watchThread = std::thread([source, destination, intervalSeconds, filter, user]()
                                {
        auto previous = snapshotFiles(source);

        while (g_watching) {
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));

            auto current = snapshotFiles(source);
            if (current != previous) {
                int taskId = addTask("realtime-backup", source.string(), destination.string(), user);

                try {
                    fs::path path = createBackup(source, destination, true, false, "", filter);
                    updateTask(taskId, "success", "实时备份完成：" + path.string());
                    previous = current;
                } catch (const std::exception& e) {
                    updateTask(taskId, "failed", e.what());
                }
            }
        } });

    g_watchThread.detach();
}

// ===== 数据淘汰（retention）=====
// 列出某目录下所有 backup_* 备份，按时间排序
std::vector<fs::path> listBackups(const fs::path &destination)
{
    std::vector<fs::path> backups;
    if (!fs::exists(destination) || !fs::is_directory(destination))
    {
        return backups;
    }

    for (const auto &entry : fs::directory_iterator(destination))
    {
        std::string name = entry.path().filename().string();
        // 形如 backup_YYYYMMDD_HHMMSS / backup_YYYYMMDD_HHMMSS.zip / .tar.gz / .enc
        if (name.rfind("backup_", 0) == 0)
        {
            backups.push_back(entry.path());
        }
    }

    std::sort(backups.begin(), backups.end());
    return backups;
}

// 从备份名解析其时间戳（YYYYMMDD_HHMMSS）
std::time_t backupTimestamp(const fs::path &path)
{
    std::string name = path.filename().string();
    // 跳过 "backup_" 前缀
    size_t pos = name.find("backup_");
    if (pos == std::string::npos)
    {
        return 0;
    }
    std::string rest = name.substr(pos + 7); // 7 = strlen("backup_")
    // 取前 15 个字符 YYYYMMDD_HHMMSS
    if (rest.size() < 15)
    {
        return 0;
    }
    std::string ts = rest.substr(0, 15);

    std::tm tm{};
    std::istringstream iss(ts);
    iss >> std::get_time(&tm, "%Y%m%d_%H%M%S");
    if (iss.fail())
    {
        return 0;
    }
    return std::mktime(&tm);
}

// 按保留策略删除旧备份
//   maxBackups: 最多保留数量（0 表示不限）
//   maxAgeDays: 最多保留天数（0 表示不限）
int pruneBackups(const fs::path &destination, int maxBackups, int maxAgeDays)
{
    auto backups = listBackups(destination);
    int removed = 0;
    auto now = std::time(nullptr);

    // 按数量淘汰：保留最新的 maxBackups 个，删除其余
    if (maxBackups > 0 && static_cast<int>(backups.size()) > maxBackups)
    {
        int toRemove = static_cast<int>(backups.size()) - maxBackups;
        // backups 已按名称升序（即时间升序），从最旧的开始删
        for (int i = 0; i < toRemove; ++i)
        {
            std::error_code ec;
            fs::remove_all(backups[i], ec);
            if (!ec)
            {
                removed++;
            }
        }
        // 更新剩余列表
        backups.erase(backups.begin(), backups.begin() + toRemove);
    }

    // 按天数淘汰
    if (maxAgeDays > 0)
    {
        for (const auto &p : backups)
        {
            std::time_t ts = backupTimestamp(p);
            if (ts == 0)
            {
                continue;
            }
            double ageDays = std::difftime(now, ts) / 86400.0;
            if (ageDays > maxAgeDays)
            {
                std::error_code ec;
                fs::remove_all(p, ec);
                if (!ec)
                {
                    removed++;
                }
            }
        }
    }

    return removed;
}

// ===== 定时备份调度器 =====
int startSchedule(ScheduleConfig config)
{
    if (config.intervalSeconds <= 0)
    {
        throw std::runtime_error("定时间隔必须大于 0");
    }

    int id = g_nextScheduleId++;
    config.id = id;

    {
        std::lock_guard<std::mutex> lock(g_scheduleMutex);
        g_schedules[id] = config;
    }

    g_scheduleRunning = true;

    g_scheduleThreads[id] = std::thread([config]()
                                        {
        ScheduleConfig cfg = config;
        while (g_scheduleRunning) {
            // 等待间隔，每秒检查是否仍运行
            for (int i = 0; i < cfg.intervalSeconds && g_scheduleRunning; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            if (!g_scheduleRunning) {
                break;
            }

            // 检查调度是否仍存在（可能已被删除）
            {
                std::lock_guard<std::mutex> lock(g_scheduleMutex);
                if (g_schedules.find(cfg.id) == g_schedules.end()) {
                    break;
                }
            }

            int taskId = addTask("scheduled-backup", cfg.source.string(), cfg.destination.string(), cfg.user);
            try {
                fs::path path = createBackup(cfg.source, cfg.destination, cfg.compress, cfg.encrypt, cfg.password, cfg.filter);
                updateTask(taskId, "success", "定时备份完成：" + path.string());

                // 数据淘汰
                if (cfg.maxBackups > 0 || cfg.maxAgeDays > 0) {
                    int removed = pruneBackups(cfg.destination, cfg.maxBackups, cfg.maxAgeDays);
                    if (removed > 0) {
                        updateTask(taskId, "success", "定时备份完成：" + path.string() + "；淘汰旧备份 " + std::to_string(removed) + " 个");
                    }
                }
            } catch (const std::exception& e) {
                updateTask(taskId, "failed", e.what());
            }
        } });

    g_scheduleThreads[id].detach();

    return id;
}

bool stopSchedule(int id)
{
    std::lock_guard<std::mutex> lock(g_scheduleMutex);
    auto it = g_schedules.find(id);
    if (it == g_schedules.end())
    {
        return false;
    }
    g_schedules.erase(it);
    // 线程会在下次循环检测到调度不存在后退出
    return true;
}

json schedulesToJson()
{
    std::lock_guard<std::mutex> lock(g_scheduleMutex);
    json arr = json::array();
    for (const auto &kv : g_schedules)
    {
        const auto &s = kv.second;
        arr.push_back({{"id", s.id},
                       {"source", s.source.string()},
                       {"destination", s.destination.string()},
                       {"intervalSeconds", s.intervalSeconds},
                       {"maxBackups", s.maxBackups},
                       {"maxAgeDays", s.maxAgeDays},
                       {"compress", s.compress},
                       {"encrypt", s.encrypt},
                       {"username", s.user.username}});
    }
    return arr;
}

int main()
{
    // 传输加密（HTTPS）：通过环境变量 BACKUP_CERT / BACKUP_KEY 指定证书与私钥
    const char *certPath = std::getenv("BACKUP_CERT");
    const char *keyPath = std::getenv("BACKUP_KEY");

    std::unique_ptr<httplib::Server> server;
    bool https = false;
    if (certPath && keyPath && certPath[0] && keyPath[0])
    {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        server = std::make_unique<httplib::SSLServer>(certPath, keyPath);
        https = true;
#else
        std::cerr << "[警告] httplib 未启用 OpenSSL 支持，回退到 HTTP\n";
        server = std::make_unique<httplib::Server>();
#endif
    }
    else
    {
        server = std::make_unique<httplib::Server>();
    }

    if (!server->is_valid())
    {
        std::cerr << "[错误] 服务器初始化失败\n";
        return 1;
    }

    server->set_default_headers({{"Access-Control-Allow-Origin", "*"},
                                {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                                {"Access-Control-Allow-Headers", "Content-Type, Authorization"}});

    server->Options(R"(.*)", [](const httplib::Request &, httplib::Response &res)
                   { res.status = 204; });

    server->Post("/api/register", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            UserManager userManager("users.db");
            User user = userManager.registerUser(body.value("username", ""), body.value("password", ""));
            std::string token = createTokenForUser(user);

            res.set_content(json{{"userId", user.id}, {"username", user.username}, {"token", token}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server->Post("/api/login", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            UserManager userManager("users.db");
            User user = userManager.authenticate(body.value("username", ""), body.value("password", ""));
            std::string token = createTokenForUser(user);

            res.set_content(json{{"userId", user.id}, {"username", user.username}, {"token", token}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 401;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server->Post("/api/backup", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            std::string source = body.value("source", "");
            std::string destination = body.value("destination", "");
            bool compress = body.value("compress", false);
            bool encrypt = body.value("encrypt", false);
            std::string password = body.value("password", "");

            User user = optionalUserFromRequest(req, &body);
            BackupFilter filter = parseBackupFilter(body, user);

            int taskId = addTask("backup", source, destination, user);

            std::thread([taskId, source, destination, compress, encrypt, password, filter]() {
                try {
                    fs::path result = createBackup(source, destination, compress, encrypt, password, filter);
                    updateTask(taskId, "success", "备份完成：" + result.string());
                } catch (const std::exception& e) {
                    updateTask(taskId, "failed", e.what());
                }
            }).detach();

            res.set_content(json{{"taskId", taskId}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server->Post("/api/restore", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            std::string backupPath = body.value("backupPath", "");
            std::string destination = body.value("destination", "");
            std::string password = body.value("password", "");
            std::string encryptAlgo = body.value("encryptAlgo", "aes-256-cbc");

            User user = optionalUserFromRequest(req, &body);
            int taskId = addTask("restore", backupPath, destination, user);

            std::thread([taskId, backupPath, destination, password, encryptAlgo]() {
                try {
                    restoreBackup(backupPath, destination, password, encryptAlgo);
                    updateTask(taskId, "success", "还原完成");
                } catch (const std::exception& e) {
                    updateTask(taskId, "failed", e.what());
                }
            }).detach();

            res.set_content(json{{"taskId", taskId}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 定时备份调度 =====
    server->Post("/api/schedule/start", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            ScheduleConfig cfg;
            cfg.source = body.value("source", "");
            cfg.destination = body.value("destination", "");
            cfg.intervalSeconds = body.value("intervalSeconds", 3600);
            cfg.maxBackups = body.value("maxBackups", 0);
            cfg.maxAgeDays = body.value("maxAgeDays", 0);
            cfg.compress = body.value("compress", false);
            cfg.encrypt = body.value("encrypt", false);
            cfg.password = body.value("password", "");

            cfg.user = optionalUserFromRequest(req, &body);
            cfg.filter = parseBackupFilter(body, cfg.user);

            int id = startSchedule(cfg);

            res.set_content(json{{"scheduleId", id}, {"message", "定时备份已启动"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server->Post("/api/schedule/stop", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            int id = body.value("scheduleId", 0);
            bool ok = stopSchedule(id);
            res.set_content(json{{"success", ok}, {"message", ok ? "定时备份已停止" : "调度不存在"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server->Get("/api/schedules", [](const httplib::Request &, httplib::Response &res)
               { res.set_content(schedulesToJson().dump(), "application/json"); });

    // ===== 数据淘汰（手动）=====
    server->Post("/api/prune", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            std::string destination = body.value("destination", "");
            int maxBackups = body.value("maxBackups", 0);
            int maxAgeDays = body.value("maxAgeDays", 0);

            int removed = pruneBackups(destination, maxBackups, maxAgeDays);
            res.set_content(json{{"removed", removed}, {"message", "已淘汰 " + std::to_string(removed) + " 个旧备份"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 备份列表查询 =====
    server->Get("/api/backups", [](const httplib::Request &req, httplib::Response &res)
               {
        try {
            std::string destination = req.get_param_value("destination");
            auto backups = listBackups(destination);
            json arr = json::array();
            for (const auto& p : backups) {
                std::error_code ec;
                auto size = fs::is_directory(p) ? directorySize(p) : fs::file_size(p, ec);
                if (ec) size = 0;
                arr.push_back({
                    {"name", p.filename().string()},
                    {"path", p.string()},
                    {"size", size},
                    {"timestamp", backupTimestamp(p)}
                });
            }
            res.set_content(arr.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 备份元数据查询（网络备份的元数据管理）=====
    server->Get("/api/metadata", [](const httplib::Request &req, httplib::Response &res)
               {
        try {
            std::string backupPath = req.get_param_value("path");
            fs::path metaPath = fs::path(backupPath) / "metadata.json";
            if (fs::is_directory(backupPath) && fs::exists(metaPath)) {
                std::ifstream in(metaPath);
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                res.set_content(content, "application/json");
            } else {
                res.status = 404;
                res.set_content(json{{"error", "未找到 metadata.json"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server->Post("/api/watch/start", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            std::string source = body.value("source", "");
            std::string destination = body.value("destination", "");
            int interval = body.value("intervalSeconds", 10);

            User user = optionalUserFromRequest(req, &body);
            BackupFilter filter = parseBackupFilter(body, user);

            startWatcher(source, destination, interval, filter, user);

            res.set_content(json{{"message", "实时备份已启动"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server->Post("/api/watch/stop", [](const httplib::Request &, httplib::Response &res)
                {
        g_watching = false;
        res.set_content(json{{"message", "实时备份已停止"}}.dump(), "application/json"); });

    server->Get("/api/tasks", [](const httplib::Request &req, httplib::Response &res)
               {
        User user = optionalUserFromRequest(req);
        if (user.id == 0) {
            res.set_content(tasksToJson().dump(), "application/json");
        } else {
            res.set_content(tasksToJsonForUser(user).dump(), "application/json");
        } });

    // 端口配置：可通过环境变量 BACKUP_PORT 覆盖
    int port = 8080;
    if (const char *envPort = std::getenv("BACKUP_PORT"))
    {
        port = std::atoi(envPort);
        if (port <= 0)
        {
            port = 8080;
        }
    }

    std::cout << "Backup server running at " << (https ? "https" : "http")
              << "://localhost:" << port << "\n";
    if (https)
    {
        std::cout << "（HTTPS 传输加密已启用）\n";
    }
    else
    {
        std::cout << "提示：设置环境变量 BACKUP_CERT / BACKUP_KEY 可启用 HTTPS 传输加密\n";
    }
    server->listen("0.0.0.0", port);
}
