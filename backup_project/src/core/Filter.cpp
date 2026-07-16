// =====================================================================
//  Filter.cpp - 备份文件筛选实现
// =====================================================================
 
#include "core/Filter.h"
#include "core/Metadata.h"
#include "core/Utils.h"
 
#include <fnmatch.h>
#include <string>
#include <sys/stat.h>
 
namespace backup {
 
namespace {
 
// 判定 pattern 是否包含通配符（* ? [）
bool hasWildcard(const std::string &pattern)
{
    return pattern.find('*') != std::string::npos ||
           pattern.find('?') != std::string::npos ||
           pattern.find('[') != std::string::npos;
}
 
// 通配符匹配：若 pattern 不含通配符则退化为子串匹配，向后兼容
bool patternMatches(const std::string &text, const std::string &pattern)
{
    if (pattern.empty())
    {
        return true;
    }
    if (!hasWildcard(pattern))
    {
        // 兼容旧行为：子串包含
        return text.find(pattern) != std::string::npos;
    }
    // 使用 POSIX fnmatch：
    //  FNM_PATHNAME: * 不匹配 /，避免子目录被无意匹配
    //  FNM_PERIOD:  以 . 开头的文件名需显式匹配
    return fnmatch(pattern.c_str(), text.c_str(), FNM_PATHNAME) == 0;
}
 
bool containsAnyPathPart(const std::string &path, const std::vector<std::string> &patterns)
{
    if (patterns.empty())
    {
        return true;
    }
 
    for (const auto &pattern : patterns)
    {
        if (patternMatches(path, pattern))
        {
            return true;
        }
    }
 
    return false;
}
 
bool containsExcludedPathPart(const std::string &path, const std::vector<std::string> &patterns)
{
    for (const auto &pattern : patterns)
    {
        if (patternMatches(path, pattern))
        {
            return true;
        }
    }
 
    return false;
}
 
bool extensionMatches(const fs::path &path, const std::vector<std::string> &extensions)
{
    if (extensions.empty())
    {
        return true;
    }
 
    auto ext = path.extension().string();
    for (const auto &allowed : extensions)
    {
        if (ext == allowed)
        {
            return true;
        }
    }
 
    return false;
}
 
} // namespace
 
BackupFilter parseBackupFilter(const nlohmann::json &body, const User &user)
{
    BackupFilter filter;
    filter.userId = user.id;
    filter.username = user.username;
 
    if (!body.contains("filter") || body["filter"].is_null())
    {
        filter.archiveType = body.value("archiveType", "zip");
        filter.encryptAlgo = body.value("encryptAlgo", "aes-256-cbc");
        filter.preserveMetadata = body.value("preserveMetadata", true);
        filter.includeSpecialFiles = body.value("includeSpecialFiles", true);
        filter.incremental = body.value("incremental", false);
        filter.incrementalBase = body.value("incrementalBase", "");
        return filter;
    }
 
    const auto &f = body["filter"];
 
    if (f.contains("includePaths") && f["includePaths"].is_array())
    {
        filter.includePaths = f["includePaths"].get<std::vector<std::string>>();
    }
 
    if (f.contains("excludePaths") && f["excludePaths"].is_array())
    {
        filter.excludePaths = f["excludePaths"].get<std::vector<std::string>>();
    }
 
    if (f.contains("extensions") && f["extensions"].is_array())
    {
        filter.extensions = f["extensions"].get<std::vector<std::string>>();
        for (auto &ext : filter.extensions)
        {
            if (!ext.empty() && ext[0] != '.')
            {
                ext = "." + ext;
            }
        }
    }
 
    filter.fileNameContains = f.value("fileNameContains", f.value("name", ""));
 
    if (f.contains("minSize"))
    {
        filter.minSize = f["minSize"].get<std::uintmax_t>();
    }
 
    if (f.contains("maxSize"))
    {
        filter.maxSize = f["maxSize"].get<std::uintmax_t>();
    }
 
    std::chrono::system_clock::time_point tp;
    if (parseTimeString(f.value("modifiedAfter", ""), tp))
    {
        filter.modifiedAfter = systemTimeToFileTime(tp);
    }
 
    if (parseTimeString(f.value("modifiedBefore", ""), tp))
    {
        filter.modifiedBefore = systemTimeToFileTime(tp);
    }
 
    // 用户筛选（属主 uid/gid/名称）
    filter.filterUid = f.value("uid", -1);
    filter.filterGid = f.value("gid", -1);
    filter.filterOwnerName = f.value("owner", "");
    filter.filterGroupName = f.value("group", "");
 
    // 元数据与特殊文件开关
    filter.preserveMetadata = f.value("preserveMetadata", true);
    filter.includeSpecialFiles = f.value("includeSpecialFiles", true);
 
    // 归档/加密算法（也可从顶层读取，便于旧前端使用）
    filter.archiveType = body.value("archiveType", f.value("archiveType", "zip"));
    filter.encryptAlgo = body.value("encryptAlgo", f.value("encryptAlgo", "aes-256-cbc"));
 
    // 增量备份
    filter.incremental = body.value("incremental", f.value("incremental", false));
    filter.incrementalBase = body.value("incrementalBase", f.value("incrementalBase", ""));
 
    return filter;
}
 
bool fileMatchesFilter(const fs::directory_entry &entry, const fs::path &source, const BackupFilter &filter)
{
    // 用 lexically_relative 而非 fs::relative，避免跟随符号链接导致路径解析错误
    fs::path relativePath = entry.path().lexically_relative(source);
    std::string relative = relativePath.string();
 
    // 用户筛选：通过 lstat 获取属主/属组
    if (filter.filterUid != -1 || filter.filterGid != -1 ||
        !filter.filterOwnerName.empty() || !filter.filterGroupName.empty())
    {
        FileMetadata meta;
        if (!getFileMetadata(entry.path(), meta))
        {
            return false;
        }
        if (filter.filterUid != -1 && static_cast<int>(meta.uid) != filter.filterUid)
        {
            return false;
        }
        if (filter.filterGid != -1 && static_cast<int>(meta.gid) != filter.filterGid)
        {
            return false;
        }
        if (!filter.filterOwnerName.empty() && meta.ownerName != filter.filterOwnerName)
        {
            return false;
        }
        if (!filter.filterGroupName.empty() && meta.groupName != filter.filterGroupName)
        {
            return false;
        }
    }
 
    // 用 symlink_status 判断类型，避免跟随符号链接导致断链抛异常或类型误判
    std::error_code stEc;
    auto st = entry.symlink_status(stEc);
    if (stEc)
    {
        return false;
    }
    bool isSymlink = fs::is_symlink(st);
    bool isRegular = fs::is_regular_file(st);

    // 符号链接按特殊文件处理（不跟随）；尺寸/时间仍豁免，避免断链上
    // file_size/last_write_time 失败误排除。扩展名与路径/文件名筛选照常生效。
    if (isSymlink)
    {
        if (!filter.includeSpecialFiles)
        {
            return false;
        }
        if (!containsAnyPathPart(relative, filter.includePaths))
        {
            return false;
        }
        if (containsExcludedPathPart(relative, filter.excludePaths))
        {
            return false;
        }
        if (!filter.fileNameContains.empty() &&
            entry.path().filename().string().find(filter.fileNameContains) == std::string::npos)
        {
            return false;
        }
        if (!extensionMatches(entry.path(), filter.extensions))
        {
            return false;
        }
        return true;
    }

    if (!isRegular)
    {
        // 非常规文件：includeSpecialFiles 为类型开关；扩展名等高级筛选仍须满足。
        // 尺寸/时间豁免（FIFO/设备上 file_size 等不稳定）。
        if (!filter.includeSpecialFiles)
        {
            return false;
        }
        if (!containsAnyPathPart(relative, filter.includePaths))
        {
            return false;
        }
        if (containsExcludedPathPart(relative, filter.excludePaths))
        {
            return false;
        }
        if (!filter.fileNameContains.empty() &&
            entry.path().filename().string().find(filter.fileNameContains) == std::string::npos)
        {
            return false;
        }
        if (!extensionMatches(entry.path(), filter.extensions))
        {
            return false;
        }
        return true;
    }

    if (!containsAnyPathPart(relative, filter.includePaths))
    {
        return false;
    }

    if (containsExcludedPathPart(relative, filter.excludePaths))
    {
        return false;
    }

    if (!extensionMatches(entry.path(), filter.extensions))
    {
        return false;
    }

    if (!filter.fileNameContains.empty() &&
        entry.path().filename().string().find(filter.fileNameContains) == std::string::npos)
    {
        return false;
    }

    // 用 error_code 重载避免断链/权限问题抛异常
    std::error_code sizeEc;
    auto size = entry.file_size(sizeEc);
    if (sizeEc)
    {
        return false;
    }
    if (size < filter.minSize || size > filter.maxSize)
    {
        return false;
    }

    std::error_code timeEc;
    auto modified = fs::last_write_time(entry.path(), timeEc);
    if (timeEc)
    {
        return false;
    }
    if (modified < filter.modifiedAfter || modified > filter.modifiedBefore)
    {
        return false;
    }

    return true;
}
 
} // namespace backup