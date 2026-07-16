#include <gtest/gtest.h>

#include "cloud/LocalCloudStorage.h"
#include "test_utils.h"

using namespace backup::cloud;

TEST(CloudStorageComponent, TypeIsLocal)
{
    auto dir = test_support::caseDir("cloud_type");
    LocalCloudStorage storage(dir.string());
    EXPECT_EQ(storage.type(), "local");
}

TEST(CloudStorageComponent, UploadListDownloadAndDelete)
{
    auto dir = test_support::caseDir("cloud_roundtrip");
    LocalCloudStorage storage((dir / "cloud").string());
    auto local = dir / "local.txt";
    auto downloaded = dir / "downloaded.txt";
    test_support::writeFile(local, "cloud-content");

    ASSERT_TRUE(storage.upload(local, "folder/remote.txt"));
    auto files = storage.list("folder");
    ASSERT_EQ(files.size(), 1U);
    EXPECT_EQ(files[0].path, "remote.txt");
    EXPECT_EQ(files[0].size, 13U);

    ASSERT_TRUE(storage.download("folder/remote.txt", downloaded));
    EXPECT_EQ(test_support::readFile(downloaded), "cloud-content");
    EXPECT_TRUE(storage.remove("folder/remote.txt"));
    EXPECT_TRUE(storage.list("folder").empty());
}

TEST(CloudStorageComponent, UnicodeAndSpaceRemotePath_RoundTrips)
{
    auto dir = test_support::caseDir("cloud_unicode_space");
    LocalCloudStorage storage((dir / "cloud").string());
    auto local = dir / "本地 文件.txt";
    auto downloaded = dir / "下载 文件.txt";
    test_support::writeFile(local, "unicode");
    ASSERT_TRUE(storage.upload(local, "中文 目录/远程 文件.txt"));
    ASSERT_TRUE(storage.download("中文 目录/远程 文件.txt", downloaded));
    EXPECT_EQ(test_support::readFile(downloaded), "unicode");
}

TEST(CloudStorageComponent, MissingDownload_ReturnsFalse)
{
    auto dir = test_support::caseDir("cloud_missing");
    LocalCloudStorage storage((dir / "cloud").string());
    EXPECT_FALSE(storage.download("missing.txt", dir / "out.txt"));
}

TEST(CloudStorageComponent, PathTraversal_IsRejected)
{
    auto dir = test_support::caseDir("cloud_traversal");
    LocalCloudStorage storage((dir / "cloud").string());
    auto local = dir / "local.txt";
    test_support::writeFile(local, "escape");
    EXPECT_FALSE(storage.upload(local, "../escape.txt"));
    EXPECT_FALSE(fs::exists(dir / "escape.txt"));
}
