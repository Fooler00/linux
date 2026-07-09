// =====================================================================
//  main.cpp - HTTP 服务入口
//
//  支持两种配置方式（环境变量优先，其次配置文件 config/server.conf）：
//    BACKUP_PORT      监听端口
//    BACKUP_CERT      TLS 证书路径（启用 HTTPS）
//    BACKUP_KEY       TLS 私钥路径
//    BACKUP_DB        用户数据库路径
//    BACKUP_CLOUD_DIR 云存储根目录（local 模式）
// =====================================================================

#include "server/BackupServer.h"
#include "cloud/CloudConfig.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
// 从配置文件加载（可选）
backup::server::ServerConfig loadServerConfig()
{
    backup::server::ServerConfig cfg;

    // 环境变量优先
    if (const char *env = std::getenv("BACKUP_PORT"))
    {
        cfg.port = std::atoi(env);
        if (cfg.port <= 0) cfg.port = 8080;
    }
    if (const char *env = std::getenv("BACKUP_CERT")) cfg.certPath = env;
    if (const char *env = std::getenv("BACKUP_KEY")) cfg.keyPath = env;
    if (const char *env = std::getenv("BACKUP_DB")) cfg.dbPath = env;
    if (const char *env = std::getenv("BACKUP_CLOUD_DIR")) cfg.cloudRootDir = env;

    // 尝试读取配置文件（若存在）
    std::ifstream in("config/server.conf");
    if (in)
    {
        try
        {
            nlohmann::json j = nlohmann::json::parse(in);
            if (j.contains("port")) cfg.port = j["port"].get<int>();
            if (j.contains("tls")) cfg.tls = j["tls"].get<bool>();
            if (j.contains("certPath")) cfg.certPath = j["certPath"].get<std::string>();
            if (j.contains("keyPath")) cfg.keyPath = j["keyPath"].get<std::string>();
            if (j.contains("dbPath")) cfg.dbPath = j["dbPath"].get<std::string>();
            if (j.contains("cloudRootDir")) cfg.cloudRootDir = j["cloudRootDir"].get<std::string>();
        }
        catch (const std::exception &e)
        {
            std::cerr << "[警告] 配置文件解析失败: " << e.what() << "，使用默认配置\n";
        }
    }

    // 若设置了证书路径则自动启用 TLS
    if (!cfg.certPath.empty() && !cfg.keyPath.empty())
    {
        cfg.tls = true;
    }

    return cfg;
}
} // namespace

int main()
{
    auto cfg = loadServerConfig();

    // 创建云存储后端（当前 local 模式，预留 remote 扩展）
    backup::cloud::CloudConfig cloudCfg;
    cloudCfg.type = "local";
    cloudCfg.rootDir = cfg.cloudRootDir.empty() ? "./cloud_storage" : cfg.cloudRootDir;
    auto storage = backup::cloud::createCloudStorage(cloudCfg);

    backup::server::BackupServer server(cfg, std::move(storage));
    return server.run();
}
