#pragma once

// =====================================================================
//  AuthManager.h - 会话令牌管理（Token / Bearer）
// =====================================================================

#include "core/Types.h"
#include <map>
#include <mutex>
#include <string>

namespace backup {

class AuthManager
{
public:
    static AuthManager &instance();

    // 为用户创建令牌
    std::string createTokenForUser(const User &user);

    // 通过令牌获取用户，失败返回 id=0
    User getUserByToken(const std::string &token);

private:
    AuthManager() = default;

    std::mutex mutex_;
    std::map<std::string, User> tokens_;
};

} // namespace backup
