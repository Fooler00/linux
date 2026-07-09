#pragma once

// =====================================================================
//  BackupServer.h - HTTP 服务层封装
//
//  封装 httplib 路由注册、HTTPS 传输加密、CORS、鉴权。
//  依赖 backup_core + backup_cloud，对外暴露 RESTful API。
// =====================================================================

#include "cloud/ICloudStorage.h"
#include <memory>
#include <string>

namespace backup::server {

struct ServerConfig
{
    int port = 8080;
    bool tls = false;               // 是否启用 HTTPS 传输加密
    std::string certPath;           // TLS 证书路径
    std::string keyPath;            // TLS 私钥路径
    std::string dbPath = "users.db"; // 用户数据库路径
    std::string cloudRootDir;       // 云存储根目录（local 模式）
};

class BackupServer
{
public:
    BackupServer(ServerConfig config, std::shared_ptr<cloud::ICloudStorage> storage);
    ~BackupServer();

    // 注册所有路由
    void registerRoutes();

    // 启动监听（阻塞）
    int run();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace backup::server
