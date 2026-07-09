// =====================================================================
//  RemoteCloudStorage.cpp - 远程网盘存储实现
//
//  通过 httplib::Client 调用远端 backup_server 的 cloud 接口。
//  upload 走 multipart（文件流），download 走 GET 流式接收。
// =====================================================================

#include "cloud/RemoteCloudStorage.h"

#include <nlohmann/json.hpp>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace backup::cloud {

using json = nlohmann::json;

RemoteCloudStorage::RemoteCloudStorage(const CloudConfig &cfg)
    : host_(cfg.endpoint), tls_(cfg.tls), token_(cfg.token)
{
    // endpoint 形如 "192.168.1.100:8080" 或 "host:port"，拆分出 host 与 port
    auto pos = host_.find(':');
    if (pos != std::string::npos)
    {
        port_ = std::atoi(host_.c_str() + pos + 1);
        host_ = host_.substr(0, pos);
    }

    client_ = std::make_unique<httplib::Client>(host_.c_str(), port_);
    client_->set_connection_timeout(10);
    client_->set_read_timeout(300);   // 备份文件可能较大，放宽读超时
}

httplib::Headers RemoteCloudStorage::authHeaders()
{
    httplib::Headers h;
    if (!token_.empty())
    {
        h.emplace("Authorization", "Bearer " + token_);
    }
    return h;
}

bool RemoteCloudStorage::upload(const fs::path &localPath, const std::string &remotePath)
{
    if (!fs::exists(localPath))
    {
        return false;
    }

    // 读取本地文件内容（中等规模备份可直接入内存；超大文件可改用 ContentProvider）
    std::ifstream in(localPath, std::ios::binary);
    if (!in)
    {
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    httplib::UploadFormDataItems items = {
        {"remotePath", remotePath, "", ""},
        {"file", content, localPath.filename().string(), "application/octet-stream"},
    };

    auto res = client_->Post("/api/cloud/upload", authHeaders(), items);
    return res && res->status == 200;
}

bool RemoteCloudStorage::download(const std::string &remotePath, const fs::path &localPath)
{
    fs::create_directories(localPath.parent_path());
    std::ofstream out(localPath, std::ios::binary);
    if (!out)
    {
        return false;
    }

    // 流式接收，避免大文件占满内存
    auto res = client_->Get(
        "/api/cloud/download?remotePath=" + httplib::encode_query_component(remotePath),
        authHeaders(),
        [&out](const char *data, size_t len) {
            out.write(data, static_cast<std::streamsize>(len));
            return true;
        });
    out.close();

    if (!res || res->status != 200)
    {
        std::error_code ec;
        fs::remove(localPath, ec);
        return false;
    }
    return true;
}

std::vector<CloudFileInfo> RemoteCloudStorage::list(const std::string &remoteDir)
{
    std::vector<CloudFileInfo> result;
    auto res = client_->Get(
        "/api/cloud/list?dir=" + httplib::encode_query_component(remoteDir),
        authHeaders());
    if (!res || res->status != 200)
    {
        return result;
    }

    try
    {
        json arr = json::parse(res->body);
        for (const auto &f : arr)
        {
            CloudFileInfo info;
            info.path = f.value("path", "");
            info.size = f.value("size", 0);
            info.modified = f.value("modified", 0);
            result.push_back(info);
        }
    }
    catch (...)
    {
        // 解析失败则返回空列表
    }
    return result;
}

bool RemoteCloudStorage::remove(const std::string &remotePath)
{
    json body = {{"remotePath", remotePath}};
    auto res = client_->Post("/api/cloud/delete", authHeaders(),
                             body.dump(), "application/json");
    return res && res->status == 200;
}

std::string RemoteCloudStorage::type() const
{
    return "remote";
}

} // namespace backup::cloud
