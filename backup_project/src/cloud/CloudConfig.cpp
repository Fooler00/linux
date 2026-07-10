// =====================================================================
//  CloudConfig.cpp - 网盘配置 + 工厂实现
// =====================================================================

#include "cloud/CloudConfig.h"
#include "cloud/LocalCloudStorage.h"
#include "cloud/RemoteCloudStorage.h"

#include <stdexcept>

namespace backup::cloud {

CloudConfig loadCloudConfig(const nlohmann::json &j)
{
    CloudConfig cfg;
    cfg.type = j.value("type", "local");
    cfg.rootDir = j.value("rootDir", "");
    cfg.endpoint = j.value("endpoint", "");
    cfg.username = j.value("username", "");
    cfg.password = j.value("password", "");
    cfg.token = j.value("token", "");
    cfg.tls = j.value("tls", true);
    return cfg;
}

std::unique_ptr<ICloudStorage> createCloudStorage(const CloudConfig &config)
{
    if (config.type == "local")
    {
        return std::make_unique<LocalCloudStorage>(config.rootDir);
    }

    if (config.type == "remote")
    {
        return std::make_unique<RemoteCloudStorage>(config);
    }

    throw std::runtime_error("不支持的云存储类型: " + config.type +
                             "（当前支持 local / remote）");
}

} // namespace backup::cloud
