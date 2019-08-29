#include "src/sync/micro_lock.h"

#include <assert.h>
#include <stdint.h>
#include <sys/time.h>

#include "src/base/gettime.h"
#include "src/sync/atomic.h"
#include "src/thread/this_thread.h"

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

void MicroLock::Lock()
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

bool MicroLock::TryLock()
{
    uint64_t signature = getSignature();
    return AtomicCompareExchange(&mLock, signature, kLockOff);
}

void MicroLock::Unlock()
{
    AtomicSet(&mLock, kLockOff);
}

bool MicroLock::IsLocked() const
{
    return AtomicGet(&mLock) != kLockOff;
}

bool MicroLock::TimedLock(uint64_t timeoutInUs)
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
                return false; // timeout
            }
        }
    }
    while (!AtomicCompareExchange(&mLock, signature, kLockOff));
    return true;
}

int MicroLock::GetLockOwner() const
{
    return static_cast<int>(
            (AtomicGet(&mLock) & 0XFFFFFFFF00000000) >> 32);
}

FORCE_INLINE uint64_t MicroLock::getSignature() const
{
    int tid = ThisThread::GetId();
    return (static_cast<uint64_t>(tid) << 32) | kLockOn;
}

void MicroRWLock::ReadLock()
{
    AtomicInc(&mLock);
    Sleeper sleeper;
    while (IsWriteLocked())
    {
        sleeper.Pause();
    }
}

void MicroRWLock::ReadUnlock()
{
    uint64_t bits = AtomicDec(&mLock);
    if ((bits & kWriterMask) != 0)
    {
        ASSERT(false);
    }
}

void MicroRWLock::WriteLock()
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

void MicroRWLock::WriteUnlock()
{
    uint64_t value = AtomicGet(&mLock);
    while (!AtomicCompareExchange(&mLock, value & kReaderMask, value))
    {
        value = AtomicGet(&mLock);
    }
}

bool MicroRWLock::IsReadLocked() const
{
    return (AtomicGet(&mLock) & kReaderMask) != 0;
}

bool MicroRWLock::IsWriteLocked() const
{
    return (AtomicGet(&mLock) & kWriterMask) != 0;
}

bool MicroRWLock::IsLocked() const
{
    return AtomicGet(&mLock) != 0;
}

bool MicroRWLock::TryReadLock()
{
    // TODO: Implements later.
    return false;
}

bool MicroRWLock::TryWriteLock()
{
    uint64_t signature = (static_cast<uint64_t>(
                ThisThread::GetId()) << 32) & kWriterMask;
    return AtomicCompareExchange(&mLock, signature, 0UL);
}

bool MicroRWLock::TimedReadLock(uint64_t timeoutInUs)
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

bool MicroRWLock::TimedWriteLock(uint64_t timeoutInUs)
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

int MicroRWLock::GetWriteLockOwner() const
{
    return static_cast<int>((AtomicGet(&mLock) & kWriterMask) >> 32);
}

void MicroRWLockPreferWrite::ReadLock()
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

void MicroRWLockPreferWrite::ReadUnlock()
{
    AtomicDec(&mLock);
}

void MicroRWLockPreferWrite::WriteLock()
{
    Sleeper sleeper;
    uint64_t value = 0;
    // Set write lock bit firstly.
    uint64_t writerOwner = ThisThread::GetId();
    writerOwner = (writerOwner << 32) & kWriterMask;

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

void MicroRWLockPreferWrite::WriteUnlock()
{
    uint64_t value = AtomicGet(&mLock);
    while (!AtomicCompareExchange(&mLock, value & kReaderMask, value))
    {
        value = AtomicGet(&mLock);
    }
}

bool MicroRWLockPreferWrite::IsReadLocked() const
{
    return (AtomicGet(&mLock) & kReaderMask) != 0;
}

bool MicroRWLockPreferWrite::IsWriteLocked() const
{
    // Same as read lock.
    return (AtomicGet(&mLock) & kWriterMask) != 0;
}

bool MicroRWLockPreferWrite::IsLocked() const
{
    return AtomicGet(&mLock) != 0;
}

bool MicroRWLockPreferWrite::TimedReadLock(uint64_t timeoutInUs)
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

bool MicroRWLockPreferWrite::TimedWriteLock(uint64_t timeoutInUs)
{
    uint64_t begin = GetCurrentTimeInUs();
    Sleeper sleeper;
    uint64_t value = 0;
    // Set write lock bit firstly.
    uint64_t writerOwner = ThisThread::GetId();
    writerOwner = (writerOwner << 32) & kWriterMask;

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

int MicroRWLockPreferWrite::GetWriteLockOwner() const
{
    return static_cast<int>(AtomicGet(&mLock) >> 32);
}

MicroLockVector::MicroLockVector(uint32_t size)
{
    mLock.assign(((size + 31) >> 5), 0UL);
}

void MicroLockVector::Lock(uint32_t index)
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

bool MicroLockVector::TryLock(uint32_t index)
{
    const uint32_t _mask = mask(index);
    const uint32_t value = AtomicGet(&mLock[index >> 5]);
    return AtomicCompareExchange(
            &(mLock[index >> 5]), value | _mask, value & ~_mask);
}

void MicroLockVector::Unlock(uint32_t index)
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

bool MicroLockVector::IsLocked(uint32_t index) const
{
    return (AtomicGet(&mLock[index >> 5]) & mask(index)) != 0;
}

#if 0

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

void MicroRWLockRefCounted::ReadLock()
{
    AddRef();
    MicroRWLock::ReadLock();
}

void MicroRWLockRefCounted::ReadUnlock()
{
    MicroRWLock::ReadUnlock();
    RefCounted<MicroRWLockRefCounted>::Release();
}

void MicroRWLockRefCounted::WriteLock()
{
    AddRef();
    MicroRWLock::WriteLock();
}

void MicroRWLockRefCounted::WriteUnlock()
{
    MicroRWLock::WriteUnlock();
    RefCounted<MicroRWLockRefCounted>::Release();
}

#endif
