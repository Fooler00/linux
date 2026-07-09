// =====================================================================
//  Metadata.cpp - 文件元数据采集/恢复 + 特殊文件支持实现
// =====================================================================

#include "core/Metadata.h"

#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include <filesystem>
#include <iostream>

namespace backup {

bool getFileMetadata(const fs::path &path, FileMetadata &meta)
{
    struct stat st{};
    if (lstat(path.c_str(), &st) != 0)
    {
        return false;
    }

    meta.mode = st.st_mode;
    meta.uid = st.st_uid;
    meta.gid = st.st_gid;
    meta.mtime = st.st_mtime;
    meta.atime = st.st_atime;
    meta.rdev = st.st_rdev;

    if (S_ISLNK(st.st_mode))
    {
        meta.isSymlink = true;
        std::error_code ec;
        meta.symlinkTarget = fs::read_symlink(path, ec).string();
    }
    else if (S_ISFIFO(st.st_mode))
    {
        meta.isPipe = true;
    }
    else if (S_ISBLK(st.st_mode))
    {
        meta.isBlockDevice = true;
    }
    else if (S_ISCHR(st.st_mode))
    {
        meta.isCharDevice = true;
    }
    else if (S_ISSOCK(st.st_mode))
    {
        meta.isSocket = true;
    }

    if (auto *pw = getpwuid(st.st_uid))
    {
        meta.ownerName = pw->pw_name;
    }
    if (auto *gr = getgrgid(st.st_gid))
    {
        meta.groupName = gr->gr_name;
    }

    return true;
}

void applyFileMetadata(const fs::path &path, const FileMetadata &meta)
{
    chmod(path.c_str(), meta.mode);
    // 仅 root 或具有 CAP_CHOWN 才能成功改变属主；普通用户调用失败可忽略
    chown(path.c_str(), meta.uid, meta.gid);

    struct utimbuf times{};
    times.actime = meta.atime;
    times.modtime = meta.mtime;
    utime(path.c_str(), &times);
}

bool createSpecialFile(const fs::path &dst, const FileMetadata &meta, std::string &reason)
{
    fs::create_directories(dst.parent_path());

    if (meta.isSymlink)
    {
        std::error_code ec;
        fs::remove(dst, ec);
        fs::create_symlink(meta.symlinkTarget, dst, ec);
        if (ec)
        {
            reason = "创建符号链接失败: " + ec.message();
            return false;
        }
        return true;
    }

    if (meta.isPipe)
    {
        if (mkfifo(dst.c_str(), meta.mode) != 0)
        {
            reason = "创建管道失败";
            return false;
        }
        return true;
    }

    if (meta.isBlockDevice || meta.isCharDevice)
    {
        if (mknod(dst.c_str(), meta.mode, meta.rdev) != 0)
        {
            reason = "创建设备文件失败（需要 root 权限）";
            return false;
        }
        return true;
    }

    if (meta.isSocket)
    {
        reason = "套接字无法备份";
        return false;
    }

    reason = "未知特殊文件类型";
    return false;
}

void copyTreeWithSpecial(const fs::path &src, const fs::path &dst)
{
    fs::create_directories(dst);

    for (const auto &entry : fs::directory_iterator(src))
    {
        fs::path target = dst / entry.path().filename();

        FileMetadata meta;
        bool hasMeta = getFileMetadata(entry.path(), meta);

        if (fs::is_directory(entry.path()))
        {
            copyTreeWithSpecial(entry.path(), target);
        }
        else if (entry.is_symlink())
        {
            std::string reason;
            createSpecialFile(target, meta, reason);
        }
        else if (hasMeta && (meta.isPipe || meta.isBlockDevice || meta.isCharDevice))
        {
            std::string reason;
            if (!createSpecialFile(target, meta, reason))
            {
                std::cerr << "[警告] 还原时跳过特殊文件 " << entry.path() << ": " << reason << "\n";
            }
        }
        else if (entry.is_regular_file())
        {
            fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing);
        }

        if (hasMeta)
        {
            applyFileMetadata(target, meta);
        }
    }
}

} // namespace backup
