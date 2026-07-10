#pragma once

// =====================================================================
//  UserManager.h - 用户管理（基于 SQLite，网盘模式用户体系）
// =====================================================================

#include "core/Types.h"
#include <sqlite3.h>
#include <string>

namespace backup {

class UserManager
{
public:
    explicit UserManager(const std::string &dbPath = "users.db");
    ~UserManager();

    UserManager(const UserManager &) = delete;
    UserManager &operator=(const UserManager &) = delete;

    // 注册用户
    User registerUser(const std::string &username, const std::string &password);

    // 用户认证
    User authenticate(const std::string &username, const std::string &password);

private:
    sqlite3 *db_ = nullptr;

    void exec(const std::string &sql);
    void prepare(const char *sql, sqlite3_stmt **stmt);
};

} // namespace backup
