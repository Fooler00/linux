#include <gtest/gtest.h>

#include "core/Incremental.h"
#include "test_utils.h"

using namespace backup;

TEST(ManifestUnit, MissingManifest_ReturnsEmptyMap)
{
    auto dir = test_support::caseDir("manifest_missing");
    EXPECT_TRUE(loadIncrementalManifest(dir).empty());
}

TEST(ManifestUnit, WriteAndLoadManifest_RoundTripsHashes)
{
    auto dir = test_support::caseDir("manifest_roundtrip");
    std::map<std::string, std::string> hashes = {{"a.txt", "abc"}, {"nested/b.txt", "def"}};
    writeIncrementalManifest(dir, hashes);
    EXPECT_EQ(loadIncrementalManifest(dir), hashes);
}

TEST(ManifestUnit, CorruptManifest_CurrentCompatibilityReturnsEmpty)
{
    auto dir = test_support::caseDir("manifest_corrupt");
    test_support::writeFile(dir / "manifest.json", "{not-json");
    EXPECT_TRUE(loadIncrementalManifest(dir).empty());
}

TEST(ManifestUnit, EmptyManifestObject_ReturnsEmptyMap)
{
    auto dir = test_support::caseDir("manifest_empty");
    test_support::writeFile(dir / "manifest.json", "{}");
    EXPECT_TRUE(loadIncrementalManifest(dir).empty());
}

TEST(ManifestUnit, ManifestFileName_IsManifestJson)
{
    auto dir = test_support::caseDir("manifest_filename");
    writeIncrementalManifest(dir, {{"file.txt", "hash"}});
    EXPECT_TRUE(fs::exists(dir / "manifest.json"));
}
