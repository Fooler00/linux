#pragma once

// =====================================================================
//  Incremental.h - 增量备份支持
//  基于文件内容 SHA-256 与上次备份 manifest 对比
// =====================================================================

#include <filesystem>
#include <map>
#include <string>

namespace backup {

namespace fs = std::filesystem;

// 读取上次备份的文件哈希表
std::map<std::string, std::string> loadIncrementalManifest(const fs::path &baseDir);

// 写入本次备份的文件哈希清单（供下一次增量使用）
void writeIncrementalManifest(const fs::path &backupDir, const std::map<std::string, std::string> &hashes);

} // namespace backup
