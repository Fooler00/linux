#pragma once

// =====================================================================
//  Archive.h - 打包解包（多算法：zip / tar / tar.gz）
// =====================================================================

#include <filesystem>
#include <string>

namespace backup {

namespace fs = std::filesystem;

// 归档类型白名单校验
std::string normalizeArchiveType(const std::string &type);

// 将 backupDir 打包为指定格式，返回归档文件路径
fs::path createArchive(const fs::path &backupDir, const std::string &archiveType);

// 解包归档文件到指定目录，返回归档内层目录路径
fs::path extractArchive(const fs::path &archivePath, const fs::path &workDir);

} // namespace backup
