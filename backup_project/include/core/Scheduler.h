#pragma once

// =====================================================================
//  Scheduler.h - 定时备份调度器 + 数据淘汰
// =====================================================================

#include "core/Types.h"
#include <atomic>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>

namespace backup {

namespace fs = std::filesystem;

// 按保留策略删除旧备份
//   maxBackups  最多保留数量（0 不限）
//   maxAgeDays  最多保留天数（0 不限）
int pruneBackups(const fs::path &destination, int maxBackups, int maxAgeDays);

// 定时备份调度器（单例，管理所有调度任务的生命周期）
class Scheduler
{
public:
    static Scheduler &instance();

    // 启动一个定时备份调度，返回调度 ID
    int start(ScheduleConfig config);

    // 停止调度
    bool stop(int id);

    // 列出所有调度（JSON）
    nlohmann::json toJson() const;

    // 停止所有调度（程序退出时调用）
    void stopAll();

private:
    Scheduler() = default;
    ~Scheduler();

    mutable std::mutex mutex_;
    std::map<int, ScheduleConfig> schedules_;
    std::map<int, std::thread> threads_;
    std::atomic<bool> running_{false};
    std::atomic<int> nextId_{1};
};

} // namespace backup
