#pragma once

// =====================================================================
//  BackupEngine.h - 备份引擎（核心）
//  本地前端可直接调用此类完成备份/还原，无需经过 HTTP
// =====================================================================

#include "core/Types.h"

#include <vector>

namespace backup {

namespace fs = std::filesystem;

// 写入备份元数据 metadata.json
void writeMetadata(
    const fs::path &backupDir,
    const fs::path &source,
    const std::string &mode,
    const BackupFilter &filter,
    int copiedFiles,
    const std::string &archiveType = "",
    const std::string &encryptAlgo = "",
    bool incremental = false,
    const std::vector<fs::path> &sources = {});

// 创建备份
//   source       源路径（文件或目录）
//   destination  备份存放目录
//   compress     是否压缩（兼容旧接口，实际由 filter.archiveType 决定）
//   encrypt      是否加密
//   password     加密密码
//   filter       筛选与算法配置
fs::path createBackup(
    const fs::path &source,
    const fs::path &destination,
    bool compress,
    bool encrypt,
    const std::string &password,
    const BackupFilter &filter);

// 从多个绝对路径创建备份（跨目录多文件，合并为同一次备份）
fs::path createBackupFromSources(
    const std::vector<fs::path> &sources,
    const fs::path &destination,
    bool compress,
    bool encrypt,
    const std::string &password,
    const BackupFilter &filter);

// 创建备份
//   source       源路径（文件或目录）
//   destination  备份存放目录
//   compress     是否压缩（兼容旧接口，实际由 filter.archiveType 决定）
//   encrypt      是否加密
//   password     加密密码
//   filter       筛选与算法配置
fs::path createBackup(
    const fs::path &source,
    const fs::path &destination,
    bool compress,
    bool encrypt,
    const std::string &password,
    const BackupFilter &filter);

// 还原备份
void restoreBackup(
    const fs::path &backupPath,
    const fs::path &destination,
    const std::string &password,
    const std::string &encryptAlgo = "aes-256-cbc");

// 列出某目录下所有 backup_* 备份（按时间排序）
std::vector<fs::path> listBackups(const fs::path &destination);

// 从备份名解析时间戳
std::time_t backupTimestamp(const fs::path &path);

} // namespace backup
