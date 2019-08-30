#ifndef _SRC_THREAD_THIS_THREAD_H
#define _SRC_THREAD_THIS_THREAD_H

#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "src/thread/thread_check.h"

class ThisThread {
public:
    static void SleepInMs(int ms)
    {
        ::usleep(ms * 1000);
    }

    static void SleepInUs(int us)
    {
        ::usleep(us);
    }

    static void Yield()
    {
        ::pthread_yield();
    }

    static void Pause()
    {
        asm volatile("pause\n": : :"memory");
    }

    static int GetId()
    {
        static __thread pid_t tid = 0;
        if (tid == 0)
        {
            tid = ::syscall(SYS_gettid);
        }
        return tid;
    }

    static pthread_t GetThreadId()
    {
        return ::pthread_self();
    }

private:
    ThisThread();
};

#endif // _SRC_THREAD_THIS_THREAD_H
