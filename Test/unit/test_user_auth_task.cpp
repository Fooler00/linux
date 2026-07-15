#include <gtest/gtest.h>

#include "core/AuthManager.h"
#include "core/TaskManager.h"
#include "core/UserManager.h"
#include "test_utils.h"

using namespace backup;

TEST(UserAuthTaskUnit, UserManager_RegisterAndAuthenticateWithTempSqlite)
{
    auto dir = test_support::caseDir("user_register_login");
    UserManager users((dir / "users.db").string());
    auto user = users.registerUser("alice", "password");
    EXPECT_GT(user.id, 0);
    EXPECT_EQ(user.username, "alice");
    auto loggedIn = users.authenticate("alice", "password");
    EXPECT_EQ(loggedIn.id, user.id);
}

TEST(UserAuthTaskUnit, UserManager_RejectsDuplicateUser)
{
    auto dir = test_support::caseDir("user_duplicate");
    UserManager users((dir / "users.db").string());
    users.registerUser("alice", "password");
    EXPECT_THROW(users.registerUser("alice", "password2"), std::runtime_error);
}

TEST(UserAuthTaskUnit, UserManager_RejectsWrongPassword)
{
    auto dir = test_support::caseDir("user_wrong_password");
    UserManager users((dir / "users.db").string());
    users.registerUser("bob", "password");
    EXPECT_THROW(users.authenticate("bob", "wrong"), std::runtime_error);
}

TEST(UserAuthTaskUnit, AuthManager_TokenReturnsOriginalUser)
{
    User user{77, "token-user"};
    auto token = AuthManager::instance().createTokenForUser(user);
    ASSERT_FALSE(token.empty());
    auto resolved = AuthManager::instance().getUserByToken(token);
    EXPECT_EQ(resolved.id, user.id);
    EXPECT_EQ(resolved.username, user.username);
}

TEST(UserAuthTaskUnit, AuthManager_UnknownTokenReturnsEmptyUser)
{
    auto resolved = AuthManager::instance().getUserByToken("missing-token");
    EXPECT_EQ(resolved.id, 0);
    EXPECT_TRUE(resolved.username.empty());
}

TEST(UserAuthTaskUnit, TaskManager_AddUpdateAndFilterByUser)
{
    User alice{101, "alice"};
    User bob{102, "bob"};
    int aliceTask = TaskManager::instance().add("backup", "/src", "/dst", alice);
    int bobTask = TaskManager::instance().add("restore", "/b", "/r", bob);
    TaskManager::instance().update(aliceTask, "success", "done");

    auto all = TaskManager::instance().toJson();
    EXPECT_GE(all.size(), 2U);
    auto aliceOnly = TaskManager::instance().toJsonForUser(alice);
    bool foundAlice = false;
    bool foundBob = false;
    for (const auto &item : aliceOnly)
    {
        if (item.at("id").get<int>() == aliceTask)
        {
            foundAlice = true;
            EXPECT_EQ(item.at("status").get<std::string>(), "success");
        }
        if (item.at("id").get<int>() == bobTask)
        {
            foundBob = true;
        }
    }
    EXPECT_TRUE(foundAlice);
    EXPECT_FALSE(foundBob);
}
