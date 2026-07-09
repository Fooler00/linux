#pragma once

// =====================================================================
//  RemoteCloudStorage.h - 远程网盘存储（通过 HTTP 连接 VM 服务器）
//
//  连接到一台运行 backup_server（local 模式）的虚拟机，将其当作网盘。
//  通过 HTTP 调用远端的 /api/cloud/upload|download|list|delete 接口，
//  实现跨机文件传输。远端需配置相同 BACKUP_CLOUD_TOKEN 进行鉴权。
// =====================================================================

#include "cloud/CloudConfig.h"
#include "cloud/ICloudStorage.h"

#include <httplib.h>
#include <memory>

namespace backup::cloud {

class RemoteCloudStorage : public ICloudStorage
{
public:
    explicit RemoteCloudStorage(const CloudConfig &cfg);

    bool upload(const fs::path &localPath, const std::string &remotePath) override;
    bool download(const std::string &remotePath, const fs::path &localPath) override;
    std::vector<CloudFileInfo> list(const std::string &remoteDir) override;
    bool remove(const std::string &remotePath) override;
    std::string type() const override;

private:
    std::string host_;
    int port_ = 8080;
    bool tls_ = false;
    std::string token_;                         // 固定 token（与远端 BACKUP_CLOUD_TOKEN 一致）
    std::unique_ptr<httplib::Client> client_;
    httplib::Headers authHeaders();             // 构造 Authorization 头
};

} // namespace backup::cloud
