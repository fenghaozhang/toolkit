#ifndef _SRC_THREAD_THREAD_CHECK_H
#define _SRC_THREAD_THREAD_CHECK_H

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "src/common/macro.h"

inline static void CheckPthreadError(int status)
{
    if (UNLIKELY(status != 0)) {
        fprintf(stderr, "PthreadError: %s\n", ::strerror(status));
        ABORT();
    }
}

inline static bool CheckPthreadTimedLockError(int status)
{
    if (UNLIKELY(status != 0)) {
        if (status == ETIMEDOUT) {
            return false;
        } else {
            fprintf(stderr, "PthreadError: %s\n", ::strerror(status));
            ABORT();
        }
    }
    return true;
}

inline static int CheckPthreadTryLockError(int status, char* msg = NULL)
{
    if (UNLIKELY(status != 0)) {
        if (status == EBUSY) {
            return false;
        } else {
            fprintf(stderr, "PthreadError: %s\n", ::strerror(status));
            ABORT();
        }
    }
    return true;
}

#endif // _SRC_THREAD_THREAD_CHECK_H
