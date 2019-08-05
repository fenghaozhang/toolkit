#include <gtest/gtest.h>

#include <stdio.h>
#include <time.h>
#include <vector>

#include "src/base/gettime.h"
#include "src/common/assert.h"
#include "src/memory/memcache.h"

class TestItem
{
public:
    TestItem() { sConstructorCalls++; }
    TestItem(int a_, int b_, int c_, int d_)
        : a(a_), b(b_), c(c_), d(d_)
    {
        sConstructorCalls++;
    }
    ~TestItem() { sDestructorCalls++; }

    bool operator==(const TestItem& other)
    {
        return a == other.a && b == other.b && c == other.c && d == other.d;
    }

    static void ConstructHelper(void* p) { new (p) TestItem(); }
    static void DestructHelper(void* p)
    {
        static_cast<TestItem*>(p)->~TestItem();
    }

    static int sConstructorCalls;
    static int sDestructorCalls;

private:
    int a, b, c, d;
};

int TestItem::sConstructorCalls;
int TestItem::sDestructorCalls;

TEST(MemCacheTest, TestAllocFree)
{
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    MemCache pool;
    pool.Init(options);
    int* a = reinterpret_cast<int*>(pool.Alloc());
    int* b = reinterpret_cast<int*>(pool.Alloc());
    EXPECT_TRUE(a != NULL);
    EXPECT_TRUE(b != NULL);
    EXPECT_NE(a, b);
    pool.Dealloc(a);
    pool.Dealloc(b);
}

TEST(MemCacheTest, TestReuse)
{
    MemCache pool;
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    pool.Init(options);
    int* a = reinterpret_cast<int*>(pool.Alloc());
    pool.Dealloc(a);
    int* b = reinterpret_cast<int*>(pool.Alloc());
    EXPECT_EQ(a, b);
    pool.Dealloc(b);
}

TEST(MemCacheTest, TestNoSpace)
{
    MemCache pool;
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    options.limit = 1;
    pool.Init(options);
    int* a = reinterpret_cast<int*>(pool.Alloc());
    int* b = reinterpret_cast<int*>(pool.Alloc());
    EXPECT_TRUE(a != NULL);
    EXPECT_TRUE(b == NULL);
    pool.Dealloc(a);
    int* c = reinterpret_cast<int*>(pool.Alloc());
    EXPECT_TRUE(c != NULL);
    pool.Dealloc(c);
}

TEST(MemCacheTest, TestIfNotFree)
{
    MemCache pool;
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    options.ctor = &TestItem::ConstructHelper;
    options.dtor = &TestItem::DestructHelper;
    pool.Init(options);
    int* a = reinterpret_cast<int*>(pool.Alloc());
    int* b = reinterpret_cast<int*>(pool.Alloc());
    int* c = reinterpret_cast<int*>(pool.Alloc());
    int* d = reinterpret_cast<int*>(pool.Alloc());
    EXPECT_TRUE(a != NULL);
    EXPECT_TRUE(b != NULL);
    EXPECT_TRUE(c != NULL);
    EXPECT_TRUE(d != NULL);
    pool.Dealloc(a);
    // Do NOT free b, c, d
}

TEST(MemCacheTest, TestConstructDestruct)
{
    TestItem::sConstructorCalls = 0;
    TestItem::sDestructorCalls = 0;
    {
        MemCache pool;
        MemCache::Options options;
        options.ctor = &TestItem::ConstructHelper;
        options.dtor = &TestItem::DestructHelper;
        options.objSize = sizeof(TestItem);
        options.reserve = 0;
        pool.Init(options);
        EXPECT_EQ(0, TestItem::sConstructorCalls);
        EXPECT_EQ(0, TestItem::sDestructorCalls);
        TestItem* a = reinterpret_cast<TestItem*>(pool.Alloc());
        EXPECT_LT(0, TestItem::sConstructorCalls);
        EXPECT_EQ(0, TestItem::sDestructorCalls);
        pool.Dealloc(a);
    }
    EXPECT_EQ(TestItem::sConstructorCalls, TestItem::sDestructorCalls);
}

TEST(MemCacheTest, TestFreeNull)
{
    MemCache pool;
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    pool.Init(options);
    pool.Dealloc(NULL);  // expect no crash
}

TEST(MemCacheTest, TestReserve)
{
    MemCache pool;
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    options.reserve = 1;
    pool.Init(options);
    MemCacheStat stats;
    pool.Stat(&stats);
    EXPECT_EQ(1UL, stats.pages);
}

TEST(MemCacheTest, TestGrowShrink)
{
    MemCache pool;
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    options.reserve = 0;
    pool.Init(options);
    std::vector<int*> mem;
    MemCacheStat stats;
    pool.Stat(&stats);
    EXPECT_EQ(0UL, stats.reservedPages);
    do
    {
        mem.push_back(reinterpret_cast<int*>(pool.Alloc()));
        pool.Stat(&stats);
    } while (stats.pages < 10UL);
    for (size_t i = 0; i < mem.size(); i++)
    {
        pool.Dealloc(mem[i]);
    }
    pool.Stat(&stats);
    EXPECT_EQ(0UL, stats.pages);
}

TEST(MemCacheTest, TestReserveMore)
{
    MemCache pool;
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    options.reserve = 64 * 1024;
    pool.Init(options);
    MemCacheStat stats;
    pool.Stat(&stats);
    EXPECT_LE(options.reserve / stats.objPerPage, stats.reservedPages);
    EXPECT_EQ(stats.reservedPages, stats.pages);
}

TEST(MemCacheTest, FreeAllPages)
{
    TestItem::sConstructorCalls = 0;
    TestItem::sDestructorCalls = 0;
    MemCache pool;
    MemCache::Options options;
    options.reserve = 0;
    options.objSize = sizeof(TestItem);
    options.ctor = &TestItem::ConstructHelper;
    options.dtor = &TestItem::DestructHelper;
    pool.Init(options);
    EXPECT_EQ(0, TestItem::sConstructorCalls);
    EXPECT_EQ(0, TestItem::sDestructorCalls);
    void* a = pool.Alloc();
    EXPECT_NE(a, nullptr);
    EXPECT_LT(0, TestItem::sConstructorCalls);
    EXPECT_EQ(0, TestItem::sDestructorCalls);
    pool.Dealloc(a);
    pool.freeAllPages();
    EXPECT_EQ(TestItem::sConstructorCalls, TestItem::sDestructorCalls);
    MemCacheStat stats;
    pool.Stat(&stats);
    EXPECT_EQ(0, stats.pages);
    EXPECT_EQ(0, stats.objCount);
}

TEST(MemCacheTest, Simple)
{
    std::vector<TestItem> trainingSets;
    std::vector<TestItem*> validateSets;
    srand((unsigned)time(NULL));
    MemCache pool;
    MemCache::Options options;
    options.objSize = sizeof(TestItem);
    pool.Init(options);

    uint64_t start = GetCurrentTimeInUs();
    int rounds = 1UL * 10000000;
    for (int i = 0; i < rounds; ++i)
    {
        TestItem obj(rand(), rand(), rand(), rand());   // NOLINT(runtime/threadsafe_fn)
        trainingSets.push_back(obj);
        TestItem* ptr = reinterpret_cast<TestItem*>(pool.Alloc());
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
