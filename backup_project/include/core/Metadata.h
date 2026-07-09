#pragma once

// =====================================================================
//  Metadata.h - 文件元数据采集/恢复 + 特殊文件支持
//  支持符号链接/管道/设备文件，保留属主/权限/时间
// =====================================================================

#include "core/Types.h"

namespace backup {

namespace fs = std::filesystem;

// 通过 lstat 获取文件元数据（不跟随符号链接）
bool getFileMetadata(const fs::path &path, FileMetadata &meta);

// 将元数据应用到目标文件（属主/属组/权限/时间）
void applyFileMetadata(const fs::path &path, const FileMetadata &meta);

// 创建特殊文件（符号链接/管道/设备），成功返回 true
bool createSpecialFile(const fs::path &dst, const FileMetadata &meta, std::string &reason);

// 递归复制目录树，正确处理符号链接/管道/设备并保留元数据
void copyTreeWithSpecial(const fs::path &src, const fs::path &dst);

} // namespace backup
