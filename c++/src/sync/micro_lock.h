#ifndef _SRC_SYNC_MICRO_LOCK_H
#define _SRC_SYNC_MICRO_LOCK_H

#include <pthread.h>
#include <stdint.h>
#include <vector>

#include "src/base/gettime.h"
#include "src/common/assert.h"
#include "src/common/macros.h"
#include "src/sync/atomic.h"
#include "src/sync/scoped_lock.h"
#include "src/thread/this_thread.h"
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

    int Owner() const;

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

    bool TryReadLock();

    bool TryWriteLock();

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

class Sleeper
{
public:
    Sleeper() : mSpins(0) {}

    void Pause();

private:
    enum
    {
        kMicroLockMaxSpins = 1000,
        kMicroLockSleepIntervalNs = 20 * 1000,
    };

    uint32_t mSpins;

    DISALLOW_COPY_AND_ASSIGN(Sleeper);
};

FORCE_INLINE void Sleeper::Pause()
{
    if (mSpins < kMicroLockMaxSpins)
    {
        ++mSpins;
        asm volatile("pause");
    }
    else
    {
        struct timespec ts = {0, kMicroLockSleepIntervalNs};
        nanosleep(&ts, NULL);
    }
}

inline void MicroLock::Lock()
{
    uint64_t signature = getSignature();
    Sleeper sleeper;
    do
    {
        while (IsLocked())
        {
            sleeper.Pause();
        }
    }
    while (!AtomicCompareExchange(&mLock, signature, kLockOff));
}

inline bool MicroLock::TryLock()
{
    uint64_t signature = getSignature();
    return AtomicCompareExchange(&mLock, signature, kLockOff);
}

inline void MicroLock::Unlock()
{
    AtomicSet(&mLock, kLockOff);
}

inline bool MicroLock::IsLocked() const
{
    return AtomicGet(&mLock) != kLockOff;
}

inline bool MicroLock::TimedLock(uint64_t timeoutInUs)
{
    uint64_t begin = GetCurrentTimeInUs();
    uint64_t signature = getSignature();
    Sleeper sleeper;
    do
    {
        while (IsLocked())
        {
            sleeper.Pause();
            uint64_t now = GetCurrentTimeInUs();
            if (now >= begin && now - begin >= timeoutInUs)
            {
                return false;
            }
        }
    }
    while (!AtomicCompareExchange(&mLock, signature, kLockOff));
    return true;
}

inline int MicroLock::Owner() const
{
    return static_cast<int>(
            (AtomicGet(&mLock) & 0XFFFFFFFF00000000) >> 32);
}

FORCE_INLINE uint64_t MicroLock::getSignature() const
{
    int tid = ThisThread::GetId();
    return (static_cast<uint64_t>(tid) << 32) | kLockOn;
}

inline void MicroRWLock::ReadLock()
{
    AtomicInc(&mLock);
    Sleeper sleeper;
    while (IsWriteLocked())
    {
        sleeper.Pause();
    }
}

inline void MicroRWLock::ReadUnlock()
{
    uint64_t bits = AtomicDec(&mLock);
    assert((bits & kWriterMask) == 0);
}

inline void MicroRWLock::WriteLock()
{
    Sleeper sleeper;
    uint64_t signature =
        (static_cast<uint64_t>(ThisThread::GetId()) << 32) & kWriterMask;

    do
    {
        while (IsLocked())
        {
            sleeper.Pause();
        }
    }
    while (!AtomicCompareExchange(&mLock, signature, 0UL));
}

inline void MicroRWLock::WriteUnlock()
{
    uint64_t value = AtomicGet(&mLock);
    while (!AtomicCompareExchange(&mLock, value & kReaderMask, value))
    {
        value = AtomicGet(&mLock);
    }
}

inline bool MicroRWLock::IsReadLocked() const
{
    return (AtomicGet(&mLock) & kReaderMask) != 0;
}

inline bool MicroRWLock::IsWriteLocked() const
{
    return (AtomicGet(&mLock) & kWriterMask) != 0;
}

inline bool MicroRWLock::IsLocked() const
{
    return AtomicGet(&mLock) != 0;
}

inline bool MicroRWLock::TryReadLock()
{
    if (!IsWriteLocked())
    {
        // Here might be interfered with write lock
        AtomicInc(&mLock);
        if (!IsWriteLocked())
        {
            return true;
        }
        AtomicDec(&mLock);
    }
    return false;
}

inline bool MicroRWLock::TryWriteLock()
{
    uint64_t writerOwner = (static_cast<uint64_t>(
                ThisThread::GetId()) << 32) & kWriterMask;
    return AtomicCompareExchange(&mLock, writerOwner, 0UL);
}

inline bool MicroRWLock::TimedReadLock(uint64_t timeoutInUs)
{
    uint64_t begin = GetCurrentTimeInUs();
    AtomicInc(&mLock);
    Sleeper sleeper;
    while (IsWriteLocked())
    {
        uint64_t now = GetCurrentTimeInUs();
        if (now >= begin && now - begin >= timeoutInUs)
        {
            AtomicDec(&mLock);
            return false;
        }

        sleeper.Pause();
    }
    return true;
}

inline bool MicroRWLock::TimedWriteLock(uint64_t timeoutInUs)
{
    uint64_t begin = GetCurrentTimeInUs();
    Sleeper sleeper;
    uint64_t signature = (static_cast<uint64_t>(
                ThisThread::GetId()) << 32) & kWriterMask;

    do
    {
        while (IsLocked())
        {
            uint64_t now = GetCurrentTimeInUs();
            if (now >= begin && now - begin >= timeoutInUs)
            {
                return false;
            }

            sleeper.Pause();
        }
    }
    while (!AtomicCompareExchange(&mLock, signature, 0UL));
    return true;
}

inline int MicroRWLock::GetWriteLockOwner() const
{
    return static_cast<int>((AtomicGet(&mLock) & kWriterMask) >> 32);
}

inline void MicroRWLockPreferWrite::ReadLock()
{
    Sleeper sleeper;
    uint64_t value = 0;
    bool firstTry = true;
    do
    {
        if (!firstTry)
        {
            sleeper.Pause();
        }
        else
        {
            firstTry = false;
        }
        while (((value = AtomicGet(&mLock)) & kWriterMask) != 0)
        {
            sleeper.Pause();
        }
    }
    while (!AtomicCompareExchange(&mLock, value + 1, value));
}

inline void MicroRWLockPreferWrite::ReadUnlock()
{
    AtomicDec(&mLock);
}

inline void MicroRWLockPreferWrite::WriteLock()
{
    Sleeper sleeper;
    uint64_t value = 0;
    // Set write lock bit firstly.
    uint64_t writerOwner = (static_cast<uint64_t>
            (ThisThread::GetId()) << 32) & kWriterMask;

    bool firstTry = true;
    do
    {
        if (!firstTry)
        {
            sleeper.Pause();
        }
        else
        {
            firstTry = false;
        }
        while (((value = AtomicGet(&mLock)) & kWriterMask) != 0)
        {
            sleeper.Pause();
        }
    }
    while (!AtomicCompareExchange(&mLock, value | writerOwner, value));

    // Wait reads count down to 0.
    while ((AtomicGet(&mLock) & kReaderMask) != 0)
    {
        sleeper.Pause();
    }
}

inline void MicroRWLockPreferWrite::WriteUnlock()
{
    uint64_t value = AtomicGet(&mLock);
    while (!AtomicCompareExchange(&mLock, value & kReaderMask, value))
    {
        value = AtomicGet(&mLock);
    }
}

inline bool MicroRWLockPreferWrite::IsReadLocked() const
{
    return (AtomicGet(&mLock) & kReaderMask) != 0;
}

inline bool MicroRWLockPreferWrite::IsWriteLocked() const
{
    // Same as read lock.
    return (AtomicGet(&mLock) & kWriterMask) != 0;
}

inline bool MicroRWLockPreferWrite::IsLocked() const
{
    return AtomicGet(&mLock) != 0;
}

inline bool MicroRWLockPreferWrite::TryReadLock()
{
    if (!IsWriteLocked())
    {
        AtomicInc(&mLock);
        if (!IsWriteLocked())
        {
            return true;
        }
        AtomicDec(&mLock);
    }
    return false;
}

inline bool MicroRWLockPreferWrite::TryWriteLock()
{
    uint64_t writerOwner = (static_cast<uint64_t>
            (ThisThread::GetId()) << 32) & kWriterMask;
    return AtomicCompareExchange(&mLock, writerOwner, 0UL);
}

inline bool MicroRWLockPreferWrite::TimedReadLock(uint64_t timeoutInUs)
{
    uint64_t begin = GetCurrentTimeInUs();
    Sleeper sleeper;
    uint64_t value = 0;
    bool firstTry = true;
    do
    {
        if (!firstTry)
        {
            sleeper.Pause();
        }
        else
        {
            firstTry = false;
        }
        while (((value = AtomicGet(&mLock)) & kWriterMask) != 0)
        {
            uint64_t now = GetCurrentTimeInUs();
            if (now >= begin && now - begin >= timeoutInUs)
            {
                return false;
            }

            sleeper.Pause();
        }
    }
    while (!AtomicCompareExchange(&mLock, value + 1, value));
    return true;
}

inline bool MicroRWLockPreferWrite::TimedWriteLock(uint64_t timeoutInUs)
{
    uint64_t begin = GetCurrentTimeInUs();
    Sleeper sleeper;
    uint64_t value = 0;
    // Set write lock bit firstly.
    uint64_t writerOwner = (static_cast<uint64_t>
            (ThisThread::GetId()) << 32) & kWriterMask;

    bool firstTry = true;
    do
    {
        if (!firstTry)
        {
            sleeper.Pause();
        }
        else
        {
            firstTry = false;
        }
        while (((value = AtomicGet(&mLock)) & kWriterMask) != 0)
        {
            uint64_t now = GetCurrentTimeInUs();
            if (now >= begin && now - begin >= timeoutInUs)
            {
                return false;
            }

            sleeper.Pause();
        }
    }
    while (!AtomicCompareExchange(&mLock, value | writerOwner, value));
    // Wait reads count down to 0.
    while (((value = AtomicGet(&mLock)) & kReaderMask) != 0)
    {
        uint64_t now = GetCurrentTimeInUs();
        if (now >= begin && now - begin >= timeoutInUs)
        {
            // WriteUnlock.
            while (!AtomicCompareExchange(&mLock, value & kReaderMask, value))
            {
                value = AtomicGet(&mLock);
            }
            return false;
        }

        sleeper.Pause();
    }
    return true;
}

inline int MicroRWLockPreferWrite::GetWriteLockOwner() const
{
    return static_cast<int>(AtomicGet(&mLock) >> 32);
}

#if 0

class MicroLockVector
{
public:
    explicit MicroLockVector(uint32_t size);

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

MicroLockVector::MicroLockVector(uint32_t size)
{
    mLock.assign(((size + 31) >> 5), 0UL);
}

inline void MicroLockVector::Lock(uint32_t index)
{
    const uint32_t _mask = mask(index);
    Sleeper sleeper;
    do
    {
        while ((AtomicGet(&mLock[index >> 5]) & _mask) != 0)
        {
            sleeper.Pause();
        }
    }
    while (!TryLock(index));
}

inline bool MicroLockVector::TryLock(uint32_t index)
{
    const uint32_t _mask = mask(index);
    const uint32_t value = AtomicGet(&mLock[index >> 5]);
    return AtomicCompareExchange(
            &(mLock[index >> 5]), value | _mask, value & ~_mask);
}

inline void MicroLockVector::Unlock(uint32_t index)
{
    const uint32_t _mask = mask(index);
    uint32_t value = AtomicGet(&mLock[index >> 5]);
    Sleeper sleeper;
    while ((value & _mask) != 0 &&
            !AtomicCompareExchange(
                &mLock[index >> 5], value & ~_mask, value))
    {
        sleeper.Pause();
        value = AtomicGet(&mLock[index >> 5]);
    }
}

inline bool MicroLockVector::IsLocked(uint32_t index) const
{
    return (AtomicGet(&mLock[index >> 5]) & mask(index)) != 0;
}

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

MicroRWLockRefCounted::MicroRWLockRefCounted(LockType type)
{
    AddRef();
    switch (type)
    {
        case READ_LOCK:
            ReadLock();
            break;
        case WRITE_LOCK:
            WriteLock();
            break;
        case RELEASE:
            abort();
    }
}

inline void MicroRWLockRefCounted::ReadLock()
{
    AddRef();
    MicroRWLock::ReadLock();
}

inline void MicroRWLockRefCounted::ReadUnlock()
{
    MicroRWLock::ReadUnlock();
    RefCounted<MicroRWLockRefCounted>::Release();
}

inline void MicroRWLockRefCounted::WriteLock()
{
    AddRef();
    MicroRWLock::WriteLock();
}

inline void MicroRWLockRefCounted::WriteUnlock()
{
    MicroRWLock::WriteUnlock();
    RefCounted<MicroRWLockRefCounted>::Release();
}

#endif // if 0

#endif  // _SRC_SYNC_MICRO_LOCK_H
