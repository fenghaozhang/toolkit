#include <gtest/gtest.h>

#include <stdio.h>
#include <time.h>
#include <vector>

#include "src/base/gettime.h"
#include "src/common/assert.h"
#include "src/memory/memcache.h"

struct Item
{
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;

    Item(uint64_t a_, uint64_t b_, uint64_t c_, uint64_t d_)
        : a(a_), b(b_), c(c_), d(d_)
    {
    }

    bool operator==(const Item& other)
    {
        return a == other.a && b == other.b &&
               c == other.c && d == other.d;
    }
};

class MemCacheFixture : public ::testing::Test
{
public:
    MemCacheFixture() {}

    void SetUp() override
    {
    }

    void TearDown() override
    { }
};

TEST_F(MemCacheFixture, Simple)
{
    std::vector<Item> trainingSets;
    std::vector<Item*> validateSets;
    srand((unsigned)time(NULL));
    MemCache::Options options;
    options.objSize = sizeof(Item);
    MemCache pool;
    pool.Init(options);

    uint64_t start = GetCurrentTimeInUs();
    int rounds = 1UL * 10000000;
    for (int i = 0; i < rounds; ++i)
    {
        Item obj(rand(), rand(), rand(), rand());   // NOLINT(runtime/threadsafe_fn)
        trainingSets.push_back(obj);
        Item* ptr = reinterpret_cast<Item*>(pool.Alloc());
        *ptr = trainingSets[i];
        validateSets.push_back(ptr);
    }

    for (int i = 0; i < rounds; ++i)
    {
        ASSERT(trainingSets[i] == *validateSets[i]);
    }

    for (int i = 0; i < rounds; ++i)
    {
        pool.Dealloc(validateSets[i]);
    }

    uint64_t end = GetCurrentTimeInUs();
    printf("Successfully! Time consumes: %ld (us)\n", end - start);
}
