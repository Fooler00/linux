#include "httplib.h"
#include "json.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

struct Task
{
    int id;
    std::string type;
    std::string source;
    std::string destination;
    std::string status;
    std::string message;
    std::string createdAt;
};

std::mutex g_mutex;
std::vector<Task> g_tasks;
std::atomic<int> g_nextId{1};
std::atomic<bool> g_watching{false};
std::thread g_watchThread;

std::string nowString()
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

int addTask(
    const std::string &type,
    const std::string &source,
    const std::string &destination)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    int id = g_nextId++;
    g_tasks.push_back({id,
                       type,
                       source,
                       destination,
                       "running",
                       "",
                       nowString()});

    return id;
}

void updateTask(int id, const std::string &status, const std::string &message)
{
    std::lock_guard<std::mutex> lock(g_mutex);

    for (auto &task : g_tasks)
    {
        if (task.id == id)
        {
            task.status = status;
            task.message = message;
            return;
        }
    }
}

json tasksToJson()
{
    std::lock_guard<std::mutex> lock(g_mutex);

    json arr = json::array();
    for (const auto &task : g_tasks)
    {
        arr.push_back({{"id", task.id},
                       {"type", task.type},
                       {"source", task.source},
                       {"destination", task.destination},
                       {"status", task.status},
                       {"message", task.message},
                       {"createdAt", task.createdAt}});
    }

    return arr;
}

bool runCommand(const std::string &command)
{
    int result = std::system(command.c_str());
    return result == 0;
}

std::string quote(const std::string &value)
{
    std::string result = "\"";

    for (char ch : value)
    {
        if (ch == '"')
        {
            result += "\\\"";
        }
        else
        {
            result += ch;
        }
    }

    result += "\"";
    return result;
}

std::uintmax_t directorySize(const fs::path &path)
{
    std::uintmax_t total = 0;

    if (!fs::exists(path))
    {
        return total;
    }

    for (const auto &entry : fs::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            total += entry.file_size();
        }
    }

    return total;
}

void writeMetadata(
    const fs::path &backupDir,
    const fs::path &source,
    const std::string &mode)
{
    json meta;
    meta["source"] = source.string();
    meta["backupTime"] = nowString();
    meta["mode"] = mode;
    meta["size"] = directorySize(backupDir);

    std::ofstream out(backupDir / "metadata.json");
    out << meta.dump(4);
}

fs::path createBackup(
    const fs::path &source,
    const fs::path &destination,
    bool compress,
    bool encrypt,
    const std::string &password)
{
    if (!fs::exists(source))
    {
        throw std::runtime_error("源目录不存在");
    }

    fs::create_directories(destination);

    fs::path backupDir = destination / ("backup_" + nowString());
    fs::create_directories(backupDir);

    fs::copy(
        source,
        backupDir,
        fs::copy_options::recursive |
            fs::copy_options::overwrite_existing);

    writeMetadata(backupDir, source, compress ? "compressed" : "normal");

    fs::path resultPath = backupDir;

    if (compress)
    {
        fs::path zipPath = backupDir;
        zipPath += ".zip";

        std::string cmd =
            "cd " + quote(backupDir.parent_path().string()) +
            " && zip -r " + quote(zipPath.filename().string()) +
            " " + quote(backupDir.filename().string());

        if (!runCommand(cmd))
        {
            throw std::runtime_error("压缩失败，请确认系统已安装 zip");
        }

        fs::remove_all(backupDir);
        resultPath = zipPath;
    }

    if (encrypt)
    {
        if (password.empty())
        {
            throw std::runtime_error("启用加密时密码不能为空");
        }

        fs::path encPath = resultPath;
        encPath += ".enc";

        std::string cmd =
            "openssl enc -aes-256-cbc -salt -pbkdf2 "
            "-in " +
            quote(resultPath.string()) +
            " -out " + quote(encPath.string()) +
            " -pass pass:" + quote(password);

        if (!runCommand(cmd))
        {
            throw std::runtime_error("加密失败，请确认系统已安装 openssl");
        }

        if (fs::is_directory(resultPath))
        {
            fs::remove_all(resultPath);
        }
        else
        {
            fs::remove(resultPath);
        }

        resultPath = encPath;
    }

    return resultPath;
}

void restoreBackup(
    const fs::path &backupPath,
    const fs::path &destination,
    const std::string &password)
{
    if (!fs::exists(backupPath))
    {
        throw std::runtime_error("备份文件或目录不存在");
    }

    fs::create_directories(destination);

    fs::path workPath = backupPath;
    fs::path tempPath;

    if (backupPath.extension() == ".enc")
    {
        if (password.empty())
        {
            throw std::runtime_error("还原加密备份时密码不能为空");
        }

        tempPath = fs::temp_directory_path() / ("restore_" + nowString());
        std::string fileName = backupPath.stem().string();
        fs::path decryptedPath = tempPath / fileName;

        fs::create_directories(tempPath);

        std::string cmd =
            "openssl enc -d -aes-256-cbc -pbkdf2 "
            "-in " +
            quote(backupPath.string()) +
            " -out " + quote(decryptedPath.string()) +
            " -pass pass:" + quote(password);

        if (!runCommand(cmd))
        {
            throw std::runtime_error("解密失败，请检查密码");
        }

        workPath = decryptedPath;
    }

    if (workPath.extension() == ".zip")
    {
        fs::path unzipDir = fs::temp_directory_path() / ("unzip_" + nowString());
        fs::create_directories(unzipDir);

        std::string cmd =
            "unzip -o " + quote(workPath.string()) +
            " -d " + quote(unzipDir.string());

        if (!runCommand(cmd))
        {
            throw std::runtime_error("解压失败，请确认系统已安装 unzip");
        }

        for (const auto &entry : fs::directory_iterator(unzipDir))
        {
            fs::copy(
                entry.path(),
                destination / entry.path().filename(),
                fs::copy_options::recursive |
                    fs::copy_options::overwrite_existing);
        }

        fs::remove_all(unzipDir);
    }
    else if (fs::is_directory(workPath))
    {
        fs::copy(
            workPath,
            destination / workPath.filename(),
            fs::copy_options::recursive |
                fs::copy_options::overwrite_existing);
    }
    else
    {
        fs::copy_file(
            workPath,
            destination / workPath.filename(),
            fs::copy_options::overwrite_existing);
    }

    if (!tempPath.empty())
    {
        fs::remove_all(tempPath);
    }
}

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

void startWatcher(
    const fs::path &source,
    const fs::path &destination,
    int intervalSeconds)
{
    if (g_watching)
    {
        throw std::runtime_error("实时备份已经在运行");
    }

    g_watching = true;

    g_watchThread = std::thread([source, destination, intervalSeconds]()
                                {
        auto previous = snapshotFiles(source);

        while (g_watching) {
            std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));

            auto current = snapshotFiles(source);
            if (current != previous) {
                int taskId = addTask("realtime-backup", source.string(), destination.string());

                try {
                    fs::path path = createBackup(source, destination, true, false, "");
                    updateTask(taskId, "success", "实时备份完成：" + path.string());
                    previous = current;
                } catch (const std::exception& e) {
                    updateTask(taskId, "failed", e.what());
                }
            }
        } });

    g_watchThread.detach();
}

int main()
{
    httplib::Server server;

    server.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                                {"Access-Control-Allow-Methods", "GET, POST, OPTIONS"},
                                {"Access-Control-Allow-Headers", "Content-Type"}});

    server.Options(R"(.*)", [](const httplib::Request &, httplib::Response &res)
                   { res.status = 204; });

    server.Post("/api/backup", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            std::string source = body.value("source", "");
            std::string destination = body.value("destination", "");
            bool compress = body.value("compress", false);
            bool encrypt = body.value("encrypt", false);
            std::string password = body.value("password", "");

            int taskId = addTask("backup", source, destination);

            std::thread([taskId, source, destination, compress, encrypt, password]() {
                try {
                    fs::path result = createBackup(source, destination, compress, encrypt, password);
                    updateTask(taskId, "success", "备份完成：" + result.string());
                } catch (const std::exception& e) {
                    updateTask(taskId, "failed", e.what());
                }
            }).detach();

            res.set_content(json{{"taskId", taskId}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server.Post("/api/restore", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            std::string backupPath = body.value("backupPath", "");
            std::string destination = body.value("destination", "");
            std::string password = body.value("password", "");

            int taskId = addTask("restore", backupPath, destination);

            std::thread([taskId, backupPath, destination, password]() {
                try {
                    restoreBackup(backupPath, destination, password);
                    updateTask(taskId, "success", "还原完成");
                } catch (const std::exception& e) {
                    updateTask(taskId, "failed", e.what());
                }
            }).detach();

            res.set_content(json{{"taskId", taskId}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server.Post("/api/watch/start", [](const httplib::Request &req, httplib::Response &res)
                {
        try {
            json body = json::parse(req.body);

            std::string source = body.value("source", "");
            std::string destination = body.value("destination", "");
            int interval = body.value("intervalSeconds", 10);

            startWatcher(source, destination, interval);

            res.set_content(json{{"message", "实时备份已启动"}}.dump(), "application/json");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(json{{"error", e.what()}}.dump(), "application/json");
        } });

    server.Post("/api/watch/stop", [](const httplib::Request &, httplib::Response &res)
                {
        g_watching = false;
        res.set_content(json{{"message", "实时备份已停止"}}.dump(), "application/json"); });

    server.Get("/api/tasks", [](const httplib::Request &, httplib::Response &res)
               { res.set_content(tasksToJson().dump(), "application/json"); });

    std::cout << "Backup server running at http://localhost:8080\n";
    server.listen("0.0.0.0", 8080);
}

