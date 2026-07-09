#pragma once

// =====================================================================
//  Utils.h - 通用工具函数
// =====================================================================

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

namespace backup {

namespace fs = std::filesystem;

// 当前时间格式化为 YYYYMMDD_HHMMSS
std::string nowString();

// 字节数组转十六进制字符串
std::string bytesToHex(const unsigned char *data, std::size_t size);

// SHA-256 哈希（返回十六进制）
std::string sha256Hex(const std::string &value);

// 生成指定字节数的随机十六进制串
std::string randomHex(std::size_t bytes);

// 密码加盐哈希
std::string hashPassword(const std::string &password, const std::string &salt);

// Shell 转义（防止命令注入）
std::string shellQuote(const std::string &value);

// 执行 shell 命令，返回是否成功
bool runCommand(const std::string &command);

// 解析时间字符串为系统时间点
bool parseTimeString(const std::string &value, std::chrono::system_clock::time_point &tp);

// 系统时间点转换为文件时间
fs::file_time_type systemTimeToFileTime(std::chrono::system_clock::time_point tp);

// 计算目录总大小
std::uintmax_t directorySize(const fs::path &path);

} // namespace backup
