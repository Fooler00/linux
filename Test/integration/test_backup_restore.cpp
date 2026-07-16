#include <gtest/gtest.h>

#include "core/BackupEngine.h"
#include "test_utils.h"

using namespace backup;

namespace {

BackupFilter noneArchiveFilter()
{
    BackupFilter filter;
    filter.archiveType = "none";
    return filter;
}

fs::path restoreAndPayload(const fs::path &backup, const fs::path &restoreDir)
{
    restoreBackup(backup, restoreDir, "");
    return test_support::firstBackupPayloadRoot(restoreDir);
}

} // namespace

TEST(BackupRestoreIntegration, DirectoryBackupRestore_PreservesNestedContent)
{
    auto dir = test_support::caseDir("int_basic_restore");
    test_support::writeFile(dir / "source" / "a.txt", "A");
    test_support::writeFile(dir / "source" / "nested" / "b.txt", "B");
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", noneArchiveFilter());
    auto payload = restoreAndPayload(backup, dir / "restore");
    EXPECT_EQ(test_support::readFile(payload / "a.txt"), "A");
    EXPECT_EQ(test_support::readFile(payload / "nested" / "b.txt"), "B");
}

TEST(BackupRestoreIntegration, EmptyDirectory_CreatesBackupDirectory)
{
    auto dir = test_support::caseDir("int_empty_dir");
    fs::create_directories(dir / "source" / "empty");
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", noneArchiveFilter());
    EXPECT_TRUE(fs::exists(backup));
    EXPECT_TRUE(fs::exists(backup / "empty"));
}

TEST(BackupRestoreIntegration, EmptyFile_IsRestored)
{
    auto dir = test_support::caseDir("int_empty_file");
    test_support::writeFile(dir / "source" / "empty.txt", "");
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", noneArchiveFilter());
    auto payload = restoreAndPayload(backup, dir / "restore");
    EXPECT_TRUE(fs::exists(payload / "empty.txt"));
    EXPECT_EQ(fs::file_size(payload / "empty.txt"), 0U);
}

TEST(BackupRestoreIntegration, UnicodeSpaceAndShellCharacters_RestoreContent)
{
    auto dir = test_support::caseDir("int_special_names");
    test_support::writeFile(dir / "source" / "中文 目录" / "semi;quote'.txt", "special");
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", noneArchiveFilter());
    auto payload = restoreAndPayload(backup, dir / "restore");
    EXPECT_EQ(test_support::readFile(payload / "中文 目录" / "semi;quote'.txt"), "special");
}

TEST(BackupRestoreIntegration, SourceFile_IsNotModifiedByBackup)
{
    auto dir = test_support::caseDir("int_source_protection");
    auto sourceFile = dir / "source" / "keep.txt";
    test_support::writeFile(sourceFile, "original");
    auto before = fs::last_write_time(sourceFile);
    createBackup(dir / "source", dir / "backups", false, false, "", noneArchiveFilter());
    EXPECT_EQ(test_support::readFile(sourceFile), "original");
    EXPECT_EQ(fs::last_write_time(sourceFile), before);
}

TEST(BackupRestoreIntegration, MissingSource_Throws)
{
    auto dir = test_support::caseDir("int_missing_source");
    EXPECT_THROW(createBackup(dir / "missing", dir / "backups", false, false, "", noneArchiveFilter()), std::runtime_error);
}

TEST(BackupRestoreIntegration, EncryptedBackup_WrongPasswordThrows)
{
    if (!test_support::commandExists("tar") || !test_support::commandExists("openssl"))
    {
        GTEST_SKIP() << "tar/openssl not installed";
    }
    auto dir = test_support::caseDir("int_encrypted_wrong_password");
    test_support::writeFile(dir / "source" / "secret.txt", "secret");
    BackupFilter filter = noneArchiveFilter();
    auto backup = createBackup(dir / "source", dir / "backups", false, true, "correct-password", filter);
    EXPECT_THROW(restoreBackup(backup, dir / "restore", "wrong-password"), std::runtime_error);
}

TEST(BackupRestoreIntegration, ZipBackupRestore_RoundTrips)
{
    if (!test_support::commandExists("zip") || !test_support::commandExists("unzip"))
    {
        GTEST_SKIP() << "zip/unzip not installed";
    }
    auto dir = test_support::caseDir("int_zip_restore");
    test_support::writeFile(dir / "source" / "file.txt", "zip");
    BackupFilter filter;
    filter.archiveType = "zip";
    auto backup = createBackup(dir / "source", dir / "backups", true, false, "", filter);
    restoreBackup(backup, dir / "restore", "");
    auto found = test_support::findFilesNamed(dir / "restore", "file.txt");
    ASSERT_FALSE(found.empty());
    EXPECT_EQ(test_support::readFile(found.front()), "zip");
}
