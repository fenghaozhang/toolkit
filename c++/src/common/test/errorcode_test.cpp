#include <gtest/gtest.h>

#include "src/common/errorcode.h"

TEST(ErrorCodeTest, Simple)
{
    EXPECT_EQ(GetErrorString(OK), "OK");
    EXPECT_EQ(GetErrorString(INTERNAL_ERROR), "Internal error");
}
