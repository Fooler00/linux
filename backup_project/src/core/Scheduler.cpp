// =====================================================================
//  Scheduler.cpp - 定时备份调度器 + 数据淘汰实现
// =====================================================================

#include "core/Scheduler.h"
#include "core/BackupEngine.h"
#include "core/TaskManager.h"
#include "core/Utils.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

namespace backup {

int pruneBackups(const fs::path &destination, int maxBackups, int maxAgeDays)
{
    auto backups = listBackups(destination);
    int removed = 0;
    auto now = std::time(nullptr);

    // 按数量淘汰：-1=不限，0=全部删除，N>0=只保留最新 N 个
    if (maxBackups >= 0)
    {
        int toRemove = static_cast<int>(backups.size()) - maxBackups;
        if (toRemove > 0)
        {
            for (int i = 0; i < toRemove; ++i)
            {
                std::error_code ec;
                fs::remove_all(backups[i], ec);
                if (!ec)
                {
                    removed++;
                }
            }
            backups.erase(backups.begin(), backups.begin() + toRemove);
        }
    }

    // 按天数淘汰
    if (maxAgeDays > 0)
    {
        for (const auto &p : backups)
        {
            std::time_t ts = backupTimestamp(p);
            if (ts == 0)
            {
                continue;
            }
            double ageDays = std::difftime(now, ts) / 86400.0;
            if (ageDays > maxAgeDays)
            {
                std::error_code ec;
                fs::remove_all(p, ec);
                if (!ec)
                {
                    removed++;
                }
            }
        }
    }

    return removed;
}

Scheduler &Scheduler::instance()
{
    static Scheduler inst;
    return inst;
}

Scheduler::~Scheduler()
{
    stopAll();
}

int Scheduler::start(ScheduleConfig config)
{
    if (config.intervalSeconds <= 0)
    {
        throw std::runtime_error("定时间隔必须大于 0");
    }

    int id;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        id = nextId_++;
        config.id = id;
        schedules_[id] = config;
    }

    running_ = true;

    threads_[id] = std::thread([this, config]()
                               {
        ScheduleConfig cfg = config;
        // 启动后先备份一次，再按间隔等待，避免首次要空等一整周期。
        while (running_) {
            // 检查调度是否仍存在（可能已被删除）
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (schedules_.find(cfg.id) == schedules_.end()) {
                    break;
                }
            }

            int taskId = TaskManager::instance().add(
                "scheduled-backup", cfg.source.string(), cfg.destination.string(), cfg.user);
            try {
                fs::path path = createBackup(cfg.source, cfg.destination, cfg.compress,
                                             cfg.encrypt, cfg.password, cfg.filter);
                TaskManager::instance().update(taskId, "success", "定时备份完成：" + path.string());

                // 数据淘汰：maxBackups>=0 或 maxAgeDays>0 时执行
                if (cfg.maxBackups >= 0 || cfg.maxAgeDays > 0) {
                    int removed = pruneBackups(cfg.destination, cfg.maxBackups, cfg.maxAgeDays);
                    if (removed > 0) {
                        TaskManager::instance().update(
                            taskId, "success",
                            "定时备份完成：" + path.string() + "；淘汰旧备份 " + std::to_string(removed) + " 个");
                    }
                }
            } catch (const std::exception& e) {
                TaskManager::instance().update(taskId, "failed", e.what());
            }

            for (int i = 0; i < cfg.intervalSeconds && running_; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } });

    threads_[id].detach();

    return id;
}

bool Scheduler::stop(int id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = schedules_.find(id);
    if (it == schedules_.end())
    {
        return false;
    }
    schedules_.erase(it);
    // 线程会在下次循环检测到调度不存在后退出
    return true;
}

nlohmann::json Scheduler::toJson() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    nlohmann::json arr = nlohmann::json::array();
    for (const auto &kv : schedules_)
    {
        const auto &s = kv.second;
        arr.push_back({{"id", s.id},
                       {"source", s.source.string()},
                       {"destination", s.destination.string()},
                       {"intervalSeconds", s.intervalSeconds},
                       {"maxBackups", s.maxBackups},
                       {"maxAgeDays", s.maxAgeDays},
                       {"compress", s.compress},
                       {"encrypt", s.encrypt},
                       {"username", s.user.username}});
    }
    return arr;
}

void Scheduler::stopAll()
{
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
    schedules_.clear();
}

} // namespace backup
