#pragma once

// =====================================================================
//  Filter.h - 备份文件筛选
// =====================================================================

#include "core/Types.h"
#include <nlohmann/json.hpp>

namespace backup {

namespace fs = std::filesystem;

// 从 JSON 请求体解析筛选条件
BackupFilter parseBackupFilter(const nlohmann::json &body, const User &user);

// 判断目录项是否匹配筛选条件
bool fileMatchesFilter(const fs::directory_entry &entry, const fs::path &source, const BackupFilter &filter);

} // namespace backup
