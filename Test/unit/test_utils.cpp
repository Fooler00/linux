#include <gtest/gtest.h>

#include "core/Utils.h"
#include "test_utils.h"

#include <chrono>

using namespace backup;

TEST(UtilsUnit, BytesToHex_ConvertsLowercaseHex)
{
    const unsigned char bytes[] = {0x00, 0x0f, 0xa5, 0xff};
    EXPECT_EQ(bytesToHex(bytes, sizeof(bytes)), "000fa5ff");
}

TEST(UtilsUnit, Sha256Hex_KnownDigest)
{
    EXPECT_EQ(sha256Hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(UtilsUnit, RandomHex_ReturnsTwoCharsPerByte)
{
    auto value = randomHex(16);
    EXPECT_EQ(value.size(), 32U);
    EXPECT_TRUE(value.find_first_not_of("0123456789abcdef") == std::string::npos);
}

TEST(UtilsUnit, HashPassword_IsStableForSameSalt)
{
    EXPECT_EQ(hashPassword("secret", "salt"), hashPassword("secret", "salt"));
    EXPECT_NE(hashPassword("secret", "salt"), hashPassword("other", "salt"));
}

TEST(UtilsUnit, ShellQuote_QuotesShellMetacharacters)
{
    EXPECT_EQ(shellQuote("simple"), "'simple'");
    EXPECT_EQ(shellQuote("a b;c"), "'a b;c'");
    EXPECT_EQ(shellQuote("a'b"), "'a'\\''b'");
}

TEST(UtilsUnit, ParseTimeString_AcceptsDateTime)
{
    std::chrono::system_clock::time_point tp;
    EXPECT_TRUE(parseTimeString("2026-07-16 12:34:56", tp));
    EXPECT_FALSE(parseTimeString("not-a-time", tp));
}

TEST(UtilsUnit, DirectorySize_SumsNestedFiles)
{
    auto dir = test_support::caseDir("utils_directory_size");
    test_support::writeFile(dir / "a.txt", "abc");
    test_support::writeFile(dir / "nested" / "b.txt", "12345");
    EXPECT_EQ(directorySize(dir), 8U);
}
