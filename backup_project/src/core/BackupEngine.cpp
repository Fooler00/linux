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

void writeMetadata(
    const fs::path &backupDir,
    const fs::path &source,
    const std::string &mode,
    const BackupFilter &filter,
    int copiedFiles,
    const std::string &archiveType,
    const std::string &encryptAlgo,
    bool incremental)
{
    nlohmann::json meta;
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
            copied = createSpecialFile(targetPath, meta, reason);
            if (!copied)
            {
                std::cerr << "[警告] 跳过特殊文件 " << entry.path() << ": " << reason << "\n";
            }
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
    }

    // 始终写入 manifest，使任意备份都可作为后续增量备份的基线
    writeIncrementalManifest(backupDir, newHashes);

    std::string archiveType = filter.archiveType;
    // 兼容旧字段：compress=true 且 archiveType 默认时使用 zip
    if (compress && archiveType == "zip")
    {
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
