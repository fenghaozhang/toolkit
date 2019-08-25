#include <gtest/gtest.h>

#include <stdio.h>
#include <time.h>
#include <vector>

#include "src/common/macros.h"
#include "src/memory/arena.h"
#include "src/common/assert.h"

static void testAlloc(
        size_t count, int size, bool aligned)
{
    Arena pool;
    std::vector<void*> results;
    for (size_t i = 0; i < count; ++i)
    {
        void* ptr = aligned ? pool.AllocAligned(size) : pool.Alloc(size);
        memset(ptr, 'a' + i % 26, size);
        results.push_back(ptr);
    }
    void* comparePtr = malloc(size);
    for (size_t i = 0; i < count; ++i)
    {
        memset(comparePtr, 'a' + i % 26, size);
        EXPECT_EQ(memcmp(results[i], comparePtr, size), 0);
    }
    free(comparePtr);
}

static void testMemoryUsage(
        size_t bufferSize, size_t count, size_t size, bool aligned)
{
    if (size > bufferSize)
    {
        return;
    }
    const size_t kAlign = 8;
    size_t totalUsage = 0;
    size_t usageInPage = 0;
    size_t actualSize = aligned
        ? (size + kAlign - 1) / kAlign * kAlign : size;
    Arena pool(bufferSize);
    for (size_t i = 0; i < count; ++i)
    {
        aligned ? pool.AllocAligned(size) : pool.Alloc(size);
        if (usageInPage + actualSize > bufferSize)
        {
            totalUsage += bufferSize - usageInPage;
            usageInPage = 0;
        }
        usageInPage += actualSize;
        totalUsage += actualSize;
    }
    totalUsage = (totalUsage + bufferSize - 1)
        / bufferSize * bufferSize;
    totalUsage += sizeof(Arena);
    ASSERT_EQ(pool.GetMemoryUsage(), totalUsage);
}

TEST(ArenaTest, Alloc)
{
    size_t eleCount[] = { 10, 100, 1000, 10000, 100000, 1000000 };
    size_t eleSize[] = { 5, 10, 20, 43, 120, 255, 1024 };
    for (size_t i = 0; i < COUNT_OF(eleCount); ++i)
    {
        for (size_t j = 0; j < COUNT_OF(eleSize); ++j)
        {
            testAlloc(eleCount[i], eleSize[j], true);
            testAlloc(eleCount[i], eleSize[j], false);
        }
    }
}

TEST(ArenaTest, AllocLarge)
{
    Arena pool(4096);
    EXPECT_TRUE(pool.Alloc(0) != NULL);
    EXPECT_TRUE(pool.Alloc(4095) != NULL);
    EXPECT_TRUE(pool.Alloc(4096) !=  NULL);
    EXPECT_TRUE(pool.Alloc(4097) == NULL);
    EXPECT_TRUE(pool.Alloc(10240) == NULL);
}

TEST(ArenaTest, MemorySizeTest)
{
    size_t bufferSize[] = { 128, 256, 512, 1024, 4096, 10240};
    size_t eleCount[] = { 10, 100, 1000, 10000 };
    size_t eleSize[] = { 5, 10, 20, 43, 120, 255};
    {
        for (size_t i = 0; i < COUNT_OF(bufferSize); ++i)
        {
            for (size_t j = 0; j < COUNT_OF(eleCount); ++j)
            {
                for (size_t k = 0; k < COUNT_OF(eleSize); ++k)
                {
                    testMemoryUsage(bufferSize[i], eleCount[j], eleSize[k], false);
                    testMemoryUsage(bufferSize[i], eleCount[j], eleSize[k], true);
                }
            }
        }
    }
}
