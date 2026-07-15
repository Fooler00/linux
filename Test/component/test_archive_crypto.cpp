#include <gtest/gtest.h>

#include "core/Archive.h"
#include "core/Crypto.h"
#include "test_utils.h"

using namespace backup;

TEST(ArchiveCryptoComponent, NormalizeArchiveType_WhitelistsSupportedTypes)
{
    EXPECT_EQ(normalizeArchiveType("none"), "none");
    EXPECT_EQ(normalizeArchiveType("zip"), "zip");
    EXPECT_EQ(normalizeArchiveType("tar"), "tar");
    EXPECT_EQ(normalizeArchiveType("tgz"), "tar.gz");
    EXPECT_EQ(normalizeArchiveType("unknown"), "zip");
}

TEST(ArchiveCryptoComponent, NormalizeEncryptAlgo_DefaultsUnsupportedToAes256)
{
    EXPECT_EQ(normalizeEncryptAlgo("aes-128-cbc"), "aes-128-cbc");
    EXPECT_EQ(normalizeEncryptAlgo("invalid"), "aes-256-cbc");
}

TEST(ArchiveCryptoComponent, ZipArchive_RoundTripsDirectory)
{
    if (!test_support::commandExists("zip") || !test_support::commandExists("unzip"))
    {
        GTEST_SKIP() << "zip/unzip not installed";
    }
    auto dir = test_support::caseDir("archive_zip");
    auto backupDir = dir / "backup_payload";
    test_support::writeFile(backupDir / "nested" / "file.txt", "zip-content");
    auto archive = createArchive(backupDir, "zip");
    ASSERT_TRUE(fs::exists(archive));
    auto extractedRoot = extractArchive(archive, dir / "extract");
    EXPECT_EQ(test_support::readFile(extractedRoot / "nested" / "file.txt"), "zip-content");
}

TEST(ArchiveCryptoComponent, TarArchive_RoundTripsDirectory)
{
    if (!test_support::commandExists("tar"))
    {
        GTEST_SKIP() << "tar not installed";
    }
    auto dir = test_support::caseDir("archive_tar");
    auto backupDir = dir / "backup_payload";
    test_support::writeFile(backupDir / "file.txt", "tar-content");
    auto archive = createArchive(backupDir, "tar");
    ASSERT_TRUE(fs::exists(archive));
    auto extractedRoot = extractArchive(archive, dir / "extract");
    EXPECT_EQ(test_support::readFile(extractedRoot / "file.txt"), "tar-content");
}

TEST(ArchiveCryptoComponent, TarGzArchive_RoundTripsDirectory)
{
    if (!test_support::commandExists("tar"))
    {
        GTEST_SKIP() << "tar not installed";
    }
    auto dir = test_support::caseDir("archive_targz");
    auto backupDir = dir / "backup_payload";
    test_support::writeFile(backupDir / "file.txt", "tgz-content");
    auto archive = createArchive(backupDir, "tar.gz");
    ASSERT_TRUE(fs::exists(archive));
    auto extractedRoot = extractArchive(archive, dir / "extract");
    EXPECT_EQ(test_support::readFile(extractedRoot / "file.txt"), "tgz-content");
}

TEST(ArchiveCryptoComponent, EncryptDecrypt_RoundTripsFile)
{
    if (!test_support::commandExists("openssl"))
    {
        GTEST_SKIP() << "openssl not installed";
    }
    auto dir = test_support::caseDir("crypto_roundtrip");
    auto plain = dir / "plain.txt";
    test_support::writeFile(plain, "secret-content");
    auto encrypted = encryptFile(plain, "correct-password", "aes-256-cbc");
    ASSERT_TRUE(fs::exists(encrypted));
    EXPECT_FALSE(fs::exists(plain));
    auto decrypted = decryptFile(encrypted, "correct-password", "aes-256-cbc");
    EXPECT_EQ(test_support::readFile(decrypted), "secret-content");
}

TEST(ArchiveCryptoComponent, EncryptDecrypt_WrongPasswordThrows)
{
    if (!test_support::commandExists("openssl"))
    {
        GTEST_SKIP() << "openssl not installed";
    }
    auto dir = test_support::caseDir("crypto_wrong_password");
    auto plain = dir / "plain.txt";
    test_support::writeFile(plain, "secret-content");
    auto encrypted = encryptFile(plain, "correct-password", "aes-256-cbc");
    EXPECT_THROW(decryptFile(encrypted, "wrong-password", "aes-256-cbc"), std::runtime_error);
}

TEST(ArchiveCryptoComponent, FileContentHash_ReturnsEmptyForMissingFile)
{
    auto dir = test_support::caseDir("crypto_missing_hash");
    EXPECT_TRUE(fileContentHash(dir / "missing.txt").empty());
}

TEST(ArchiveCryptoComponent, FileContentHash_ChangesWhenContentChanges)
{
    auto dir = test_support::caseDir("crypto_hash_change");
    auto file = dir / "file.txt";
    test_support::writeFile(file, "one");
    auto first = fileContentHash(file);
    test_support::writeFile(file, "two");
    auto second = fileContentHash(file);
    EXPECT_FALSE(first.empty());
    EXPECT_NE(first, second);
}
