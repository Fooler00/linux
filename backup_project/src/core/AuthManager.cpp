// =====================================================================
//  AuthManager.cpp - 会话令牌管理实现
// =====================================================================

#include "core/AuthManager.h"
#include "core/Utils.h"

namespace backup {

AuthManager &AuthManager::instance()
{
    static AuthManager inst;
    return inst;
}

std::string AuthManager::createTokenForUser(const User &user)
{
    std::string token = randomHex(32);

    std::lock_guard<std::mutex> lock(mutex_);
    tokens_[token] = user;

    return token;
}

User AuthManager::getUserByToken(const std::string &token)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tokens_.find(token);
    if (it == tokens_.end())
    {
        return {};
    }
    return it->second;
}

} // namespace backup
