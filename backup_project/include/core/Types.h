#pragma once

// =====================================================================
//  Types.h - 核心数据类型定义
//  整个备份系统的公共数据结构，被 core / cloud / server 各层共享
// =====================================================================

#include <cstdint>
#include <ctime>
#include <filesystem>
#include <limits>
#include <string>
#include <sys/types.h>
#include <vector>

namespace backup {

// 文件系统命名空间别名（全工程统一使用 fs::）
namespace fs = std::filesystem;

// ---------------- 用户 ----------------
struct User
{
    int id = 0;
    std::string username;
};

// ---------------- 任务 ----------------
struct Task
{
    int id = 0;
    int userId = 0;
    std::string username;
    std::string type;        // backup | restore | realtime-backup | scheduled-backup
    std::string source;
    std::string destination;
    std::string status;      // running | success | failed
    std::string message;
    std::string createdAt;
};

// ---------------- 文件元数据 ----------------
// 属主/属组/权限/时间/类型/链接目标，用于元数据保留与特殊文件支持
struct FileMetadata
{
    mode_t mode = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    time_t mtime = 0;
    time_t atime = 0;
    std::string ownerName;
    std::string groupName;
    std::string symlinkTarget;
    bool isSymlink = false;
    bool isPipe = false;
    bool isBlockDevice = false;
    bool isCharDevice = false;
    bool isSocket = false;
    dev_t rdev = 0;       // 设备文件主从设备号
    ino_t inode = 0;      // inode 号，用于硬链接识别
    nlink_t nlink = 0;    // 硬链接计数（>1 表示存在硬链接）
};

// ---------------- 备份过滤条件 ----------------
// 支持按路径/类型/名字/时间/尺寸/用户筛选
struct BackupFilter
{
    std::vector<std::string> includePaths;
    std::vector<std::string> excludePaths;
    std::vector<std::string> extensions;
    std::string fileNameContains;
    std::uintmax_t minSize = 0;
    std::uintmax_t maxSize = std::numeric_limits<std::uintmax_t>::max();
    fs::file_time_type modifiedAfter = fs::file_time_type::min();
    fs::file_time_type modifiedBefore = fs::file_time_type::max();
    int userId = 0;
    std::string username;

    // 用户筛选（按文件属主 uid/gid/名称）
    int filterUid = -1;          // -1 表示不筛选
    int filterGid = -1;
    std::string filterOwnerName;
    std::string filterGroupName;

    // 元数据/特殊文件开关
    bool preserveMetadata = true;
    bool includeSpecialFiles = true;

    // 归档与加密算法
    std::string archiveType = "zip";   // none | zip | tar | tar.gz
    std::string encryptAlgo = "aes-256-cbc";

    // 增量备份
    bool incremental = false;
    std::string incrementalBase;       // 上一次完整备份目录路径
};

// ---------------- 定时备份调度配置（含数据淘汰策略）----------------
struct ScheduleConfig
{
    int id = 0;
    fs::path source;
    fs::path destination;
    int intervalSeconds = 3600;
    int maxBackups = 0;       // 0 表示不限
    int maxAgeDays = 0;       // 0 表示不限
    bool compress = false;
    bool encrypt = false;
    std::string password;
    BackupFilter filter;
    User user;
};

} // namespace backup
