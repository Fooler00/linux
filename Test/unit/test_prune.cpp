#include <gtest/gtest.h>

#include "core/BackupEngine.h"
#include "core/Scheduler.h"
#include "test_utils.h"

using namespace backup;

namespace {

void makeFakeBackup(const fs::path &destination, const std::string &name)
{
    auto dir = destination / name;
    fs::create_directories(dir);
    test_support::writeFile(dir / "marker.txt", name);
}

} // namespace

TEST(PruneUnit, MaxBackupsZero_RemovesAll)
{
    auto dir = test_support::caseDir("prune_zero");
    auto dest = dir / "backups";
    makeFakeBackup(dest, "backup_20200101_000000");
    makeFakeBackup(dest, "backup_20200102_000000");
    makeFakeBackup(dest, "backup_20200103_000000");
    ASSERT_EQ(listBackups(dest).size(), 3U);

    int removed = pruneBackups(dest, 0, 0);
    EXPECT_EQ(removed, 3);
    EXPECT_TRUE(listBackups(dest).empty());
}

TEST(PruneUnit, MaxBackupsNegOne_SkipsCountPrune)
{
    auto dir = test_support::caseDir("prune_neg_one");
    auto dest = dir / "backups";
    makeFakeBackup(dest, "backup_20200101_000000");
    makeFakeBackup(dest, "backup_20200102_000000");
    ASSERT_EQ(listBackups(dest).size(), 2U);

    int removed = pruneBackups(dest, -1, 0);
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(listBackups(dest).size(), 2U);
}

TEST(PruneUnit, MaxBackupsKeepNewestN)
{
    auto dir = test_support::caseDir("prune_keep_n");
    auto dest = dir / "backups";
    makeFakeBackup(dest, "backup_20200101_000000");
    makeFakeBackup(dest, "backup_20200102_000000");
    makeFakeBackup(dest, "backup_20200103_000000");

    int removed = pruneBackups(dest, 1, 0);
    EXPECT_EQ(removed, 2);
    auto left = listBackups(dest);
    ASSERT_EQ(left.size(), 1U);
    EXPECT_EQ(left.front().filename().string(), "backup_20200103_000000");
}
