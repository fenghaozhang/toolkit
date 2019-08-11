#include <gtest/gtest.h>

#include "src/common/flag.h"

DEFINE_FLAG_INT32(FlagInt32, 1, "");
DEFINE_FLAG_INT64(FlagInt64, 2, "");
DEFINE_FLAG_DOUBLE(FlagDouble, 3.0, "");
DEFINE_FLAG_STRING(FlagString, "4", "");

TEST(FlagTest, Set)
{
    EXPECT_EQ(INT32_FLAG(FlagInt32), 1);
    EXPECT_EQ(INT64_FLAG(FlagInt64), 2);
    EXPECT_EQ(DOUBLE_FLAG(FlagDouble), 3.0);
    EXPECT_TRUE(STRING_FLAG(FlagString) == "4");
}

TEST(FlagTest, Modify)
{
    INT32_FLAG(FlagInt32) = 10;
    INT64_FLAG(FlagInt64) = 20;
    DOUBLE_FLAG(FlagDouble) = 30.0;
    STRING_FLAG(FlagString) = "40";

    EXPECT_EQ(INT32_FLAG(FlagInt32), 10);
    EXPECT_EQ(INT64_FLAG(FlagInt64), 20);
    EXPECT_EQ(DOUBLE_FLAG(FlagDouble), 30.0);
    EXPECT_TRUE(STRING_FLAG(FlagString) == "40");
}
