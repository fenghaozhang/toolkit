#include <gtest/gtest.h>

#include <stdio.h>
#include <time.h>
#include <vector>

#include "src/base/gettime.h"
#include "src/common/assert.h"
#include "src/memory/objcache.h"

class TestObject
{
public:
    TestObject() { sConstructorCalls++; }
    TestObject(int a_, int b_, int c_, int d_)
        : a(a_), b(b_), c(c_), d(d_)
    {
        sConstructorCalls++;
    }
    ~TestObject() { sDestructorCalls++; }

    bool operator==(const TestObject& other)
    {
        return a == other.a && b == other.b && c == other.c && d == other.d;
    }

    static void ConstructHelper(void* p) { new (p) TestObject(); }
    static void DestructHelper(void* p)
    {
        static_cast<TestObject*>(p)->~TestObject();
    }

    static int sConstructorCalls;
    static int sDestructorCalls;

private:
    int a, b, c, d;
};

int TestObject::sConstructorCalls;
int TestObject::sDestructorCalls;

TEST(ObjectCacheTest, TestAllocFree)
{
    ObjectCache<int> pool;
    int* a = reinterpret_cast<int*>(pool.Alloc());
    int* b = reinterpret_cast<int*>(pool.Alloc());
    EXPECT_TRUE(a != NULL);
    EXPECT_TRUE(b != NULL);
    EXPECT_NE(a, b);
    pool.Dealloc(a);
    pool.Dealloc(b);
}

TEST(ObjectCacheTest, TestReuse)
{
    ObjectCache<int> pool;
    int* a = reinterpret_cast<int*>(pool.Alloc());
    pool.Dealloc(a);
    int* b = reinterpret_cast<int*>(pool.Alloc());
    EXPECT_EQ(a, b);
    pool.Dealloc(b);
}

TEST(ObjectCacheTest, TestIfNotFree)
{
    ObjectCache<int> pool(NULL, /* construct */ true);
    int* a = reinterpret_cast<int*>(pool.Alloc());
    int* b = reinterpret_cast<int*>(pool.Alloc());
    int* c = reinterpret_cast<int*>(pool.Alloc());
    int* d = reinterpret_cast<int*>(pool.Alloc());
    EXPECT_TRUE(a != NULL);
    EXPECT_TRUE(b != NULL);
    EXPECT_TRUE(c != NULL);
    EXPECT_TRUE(d != NULL);
    // Do NOT free b, c, d
}

TEST(ObjectCacheTest, TestConstructDestruct)
{
    TestObject::sConstructorCalls = 0;
    TestObject::sDestructorCalls = 0;
    {
        ObjectCache<TestObject> pool(NULL, /* construct */ true);
        EXPECT_EQ(0, TestObject::sConstructorCalls);
        EXPECT_EQ(0, TestObject::sDestructorCalls);
        TestObject* a = reinterpret_cast<TestObject*>(pool.Alloc());
        EXPECT_LT(0, TestObject::sConstructorCalls);
        EXPECT_EQ(0, TestObject::sDestructorCalls);
        pool.Dealloc(a);
    }
    EXPECT_EQ(TestObject::sConstructorCalls, TestObject::sDestructorCalls);
}

TEST(ObjectCacheTest, TestFreeNull)
{
    ObjectCache<TestObject> pool;
    pool.Dealloc(NULL);  // expect no crash
}

TEST(ObjectCacheTest, TestReserve)
{
    ObjectCache<TestObject> pool;
    MemCacheStat stats;
    pool.Stat(&stats);
    EXPECT_EQ(1UL, stats.pages);
}

TEST(ObjectCacheTest, TestGrowShrink)
{
    ObjectCache<int> pool;
    std::vector<int*> mem;
    MemCacheStat stats;
    pool.Stat(&stats);
    uint32_t reservedPages = stats.reservedPages;
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
    EXPECT_EQ(reservedPages, stats.pages);
}

TEST(ObjectCacheTest, Simple)
{
    std::vector<TestObject> trainingSets;
    std::vector<TestObject*> validateSets;
    srand((unsigned)time(NULL));
    ObjectCache<TestObject> pool;

    uint64_t start = GetCurrentTimeInUs();
    int rounds = 1UL * 10000000;
    for (int i = 0; i < rounds; ++i)
    {
        TestObject obj(rand(), rand(), rand(), rand());   // NOLINT(runtime/threadsafe_fn)
        trainingSets.push_back(obj);
        TestObject* ptr = reinterpret_cast<TestObject*>(pool.Alloc());
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
