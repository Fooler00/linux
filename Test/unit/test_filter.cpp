#include <gtest/gtest.h>

#include "core/Filter.h"
#include "test_utils.h"

using namespace backup;
using json = nlohmann::json;

namespace {

User testUser()
{
    return {42, "tester"};
}

fs::directory_entry entryFor(const fs::path &path)
{
    return fs::directory_entry(path);
}

} // namespace

TEST(FilterUnit, ParseEmptyFilter_UsesTopLevelOptions)
{
    json body = {{"archiveType", "tar.gz"}, {"encryptAlgo", "aes-128-cbc"}, {"incremental", true}, {"incrementalBase", "/base"}};
    auto filter = parseBackupFilter(body, testUser());
    EXPECT_EQ(filter.userId, 42);
    EXPECT_EQ(filter.username, "tester");
    EXPECT_EQ(filter.archiveType, "tar.gz");
    EXPECT_EQ(filter.encryptAlgo, "aes-128-cbc");
    EXPECT_TRUE(filter.incremental);
    EXPECT_EQ(filter.incrementalBase, "/base");
}

TEST(FilterUnit, ParseExtensions_AddsLeadingDot)
{
    json body = {{"filter", {{"extensions", json::array({"txt", ".md"})}}}};
    auto filter = parseBackupFilter(body, testUser());
    ASSERT_EQ(filter.extensions.size(), 2U);
    EXPECT_EQ(filter.extensions[0], ".txt");
    EXPECT_EQ(filter.extensions[1], ".md");
}

TEST(FilterUnit, ExtensionFilter_MatchesOnlyConfiguredExtensions)
{
    auto dir = test_support::caseDir("filter_extensions");
    test_support::writeFile(dir / "keep.txt", "1");
    test_support::writeFile(dir / "drop.bin", "1");
    BackupFilter filter;
    filter.extensions = {".txt"};
    EXPECT_TRUE(fileMatchesFilter(entryFor(dir / "keep.txt"), dir, filter));
    EXPECT_FALSE(fileMatchesFilter(entryFor(dir / "drop.bin"), dir, filter));
}

TEST(FilterUnit, GlobLogPattern_MatchesVisibleLogFile)
{
    auto dir = test_support::caseDir("filter_glob_log");
    test_support::writeFile(dir / "app.log", "log");
    BackupFilter filter;
    filter.includePaths = {"*.log"};
    EXPECT_TRUE(fileMatchesFilter(entryFor(dir / "app.log"), dir, filter));
}

TEST(FilterUnit, GlobLogPattern_CurrentlyMatchesDotLogCompatibility)
{
    auto dir = test_support::caseDir("filter_dot_log");
    test_support::writeFile(dir / ".log", "hidden");
    BackupFilter globFilter;
    globFilter.includePaths = {"*.log"};
    BackupFilter explicitFilter;
    explicitFilter.includePaths = {".log"};
    EXPECT_TRUE(fileMatchesFilter(entryFor(dir / ".log"), dir, globFilter));
    EXPECT_TRUE(fileMatchesFilter(entryFor(dir / ".log"), dir, explicitFilter));
}

TEST(FilterUnit, ExcludePath_OverridesInclude)
{
    auto dir = test_support::caseDir("filter_exclude");
    test_support::writeFile(dir / "logs" / "app.log", "log");
    BackupFilter filter;
    filter.includePaths = {"logs/*"};
    filter.excludePaths = {"logs/app.log"};
    EXPECT_FALSE(fileMatchesFilter(entryFor(dir / "logs" / "app.log"), dir, filter));
}

TEST(FilterUnit, FileNameContains_FiltersByFilename)
{
    auto dir = test_support::caseDir("filter_name_contains");
    test_support::writeFile(dir / "report final.txt", "x");
    test_support::writeFile(dir / "draft.txt", "x");
    BackupFilter filter;
    filter.fileNameContains = "final";
    EXPECT_TRUE(fileMatchesFilter(entryFor(dir / "report final.txt"), dir, filter));
    EXPECT_FALSE(fileMatchesFilter(entryFor(dir / "draft.txt"), dir, filter));
}

TEST(FilterUnit, SizeBounds_AcceptOnlyWithinRange)
{
    auto dir = test_support::caseDir("filter_size");
    test_support::writeFile(dir / "small.txt", "12");
    test_support::writeFile(dir / "large.txt", "123456");
    BackupFilter filter;
    filter.minSize = 3;
    filter.maxSize = 5;
    EXPECT_FALSE(fileMatchesFilter(entryFor(dir / "small.txt"), dir, filter));
    EXPECT_FALSE(fileMatchesFilter(entryFor(dir / "large.txt"), dir, filter));
}

TEST(FilterUnit, UnicodeAndSpacePath_MatchesLexicalRelativePath)
{
    auto dir = test_support::caseDir("filter_unicode_space");
    test_support::writeFile(dir / "中文 目录" / "报告.txt", "x");
    BackupFilter filter;
    filter.includePaths = {"中文 目录/*"};
    EXPECT_TRUE(fileMatchesFilter(entryFor(dir / "中文 目录" / "报告.txt"), dir, filter));
}
