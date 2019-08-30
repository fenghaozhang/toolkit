#ifndef _SRC_SYNC_SCOPED_LOCK_H
#define _SRC_SYNC_SCOPED_LOCK_H

#include <pthread.h>

#include "src/thread/thread_check.h"

template<typename T>
class ScopedLock
{
public:
    typedef T LockType;

    explicit ScopedLock(T* lock) : mLock(lock)
    {
        CheckPthreadError(pthread_mutex_lock(mLock));
    }

    explicit ScopedLock(T& lock) : mLock(&lock) // NOLINT(runtime/references)
    {
        mLock->Lock();
    }

    ~ScopedLock()
    {
        mLock->Unlock();
    }

private:
    T* mLock;

    DISALLOW_COPY_AND_ASSIGN(ScopedLock<T>);
};

template<>
class ScopedLock<pthread_mutex_t>
{
public:
    typedef pthread_mutex_t T;
    typedef T LockType;

    explicit ScopedLock(T* lock) : mLock(lock)
    {
        CheckPthreadError(pthread_mutex_lock(mLock));
    }

    explicit ScopedLock(T& lock) : mLock(&lock) // NOLINT(runtime/references)
    {
        CheckPthreadError(pthread_mutex_lock(mLock));
    }

    ~ScopedLock()
    {
        CheckPthreadError(pthread_mutex_unlock(mLock));
    }

private:
    T* mLock;

    DISALLOW_COPY_AND_ASSIGN(ScopedLock);
};

// Read-Write lock
template <typename T>
class ScopedRWLock
{
public:
    typedef T LockType;

    ScopedRWLock(T* lock, const char mode) : mLock(lock), mMode(mode)
    {
        doLock();
    }

    ScopedRWLock(T& lock, const char mode) : mLock(&lock), mMode(mode)  // NOLINT
    {
        doLock();
    }

    ~ScopedRWLock()
    {
        doUnlock();
    }

private:
    void doLock();
    void doUnlock();

    T* mLock;
    const char mMode;

    DISALLOW_COPY_AND_ASSIGN(ScopedRWLock);
};

template<typename T>
inline void ScopedRWLock<T>::doLock()
{
    if (mMode == 'r' || mMode == 'R')
    {
        mLock->ReadLock();
    }
    if (mMode == 'w' || mMode == 'W')
    {
        mLock->WriteLock();
    }
}

template<typename T>
inline void ScopedRWLock<T>::doUnlock()
{
    if (mMode == 'r' || mMode == 'R')
    {
        mLock->ReadUnlock();
    }
    if (mMode == 'w' || mMode == 'W')
    {
        mLock->WriteUnlock();
    }
}

#ifndef NO_STONE_SCOPED_LOCK_MACRO

template<typename Lock>
struct ScopedLockerHelper
{
    Lock lock;
    bool done;

    explicit ScopedLockerHelper(typename Lock::LockType* _lock)
      : lock(_lock), done(false)
    {
    }
};

#define STONE_SCOPED_LOCK_X(lock, x) \
    /* Please make sure that you have not written */ \
    /* STONE_SCOPED_xxLOCK(lock); {...} */ \
    for (ScopedLockerHelper<x> locker(&lock); !locker.done; locker.done = true) \
        if (true) \

#define STONE_SCOPED_LOCK(lock) \
        STONE_SCOPED_LOCK_X(lock, ScopedLocker<typeof(lock)>)

#define STONE_SCOPED_RLOCK(lock) \
        STONE_SCOPED_LOCK_X(lock, ScopedReaderLocker<typeof(lock)>)

#define STONE_SCOPED_WLOCK(lock) \
        STONE_SCOPED_LOCK_X(lock, ScopedWriterLocker<typeof(lock)>)

#endif // NO_STONE_SCOPED_LOCK_MACRO

#endif  // _SRC_SYNC_SCOPED_LOCK_H
