#ifndef _SRC_SYNC_POSIX_LOCK_H
#define _SRC_SYNC_POSIX_LOCK_H

#include <pthread.h>

#include "src/base/gettime.h"
#include "src/common/macros.h"
#include "src/sync/atomic.h"
#include "src/sync/scoped_lock.h"
#include "src/thread/this_thread.h"
#include "src/thread/thread_check.h"

static struct timespec getTimeSpec(int64_t time);

class NullLock
{
public:
    NullLock() : mLocked(false) {}
    ~NullLock() {}

    void Lock();
    void Unlock();
    bool TryLock();
    bool IsLocked() const;

private:
    bool mLocked;
    DISALLOW_COPY_AND_ASSIGN(NullLock);
};

class SpinLock
{
public:
    SpinLock();
    ~SpinLock();

    void Lock();
    void Unlock();
    bool TryLock();
    bool IsLocked() const;

private:
    pthread_spinlock_t mSpin;
    DISALLOW_COPY_AND_ASSIGN(SpinLock);
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

    explicit RWLock(Mode mode = kModeDefault);
    ~RWLock();

    void ReadLock();
    void WriteLock();
    bool TryReadLock();
    bool TryWriteLock();
    bool TimedReadLock(int64_t us);
    bool TimedWriteLock(int64_t us);
    void ReadUnlock();
    void WriteUnlock();
    void Unlock();
    bool IsLocked();
    pthread_rwlock_t* NativeLock();

private:
    pthread_rwlock_t mRWLock;
    DISALLOW_COPY_AND_ASSIGN(RWLock);
};

class MutexBase
{
public:
    explicit MutexBase(int type);
    virtual ~MutexBase();

    void Lock();
    void Unlock();
    bool TryLock();
    bool IsLocked() const;
    pthread_mutex_t* NativeLock();

private:
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

inline void NullLock::Lock()
{
    mLocked = true;
}

inline void NullLock::Unlock()
{
    mLocked = false;
}

inline bool NullLock::TryLock()
{
    return mLocked ? false : (mLocked = true, true);
}

inline bool NullLock::IsLocked() const
{
    return mLocked;
}

inline SpinLock::SpinLock()
{
    CheckPthreadError(::pthread_spin_init(&mSpin, PTHREAD_PROCESS_PRIVATE));
}

inline SpinLock::~SpinLock()
{
    CheckPthreadError(::pthread_spin_destroy(&mSpin));
}

inline void SpinLock::Lock()
{
    CheckPthreadError(::pthread_spin_lock(&mSpin));
}

inline void SpinLock::Unlock()
{
    CheckPthreadError(::pthread_spin_unlock(&mSpin));
}

inline bool SpinLock::TryLock()
{
    return CheckPthreadTryLockError(
            ::pthread_spin_trylock(&mSpin));
}

inline bool SpinLock::IsLocked() const
{
    return mSpin == 0;
}

inline MutexBase::MutexBase(int type)
{
    pthread_mutexattr_t attr;
    CheckPthreadError(::pthread_mutexattr_init(&attr));
    CheckPthreadError(::pthread_mutexattr_settype(&attr, type));
    CheckPthreadError(::pthread_mutex_init(&mMutex, &attr));
    CheckPthreadError(::pthread_mutexattr_destroy(&attr));
}

inline MutexBase::~MutexBase()
{
    CheckPthreadError(::pthread_mutex_destroy(&mMutex));
}

inline void MutexBase::Lock()
{
    CheckPthreadError(::pthread_mutex_lock(&mMutex));
}

inline void MutexBase::Unlock()
{
    CheckPthreadError(::pthread_mutex_unlock(&mMutex));
}

inline bool MutexBase::TryLock()
{
    return CheckPthreadTryLockError(
            ::pthread_mutex_trylock(&mMutex));
}

inline bool MutexBase::IsLocked() const
{
    return mMutex.__data.__lock > 0;
}

inline pthread_mutex_t* MutexBase::NativeLock()
{
    return &mMutex;
}

inline RWLock::RWLock(Mode mode)
{
    pthread_rwlockattr_t attr;
    CheckPthreadError(::pthread_rwlockattr_init(&attr));
    CheckPthreadError(::pthread_rwlockattr_setkind_np(&attr, mode));
    CheckPthreadError(::pthread_rwlock_init(&mRWLock, &attr));
    CheckPthreadError(::pthread_rwlockattr_destroy(&attr));
}

inline RWLock::~RWLock()
{
    CheckPthreadError(::pthread_rwlock_destroy(&mRWLock));
}

inline void RWLock::ReadLock()
{
    CheckPthreadError(::pthread_rwlock_rdlock(&mRWLock));
}

inline void RWLock::WriteLock()
{
    CheckPthreadError(::pthread_rwlock_wrlock(&mRWLock));
}

inline bool RWLock::TryReadLock()
{
    return CheckPthreadTryLockError(
            pthread_rwlock_tryrdlock(&mRWLock));
}

inline bool RWLock::TryWriteLock()
{
    return CheckPthreadTryLockError(
            ::pthread_rwlock_trywrlock(&mRWLock));
}

inline bool RWLock::TimedReadLock(int64_t us)
{
    struct timespec ts = getTimeSpec(GetCurrentTimeInUs() + us);
    return CheckPthreadTimedLockError(
            ::pthread_rwlock_timedrdlock(&mRWLock, &ts));
}

inline bool RWLock::TimedWriteLock(int64_t us)
{
    struct timespec ts = getTimeSpec(GetCurrentTimeInUs() + us);
    return CheckPthreadTimedLockError(
            ::pthread_rwlock_timedwrlock(&mRWLock, &ts));
}

inline void RWLock::ReadUnlock()
{
    Unlock();
}

inline void RWLock::WriteUnlock()
{
    Unlock();
}

inline void RWLock::Unlock()
{
    CheckPthreadError(::pthread_rwlock_unlock(&mRWLock));
}

inline bool RWLock::IsLocked()
{
    bool success = TryWriteLock();
    if (success)
    {
        WriteUnlock();
    }
    return !success;
}

inline pthread_rwlock_t* RWLock::NativeLock()
{
    return &mRWLock;
}

static inline struct timespec getTimeSpec(int64_t time)
{
    struct timespec ts;
    ts.tv_sec  = time / 1000000L;
    ts.tv_nsec = (time % 1000000) * 1000;
    return ts;
}

#endif // _SRC_SYNC_POSIX_LOCK_H
