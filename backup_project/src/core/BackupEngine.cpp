// =====================================================================
//  BackupEngine.cpp - 备份引擎实现
// =====================================================================
 
#include "core/BackupEngine.h"
#include "core/Archive.h"
#include "core/Crypto.h"
#include "core/Filter.h"
#include "core/Incremental.h"
#include "core/Metadata.h"
#include "core/Utils.h"
 
#include <nlohmann/json.hpp>
 
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
 
namespace backup {
 
namespace {
 
fs::path absolutePathInBackup(const fs::path &absolutePath)
{
    fs::path normalized = absolutePath.lexically_normal();
    std::string pathStr = normalized.string();
    if (!pathStr.empty() && pathStr.front() == '/')
    {
        pathStr.erase(pathStr.begin());
    }
    if (pathStr.empty())
    {
        throw std::runtime_error("无法解析源文件路径：" + absolutePath.string());
    }
    return fs::path(pathStr);
}
 
fs::path finalizeBackup(
    const fs::path &backupDir,
    const fs::path &metadataSource,
    bool compress,
    bool encrypt,
    const std::string &password,
    const BackupFilter &filter,
    int copiedFiles,
    const std::map<std::string, std::string> &newHashes,
    const std::vector<fs::path> &metadataSources = {})
{
    writeIncrementalManifest(backupDir, newHashes);
 
    std::string archiveType = filter.archiveType;
    if (compress && archiveType == "zip")
    {
        archiveType = "zip";
    }
 
    writeMetadata(
        backupDir,
        metadataSource,
        archiveType,
        filter,
        copiedFiles,
        archiveType,
        filter.encryptAlgo,
        filter.incremental,
        metadataSources);
 
    fs::path resultPath = backupDir;
 
    std::string type = normalizeArchiveType(archiveType);
    if (type != "none")
    {
        resultPath = createArchive(backupDir, type);
    }
 
    if (encrypt)
    {
        if (password.empty())
        {
            throw std::runtime_error("启用加密时密码不能为空");
        }
 
        if (fs::is_directory(resultPath))
        {
            resultPath = createArchive(resultPath, "tar.gz");
        }
 
        resultPath = encryptFile(resultPath, password, filter.encryptAlgo);
    }
 
    return resultPath;
}
 
} // namespace
 
void writeMetadata(
    const fs::path &backupDir,
    const fs::path &source,
    const std::string &mode,
    const BackupFilter &filter,
    int copiedFiles,
    const std::string &archiveType,
    const std::string &encryptAlgo,
    bool incremental,
    const std::vector<fs::path> &sources)
{
    nlohmann::json meta;
    meta["source"] = source.string();
    if (!sources.empty())
    {
        nlohmann::json sourceList = nlohmann::json::array();
        for (const auto &item : sources)
        {
            sourceList.push_back(item.string());
        }
        meta["sources"] = sourceList;
    }
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
        throw std::runtime_error("源路径不存在");
    }
 
    const bool sourceIsDirectory = fs::is_directory(source);
    if (!sourceIsDirectory && !fs::is_regular_file(source) && !fs::is_symlink(source))
    {
        FileMetadata probeMeta;
        const bool hasMeta = getFileMetadata(source, probeMeta);
        if (!hasMeta || !(probeMeta.isPipe || probeMeta.isBlockDevice || probeMeta.isCharDevice))
        {
            throw std::runtime_error("源路径必须是文件或目录");
        }
    }
 
    fs::create_directories(destination);
 
    fs::path backupDir = destination / ("backup_" + nowString());
    fs::create_directories(backupDir);
 
    int copiedFiles = 0;
    std::map<std::string, std::string> newHashes;
    std::map<std::string, std::string> baseHashes;
    std::map<ino_t, fs::path> seenInodes; // inode → 已备份路径，用于硬链接重建
    if (filter.incremental)
    {
        baseHashes = loadIncrementalManifest(filter.incrementalBase);
    }
 
    // 目录与单文件共用同一拷贝逻辑；单文件时以父目录作为筛选相对路径基准。
    const auto copyEntry = [&](const fs::directory_entry &entry, const fs::path &sourceRoot) {
        if (!fileMatchesFilter(entry, sourceRoot, filter))
        {
            return;
        }
 
        // 用 lexically_relative 而非 fs::relative：后者会调用 weakly_canonical
        // 跟随符号链接，导致符号链接文件被解析成其目标，相对路径计算错误
        // （如 link.txt -> a.txt 会被误算成 a.txt 的相对路径）。
        fs::path relativePath = entry.path().lexically_relative(sourceRoot);
        fs::path targetPath = backupDir / relativePath;
 
        fs::create_directories(targetPath.parent_path());
 
        FileMetadata meta;
        bool hasMeta = getFileMetadata(entry.path(), meta);
 
        bool copied = false;
        // BUG-SPECIAL-001：必须先判断 is_symlink()，因为 is_regular_file() 会跟随符号链接。
        // 若符号链接指向普通文件，is_regular_file() 返回 true，会错误地走 copy_file
        // 分支复制文件内容而非保存链接本身。
        if (entry.is_symlink() && filter.includeSpecialFiles)
        {
            std::string reason;
            copied = createSpecialFile(targetPath, meta, reason);
            if (!copied)
            {
                std::cerr << "[警告] 跳过特殊文件 " << entry.path() << ": " << reason << "\n";
            }
        }
        else if (entry.is_regular_file())
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
                    return;
                }
            }
 
            // 硬链接检测：同一 inode 已备份过则创建硬链接，节省空间并保留链接关系
            bool hardlinked = false;
            if (meta.nlink > 1 && meta.inode != 0)
            {
                auto it = seenInodes.find(meta.inode);
                if (it != seenInodes.end())
                {
                    std::error_code ec;
                    fs::create_hard_link(it->second, targetPath, ec);
                    if (!ec)
                    {
                        hardlinked = true;
                    }
                }
            }
 
            if (!hardlinked)
            {
                fs::copy_file(entry.path(), targetPath, fs::copy_options::overwrite_existing);
                if (meta.nlink > 1 && meta.inode != 0)
                {
                    seenInodes[meta.inode] = targetPath;
                }
            }
            copied = true;
        }
        else if (hasMeta && filter.includeSpecialFiles &&
                 (meta.isPipe || meta.isBlockDevice || meta.isCharDevice))
        {
            std::string reason;
            copied = createSpecialFile(targetPath, meta, reason);
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
 
        // 元数据保留
        if (copied && filter.preserveMetadata && hasMeta)
        {
            applyFileMetadata(targetPath, meta);
        }
 
        if (copied)
        {
            copiedFiles++;
        }
    };
 
    if (sourceIsDirectory)
    {
        for (const auto &entry : fs::recursive_directory_iterator(source))
        {
            copyEntry(entry, source);
        }
    }
    else
    {
        copyEntry(fs::directory_entry(source), source.parent_path());
    }
 
    // 始终写入 manifest，使任意备份都可作为后续增量备份的基线
    return finalizeBackup(
        backupDir,
        source,
        compress,
        encrypt,
        password,
        filter,
        copiedFiles,
        newHashes);
}
 
fs::path createBackupFromSources(
    const std::vector<fs::path> &sources,
    const fs::path &destination,
    bool compress,
    bool encrypt,
    const std::string &password,
    const BackupFilter &filter)
{
    if (sources.empty())
    {
        throw std::runtime_error("sources 不能为空");
    }
 
    if (filter.incremental)
    {
        throw std::runtime_error("多文件备份不支持增量模式");
    }
 
    std::vector<fs::path> normalizedSources;
    normalizedSources.reserve(sources.size());
 
    for (const auto &src : sources)
    {
        fs::path absolutePath = fs::absolute(src);
        if (!fs::exists(absolutePath))
        {
            throw std::runtime_error("源文件不存在：" + absolutePath.string());
        }
 
        if (!fs::is_regular_file(absolutePath) && !fs::is_symlink(absolutePath))
        {
            // 允许管道/块设备/字符设备作为源（与 createBackup 单源行为一致）
            FileMetadata probeMeta;
            const bool probeOk = getFileMetadata(absolutePath, probeMeta);
            if (!probeOk ||
                !(probeMeta.isPipe || probeMeta.isBlockDevice || probeMeta.isCharDevice))
            {
                throw std::runtime_error("多文件备份仅支持文件或特殊文件：" + absolutePath.string());
            }
        }
 
        if (std::find(normalizedSources.begin(), normalizedSources.end(), absolutePath) == normalizedSources.end())
        {
            normalizedSources.push_back(absolutePath);
        }
    }
 
    fs::create_directories(destination);
 
    fs::path backupDir = destination / ("backup_" + nowString());
    fs::create_directories(backupDir);
 
    int copiedFiles = 0;
    std::map<std::string, std::string> newHashes;
    std::map<ino_t, fs::path> seenInodes; // inode → 已备份路径，用于硬链接重建
 
    for (const auto &sourcePath : normalizedSources)
    {
        fs::directory_entry entry(sourcePath);
        if (!fileMatchesFilter(entry, sourcePath.parent_path(), filter))
        {
            continue;
        }
 
        fs::path relativePath = absolutePathInBackup(sourcePath);
        fs::path targetPath = backupDir / relativePath;
        fs::create_directories(targetPath.parent_path());
 
        FileMetadata meta;
        bool hasMeta = getFileMetadata(sourcePath, meta);
 
        bool copied = false;
        // BUG-SPECIAL-001：必须先判断 is_symlink()，因为 is_regular_file() 会跟随符号链接。
        if (entry.is_symlink() && filter.includeSpecialFiles)
        {
            std::string reason;
            copied = createSpecialFile(targetPath, meta, reason);
            if (!copied)
            {
                std::cerr << "[警告] 跳过特殊文件 " << sourcePath << ": " << reason << "\n";
            }
        }
        else if (entry.is_regular_file())
        {
            std::string relStr = relativePath.string();
            std::string hash = fileContentHash(sourcePath);
            if (!hash.empty())
            {
                newHashes[relStr] = hash;
            }
 
            // 硬链接检测：同一 inode 已备份过则创建硬链接
            bool hardlinked = false;
            if (meta.nlink > 1 && meta.inode != 0)
            {
                auto it = seenInodes.find(meta.inode);
                if (it != seenInodes.end())
                {
                    std::error_code ec;
                    fs::create_hard_link(it->second, targetPath, ec);
                    if (!ec)
                    {
                        hardlinked = true;
                    }
                }
            }
 
            if (!hardlinked)
            {
                fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing);
                if (meta.nlink > 1 && meta.inode != 0)
                {
                    seenInodes[meta.inode] = targetPath;
                }
            }
            copied = true;
        }
        else if (hasMeta && filter.includeSpecialFiles &&
                 (meta.isPipe || meta.isBlockDevice || meta.isCharDevice))
        {
            // 管道/块设备/字符设备：用 mkfifo/mknod 重建
            std::string reason;
            copied = createSpecialFile(targetPath, meta, reason);
            if (!copied)
            {
                std::cerr << "[警告] 跳过特殊文件 " << sourcePath << ": " << reason << "\n";
            }
        }
 
        if (copied && filter.preserveMetadata && hasMeta)
        {
            applyFileMetadata(targetPath, meta);
        }
 
        if (copied)
        {
            copiedFiles++;
        }
    }
 
    if (copiedFiles == 0)
    {
        throw std::runtime_error("没有符合条件的文件可备份");
    }
 
    return finalizeBackup(
        backupDir,
        normalizedSources.front(),
        compress,
        encrypt,
        password,
        filter,
        copiedFiles,
        newHashes,
        normalizedSources);
}
 
void restoreBackup(
    const fs::path &backupPath,
    const fs::path &destination,
    const std::string &password,
    const std::string &encryptAlgo)
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
        if (name.rfind("backup_", 0) == 0)
        {
            backups.push_back(entry.path());
        }
    }
 
    std::sort(backups.begin(), backups.end());
    return backups;
}
 
std::time_t backupTimestamp(const fs::path &path)
{
    std::string name = path.filename().string();
    size_t pos = name.find("backup_");
    if (pos == std::string::npos)
    {
        return 0;
    }
    std::string rest = name.substr(pos + 7);
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
 
} // namespace backup