#ifndef _SRC_SYNC_MICRO_LOCK_H
#define _SRC_SYNC_MICRO_LOCK_H

#include <pthread.h>
#include <stdint.h>
#include <vector>

#include "src/common/assert.h"
#include "src/common/macros.h"
#include "src/sync/atomic.h"
#include "src/thread/thread_check.h"

/** Non-fair light-weighted lock */
class MicroLock
{
public:
    MicroLock() : mLock(kLockOff) {}

    void Lock();

    bool TryLock();

    void Unlock();

    bool IsLocked() const;

    bool TimedLock(uint64_t timeoutInUs);

    int GetLockOwner() const;

private:
    uint64_t getSignature() const;

    static const uint64_t kLockOn = 1;
    static const uint64_t kLockOff = 0;
    uint64_t mLock;

    DISALLOW_COPY_AND_ASSIGN(MicroLock);
};

/** Non-fair read-write lock. Prefer read */
class MicroRWLock
{
public:
    MicroRWLock() : mLock(0U) {}

    void ReadLock();
    
    void ReadUnlock();

    void WriteLock();
    
    void WriteUnlock();

    bool IsReadLocked() const;

    bool IsWriteLocked() const;

    bool IsLocked() const;

    bool TryReadLock();

    bool TryWriteLock();

    bool TimedReadLock(uint64_t timeoutInUs);
    
    bool TimedWriteLock(uint64_t timeoutInUs);

    int GetWriteLockOwner() const;
    
private:
    // uint32_t writerOwnerId | uint32_t readerCount
    static const uint64_t kReaderMask = 0x00000000FFFFFFFF;
    static const uint64_t kWriterMask = 0xFFFFFFFF00000000;
    uint64_t mLock;

    DISALLOW_COPY_AND_ASSIGN(MicroRWLock);
};

class MicroRWLockPreferWrite
{
public:
    MicroRWLockPreferWrite() : mLock(0U) {}

    void ReadLock();

    void ReadUnlock();

    void WriteLock();

    void WriteUnlock();

    bool IsReadLocked() const;

    bool IsWriteLocked() const;

    bool IsLocked() const;

    bool TimedReadLock(uint64_t timeoutInUs);
    
    bool TimedWriteLock(uint64_t timeoutInUs);

    int GetWriteLockOwner() const;
private:
    // 32bit writerOwnerId | 32bit readerCount
    static const uint64_t kWriterMask = 0xFFFFFFFF00000000ULL;
    static const uint64_t kReaderMask = 0x00000000FFFFFFFFULL;
    uint64_t mLock;

    DISALLOW_COPY_AND_ASSIGN(MicroRWLockPreferWrite);
};

class MicroLockVector
{
public:
    MicroLockVector(uint32_t size);

    void Lock(uint32_t index);

    bool TryLock(uint32_t index);

    void Unlock(uint32_t index);

    bool IsLocked(uint32_t index) const;

private:
    uint32_t mask(uint32_t index) const
    {
        return 1UL << (index & 0x1f);
    }

    std::vector<uint32_t> mLock;

    DISALLOW_COPY_AND_ASSIGN(MicroLockVector);
};

#if 0

class MicroRWLockRefCounted :
    public MicroRWLock,
    public RefCounted<MicroRWLockRefCounted>
{
public:
    explicit MicroRWLockRefCounted(LockType type);

    ~MicroRWLockRefCounted() {}

    void ReadLock();

    void ReadUnlock();

    void WriteLock();

    void WriteUnlock();

private:
    enum LockType { READ_LOCK, WRITE_LOCK,  RELEASE = 2 };

    DISALLOW_COPY_AND_ASSIGN(MicroRWLockRefCounted);
};

#endif

template<typename T>
class ScopedLock
{
public:
    typedef T LockType;

    explicit ScopedLock(T& lock) : mLock(&lock)
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

    explicit ScopedLock(T& lock) : mLock(&lock)   // NOLINT
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

    explicit ScopedRWLock(T& lock, const char mode)
        : mLock(&lock), mMode(mode)
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

    ~ScopedRWLock()
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

private:
    T* mLock;
    const char mMode;

    DISALLOW_COPY_AND_ASSIGN(ScopedRWLock);
};

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

#endif  // _SRC_SYNC_MICRO_LOCK_H
