#pragma once

// =====================================================================
//  LocalCloudStorage.h - 本地文件系统实现的云存储（单机模式）
// =====================================================================

#include "cloud/ICloudStorage.h"

namespace backup::cloud {

class LocalCloudStorage : public ICloudStorage
{
public:
    explicit LocalCloudStorage(std::string rootDir);

    bool upload(const fs::path &localPath, const std::string &remotePath) override;
    bool download(const std::string &remotePath, const fs::path &localPath) override;
    std::vector<CloudFileInfo> list(const std::string &remoteDir) override;
    bool remove(const std::string &remotePath) override;
    std::string type() const override;

private:
    std::string rootDir_;
    fs::path resolve(const std::string &remotePath) const;
};

} // namespace backup::cloud
