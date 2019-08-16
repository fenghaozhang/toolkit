#include <gtest/gtest.h>

#include "src/common/errorcode.h"

TEST(ErrorCodeTest, Simple)
{
    EXPECT_EQ(GetErrorString(UNIT_TEST), "For unit test");
}
