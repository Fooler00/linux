// =====================================================================
//  Incremental.cpp - 增量备份 manifest 读写实现
// =====================================================================

#include "core/Incremental.h"
#include "core/Utils.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

namespace backup {

std::map<std::string, std::string> loadIncrementalManifest(const fs::path &baseDir)
{
    std::map<std::string, std::string> hashes;
    if (baseDir.empty() || !fs::exists(baseDir))
    {
        return hashes;
    }

    fs::path manifestPath = baseDir / "manifest.json";
    if (!fs::exists(manifestPath))
    {
        return hashes;
    }

    std::ifstream in(manifestPath);
    if (!in)
    {
        return hashes;
    }

    try
    {
        nlohmann::json manifest = nlohmann::json::parse(in);
        if (manifest.contains("files") && manifest["files"].is_object())
        {
            for (auto it = manifest["files"].begin(); it != manifest["files"].end(); ++it)
            {
                hashes[it.key()] = it.value().get<std::string>();
            }
        }
    }
    catch (...)
    {
        // 解析失败则视为空 manifest
    }

    return hashes;
}

void writeIncrementalManifest(const fs::path &backupDir, const std::map<std::string, std::string> &hashes)
{
    nlohmann::json manifest;
    manifest["backupTime"] = nowString();
    manifest["files"] = nlohmann::json::object();
    for (const auto &kv : hashes)
    {
        manifest["files"][kv.first] = kv.second;
    }

    std::ofstream out(backupDir / "manifest.json");
    out << manifest.dump(4);
}

} // namespace backup
