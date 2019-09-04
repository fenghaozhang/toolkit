#ifndef _SRC_SYNC_COND_H
#define _SRC_SYNC_COND_H

#include "src/common/macros.h"
#include "src/sync/posix_lock.h"

class ConditionVariable
{
public:
    ConditionVariable();
    ~ConditionVariable();

    void Signal();
    void Broadcast();
    void Wait();
    bool TimedWait(int64_t us);

    void Lock();
    void Unlock();
    bool IsLocked() const;

private:
    pthread_cond_t mCond;
    pthread_mutex_t mMutex;

    DISALLOW_COPY_AND_ASSIGN(ConditionVariable);
};

inline ConditionVariable::ConditionVariable()
{
    CheckPthreadError(::pthread_cond_init(&mCond, NULL));
    CheckPthreadError(::pthread_mutex_init(&mMutex, NULL));
}

inline ConditionVariable::~ConditionVariable()
{
    CheckPthreadError(::pthread_cond_destroy(&mCond));
    CheckPthreadError(::pthread_mutex_destroy(&mMutex));
}

inline void ConditionVariable::Signal()
{
    CheckPthreadError(::pthread_cond_signal(&mCond));
}

inline void ConditionVariable::Broadcast()
{
    CheckPthreadError(::pthread_cond_broadcast(&mCond));
}

inline void ConditionVariable::Wait()
{
    CheckPthreadError(::pthread_cond_wait(&mCond, &mMutex));
}

inline bool ConditionVariable::TimedWait(int64_t us)
{
    struct timespec ts = getTimeSpec(GetCurrentTimeInUs() + us);
    return CheckPthreadTimedLockError(
            ::pthread_cond_timedwait(&mCond, &mMutex, &ts));
}

inline void ConditionVariable::Lock()
{
    CheckPthreadError(::pthread_mutex_lock(&mMutex));
}

inline void ConditionVariable::Unlock()
{
    CheckPthreadError(::pthread_mutex_unlock(&mMutex));
}

inline bool ConditionVariable::IsLocked() const
{
    return mMutex.__data.__lock > 0;
}

#endif  // _SRC_SYNC_COND_H
