#ifndef _SRC_SYNC_POSIX_LOCK_H
#define _SRC_SYNC_POSIX_LOCK_H

#include <pthread.h>

#include "src/base/gettime.h"
#include "src/common/macros.h"
#include "src/sync/atomic.h"
#include "src/thread/this_thread.h"

class NullLock
{
public:
    NullLock() : mLocked(false) {}
    ~NullLock() {}

    void Lock()
    {
        mLocked = true;
    }
    void Unlock()
    {
        mLocked = false;
    }
    bool IsLocked() const
    {
        return mLocked;
    }

private:
    bool mLocked;

    DISALLOW_COPY_AND_ASSIGN(NullLock);
};

class SpinLock
{
public:
    SpinLock()
    {
        CheckPthreadError(pthread_spin_init(&mSpin, PTHREAD_PROCESS_PRIVATE));
    }
    ~SpinLock()
    {
        CheckPthreadError(pthread_spin_destroy(&mSpin));
    }
    void Lock()
    {
        CheckPthreadError(pthread_spin_lock(&mSpin));
    }
    void Unlock()
    {
        CheckPthreadError(pthread_spin_unlock(&mSpin));
    }
    bool IsLocked() const
    {
        return mSpin == 0;
    }

private:
    pthread_spinlock_t mSpin;

    DISALLOW_COPY_AND_ASSIGN(SpinLock);
};

class MutexBase
{
protected:
    explicit MutexBase(int type)
    {
        pthread_mutexattr_t attr;
        CheckPthreadError(pthread_mutexattr_init(&attr));
        CheckPthreadError(pthread_mutexattr_settype(&attr, type));
        CheckPthreadError(pthread_mutex_init(&mMutex, &attr));
        CheckPthreadError(pthread_mutexattr_destroy(&attr));
    }

public:
    virtual ~MutexBase()
    {
        CheckPthreadError(pthread_mutex_destroy(&mMutex));
    }
    void Lock()
    {
        CheckPthreadError(pthread_mutex_lock(&mMutex));
    }
    void Unlock()
    {
        CheckPthreadError(pthread_mutex_unlock(&mMutex));
    }
    bool IsLocked() const
    {
        return mMutex.__data.__lock > 0;
    }
    pthread_mutex_t* NativeLock()
    {
        return &mMutex;
    }

protected:
    pthread_mutex_t mMutex;

    DISALLOW_COPY_AND_ASSIGN(MutexBase);
};

class SimpleMutex : public MutexBase
{
public:
    SimpleMutex() : MutexBase(PTHREAD_MUTEX_NORMAL) {}
};

class RestrictMutex : public MutexBase
{
public:
    RestrictMutex() : MutexBase(PTHREAD_MUTEX_ERRORCHECK) {}
};

class RecursiveMutex : public MutexBase
{
public:
    RecursiveMutex() : MutexBase(PTHREAD_MUTEX_RECURSIVE) {}
};

class AdaptiveMutex : public MutexBase
{
public:
    AdaptiveMutex() : MutexBase(PTHREAD_MUTEX_ADAPTIVE_NP) {}
};

class RWLock
{
public:
    enum Mode
    {
        kModePreferReader = PTHREAD_RWLOCK_PREFER_READER_NP,
        kModePreferWriter = PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP,
        kModeDefault = PTHREAD_RWLOCK_DEFAULT_NP,
        PREFER_READER = kModePreferReader,
        PREFER_WRITER = kModePreferWriter
    };

    explicit RWLock(Mode mode = kModeDefault)
    {
        pthread_rwlockattr_t attr;
        CheckPthreadError(pthread_rwlockattr_init(&attr));
        CheckPthreadError(pthread_rwlockattr_setkind_np(&attr, mode));
        CheckPthreadError(pthread_rwlock_init(&mLock, &attr));
        CheckPthreadError(pthread_rwlockattr_destroy(&attr));
    }
    ~RWLock()
    {
        CheckPthreadError(pthread_rwlock_destroy(&mLock));
    }

    void ReadLock()
    {
        CheckPthreadError(pthread_rwlock_rdlock(&mLock));
    }
    void WriteLock()
    {
        CheckPthreadError(pthread_rwlock_wrlock(&mLock));
    }
    bool TryReadLock()
    {
        return CheckPthreadTryLockError(pthread_rwlock_tryrdlock(&mLock));
    }
    bool TryWriteLock()
    {
        return CheckPthreadTryLockError(pthread_rwlock_trywrlock(&mLock));
    }
    bool TimedReadLock(int64_t timeInUs)
    {
        struct timespec ts;
        int64_t end = GetCurrentTimeInUs() + timeInUs;
        ts.tv_sec = end / 1000000L;
        ts.tv_nsec = (end % 1000000L) * 1000;
        return CheckPthreadTimedLockError(pthread_rwlock_timedrdlock(&mLock, &ts));
    }
    bool TimedWriteLock(int64_t timeInUs)
    {
        struct timespec ts;
        int64_t end = GetCurrentTimeInUs() + timeInUs;
        ts.tv_sec = end / 1000000L;
        ts.tv_nsec = (end % 1000000L) * 1000;
        return CheckPthreadTimedLockError(pthread_rwlock_timedwrlock(&mLock, &ts));
    }
    void ReadUnlock()
    {
        Unlock();
    }
    void WriteUnlock()
    {
        Unlock();
    }
    void Unlock()
    {
        CheckPthreadError(pthread_rwlock_unlock(&mLock));
    }
    pthread_rwlock_t* NativeLock()
    {
        return &mLock;
    }

private:
    pthread_rwlock_t mLock;

    DISALLOW_COPY_AND_ASSIGN(RWLock);
};

#endif // _SRC_SYNC_POSIX_LOCK_H
