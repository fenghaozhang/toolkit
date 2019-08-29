#include <gtest/gtest.h>

#include <map>
#include "src/base/gettime.h"
#include "src/memory/scoped_ptr.h"
#include "src/sync/micro_lock.h"
#include "src/thread/this_thread.h"

std::map<int, int> gMap;
std::map<int, int> gMapArray[3];

struct ThreadArgs
{
    MicroLock* microLock;
    MicroLockVector* microLockVector;
    MicroRWLock* rLock;
    MicroRWLockPreferWrite* wLock;
};

void* ProcessMicroLock(void* args)
{
    ThreadArgs* thArgs = (ThreadArgs *)args;
    for (uint32_t i = 1; i <= 1000; ++i)
    {
        thArgs->microLock->Lock();
        gMap.insert(std::make_pair(i, i));
        gMap.erase(i);
        thArgs->microLock->Unlock();
    }
    return NULL;
}

void* ProcessMicroLockVector(void* args)
{
    ThreadArgs* thArgs = (ThreadArgs *)args;
    for (uint32_t i = 1; i <= 1000; ++i)
    {
        thArgs->microLockVector->Lock(i % 3);
        gMapArray[i % 3].insert(std::make_pair(i, i));
        thArgs->microLockVector->Unlock(i % 3);
    }
    return NULL;
}

void* ProcessMicroRWLock(void* args)
{
    ThreadArgs* thArgs = (ThreadArgs *)args;
    // Prefer read.
    for (uint32_t i = 1; i <= 1000; ++i)
    {
        for (uint32_t j = 0; j < 9; ++j)
        {
            thArgs->rLock->ReadLock();
            gMap.find(i);
            thArgs->rLock->ReadUnlock();
        }
        for (uint32_t j = 0; j < 1; ++j)
        {
            thArgs->rLock->WriteLock();
            gMap.insert(std::make_pair(i, i));
            gMap.erase(i);
            thArgs->rLock->WriteUnlock();
        }
    }
    return NULL;
}

TEST(MicroLock, SingleThreadTest)
{
    MicroLock locker;
    locker.Lock();
    EXPECT_TRUE(locker.IsLocked());
    locker.Unlock();
    EXPECT_TRUE(locker.TryLock());
    EXPECT_TRUE(locker.IsLocked());
    locker.Unlock();
    locker.Lock();
    EXPECT_TRUE(!locker.TryLock());
    EXPECT_TRUE(!locker.TryLock());
    locker.Unlock();
    {
        ScopedLock<MicroLock> lock(locker);
        EXPECT_TRUE(locker.IsLocked());
    }
    EXPECT_TRUE(!locker.IsLocked());
}

TEST(MicroLock, MultipleThreadTest)
{
    static const uint32_t kNumThreads = 16;
    pthread_t thid[kNumThreads];
    ThreadArgs args[kNumThreads];
    scoped_ptr<MicroLock> locker(new MicroLock);
    gMap.clear();
    gMap.insert(std::make_pair(0, 0));
    for (uint32_t i = 0; i < kNumThreads; ++i)
    {
        args[i].microLock = locker.get();
        pthread_create(&thid[i], NULL, ProcessMicroLock, (void *)&args[i]);
    }
    for (uint32_t i = 0; i < kNumThreads; ++i)
    {
        pthread_join(thid[i], NULL);
    }
    EXPECT_EQ(gMap.size(), 1);
}

TEST(MicroLock, TimeoutTest)
{
    MicroLock testLock;
    // 1. test lock can not done when timeout.
    testLock.Lock();
    uint64_t testTimeout = 1000000;
    uint64_t lockBegin = GetCurrentTimeInUs();
    bool success = testLock.TimedLock(testTimeout);
    uint64_t lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(!success);
    EXPECT_TRUE(lockEnd - lockBegin >= testTimeout);
    testLock.Unlock();

    // 2. test lock can be done when timeout.
    testTimeout = 100;
    lockBegin = GetCurrentTimeInUs();
    success = testLock.TimedLock(testTimeout);
    lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(success);
    EXPECT_TRUE(lockEnd - lockBegin < testTimeout);
    testLock.Unlock();
}

TEST(MicroLock, OwnerTest)
{
    MicroLock testLock;
    int tid = ThisThread::GetId();
    EXPECT_EQ(testLock.GetLockOwner(), 0);
    testLock.Lock();
    EXPECT_EQ(testLock.GetLockOwner(), tid);
    testLock.Unlock();
    EXPECT_EQ(testLock.GetLockOwner(), 0);

    testLock.TryLock();
    EXPECT_EQ(testLock.GetLockOwner(), tid);
    testLock.TimedLock(100000); // will not succeed.
    EXPECT_EQ(testLock.GetLockOwner(), tid);
    testLock.Unlock();
    EXPECT_EQ(testLock.GetLockOwner(), 0);
}

TEST(MicroLockVector, SingleThreadTest)
{
    MicroLockVector locker(10);
    locker.Lock(0);
    EXPECT_TRUE(locker.IsLocked(0));
    EXPECT_TRUE(!locker.IsLocked(1));
    locker.Unlock(0);
    EXPECT_TRUE(locker.TryLock(0));
    EXPECT_TRUE(locker.IsLocked(0));
    locker.Unlock(0);
    locker.Lock(0);
    EXPECT_TRUE(!locker.TryLock(0));
    EXPECT_TRUE(!locker.TryLock(0));
    locker.Unlock(0);
}

TEST(MicroLockVector, MultipleThreadTest)
{
    static const uint32_t kNumThreads = 16;
    pthread_t thid[kNumThreads];
    ThreadArgs args[kNumThreads];
    scoped_ptr<MicroLockVector> locker(new MicroLockVector(3));
    gMap.clear();
    for (uint32_t i = 0; i < kNumThreads; ++i)
    {
        args[i].microLockVector = locker.get();
        pthread_create(&thid[i], NULL, ProcessMicroLockVector, (void *)&args[i]);
    }
    for (uint32_t i = 0; i < kNumThreads; ++i)
    {
        pthread_join(thid[i], NULL);
    }
    uint32_t totalSize = 0;
    for (uint32_t i = 0; i < 3; ++i)
    {
        totalSize += gMapArray[i].size();
    }
    EXPECT_EQ(totalSize, 1000);
}

TEST(MicroRWLock, SingleThreadTest)
{
    MicroRWLock locker;
    // Test read lock.
    locker.ReadLock();
    locker.ReadLock();
    EXPECT_TRUE(locker.IsReadLocked());
    locker.ReadUnlock();
    EXPECT_TRUE(locker.IsReadLocked());
    EXPECT_TRUE(locker.IsLocked());
    EXPECT_TRUE(!locker.IsWriteLocked());
    locker.ReadUnlock();
    EXPECT_TRUE(!locker.IsReadLocked());
    EXPECT_TRUE(!locker.IsWriteLocked());
    // Test write lock.
    locker.WriteLock();
    EXPECT_TRUE(locker.IsLocked());
    EXPECT_TRUE(!locker.IsReadLocked());
    EXPECT_TRUE(locker.IsWriteLocked());
    locker.WriteUnlock();
    // Test scoped rwLock
    {
        ScopedRWLock<MicroRWLock> lock(locker, 'r');
        EXPECT_TRUE(locker.IsLocked());
        EXPECT_TRUE(locker.IsReadLocked());
        EXPECT_TRUE(!locker.IsWriteLocked());
    }
    EXPECT_TRUE(!locker.IsLocked());
    {
        ScopedRWLock<MicroRWLock> lock(locker, 'w');
        EXPECT_TRUE(locker.IsLocked());
        EXPECT_TRUE(!locker.IsReadLocked());
        EXPECT_TRUE(locker.IsWriteLocked());
    }
    EXPECT_TRUE(!locker.IsLocked());
    // Test prefer write rwLock
    MicroRWLockPreferWrite wlocker;
    wlocker.ReadLock();
    wlocker.ReadLock();
    EXPECT_TRUE(wlocker.IsReadLocked());
    wlocker.ReadUnlock();
    EXPECT_TRUE(wlocker.IsReadLocked());
    EXPECT_TRUE(wlocker.IsLocked());
    EXPECT_TRUE(!wlocker.IsWriteLocked());
    wlocker.ReadUnlock();
    EXPECT_TRUE(!wlocker.IsReadLocked());
    EXPECT_TRUE(!wlocker.IsWriteLocked());
    // Test write lock.
    wlocker.WriteLock();
    EXPECT_TRUE(wlocker.IsLocked());
    EXPECT_TRUE(!wlocker.IsReadLocked());
    EXPECT_TRUE(wlocker.IsWriteLocked());
    wlocker.WriteUnlock();
}

TEST(MicroRWLock, MultipleThreadTest)
{
    static const uint32_t kNumThreads = 16;
    pthread_t thid[kNumThreads];
    ThreadArgs args[kNumThreads];
    scoped_ptr<MicroRWLock> rlocker(new MicroRWLock);
    scoped_ptr<MicroRWLockPreferWrite> wlocker(new MicroRWLockPreferWrite);
    gMap.clear();
    gMap.insert(std::make_pair(0, 0));
    for (uint32_t i = 0; i < kNumThreads; ++i)
    {
        args[i].rLock = rlocker.get();
        args[i].wLock = wlocker.get();
        pthread_create(&thid[i], NULL, ProcessMicroRWLock, (void *)&args[i]);
    }
    for (uint32_t i = 0; i < kNumThreads; ++i)
    {
        pthread_join(thid[i], NULL);
    }
    EXPECT_EQ(gMap.size(), 1);
}

TEST(MicroRWLock, TimeoutTest)
{
    MicroRWLock testLock;
    // 1. test lock can not be done when timeout.
    testLock.ReadLock();
    uint64_t testTimeout = 1000000;
    uint64_t lockBegin = GetCurrentTimeInUs();
    bool success = testLock.TimedWriteLock(testTimeout);
    uint64_t lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(!success);
    EXPECT_TRUE(lockEnd - lockBegin >= testTimeout);
    testLock.ReadUnlock();

    testLock.WriteLock();
    lockBegin = GetCurrentTimeInUs();
    success = testLock.TimedReadLock(testTimeout);
    lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(!success);
    EXPECT_TRUE(lockEnd - lockBegin >= testTimeout);
    testLock.WriteUnlock();

    // 2. test lock can be done when timeout.
    testTimeout = 100;
    lockBegin = GetCurrentTimeInUs();
    success = testLock.TimedReadLock(testTimeout);
    lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(success);
    EXPECT_TRUE(lockEnd - lockBegin < testTimeout);
    testLock.ReadUnlock();

    lockBegin = GetCurrentTimeInUs();
    success = testLock.TimedWriteLock(testTimeout);
    lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(success);
    EXPECT_TRUE(lockEnd - lockBegin < testTimeout);
    testLock.WriteUnlock();
}

TEST(MicroRWLockPerferWrite, TimeoutTest)
{
    MicroRWLockPreferWrite testLock;
    uint64_t testTimeout = 1000000;
    // 1. test lock can not be done when timeout.
    testLock.ReadLock();
    uint64_t lockBegin = GetCurrentTimeInUs();
    bool success = testLock.TimedWriteLock(testTimeout);
    uint64_t lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(!success);
    EXPECT_TRUE(lockEnd - lockBegin >= testTimeout);
    testLock.ReadUnlock();

    testLock.WriteLock();
    lockBegin = GetCurrentTimeInUs();
    success = testLock.TimedReadLock(testTimeout);
    lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(!success);
    EXPECT_TRUE(lockEnd - lockBegin >= testTimeout);
    testLock.WriteUnlock();

    // 2. test lock can be done when timeout.
    testTimeout = 100;
    lockBegin = GetCurrentTimeInUs();
    success = testLock.TimedReadLock(testTimeout);
    lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(success);
    EXPECT_TRUE(lockEnd - lockBegin < testTimeout);
    testLock.ReadUnlock();

    lockBegin = GetCurrentTimeInUs();
    success = testLock.TimedWriteLock(testTimeout);
    lockEnd = GetCurrentTimeInUs();
    EXPECT_TRUE(success);
    EXPECT_TRUE(lockEnd - lockBegin < testTimeout);
    testLock.WriteUnlock();
}

TEST(MicroRWLockPreferwrite, OwnerTest)
{
    MicroRWLock testLock;
    MicroRWLockPreferWrite testLockPreferWrite;
    int tid = ThisThread::GetId();
    // 1. test MicroRWLock.
    EXPECT_EQ(testLock.GetWriteLockOwner(), 0);
    testLock.ReadLock();
    EXPECT_EQ(testLock.GetWriteLockOwner(), 0); // readlock will not set writer owner.
    testLock.TimedWriteLock(100000); // will not succeed.
    EXPECT_EQ(testLock.GetWriteLockOwner(), 0); // still no writer owner.
    testLock.TryWriteLock();
    EXPECT_EQ(testLock.GetWriteLockOwner(), 0); // still no writer owner.
    testLock.ReadUnlock();
    testLock.WriteLock();
    EXPECT_EQ(testLock.GetWriteLockOwner(), tid); // writer owner is current threadid.
    testLock.WriteUnlock();
    EXPECT_EQ(testLock.GetWriteLockOwner(), 0);
    testLock.TimedWriteLock(100000);
    EXPECT_EQ(testLock.GetWriteLockOwner(), tid);
    testLock.WriteUnlock();
    EXPECT_EQ(testLock.GetWriteLockOwner(), 0);
    testLock.TryWriteLock();
    EXPECT_EQ(testLock.GetWriteLockOwner(), tid);
    testLock.WriteUnlock();
    EXPECT_EQ(testLock.GetWriteLockOwner(), 0);

    // 2. test MicroRWLockPreferWrite.
    EXPECT_EQ(testLockPreferWrite.GetWriteLockOwner(), 0);
    testLockPreferWrite.ReadLock();
    EXPECT_EQ(testLockPreferWrite.GetWriteLockOwner(), 0); // readlock will not set writer owner.
    testLockPreferWrite.TimedWriteLock(100000); // will not succeed.
    EXPECT_EQ(testLockPreferWrite.GetWriteLockOwner(), 0); // still no writer owner.
    testLockPreferWrite.ReadUnlock();
    testLockPreferWrite.WriteLock();
    EXPECT_EQ(testLockPreferWrite.GetWriteLockOwner(), tid); // writer owner is current threadid.
    testLockPreferWrite.WriteUnlock();
    EXPECT_EQ(testLockPreferWrite.GetWriteLockOwner(), 0);
    testLockPreferWrite.TimedWriteLock(100000);
    EXPECT_EQ(testLockPreferWrite.GetWriteLockOwner(), tid);
    testLockPreferWrite.WriteUnlock();
    EXPECT_EQ(testLockPreferWrite.GetWriteLockOwner(), 0);
}
