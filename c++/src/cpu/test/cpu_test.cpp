#include <gtest/gtest.h>

#include "src/cpu/cpu.h"

TEST(Cpu, Basic)
{
    EXPECT_TRUE(DOES_CPU_SUPPORT(sse));
    EXPECT_TRUE(DOES_CPU_SUPPORT(sse2));
    EXPECT_TRUE(DOES_CPU_SUPPORT(sse3));
    EXPECT_FALSE(DOES_CPU_SUPPORT(ptwrite));
}
