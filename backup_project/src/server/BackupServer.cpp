// =====================================================================
//  BackupServer.cpp - HTTP 服务层实现
// =====================================================================

#include "server/BackupServer.h"

#include "core/AuthManager.h"
#include "core/BackupEngine.h"
#include "core/Filter.h"
#include "core/Scheduler.h"
#include "core/TaskManager.h"
#include "core/UserManager.h"
#include "core/Utils.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace backup::server {

namespace fs = std::filesystem;
using json = nlohmann::json;

// ---------------- 私有实现 ----------------

struct BackupServer::Impl
{
    ServerConfig config;
    std::shared_ptr<cloud::ICloudStorage> storage;
    std::unique_ptr<UserManager> userManager;

    // 实时备份监听状态（stop 通过 CV 打断 wait，避免 sleep 醒来后仍备份）
    std::atomic<bool> watching{false};
    std::mutex watchMutex;
    std::condition_variable watchCv;
    std::thread watchThread;

    // 已注册路由的服务器实例（run() 时填充）
    std::unique_ptr<httplib::Server> httpServer;
};

namespace {

// 从请求中提取 Bearer 令牌
std::string getBearerToken(const httplib::Request &req)
{
    auto auth = req.get_header_value("Authorization");
    const std::string prefix = "Bearer ";
    if (auth.rfind(prefix, 0) == 0)
    {
        return auth.substr(prefix.size());
    }
    return "";
}

// 从请求或 body 中解析可选用户
User optionalUserFromRequest(const httplib::Request &req, const json *body = nullptr)
{
    std::string token = getBearerToken(req);
    if (token.empty() && body)
    {
        token = body->value("token", "");
    }
    return AuthManager::instance().getUserByToken(token);
}

// 文件快照（实时备份变化检测）
std::map<std::string, fs::file_time_type> snapshotFiles(const fs::path &root)
{
    std::map<std::string, fs::file_time_type> files;
    if (!fs::exists(root))
    {
        return files;
    }
    for (const auto &entry : fs::recursive_directory_iterator(root))
    {
        if (entry.is_regular_file())
        {
            files[entry.path().string()] = fs::last_write_time(entry.path());
        }
    }
    return files;
}

std::vector<std::string> parseSourceList(const json &body)
{
    std::vector<std::string> sources;
    if (!body.contains("sources") || !body["sources"].is_array())
    {
        return sources;
    }

    for (const auto &item : body["sources"])
    {
        if (!item.is_string())
        {
            continue;
        }

        std::string path = item.get<std::string>();
        if (path.empty())
        {
            continue;
        }

        if (std::find(sources.begin(), sources.end(), path) == sources.end())
        {
            sources.push_back(path);
        }
    }

    return sources;
}

std::string summarizeSources(const std::vector<std::string> &sources)
{
    if (sources.empty())
    {
        return "";
    }

    if (sources.size() == 1)
    {
        return sources.front();
    }

    std::string summary = std::to_string(sources.size()) + " files: " + sources[0];
    if (sources.size() > 1)
    {
        summary += ", " + sources[1];
    }
    if (sources.size() > 2)
    {
        summary += ", ...";
    }
    return summary;
}

} // namespace

// ---------------- 构造/析构 ----------------

BackupServer::BackupServer(ServerConfig config, std::shared_ptr<cloud::ICloudStorage> storage)
    : impl_(std::make_unique<Impl>())
{
    impl_->config = std::move(config);
    impl_->storage = std::move(storage);
    impl_->userManager = std::make_unique<UserManager>(impl_->config.dbPath);
}

BackupServer::~BackupServer()
{
    impl_->watching = false;
    impl_->watchCv.notify_all();
    if (impl_->watchThread.joinable())
    {
        impl_->watchThread.join();
    }
    Scheduler::instance().stopAll();
}

// ---------------- 路由注册 ----------------

void BackupServer::registerRoutes()
{
    // 选择底层服务器类型（HTTP 或 HTTPS）
    if (impl_->config.tls && !impl_->config.certPath.empty() && !impl_->config.keyPath.empty())
    {
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        impl_->httpServer = std::make_unique<httplib::SSLServer>(
            impl_->config.certPath.c_str(), impl_->config.keyPath.c_str());
#else
        std::cerr << "[警告] httplib 未启用 OpenSSL 支持，回退到 HTTP\n";
        impl_->httpServer = std::make_unique<httplib::Server>();
#endif
    }
    else
    {
        impl_->httpServer = std::make_unique<httplib::Server>();
    }

    auto &server = *impl_->httpServer;

    // CORS
    server.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                                {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                                {"Access-Control-Allow-Headers", "Content-Type, Authorization"}});
    server.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res)
                   { res.status = 204; });

    auto &userManager = *impl_->userManager;

    // ===== 用户注册 =====
    server.Post("/api/register", [&userManager](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            User user = userManager.registerUser(body.value("username", ""), body.value("password", ""));
            std::string token = AuthManager::instance().createTokenForUser(user);
            res.set_content(json{{"userId", user.id}, {"username", user.username}, {"token", token}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 用户登录 =====
    server.Post("/api/login", [&userManager](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            User user = userManager.authenticate(body.value("username", ""), body.value("password", ""));
            std::string token = AuthManager::instance().createTokenForUser(user);
            res.set_content(json{{"userId", user.id}, {"username", user.username}, {"token", token}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 401;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 创建备份 =====
    server.Post("/api/backup", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            std::string source = body.value("source", "");
            std::vector<std::string> sources = parseSourceList(body);
            std::string destination = body.value("destination", "");
            bool compress = body.value("compress", false);
            bool encrypt = body.value("encrypt", false);
            std::string password = body.value("password", "");

            if (destination.empty())
            {
                throw std::runtime_error("destination 不能为空");
            }

            const bool multiSourceMode = !sources.empty();
            if (!multiSourceMode && source.empty())
            {
                throw std::runtime_error("source 或 sources 必须提供其一");
            }

            User user = optionalUserFromRequest(req, &body);
            BackupFilter filter = parseBackupFilter(body, user);

            if (multiSourceMode && filter.incremental)
            {
                throw std::runtime_error("多文件备份不支持增量模式");
            }

            std::string taskSource = multiSourceMode ? summarizeSources(sources) : source;
            int taskId = TaskManager::instance().add("backup", taskSource, destination, user);

            if (multiSourceMode)
            {
                std::vector<fs::path> sourcePaths;
                sourcePaths.reserve(sources.size());
                for (const auto &item : sources)
                {
                    sourcePaths.emplace_back(item);
                }

                std::thread([taskId, sourcePaths, destination, compress, encrypt, password, filter]() {
                    try {
                        fs::path result = createBackupFromSources(
                            sourcePaths, destination, compress, encrypt, password, filter);
                        TaskManager::instance().update(taskId, "success", "备份完成：" + result.string());
                    } catch (const std::exception& e) {
                        TaskManager::instance().update(taskId, "failed", e.what());
                    }
                }).detach();
            }
            else
            {
                std::thread([taskId, source, destination, compress, encrypt, password, filter]() {
                    try {
                        fs::path result = createBackup(source, destination, compress, encrypt, password, filter);
                        TaskManager::instance().update(taskId, "success", "备份完成：" + result.string());
                    } catch (const std::exception& e) {
                        TaskManager::instance().update(taskId, "failed", e.what());
                    }
                }).detach();
            }

            res.set_content(json{{"taskId", taskId}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 还原备份 =====
    server.Post("/api/restore", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            std::string backupPath = body.value("backupPath", "");
            std::string destination = body.value("destination", "");
            std::string password = body.value("password", "");
            std::string encryptAlgo = body.value("encryptAlgo", "aes-256-cbc");

            User user = optionalUserFromRequest(req, &body);
            int taskId = TaskManager::instance().add("restore", backupPath, destination, user);

            std::thread([taskId, backupPath, destination, password, encryptAlgo]() {
                try {
                    restoreBackup(backupPath, destination, password, encryptAlgo);
                    TaskManager::instance().update(taskId, "success", "还原完成");
                } catch (const std::exception& e) {
                    TaskManager::instance().update(taskId, "failed", e.what());
                }
            }).detach();

            res.set_content(json{{"taskId", taskId}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 定时备份调度 =====
    server.Post("/api/schedule/start", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            ScheduleConfig cfg;
            cfg.source = body.value("source", "");
            cfg.destination = body.value("destination", "");
            cfg.intervalSeconds = body.value("intervalSeconds", 3600);
            cfg.maxBackups = body.value("maxBackups", -1);
            cfg.maxAgeDays = body.value("maxAgeDays", 0);
            cfg.compress = body.value("compress", false);
            cfg.encrypt = body.value("encrypt", false);
            cfg.password = body.value("password", "");
            cfg.user = optionalUserFromRequest(req, &body);
            cfg.filter = parseBackupFilter(body, cfg.user);

            int id = Scheduler::instance().start(cfg);
            res.set_content(json{{"scheduleId", id}, {"message", "定时备份已启动"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server.Post("/api/schedule/stop", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            int id = body.value("scheduleId", 0);
            bool ok = Scheduler::instance().stop(id);
            res.set_content(json{{"success", ok}, {"message", ok ? "定时备份已停止" : "调度不存在"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server.Get("/api/schedules", [](const httplib::Request &, httplib::Response &res)
               { res.set_content(Scheduler::instance().toJson().dump(), "application/json"); });

    // ===== 数据淘汰（手动）=====
    server.Post("/api/prune", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            std::string destination = body.value("destination", "");
            int maxBackups = body.value("maxBackups", -1);
            int maxAgeDays = body.value("maxAgeDays", 0);
            int removed = pruneBackups(destination, maxBackups, maxAgeDays);
            res.set_content(json{{"removed", removed}, {"message", "已淘汰 " + std::to_string(removed) + " 个旧备份"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 备份列表查询 =====
    server.Get("/api/backups", [](const httplib::Request &req, httplib::Response &res)
               {
        try {
            std::string destination = req.get_param_value("destination");
            auto backups = listBackups(destination);
            json arr = json::array();
            for (const auto& p : backups) {
                std::error_code ec;
                auto size = fs::is_directory(p) ? directorySize(p) : fs::file_size(p, ec);
                if (ec) size = 0;
                arr.push_back({
                    {"name", p.filename().string()},
                    {"path", p.string()},
                    {"size", size},
                    {"timestamp", backupTimestamp(p)}
                });
            }
            res.set_content(arr.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 备份元数据查询 =====
    server.Get("/api/metadata", [](const httplib::Request &req, httplib::Response &res)
               {
        try {
            std::string backupPath = req.get_param_value("path");
            fs::path metaPath = fs::path(backupPath) / "metadata.json";
            if (fs::is_directory(backupPath) && fs::exists(metaPath)) {
                std::ifstream in(metaPath);
                std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                res.set_content(content, "application/json");
            } else {
                res.status = 404;
                res.set_content(json{{"error", "未找到 metadata.json"}}.dump(), "application/json");
            }
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    // ===== 实时备份监听 =====
    server.Post("/api/watch/start", [this](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);
            std::string source = body.value("source", "");
            std::string destination = body.value("destination", "");
            int interval = body.value("intervalSeconds", 10);

            User user = optionalUserFromRequest(req, &body);
            BackupFilter filter = parseBackupFilter(body, user);

            if (interval <= 0) {
                throw std::runtime_error("监听间隔必须大于 0");
            }
            if (impl_->watching) {
                throw std::runtime_error("实时备份已经在运行");
            }

            // 上次 stop 后线程应已 join；兜底避免赋值未 join 的 thread 导致 terminate
            if (impl_->watchThread.joinable()) {
                impl_->watchThread.join();
            }

            impl_->watching = true;
            impl_->watchThread = std::thread([this, source, destination, interval, filter, user]() {
                auto previous = snapshotFiles(source);
                while (impl_->watching) {
                    {
                        std::unique_lock lock(impl_->watchMutex);
                        impl_->watchCv.wait_for(lock, std::chrono::seconds(interval),
                                                [this] { return !impl_->watching.load(); });
                    }
                    // stop 后立即退出，禁止再扫描/备份
                    if (!impl_->watching) {
                        break;
                    }

                    auto current = snapshotFiles(source);
                    if (current != previous) {
                        int taskId = TaskManager::instance().add("realtime-backup", source, destination, user);
                        try {
                            fs::path path = createBackup(source, destination, true, false, "", filter);
                            TaskManager::instance().update(taskId, "success", "实时备份完成：" + path.string());
                            previous = current;
                        } catch (const std::exception& e) {
                            TaskManager::instance().update(taskId, "failed", e.what());
                        }
                    }
                }
            });

            res.set_content(json{{"message", "实时备份已启动"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server.Post("/api/watch/stop", [this](const httplib::Request &, httplib::Response &res)
                {
        impl_->watching = false;
        impl_->watchCv.notify_all();
        if (impl_->watchThread.joinable()) {
            impl_->watchThread.join();
        }
        res.set_content(json{{"message", "实时备份已停止"}}.dump(), "application/json"); });

    // ===== 任务列表 =====
    server.Get("/api/tasks", [](const httplib::Request &req, httplib::Response &res)
               {
        User user = optionalUserFromRequest(req);
        if (user.id == 0) {
            res.set_content(TaskManager::instance().toJson().dump(), "application/json");
        } else {
            res.set_content(TaskManager::instance().toJsonForUser(user).dump(), "application/json");
        } });

    // ===== 云存储接口（网盘模式：支持跨机文件流传输 + token 鉴权）=====
    if (impl_->storage)
    {
        auto storage = impl_->storage;

        // 固定 token 校验：与远端 BACKUP_CLOUD_TOKEN 环境变量比对，未配置则放行
        auto checkToken = [](const httplib::Request &req) -> bool {
            const char *expected = std::getenv("BACKUP_CLOUD_TOKEN");
            if (!expected || expected[0] == '\0') return true;  // 未配置 token 则不校验
            auto it = req.headers.find("Authorization");
            if (it == req.headers.end()) return false;
            return it->second == ("Bearer " + std::string(expected));
        };

        // 上传：multipart（字段 remotePath + 文件 file），支持跨机文件内容传输
        server.Post("/api/cloud/upload", [storage, checkToken](const httplib::Request &req, httplib::Response &res)
                    {
            if (!checkToken(req)) {
                res.status = 401;
                res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
                return;
            }
            try {
                if (!req.form.has_file("file") || !req.form.has_field("remotePath")) {
                    res.status = 400;
                    res.set_content(json{{"error", "缺少 file 或 remotePath 字段"}}.dump(), "application/json");
                    return;
                }
                std::string remotePath = req.form.get_field("remotePath");
                const auto &file = req.form.get_file("file");
                // 落临时文件再交给 storage 写入（保持 ICloudStorage 接口不变）
                fs::path tmp = fs::temp_directory_path() / ("cloud_up_" + std::to_string(std::time(nullptr)));
                {
                    std::ofstream out(tmp, std::ios::binary);
                    out << file.content;
                }
                bool ok = storage->upload(tmp, remotePath);
                fs::remove(tmp);
                if (ok) {
                    res.set_content(json{{"message", "上传成功"}}.dump(), "application/json");
                } else {
                    res.status = 500;
                    res.set_content(json{{"error", "上传失败"}}.dump(), "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            } });

        // 下载：GET 流式返回文件内容，支持跨机大文件传输
        server.Get("/api/cloud/download", [storage, checkToken](const httplib::Request &req, httplib::Response &res)
                    {
            if (!checkToken(req)) {
                res.status = 401;
                res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
                return;
            }
            try {
                std::string remotePath = req.get_param_value("remotePath");
                fs::path tmp = fs::temp_directory_path() / ("cloud_dl_" + std::to_string(std::time(nullptr)));
                if (!storage->download(remotePath, tmp)) {
                    res.status = 500;
                    res.set_content(json{{"error", "下载失败"}}.dump(), "application/json");
                    return;
                }
                // 流式返回，避免大文件占满内存
                auto size = fs::file_size(tmp);
                res.set_content_provider(size, "application/octet-stream",
                    [tmp](size_t offset, size_t /*length*/, httplib::DataSink &sink) {
                        std::ifstream in(tmp, std::ios::binary);
                        in.seekg(offset);
                        char buf[65536];
                        while (in.read(buf, sizeof(buf)) || in.gcount() > 0) {
                            sink.write(buf, static_cast<size_t>(in.gcount()));
                        }
                        return true;
                    }, [tmp](bool) { fs::remove(tmp); });
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            } });

        // 列出云端目录
        server.Get("/api/cloud/list", [storage, checkToken](const httplib::Request &req, httplib::Response &res)
                    {
            if (!checkToken(req)) {
                res.status = 401;
                res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
                return;
            }
            try {
                std::string remoteDir = req.get_param_value("dir");
                auto files = storage->list(remoteDir);
                json arr = json::array();
                for (const auto& f : files) {
                    arr.push_back({{"path", f.path}, {"size", f.size}, {"modified", f.modified}});
                }
                res.set_content(arr.dump(), "application/json");
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            } });

        // 删除云端文件
        server.Post("/api/cloud/delete", [storage, checkToken](const httplib::Request &req, httplib::Response &res)
                    {
            if (!checkToken(req)) {
                res.status = 401;
                res.set_content(json{{"error", "未授权"}}.dump(), "application/json");
                return;
            }
            try {
                json body = json::parse(req.body);
                std::string remotePath = body.value("remotePath", "");
                if (storage->remove(remotePath)) {
                    res.set_content(json{{"message", "删除成功"}}.dump(), "application/json");
                } else {
                    res.status = 500;
                    res.set_content(json{{"error", "删除失败"}}.dump(), "application/json");
                }
            } catch (const std::exception& e) {
                res.status = 400;
                res.set_content(json{{"error", e.what()}}.dump(), "application/json");
            } });
    }
}

// ---------------- 启动 ----------------

int BackupServer::run()
{
    registerRoutes();

    int port = impl_->config.port;
    bool https = impl_->config.tls && !impl_->config.certPath.empty() && !impl_->config.keyPath.empty();

    if (!impl_->httpServer || !impl_->httpServer->is_valid())
    {
        std::cerr << "[错误] 服务器初始化失败\n";
        return 1;
    }

    std::cout << "Backup server running at " << (https ? "https" : "http")
              << "://localhost:" << port << "\n";
    if (https)
    {
        std::cout << "（HTTPS 传输加密已启用）\n";
    }
    else
    {
        std::cout << "提示：在配置文件中设置 tls/certPath/keyPath 可启用 HTTPS 传输加密\n";
    }

    impl_->httpServer->listen("0.0.0.0", port);
    return 0;
}

} // namespace backup::server
