// =====================================================================
//  Crypto.cpp - 加密解密 + 内容哈希实现
// =====================================================================

#include "core/Crypto.h"
#include "core/Utils.h"

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <fstream>
#include <stdexcept>
#include <vector>

namespace backup {

std::string normalizeEncryptAlgo(const std::string &algo)
{
    static const std::vector<std::string> allowed = {
        "aes-256-cbc", "aes-128-cbc", "camellia-256-cbc", "camellia-128-cbc",
        "des-ede3-cbc", "chacha20"};
    for (const auto &a : allowed)
    {
        if (algo == a)
        {
            return a;
        }
    }
    return "aes-256-cbc";
}

fs::path encryptFile(const fs::path &src, const std::string &password, const std::string &algo)
{
    std::string cipher = normalizeEncryptAlgo(algo);
    fs::path encPath = src;
    encPath += ".enc";

    std::string cmd =
        "openssl enc -" + cipher + " -salt -pbkdf2 "
        "-in " +
        shellQuote(src.string()) +
        " -out " + shellQuote(encPath.string()) +
        " -pass pass:" + shellQuote(password);

    if (!runCommand(cmd))
    {
        throw std::runtime_error("加密失败（算法: " + cipher + "），请确认系统已安装 openssl");
    }

    fs::remove(src);
    return encPath;
}

fs::path decryptFile(const fs::path &src, const std::string &password, const std::string &algo)
{
    std::string cipher = normalizeEncryptAlgo(algo);
    fs::path outPath = src;
    outPath = outPath.replace_extension("");

    fs::path tempDir = fs::temp_directory_path() / ("dec_" + nowString());
    fs::create_directories(tempDir);
    fs::path decryptedPath = tempDir / outPath.filename();

    std::string cmd =
        "openssl enc -d -" + cipher + " -pbkdf2 "
        "-in " +
        shellQuote(src.string()) +
        " -out " + shellQuote(decryptedPath.string()) +
        " -pass pass:" + shellQuote(password);

    if (!runCommand(cmd))
    {
        fs::remove_all(tempDir);
        throw std::runtime_error("解密失败，请检查密码或算法（" + cipher + "）");
    }

    return decryptedPath;
}

std::string fileContentHash(const fs::path &path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        return "";
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        return "";
    }

    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    char buffer[65536];
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
    {
        EVP_DigestUpdate(ctx, buffer, static_cast<size_t>(in.gcount()));
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    EVP_DigestFinal_ex(ctx, hash, &hashLen);
    EVP_MD_CTX_free(ctx);

    return bytesToHex(hash, hashLen);
}

} // namespace backup
