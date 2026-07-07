#include "httplib.h"
#include "json.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <openssl/sha.h>
#include <random>
#include <sstream>
#include <sqlite3.h>
#include <string>
#include <thread>
#include <vector>

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
};

std::mutex g_mutex;
std::vector<Task> g_tasks;
std::atomic<int> g_nextId{1};
std::atomic<bool> g_watching{false};
std::thread g_watchThread;

std::mutex g_tokenMutex;
std::map<std::string, User> g_tokens;

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

bool fileMatchesFilter(const fs::directory_entry &entry, const fs::path &source, const BackupFilter &filter)
{
    if (!entry.is_regular_file())
    {
        return false;
    }

    fs::path relativePath = fs::relative(entry.path(), source);
    std::string relative = relativePath.string();

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
    int copiedFiles)
{
    json meta;
    meta["source"] = source.string();
    meta["backupTime"] = nowString();
    meta["mode"] = mode;
    meta["size"] = directorySize(backupDir);
    meta["copiedFiles"] = copiedFiles;
    meta["userId"] = filter.userId;
    meta["username"] = filter.username;
    meta["filter"] = {
        {"includePaths", filter.includePaths},
        {"excludePaths", filter.excludePaths},
        {"extensions", filter.extensions},
        {"fileNameContains", filter.fileNameContains},
        {"minSize", filter.minSize},
        {"maxSize", filter.maxSize == std::numeric_limits<std::uintmax_t>::max() ? 0 : filter.maxSize}};

    std::ofstream out(backupDir / "metadata.json");
    out << meta.dump(4);
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

    for (const auto &entry : fs::recursive_directory_iterator(source))
    {
        if (!fileMatchesFilter(entry, source, filter))
        {
            continue;
        }

        fs::path relativePath = fs::relative(entry.path(), source);
        fs::path targetPath = backupDir / relativePath;

        fs::create_directories(targetPath.parent_path());
        fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
        copiedFiles++;
    }

    writeMetadata(backupDir, source, compress ? "compressed" : "normal", filter, copiedFiles);

    fs::path resultPath = backupDir;

    if (compress)
    {
        fs::path zipPath = backupDir;
        zipPath += ".zip";

        std::string cmd =
            "cd " + shellQuote(backupDir.parent_path().string()) +
            " && zip -r " + shellQuote(zipPath.filename().string()) +
            " " + shellQuote(backupDir.filename().string());

        if (!runCommand(cmd))
        {
            throw std::runtime_error("压缩失败，请确认系统已安装 zip");
        }

        fs::remove_all(backupDir);
        resultPath = zipPath;
    }

    if (encrypt)
    {
        if (password.empty())
        {
            throw std::runtime_error("启用加密时密码不能为空");
        }

        if (fs::is_directory(resultPath))
        {
            throw std::runtime_error("加密备份需要同时启用压缩");
        }

        fs::path encPath = resultPath;
        encPath += ".enc";

        std::string cmd =
            "openssl enc -aes-256-cbc -salt -pbkdf2 "
            "-in " +
            shellQuote(resultPath.string()) +
            " -out " + shellQuote(encPath.string()) +
            " -pass pass:" + shellQuote(password);

        if (!runCommand(cmd))
        {
            throw std::runtime_error("加密失败，请确认系统已安装 openssl");
        }

        fs::remove(resultPath);
        resultPath = encPath;
    }

    return resultPath;
}

void restoreBackup(
    const fs::path &backupPath,
    const fs::path &destination,
    const std::string &password)
{
    if (!fs::exists(backupPath))
    {
        throw std::runtime_error("备份文件或目录不存在");
    }

    fs::create_directories(destination);

    fs::path workPath = backupPath;
    fs::path tempPath;

    if (backupPath.extension() == ".enc")
    {
        if (password.empty())
        {
            throw std::runtime_error("还原加密备份时密码不能为空");
        }

        tempPath = fs::temp_directory_path() / ("restore_" + nowString());
        std::string fileName = backupPath.stem().string();
        fs::path decryptedPath = tempPath / fileName;

        fs::create_directories(tempPath);

        std::string cmd =
            "openssl enc -d -aes-256-cbc -pbkdf2 "
            "-in " +
            shellQuote(backupPath.string()) +
            " -out " + shellQuote(decryptedPath.string()) +
            " -pass pass:" + shellQuote(password);

        if (!runCommand(cmd))
        {
            throw std::runtime_error("解密失败，请检查密码");
        }

        workPath = decryptedPath;
    }

    if (workPath.extension() == ".zip")
    {
        fs::path unzipDir = fs::temp_directory_path() / ("unzip_" + nowString());
        fs::create_directories(unzipDir);

        std::string cmd =
            "unzip -o " + shellQuote(workPath.string()) +
            " -d " + shellQuote(unzipDir.string());

        if (!runCommand(cmd))
        {
            throw std::runtime_error("解压失败，请确认系统已安装 unzip");
        }

        for (const auto &entry : fs::directory_iterator(unzipDir))
        {
            fs::copy(
                entry.path(),
                destination / entry.path().filename(),
                fs::copy_options::recursive |
                    fs::copy_options::overwrite_existing);
        }

        fs::remove_all(unzipDir);
    }
    else if (fs::is_directory(workPath))
    {
        fs::copy(
            workPath,
            destination / workPath.filename(),
            fs::copy_options::recursive |
                fs::copy_options::overwrite_existing);
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

int main()
{
    httplib::Server server;

    server.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                                {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                                {"Access-Control-Allow-Headers", "Content-Type, Authorization"}});

    server.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res)
                   { res.status = 204; });

    server.Post("/api/register", [](const httplib::Request &req, httplib::Response &res)
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

    server.Post("/api/login", [](const httplib::Request &req, httplib::Response &res)
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

    server.Post("/api/backup", [](const httplib::Request &req, httplib::Response &res)
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

    server.Post("/api/restore", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            std::string backupPath = body.value("backupPath", "");
            std::string destination = body.value("destination", "");
            std::string password = body.value("password", "");

            User user = optionalUserFromRequest(req, &body);
            int taskId = addTask("restore", backupPath, destination, user);

            std::thread([taskId, backupPath, destination, password]() {
                try {
                    restoreBackup(backupPath, destination, password);
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

    server.Post("/api/watch/start", [](const httplib::Request &req, httplib::Response &res)
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

    server.Post("/api/watch/stop", [](const httplib::Request &, httplib::Response &res)
                {
        g_watching = false;
        res.set_content(json{{"message", "实时备份已停止"}}.dump(), "application/json"); });

    server.Get("/api/tasks", [](const httplib::Request &req, httplib::Response &res)
               {
        User user = optionalUserFromRequest(req);
        if (user.id == 0) {
            res.set_content(tasksToJson().dump(), "application/json");
        } else {
            res.set_content(tasksToJsonForUser(user).dump(), "application/json");
        } });

    std::cout << "Backup server running at http://localhost:8080\n";
    server.listen("0.0.0.0", 8080);
}
