#include <gtest/gtest.h>

#include "core/BackupEngine.h"
#include "core/Incremental.h"
#include "test_utils.h"

#include <nlohmann/json.hpp>

using namespace backup;
using json = nlohmann::json;

namespace {

BackupFilter noArchive()
{
    BackupFilter filter;
    filter.archiveType = "none";
    return filter;
}

json readJson(const fs::path &path)
{
    return json::parse(test_support::readFile(path));
}

} // namespace

TEST(MetadataManifestIntegration, MetadataJson_RecordsSourceAndOptions)
{
    auto dir = test_support::caseDir("meta_metadata");
    test_support::writeFile(dir / "source" / "a.txt", "A");
    auto filter = noArchive();
    filter.username = "tester";
    filter.userId = 9;
    filter.extensions = {".txt"};
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", filter);
    auto meta = readJson(backup / "metadata.json");
    EXPECT_EQ(meta.at("source").get<std::string>(), (dir / "source").string());
    EXPECT_EQ(meta.at("username").get<std::string>(), "tester");
    EXPECT_EQ(meta.at("archiveType").get<std::string>(), "none");
    EXPECT_FALSE(meta.at("encrypt").get<bool>());
    EXPECT_EQ(meta.at("encryptAlgo").get<std::string>(), "none");
    EXPECT_EQ(meta.at("filter").at("extensions")[0].get<std::string>(), ".txt");
}

TEST(MetadataManifestIntegration, ManifestJson_ContainsFileHashes)
{
    auto dir = test_support::caseDir("meta_manifest");
    test_support::writeFile(dir / "source" / "a.txt", "A");
    test_support::writeFile(dir / "source" / "nested" / "b.txt", "B");
    auto backup = createBackup(dir / "source", dir / "backups", false, false, "", noArchive());
    auto manifest = loadIncrementalManifest(backup);
    EXPECT_EQ(manifest.size(), 2U);
    EXPECT_TRUE(manifest.count("a.txt") == 1);
    EXPECT_TRUE(manifest.count("nested/b.txt") == 1);
}

TEST(MetadataManifestIntegration, IncrementalNoChanges_WritesIncskipMarkers)
{
    auto dir = test_support::caseDir("meta_incremental_no_change");
    test_support::writeFile(dir / "source" / "a.txt", "A");
    auto full = createBackup(dir / "source", dir / "backups", false, false, "", noArchive());
    test_support::waitForNextSecond();
    auto filter = noArchive();
    filter.incremental = true;
    filter.incrementalBase = full.string();
    auto incremental = createBackup(dir / "source", dir / "backups2", false, false, "", filter);
    EXPECT_TRUE(fs::exists(incremental / "a.txt.incskip"));
}

TEST(MetadataManifestIntegration, IncrementalModifiedFile_CopiesChangedContent)
{
    auto dir = test_support::caseDir("meta_incremental_modified");
    auto source = dir / "source" / "a.txt";
    test_support::writeFile(source, "A");
    auto full = createBackup(dir / "source", dir / "backups", false, false, "", noArchive());
    test_support::waitForNextSecond();
    test_support::writeFile(source, "B");
    auto filter = noArchive();
    filter.incremental = true;
    filter.incrementalBase = full.string();
    auto incremental = createBackup(dir / "source", dir / "backups2", false, false, "", filter);
    EXPECT_EQ(test_support::readFile(incremental / "a.txt"), "B");
}

TEST(MetadataManifestIntegration, IncrementalNewFile_CopiesNewFile)
{
    auto dir = test_support::caseDir("meta_incremental_new");
    test_support::writeFile(dir / "source" / "a.txt", "A");
    auto full = createBackup(dir / "source", dir / "backups", false, false, "", noArchive());
    test_support::waitForNextSecond();
    test_support::writeFile(dir / "source" / "new.txt", "N");
    auto filter = noArchive();
    filter.incremental = true;
    filter.incrementalBase = full.string();
    auto incremental = createBackup(dir / "source", dir / "backups2", false, false, "", filter);
    EXPECT_EQ(test_support::readFile(incremental / "new.txt"), "N");
}
