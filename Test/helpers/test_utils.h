#pragma once

#include <chrono>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace test_support {

namespace fs = std::filesystem;

inline fs::path outputRoot()
{
    fs::path root = TEST_OUTPUT_ROOT;
    fs::create_directories(root);
    return root;
}

inline std::string sanitize(const std::string &name)
{
    std::string out;
    for (char c : name)
    {
        out += (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-') ? c : '_';
    }
    return out.empty() ? "case" : out;
}

inline fs::path caseDir(const std::string &name)
{
    static int counter = 0;
    fs::path dir = outputRoot() / (sanitize(name) + "_" + std::to_string(++counter));
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

inline void writeFile(const fs::path &path, const std::string &content)
{
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << content;
}

inline std::string readFile(const fs::path &path)
{
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

inline bool commandExists(const std::string &command)
{
    std::string probe = "command -v " + command + " >/dev/null 2>&1";
    return std::system(probe.c_str()) == 0;
}

inline std::vector<fs::path> findFilesNamed(const fs::path &root, const std::string &filename)
{
    std::vector<fs::path> paths;
    if (!fs::exists(root))
    {
        return paths;
    }
    for (const auto &entry : fs::recursive_directory_iterator(root))
    {
        if (entry.path().filename() == filename)
        {
            paths.push_back(entry.path());
        }
    }
    return paths;
}

inline fs::path firstBackupPayloadRoot(const fs::path &restoreDir)
{
    for (const auto &entry : fs::directory_iterator(restoreDir))
    {
        if (entry.is_directory() && entry.path().filename().string().rfind("backup_", 0) == 0)
        {
            return entry.path();
        }
    }
    return restoreDir;
}

inline void waitForNextSecond()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
}

} // namespace test_support
