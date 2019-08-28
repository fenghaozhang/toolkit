#include <gtest/gtest.h>

#include "src/base/exponential_backoff.h"

TEST(ExponentialBackoff, Simple)
{
    ExponentialBackoff e;
    e.Reset(10, 50, 2);
    EXPECT_EQ(10UL, e.Next());
    EXPECT_EQ(20UL, e.Next());
    EXPECT_EQ(40UL, e.Next());
    EXPECT_EQ(50UL, e.Next());
    EXPECT_EQ(50UL, e.Next());
}
