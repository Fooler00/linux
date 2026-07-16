#include <gtest/gtest.h>

#include "core/BackupEngine.h"
#include "test_utils.h"

using namespace backup;

namespace {

BackupFilter noArchive()
{
    BackupFilter filter;
    filter.archiveType = "none";
    return filter;
}

} // namespace

TEST(DefectReproductionIntegration, GlobLogFilter_IncludesVisibleLogFiles)
{
    auto dir = test_support::caseDir("defect_glob_log");
    test_support::writeFile(dir / "source" / "app.log", "log");
    test_support::writeFile(dir / "source" / "app.txt", "txt");
    auto filter = noArchive();
    filter.includePaths = {"*.log"};
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", filter);
    EXPECT_TRUE(fs::exists(backup / "app.log"));
    EXPECT_FALSE(fs::exists(backup / "app.txt"));
}

TEST(DefectReproductionIntegration, DotLogCompatibility_RequiresExplicitDotPattern)
{
    auto dir = test_support::caseDir("defect_dot_log");
    test_support::writeFile(dir / "source" / ".log", "hidden");
    auto filter = noArchive();
    filter.includePaths = {".log"};
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", filter);
    EXPECT_TRUE(fs::exists(backup / ".log"));
}

TEST(DefectReproductionIntegration, SymlinkBackup_PreservesLinkWhenSupported)
{
    auto dir = test_support::caseDir("defect_symlink");
    test_support::writeFile(dir / "source" / "target.txt", "target");
    std::error_code ec;
    fs::create_symlink("target.txt", dir / "source" / "link.txt", ec);
    if (ec)
    {
        GTEST_SKIP() << "symlink creation failed: " << ec.message();
    }
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", noArchive());
    EXPECT_TRUE(fs::is_symlink(backup / "link.txt"));
}

TEST(DefectReproductionIntegration, BackupList_ReturnsCreatedBackups)
{
    auto dir = test_support::caseDir("defect_backup_list");
    test_support::writeFile(dir / "source" / "a.txt", "A");
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", noArchive());
    auto backups = listBackups(dir / "backups");
    ASSERT_EQ(backups.size(), 1U);
    EXPECT_EQ(backups.front(), backup);
}

TEST(DefectReproductionIntegration, RestoreMissingBackup_Throws)
{
    auto dir = test_support::caseDir("defect_restore_missing");
    EXPECT_THROW(restoreBackup(dir / "missing.zip", dir / "restore", ""), std::runtime_error);
}
