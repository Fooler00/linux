// =====================================================================
//  Archive.cpp - 打包解包实现（zip / tar / tar.gz）
// =====================================================================

#include "core/Archive.h"
#include "core/Utils.h"

#include <stdexcept>

namespace backup {

std::string normalizeArchiveType(const std::string &type)
{
    if (type == "none" || type == "zip" || type == "tar" || type == "tar.gz" || type == "tgz")
    {
        return type == "tgz" ? "tar.gz" : type;
    }
    return "zip";
}

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

} // namespace backup
