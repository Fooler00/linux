// =====================================================================
//  TaskManager.cpp - 备份任务追踪实现
// =====================================================================

#include "core/TaskManager.h"
#include "core/Utils.h"

namespace backup {

TaskManager &TaskManager::instance()
{
    static TaskManager inst;
    return inst;
}

int TaskManager::add(const std::string &type, const std::string &source,
                     const std::string &destination, const User &user)
{
    std::lock_guard<std::mutex> lock(mutex_);

    Task task;
    task.id = nextId_++;
    task.userId = user.id;
    task.username = user.username;
    task.type = type;
    task.source = source;
    task.destination = destination;
    task.status = "running";
    task.createdAt = nowString();

    tasks_.push_back(task);
    return task.id;
}

void TaskManager::update(int id, const std::string &status, const std::string &message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto &task : tasks_)
    {
        if (task.id == id)
        {
            task.status = status;
            task.message = message;
            return;
        }
    }
}

nlohmann::json TaskManager::toJson() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &t : tasks_)
    {
        arr.push_back({{"id", t.id},
                       {"userId", t.userId},
                       {"username", t.username},
                       {"type", t.type},
                       {"source", t.source},
                       {"destination", t.destination},
                       {"status", t.status},
                       {"message", t.message},
                       {"createdAt", t.createdAt}});
    }
    return arr;
}

nlohmann::json TaskManager::toJsonForUser(const User &user) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &t : tasks_)
    {
        if (t.userId == user.id)
        {
            arr.push_back({{"id", t.id},
                           {"userId", t.userId},
                           {"username", t.username},
                           {"type", t.type},
                           {"source", t.source},
                           {"destination", t.destination},
                           {"status", t.status},
                           {"message", t.message},
                           {"createdAt", t.createdAt}});
        }
    }
    return arr;
}

std::vector<Task> TaskManager::all() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_;
}

} // namespace backup
