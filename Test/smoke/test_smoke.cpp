#include <gtest/gtest.h>

TEST(TestInfrastructureSmoke, BasicAssertionPasses)
{
    EXPECT_EQ(1 + 1, 2);
}
