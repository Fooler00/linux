// =====================================================================
//  UserManager.cpp - 用户管理实现
// =====================================================================

#include "core/UserManager.h"
#include "core/Utils.h"

#include <stdexcept>

namespace backup {

UserManager::UserManager(const std::string &dbPath)
{
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK)
    {
        throw std::runtime_error("无法打开数据库");
    }

    exec("CREATE TABLE IF NOT EXISTS users ("
         "id INTEGER PRIMARY KEY AUTOINCREMENT,"
         "username TEXT NOT NULL UNIQUE,"
         "password_hash TEXT NOT NULL,"
         "password_salt TEXT NOT NULL,"
         "created_at TEXT NOT NULL"
         ");");
}

UserManager::~UserManager()
{
    if (db_)
    {
        sqlite3_close(db_);
    }
}

User UserManager::registerUser(const std::string &username, const std::string &password)
{
    if (username.empty() || password.empty())
    {
        throw std::runtime_error("用户名和密码不能为空");
    }

    std::string salt = randomHex(16);
    std::string hash = hashPassword(password, salt);

    sqlite3_stmt *stmt = nullptr;
    const char *sql =
        "INSERT INTO users (username, password_hash, password_salt, created_at) "
        "VALUES (?, ?, ?, ?);";

    prepare(sql, &stmt);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_TRANSIENT);
    std::string createdAt = nowString();
    sqlite3_bind_text(stmt, 4, createdAt.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        throw std::runtime_error("注册失败，用户名可能已存在");
    }

    sqlite3_finalize(stmt);

    return authenticate(username, password);
}

User UserManager::authenticate(const std::string &username, const std::string &password)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql =
        "SELECT id, username, password_hash, password_salt "
        "FROM users WHERE username = ?;";

    prepare(sql, &stmt);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        throw std::runtime_error("用户名或密码错误");
    }

    int id = sqlite3_column_int(stmt, 0);
    std::string dbUsername = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    std::string passwordHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    std::string salt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));

    sqlite3_finalize(stmt);

    if (hashPassword(password, salt) != passwordHash)
    {
        throw std::runtime_error("用户名或密码错误");
    }

    return {id, dbUsername};
}

void UserManager::exec(const std::string &sql)
{
    char *err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK)
    {
        std::string message = err ? err : "数据库执行失败";
        sqlite3_free(err);
        throw std::runtime_error(message);
    }
}

void UserManager::prepare(const char *sql, sqlite3_stmt **stmt)
{
    if (sqlite3_prepare_v2(db_, sql, -1, stmt, nullptr) != SQLITE_OK)
    {
        throw std::runtime_error(sqlite3_errmsg(db_));
    }
}

} // namespace backup
