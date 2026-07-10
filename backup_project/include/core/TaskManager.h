#pragma once

// =====================================================================
//  TaskManager.h - 备份任务追踪
// =====================================================================

#include "core/Types.h"
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>
#include <vector>

namespace backup {

class TaskManager
{
public:
    static TaskManager &instance();

    // 创建任务（返回任务 ID）
    int add(const std::string &type, const std::string &source,
            const std::string &destination, const User &user);

    // 更新任务状态
    void update(int id, const std::string &status, const std::string &message);

    // 所有任务（管理员）
    nlohmann::json toJson() const;

    // 某用户的任务
    nlohmann::json toJsonForUser(const User &user) const;

    // 清理已完成且过期的任务（简单实现：返回所有，不做清理）
    std::vector<Task> all() const;

private:
    TaskManager() = default;

    mutable std::mutex mutex_;
    std::vector<Task> tasks_;
    std::atomic<int> nextId_{1};
};

} // namespace backup
