#pragma once

// =====================================================================
//  CloudConfig.h - 网盘配置 + 工厂
//
//  通过配置选择不同的存储后端。当前仅实现 local，预留 remote/s3/
//  webdav。新增远程后端只需实现 ICloudStorage 并在此工厂注册。
// =====================================================================

#include "cloud/ICloudStorage.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

namespace backup::cloud {

struct CloudConfig
{
    // local（单机模式，默认）| remote（网盘模式，预留）
    std::string type = "local";

    // local 模式：本地存储根目录
    std::string rootDir;

    // remote 模式预留字段（传输加密 + 用户管理）
    std::string endpoint;   // 远程服务端点
    std::string username;
    std::string password;
    std::string token;
    bool tls = true;        // 是否启用传输加密（HTTPS）
};

// 从 JSON 配置构造 CloudConfig
CloudConfig loadCloudConfig(const nlohmann::json &j);

// 工厂方法：根据配置创建存储后端
std::unique_ptr<ICloudStorage> createCloudStorage(const CloudConfig &config);

} // namespace backup::cloud
