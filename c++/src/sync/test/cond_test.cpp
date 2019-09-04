#include <gtest/gtest.h>

#include "src/sync/cond.h"
#include "src/sync/test/lock_common_test.h"

class ConditionVariableFixture : public ::testing::Test
{
public:
    ConditionVariableFixture() {}

    void RunTest()
    {
        testSignalAndWait();
    }

private:
    void testSignalAndWait()
    {
        const uint32_t kThreadCount = 2;
        pthread_t tids[kThreadCount];
        uint32_t i = 0;
        for (; i < kThreadCount / 2; ++i)
        {
            pthread_create(&tids[i], NULL,
                    ConditionVariableFixture::processCondition1, NULL);
        }

        for (; i < kThreadCount; ++i)
        {
            pthread_create(&tids[i], NULL,
                    ConditionVariableFixture::processCondition2, NULL);
        }

        for (uint32_t i = 0; i < kThreadCount; ++i)
        {
            pthread_join(tids[i], NULL);
        }
    }

    static void* processCondition1(void* args)
    {
        ThisThread::SleepInUs(1000000);
        const uint32_t round = 100000;
        for (uint32_t i = 0; i < round; ++i)
        {
            mCond.Lock();
            int data = ++mValue;
            mCond.Signal();
            mCond.Unlock();
            mCond.Lock();
            mCond.Wait();
            EXPECT_EQ(mValue - data, 1);
            mCond.Unlock();
        }

        for (uint32_t i = 0; i < round; ++i)
        {
            mCond.Lock();
            int data = ++mValue;
            mCond.Broadcast();
            mCond.Unlock();
            mCond.Lock();
            mCond.Wait();
            EXPECT_EQ(mValue - data, 1);
            mCond.Unlock();
        }
        return NULL;
    }

    static void* processCondition2(void* args)
    {
        const uint32_t round = 100000;
        for (uint32_t i = 0; i < round; ++i)
        {
            mCond.Lock();
            int data = mValue;
            mCond.Wait();
            EXPECT_EQ(mValue - data, 1);
            ++mValue;
            mCond.Signal();
            mCond.Unlock();
        }

        for (uint32_t i = 0; i < round; ++i)
        {
            mCond.Lock();
            int data = mValue;
            mCond.Wait();
            EXPECT_EQ(mValue - data, 1);
            ++mValue;
            mCond.Broadcast();
            mCond.Unlock();
        }
        return NULL;
    }

private:
    static uint32_t mValue;
    static ConditionVariable mCond;
};

uint32_t ConditionVariableFixture::mValue = 0;
ConditionVariable ConditionVariableFixture::mCond;

TEST(ConditionVariable, Basic)
{
    typedef ConditionVariable LockType;

    LockType lock;
    lock.Lock();
    EXPECT_TRUE(lock.IsLocked());
    lock.Unlock();
    EXPECT_FALSE(lock.IsLocked());
    {
        ScopedLock<LockType> scopedLock(lock);
        EXPECT_TRUE(lock.IsLocked());
    }
    EXPECT_FALSE(lock.IsLocked());
}

TEST_F(ConditionVariableFixture, Simple)
{
    RunTest();
}
