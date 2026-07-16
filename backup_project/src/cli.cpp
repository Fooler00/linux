// =====================================================================
//  cli.cpp - 本地前端示例（直接调用核心库，不依赖 HTTP）
//
//  用法：
//    backup_cli backup   <源目录> <备份目录> [--encrypt 密码] [--algo zip|tar|tar.gz]
//    backup_cli restore  <备份路径> <还原目录> [--password 密码]
//    backup_cli list     <备份目录>
//    backup_cli prune    <备份目录> --max-backups N --max-age-days D
//
//  这是"本地前端可直接调用"的演示：只链接 backup_core，无需启动服务。
// =====================================================================

#include "core/Archive.h"
#include "core/BackupEngine.h"
#include "core/Crypto.h"
#include "core/Scheduler.h"
#include "core/Types.h"
#include "core/Utils.h"

#include <iostream>
#include <string>

namespace fs = backup::fs;

static void usage()
{
    std::cout << "用法:\n"
              << "  backup_cli backup  <源目录> <备份目录> [--encrypt 密码] [--algo zip|tar|tar.gz|none]\n"
              << "  backup_cli restore <备份路径> <还原目录> [--password 密码] [--encrypt-algo aes-256-cbc]\n"
              << "  backup_cli list    <备份目录>\n"
              << "  backup_cli prune   <备份目录> --max-backups N --max-age-days D\n"
              << "    max-backups: -1=不限, 0=全部删除, N>0=保留最新 N 个；max-age-days: 0=不限\n";
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage();
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "backup")
    {
        if (argc < 4)
        {
            usage();
            return 1;
        }

        backup::BackupFilter filter;
        fs::path source = argv[2];
        fs::path destination = argv[3];
        bool encrypt = false;
        std::string password;

        for (int i = 4; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--encrypt" && i + 1 < argc)
            {
                encrypt = true;
                password = argv[++i];
            }
            else if (arg == "--algo" && i + 1 < argc)
            {
                filter.archiveType = argv[++i];
            }
        }

        try
        {
            fs::path result = backup::createBackup(source, destination, true, encrypt, password, filter);
            std::cout << "备份完成: " << result << "\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "备份失败: " << e.what() << "\n";
            return 1;
        }
    }
    else if (cmd == "restore")
    {
        if (argc < 4)
        {
            usage();
            return 1;
        }

        fs::path backupPath = argv[2];
        fs::path destination = argv[3];
        std::string password;
        std::string algo = "aes-256-cbc";

        for (int i = 4; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--password" && i + 1 < argc)
            {
                password = argv[++i];
            }
            else if (arg == "--encrypt-algo" && i + 1 < argc)
            {
                algo = argv[++i];
            }
        }

        try
        {
            backup::restoreBackup(backupPath, destination, password, algo);
            std::cout << "还原完成\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "还原失败: " << e.what() << "\n";
            return 1;
        }
    }
    else if (cmd == "list")
    {
        fs::path destination = argv[2];
        try
        {
            auto backups = backup::listBackups(destination);
            for (const auto &p : backups)
            {
                std::cout << p.filename().string() << "  (timestamp=" << backup::backupTimestamp(p) << ")\n";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "查询失败: " << e.what() << "\n";
            return 1;
        }
    }
    else if (cmd == "prune")
    {
        fs::path destination = argv[2];
        int maxBackups = -1; // 未指定时不按数量淘汰
        int maxAgeDays = 0;
        for (int i = 3; i < argc; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--max-backups" && i + 1 < argc)
                maxBackups = std::atoi(argv[++i]);
            else if (arg == "--max-age-days" && i + 1 < argc)
                maxAgeDays = std::atoi(argv[++i]);
        }
        try
        {
            int removed = backup::pruneBackups(destination, maxBackups, maxAgeDays);
            std::cout << "已淘汰 " << removed << " 个旧备份\n";
        }
        catch (const std::exception &e)
        {
            std::cerr << "淘汰失败: " << e.what() << "\n";
            return 1;
        }
    }
    else
    {
        usage();
        return 1;
    }

    return 0;
}
