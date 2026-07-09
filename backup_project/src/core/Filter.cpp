// =====================================================================
//  Filter.cpp - 备份文件筛选实现
// =====================================================================

#include "core/Filter.h"
#include "core/Metadata.h"
#include "core/Utils.h"

#include <string>

namespace backup {

namespace {

bool containsAnyPathPart(const std::string &path, const std::vector<std::string> &patterns)
{
    if (patterns.empty())
    {
        return true;
    }

    for (const auto &pattern : patterns)
    {
        if (!pattern.empty() && path.find(pattern) != std::string::npos)
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
        if (!pattern.empty() && path.find(pattern) != std::string::npos)
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
    fs::path relativePath = fs::relative(entry.path(), source);
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

    bool isRegular = entry.is_regular_file();
    bool isSymlink = entry.is_symlink();

    if (!isRegular && !isSymlink)
    {
        // 非常规文件：根据 includeSpecialFiles 决定是否纳入
        if (!filter.includeSpecialFiles)
        {
            return false;
        }
        // 特殊文件不参与尺寸/扩展名/时间筛选
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

    if (isRegular)
    {
        auto size = entry.file_size();
        if (size < filter.minSize || size > filter.maxSize)
        {
            return false;
        }

        auto modified = fs::last_write_time(entry.path());
        if (modified < filter.modifiedAfter || modified > filter.modifiedBefore)
        {
            return false;
        }
    }

    return true;
}

} // namespace backup
