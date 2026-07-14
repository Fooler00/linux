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
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sys/file.h>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;

// 单实例文件锁：防止多个 backup_server 进程同时运行导致任务列表跳变
// 返回值：true=获得锁(唯一实例)，false=已有其他实例在运行
bool acquireSingleInstanceLock(int port)
{
    fs::create_directories("/tmp");
    // 锁文件按端口命名，不同端口可以共存
    const std::string lockPath = "/tmp/backup_server_" + std::to_string(port) + ".lock";
    int fd = open(lockPath.c_str(), O_RDWR | O_CREAT, 0600);
    if (fd < 0)
    {
        std::cerr << "[错误] 无法创建锁文件 " << lockPath << ": " << std::strerror(errno) << "\n";
        return false;
    }
    // 非阻塞排他锁
    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        if (errno == EWOULDBLOCK)
        {
            std::cerr << "[错误] backup_server 已在运行（端口 " << port << "）。\n"
                      << "       多实例会导致任务列表不一致。如需启动新实例，请先停止旧进程。\n";
        }
        else
        {
            std::cerr << "[错误] 加锁失败: " << std::strerror(errno) << "\n";
        }
        ::close(fd);
        return false;
    }
    // 写入 PID 便于排查
    const std::string pidStr = std::to_string(static_cast<long>(getpid())) + "\n";
    if (ftruncate(fd, 0) != 0) { /* 忽略 */ }
    if (write(fd, pidStr.c_str(), pidStr.size()) < 0) { /* 忽略 */ }
    // 保持 fd 打开，进程退出时自动释放锁
    return true;
}

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

    // BUG-TASK-001：单实例锁，避免多进程导致任务列表在不同历史任务集合间跳变
    if (!acquireSingleInstanceLock(cfg.port))
    {
        return 1;
    }

    // 创建云存储后端：支持 local（单机）与 remote（连接 VM 网盘服务器）
    backup::cloud::CloudConfig cloudCfg;

    // 优先环境变量
    if (const char *t = std::getenv("BACKUP_CLOUD_TYPE"))
    {
        cloudCfg.type = t;
    }
    if (cloudCfg.type == "remote")
    {
        // 远程网盘模式：连接指定 VM 服务器
        if (const char *e = std::getenv("BACKUP_CLOUD_ENDPOINT")) cloudCfg.endpoint = e;
        if (const char *tk = std::getenv("BACKUP_CLOUD_TOKEN")) cloudCfg.token = tk;
        if (const char *u = std::getenv("BACKUP_CLOUD_USER")) cloudCfg.username = u;
        if (const char *p = std::getenv("BACKUP_CLOUD_PASS")) cloudCfg.password = p;
        cloudCfg.tls = std::getenv("BACKUP_CLOUD_TLS") != nullptr;
    }
    else
    {
        // 本地单机模式
        cloudCfg.type = "local";
        cloudCfg.rootDir = cfg.cloudRootDir.empty() ? "./cloud_storage" : cfg.cloudRootDir;
    }

    auto storage = backup::cloud::createCloudStorage(cloudCfg);

    backup::server::BackupServer server(cfg, std::move(storage));
    return server.run();
}