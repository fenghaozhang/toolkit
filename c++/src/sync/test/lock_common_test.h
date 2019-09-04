#ifndef _SRC_SYNC_TEST_COMMON_TEST_H
#define _SRC_SYNC_TEST_COMMON_TEST_H

#include <gtest/gtest.h>

#include <stdio.h>
#include <typeinfo>

#include "src/sync/micro_lock.h"
#include "src/sync/posix_lock.h"

static std::map<int, int> gMap;

class LockTestFixture : public ::testing::Test
{
public:
    template <typename LockType> void testLock()
    {
        basicLockTest<LockType>();
        if (typeid(LockType) != typeid(NullLock))
        {
            multiThreadLockTest<LockType>();
        }
    }

    template <typename LockType> void testRWLock()
    {
        basicRWLockTest<LockType>();
        multiThreadRWLockTest<LockType>();
    }

    template <typename LockType> void testLockTimeout()
    {
        doTestLockTimeout<LockType>();
    }

    template <typename LockType> void testRWLockTimeout()
    {
        doTestRWLockTimeout<LockType>();
    }

    template <typename LockType> void testLockOwner()
    {
        doTestLockOwner<LockType>();
    }

    template <typename LockType> void testRWLockOwner()
    {
        doTestRWLockOwner<LockType>();
    }

private:
    template <typename LockType>
    struct LockInfo
    {
        LockType* lock;
    };

    template <typename LockType>
    static void* processLock(void* args);

    template <typename RWLockType>
    static void* processRWLock(void* args);

    template <typename LockType>
    void multiThreadLockTest();

    template <typename RWLockType>
    void multiThreadRWLockTest();

    template <typename LockType>
    void basicLockTest();

    template <typename LockType>
    void basicRWLockTest();

    template <typename LockType>
    void doTestLockTimeout();

    template <typename LockType>
    void doTestRWLockTimeout();

    template <typename LockType>
    void doTestLockOwner();

    template <typename LockType>
    void doTestRWLockOwner();

    template <typename LockType>
    static void criticalSectionOp(LockType* lock, int i);

    template <typename LockType>
    static void criticalSectionRWOp(LockType* lock, int i);
};

// Test implement

template <typename LockType>
void* LockTestFixture::processLock(void* args)
{
    LockInfo<LockType>* lockInfo =
        static_cast<LockInfo<LockType>*>(args);
    uint32_t round = 1000;
    for (uint32_t i = 1; i <= round; ++i)
    {
        criticalSectionOp(lockInfo->lock, i);
    }
    return NULL;
}

template <typename RWLockType>
void* LockTestFixture::processRWLock(void* args)
{
    LockInfo<RWLockType>* lockInfo =
        static_cast<LockInfo<RWLockType>*>(args);
    uint32_t round = 1000;
    for (uint32_t i = 1; i <= round; ++i)
    {
        criticalSectionRWOp(lockInfo->lock, i);
    }
    return NULL;
}

template <typename LockType>
void LockTestFixture::multiThreadLockTest()
{
    const uint32_t kThreadCount = 16;
    pthread_t tids[kThreadCount];
    LockInfo<LockType> args[kThreadCount];
    LockType lock;
    gMap.clear();
    gMap.insert(std::make_pair(0, 0));
    for (uint32_t i = 0; i < kThreadCount; ++i)
    {
        args[i].lock = &lock;
        pthread_create(&tids[i],
                       NULL,
                       LockTestFixture::processLock<LockType>,
                       reinterpret_cast<void*>(&args[i]));
    }
    for (uint32_t i = 0; i < kThreadCount; ++i)
    {
        pthread_join(tids[i], NULL);
    }
    EXPECT_EQ(gMap.size(), 1);
}

template <typename RWLockType>
void LockTestFixture::multiThreadRWLockTest()
{
    const uint32_t kThreadCount = 16;
    pthread_t tids[kThreadCount];
    LockInfo<RWLockType> args[kThreadCount];
    RWLockType lock;
    gMap.clear();
    gMap.insert(std::make_pair(0, 0));
    for (uint32_t i = 0; i < kThreadCount; ++i)
    {
        args[i].lock = &lock;
        pthread_create(&tids[i],
                       NULL,
                       LockTestFixture::processRWLock<RWLockType>,
                       reinterpret_cast<void*>(&args[i]));
    }
    for (uint32_t i = 0; i < kThreadCount; ++i)
    {
        pthread_join(tids[i], NULL);
    }
    EXPECT_EQ(gMap.size(), 1);
}

template <typename LockType>
void LockTestFixture::basicLockTest()
{
    LockType lock;
    lock.Lock();
    EXPECT_TRUE(lock.IsLocked());
    lock.Unlock();
    EXPECT_FALSE(lock.IsLocked());

    lock.Lock();
    EXPECT_FALSE(lock.TryLock());
    EXPECT_TRUE(lock.IsLocked());
    lock.Unlock();
    EXPECT_FALSE(lock.IsLocked());

    EXPECT_TRUE(lock.TryLock());
    EXPECT_TRUE(lock.IsLocked());
    EXPECT_FALSE(lock.TryLock());
    lock.Unlock();
    EXPECT_FALSE(lock.IsLocked());

    lock.Lock();
    EXPECT_FALSE(lock.TryLock());
    EXPECT_FALSE(lock.TryLock());
    lock.Unlock();
    EXPECT_FALSE(lock.IsLocked());
    {
        ScopedLock<LockType> scopedLock(lock);
        EXPECT_TRUE(lock.IsLocked());
    }
    EXPECT_FALSE(lock.IsLocked());
}

template <typename LockType>
void LockTestFixture::basicRWLockTest()
{
    LockType lock;
    lock.ReadLock();
    EXPECT_TRUE(lock.IsLocked());
    lock.ReadUnlock();
    EXPECT_FALSE(lock.IsLocked());

    lock.WriteLock();
    EXPECT_TRUE(lock.IsLocked());
    lock.WriteUnlock();
    EXPECT_FALSE(lock.IsLocked());

    lock.ReadLock();
    EXPECT_TRUE(lock.IsLocked());
    lock.ReadLock();
    EXPECT_TRUE(lock.TryReadLock());
    lock.ReadUnlock();  // Release ReadLock()
    lock.ReadUnlock();  // Release ReadLock()
    lock.ReadUnlock();  // Release TryReadLock()
    EXPECT_FALSE(lock.IsLocked());

    lock.ReadLock();
    EXPECT_FALSE(lock.TryWriteLock());
    EXPECT_FALSE(lock.TryWriteLock());
    EXPECT_FALSE(lock.TryWriteLock());
    lock.ReadUnlock();

    lock.WriteLock();
    EXPECT_FALSE(lock.TryReadLock());
    EXPECT_FALSE(lock.TryReadLock());
    EXPECT_FALSE(lock.TryWriteLock());
    EXPECT_FALSE(lock.TryWriteLock());
    lock.WriteUnlock();

    EXPECT_FALSE(lock.IsLocked());
    {
        ScopedRWLock<LockType> rScopedLock(lock, 'r');
        EXPECT_TRUE(lock.TryReadLock());
        EXPECT_FALSE(lock.TryWriteLock());
        lock.ReadUnlock();      // Release TryReadLock()
    }
    EXPECT_FALSE(lock.IsLocked());
    {
        ScopedRWLock<LockType> wScopedLock(lock, 'w');
        EXPECT_FALSE(lock.TryReadLock());
        EXPECT_FALSE(lock.TryWriteLock());
    }
    EXPECT_FALSE(lock.IsLocked());
}

template <typename LockType>
void LockTestFixture::doTestLockTimeout()
{
    uint64_t timeouts[] = { 10, 100, 1000, 10000, 100000 };
    for (int i = 0; i < COUNT_OF(timeouts); ++i)
    {
        LockType lock;
        lock.Lock();
        uint64_t timeout = timeouts[i];
        uint64_t begin = GetCurrentTimeInUs();
        EXPECT_TRUE(lock.IsLocked());
        EXPECT_FALSE(lock.TimedLock(timeout));
        EXPECT_TRUE(lock.IsLocked());
        uint64_t now = GetCurrentTimeInUs();
        EXPECT_GE(now - begin, timeout);
        lock.Unlock();
        EXPECT_FALSE(lock.IsLocked());

        begin = GetCurrentTimeInUs();
        EXPECT_TRUE(lock.TimedLock(timeout));
        now = GetCurrentTimeInUs();
        EXPECT_LT(now - begin, timeout);
        EXPECT_TRUE(lock.IsLocked());
        lock.Unlock();
        EXPECT_FALSE(lock.IsLocked());
    }
}

template <typename LockType>
void LockTestFixture::doTestRWLockTimeout()
{
    uint64_t timeouts[] = { 10, 100, 1000, 10000, 100000 };
    for (int i = 0; i < COUNT_OF(timeouts); ++i)
    {
        LockType lock;
        // 1. test lock can not be done when timeout.
        lock.ReadLock();
        uint64_t timeout = timeouts[i];
        uint64_t begin = GetCurrentTimeInUs();
        EXPECT_FALSE(lock.TimedWriteLock(timeout));
        uint64_t now = GetCurrentTimeInUs();
        EXPECT_TRUE(now - begin >= timeout);
        lock.ReadUnlock();
        EXPECT_FALSE(lock.IsLocked());

        lock.WriteLock();
        if (typeid(lock) != typeid(RWLock))
        {
            begin = GetCurrentTimeInUs();
            EXPECT_FALSE(lock.TimedReadLock(timeout));
            now = GetCurrentTimeInUs();
            EXPECT_GE(now - begin, timeout);
        }
        lock.WriteUnlock();
        EXPECT_FALSE(lock.IsLocked());

        // 2. test lock can be done when timeout.
        begin = GetCurrentTimeInUs();
        EXPECT_TRUE(lock.TimedReadLock(timeout));
        now = GetCurrentTimeInUs();
        EXPECT_LT(now - begin, timeout);
        lock.ReadUnlock();
        EXPECT_FALSE(lock.IsLocked());

        begin = GetCurrentTimeInUs();
        EXPECT_TRUE(lock.TimedWriteLock(timeout));
        now = GetCurrentTimeInUs();
        EXPECT_LT(now - begin, timeout);
        lock.WriteUnlock();
        EXPECT_FALSE(lock.IsLocked());
    }
}

template <typename LockType>
void LockTestFixture::doTestLockOwner()
{
    LockType lock;
    int tid = ThisThread::GetId();
    EXPECT_EQ(lock.Owner(), 0);
    lock.Lock();
    EXPECT_EQ(lock.Owner(), tid);
    lock.Unlock();
    EXPECT_EQ(lock.Owner(), 0);

    lock.TryLock();
    EXPECT_EQ(lock.Owner(), tid);
    EXPECT_FALSE(lock.TimedLock(100000));
    EXPECT_EQ(lock.Owner(), tid);
    lock.Unlock();
    EXPECT_EQ(lock.Owner(), 0);
}

template <typename LockType>
void LockTestFixture::doTestRWLockOwner()
{
    LockType lock;
    int tid = ThisThread::GetId();
    EXPECT_EQ(lock.GetWriteLockOwner(), 0);
    lock.ReadLock();
    EXPECT_EQ(lock.GetWriteLockOwner(), 0);
    EXPECT_FALSE(lock.TimedWriteLock(100000));
    EXPECT_EQ(lock.GetWriteLockOwner(), 0);
    EXPECT_FALSE(lock.TryWriteLock());
    EXPECT_EQ(lock.GetWriteLockOwner(), 0);
    lock.ReadUnlock();

    lock.WriteLock();
    EXPECT_EQ(lock.GetWriteLockOwner(), tid);
    lock.WriteUnlock();
    EXPECT_EQ(lock.GetWriteLockOwner(), 0);
    EXPECT_TRUE(lock.TimedWriteLock(100000));
    EXPECT_EQ(lock.GetWriteLockOwner(), tid);
    lock.WriteUnlock();
    EXPECT_EQ(lock.GetWriteLockOwner(), 0);
    EXPECT_TRUE(lock.TryWriteLock());
    EXPECT_EQ(lock.GetWriteLockOwner(), tid);
    lock.WriteUnlock();
    EXPECT_EQ(lock.GetWriteLockOwner(), 0);
}

template <typename LockType>
void LockTestFixture::criticalSectionOp(
        LockType* lock, int i)
{
    lock->Lock();
    gMap.insert(std::make_pair(i, i));
    gMap.erase(i);
    lock->Unlock();
}

template <typename LockType>
void LockTestFixture::criticalSectionRWOp(
        LockType* lock, int i)
{
    for (uint32_t j = 0; j < 9; ++j)
    {
        lock->ReadLock();
        gMap.find(i);
        lock->ReadUnlock();
    }
    for (uint32_t j = 0; j < 1; ++j)
    {
        lock->WriteLock();
        gMap.insert(std::make_pair(i, i));
        gMap.erase(i);
        lock->WriteUnlock();
    }
}

#endif  // _SRC_SYNC_TEST_COMMON_TEST_H
