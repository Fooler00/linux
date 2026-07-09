#pragma once

// =====================================================================
//  Crypto.h - 加密解密（多算法）+ 内容哈希
// =====================================================================

#include <filesystem>
#include <string>

namespace backup {

namespace fs = std::filesystem;

// 加密算法白名单校验
std::string normalizeEncryptAlgo(const std::string &algo);

// 加密单个文件（任意 openssl 支持的对称算法），返回密文路径
fs::path encryptFile(const fs::path &src, const std::string &password, const std::string &algo);

// 解密单个文件，返回明文路径（位于临时目录）
fs::path decryptFile(const fs::path &src, const std::string &password, const std::string &algo);

// 计算文件内容 SHA-256（用于增量备份对比）
std::string fileContentHash(const fs::path &path);

} // namespace backup
