// =====================================================================
//  Utils.cpp - 通用工具函数实现
// =====================================================================

#include "core/Utils.h"

#include <chrono>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <random>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <vector>

namespace backup {

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

std::string bytesToHex(const unsigned char *data, std::size_t size)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (std::size_t i = 0; i < size; ++i)
    {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }

    return oss.str();
}

std::string sha256Hex(const std::string &value)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(value.data()), value.size(), hash);
    return bytesToHex(hash, SHA256_DIGEST_LENGTH);
}

std::string randomHex(std::size_t bytes)
{
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);

    std::vector<unsigned char> data(bytes);
    for (auto &ch : data)
    {
        ch = static_cast<unsigned char>(dist(rd));
    }

    return bytesToHex(data.data(), data.size());
}

std::string hashPassword(const std::string &password, const std::string &salt)
{
    return sha256Hex(salt + ":" + password);
}

std::string shellQuote(const std::string &value)
{
#ifdef _WIN32
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
#else
    std::string result = "'";
    for (char ch : value)
    {
        if (ch == '\'')
        {
            result += "'\\''";
        }
        else
        {
            result += ch;
        }
    }
    result += "'";
    return result;
#endif
}

bool runCommand(const std::string &command)
{
    int result = std::system(command.c_str());
    return result == 0;
}

bool parseTimeString(const std::string &value, std::chrono::system_clock::time_point &timePoint)
{
    if (value.empty())
    {
        return false;
    }

    std::tm tm{};
    std::istringstream iss(value);

    if (value.find('-') != std::string::npos)
    {
        iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    }
    else
    {
        iss >> std::get_time(&tm, "%Y%m%d_%H%M%S");
    }

    if (iss.fail())
    {
        return false;
    }

    std::time_t t = std::mktime(&tm);
    if (t == -1)
    {
        return false;
    }

    timePoint = std::chrono::system_clock::from_time_t(t);
    return true;
}

fs::file_time_type systemTimeToFileTime(std::chrono::system_clock::time_point tp)
{
    auto nowFile = fs::file_time_type::clock::now();
    auto nowSystem = std::chrono::system_clock::now();
    return nowFile + std::chrono::duration_cast<fs::file_time_type::duration>(tp - nowSystem);
}

std::uintmax_t directorySize(const fs::path &path)
{
    std::uintmax_t size = 0;
    for (const auto &entry : fs::recursive_directory_iterator(path))
    {
        if (entry.is_regular_file())
        {
            size += entry.file_size();
        }
    }
    return size;
}

} // namespace backup
