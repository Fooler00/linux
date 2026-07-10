// =====================================================================
//  LocalCloudStorage.cpp - 本地文件系统云存储实现（单机模式）
// =====================================================================

#include "cloud/LocalCloudStorage.h"

#include <chrono>
#include <filesystem>
#include <stdexcept>

namespace backup::cloud {

LocalCloudStorage::LocalCloudStorage(std::string rootDir)
    : rootDir_(std::move(rootDir))
{
    if (!rootDir_.empty())
    {
        fs::create_directories(rootDir_);
    }
}

fs::path LocalCloudStorage::resolve(const std::string &remotePath) const
{
    // 防止路径穿越：在根目录下拼接
    fs::path full = fs::path(rootDir_) / remotePath;
    // 规范化并校验仍在根目录下
    fs::path canonicalRoot = fs::path(rootDir_).lexically_normal();
    fs::path canonicalFull = full.lexically_normal();
    if (canonicalFull.string().find(canonicalRoot.string()) != 0)
    {
        throw std::runtime_error("非法的远程路径: " + remotePath);
    }
    return canonicalFull;
}

bool LocalCloudStorage::upload(const fs::path &localPath, const std::string &remotePath)
{
    try
    {
        fs::path dst = resolve(remotePath);
        fs::create_directories(dst.parent_path());
        fs::copy_file(localPath, dst, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

bool LocalCloudStorage::download(const std::string &remotePath, const fs::path &localPath)
{
    try
    {
        fs::path src = resolve(remotePath);
        fs::create_directories(localPath.parent_path());
        fs::copy_file(src, localPath, fs::copy_options::overwrite_existing);
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

std::vector<CloudFileInfo> LocalCloudStorage::list(const std::string &remoteDir)
{
    std::vector<CloudFileInfo> result;
    try
    {
        fs::path dir = resolve(remoteDir);
        if (!fs::exists(dir) || !fs::is_directory(dir))
        {
            return result;
        }

        for (const auto &entry : fs::directory_iterator(dir))
        {
            CloudFileInfo info;
            info.path = entry.path().filename().string();
            std::error_code ec;
            if (fs::is_directory(entry.path()))
            {
                info.size = 0;
            }
            else
            {
                info.size = fs::file_size(entry.path(), ec);
                if (ec)
                {
                    info.size = 0;
                }
            }
            // 用文件写入时间转 time_t
            auto ftime = fs::last_write_time(entry.path(), ec);
            if (!ec)
            {
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                info.modified = std::chrono::system_clock::to_time_t(sctp);
            }
            result.push_back(info);
        }
    }
    catch (const std::exception &)
    {
        // 忽略错误，返回已收集的部分
    }
    return result;
}

bool LocalCloudStorage::remove(const std::string &remotePath)
{
    try
    {
        fs::path target = resolve(remotePath);
        std::error_code ec;
        fs::remove_all(target, ec);
        return !ec;
    }
    catch (const std::exception &)
    {
        return false;
    }
}

std::string LocalCloudStorage::type() const
{
    return "local";
}

} // namespace backup::cloud
