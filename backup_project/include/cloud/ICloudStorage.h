#pragma once

// =====================================================================
//  ICloudStorage.h - 网盘存储抽象接口
//
//  这是网盘模式扩展的核心接口。当前提供 LocalCloudStorage（本地文件
//  系统实现，即单机模式）。将来实现 RemoteCloudStorage（HTTP/S3/WebDAV
//  等）即可将单机模式扩展为网盘模式，无需改动 core 备份引擎。
//
//  典型用法：
//    auto storage = createCloudStorage(config);
//    engine.createBackup(...);                 // 本地生成备份
//    storage->upload(localPath, remotePath);   // 上传到云端
// =====================================================================

#include <cstdint>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

namespace backup::cloud {

namespace fs = std::filesystem;

// 云端文件信息
struct CloudFileInfo
{
    std::string path;
    std::uintmax_t size = 0;
    std::time_t modified = 0;
};

// 网盘存储抽象接口
class ICloudStorage
{
public:
    virtual ~ICloudStorage() = default;

    // 上传统一备份文件到云端
    virtual bool upload(const fs::path &localPath, const std::string &remotePath) = 0;

    // 从云端下载备份文件到本地
    virtual bool download(const std::string &remotePath, const fs::path &localPath) = 0;

    // 列出云端指定目录下的备份
    virtual std::vector<CloudFileInfo> list(const std::string &remoteDir) = 0;

    // 删除云端备份
    virtual bool remove(const std::string &remotePath) = 0;

    // 存储类型标识（local / remote / s3 / webdav ...）
    virtual std::string type() const = 0;
};

} // namespace backup::cloud
