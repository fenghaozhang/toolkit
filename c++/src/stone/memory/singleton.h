// Copyright (c) 2014, Alibaba Inc.
// All right reserved.
//
// Author: DongPing HUANG <dongping.huang@alibaba-inc.com>
// Created: 2014/06/16
// Description:

#ifndef STONE_MEMORY_SINGLETON_H
#define STONE_MEMORY_SINGLETON_H

#include "common2/stone/memory/uncopyable.h"
#include "common2/stone/threading/sync/lock.h"

// Don't use singleton whenever you have another chioce.
//
// Singletons make it hard to determine the lifetime of an object, which can
// lead to buggy code and spurious crashes.
//
// If you absolutely need a singleton, please keep them as trivial as possible
// and ideally a leaf dependency. Singletons get problematic when they attempt
// to do too much in their destructor or have circular dependencies.

namespace apsara {
namespace pangu {
namespace stone {

template<typename T>
class Singleton : private stone::Uncopyable
{
public:
    static T& Instance()
    {
        if (sInstance == NULL) {
            stone::ScopedLocker<pthread_mutex_t> locker(sLock);
            if (sInstance == NULL) {
                if (sDestroyed) {
                    // log error
                    sDestroyed = false;
                }
                T *p = new T;
                sInstance = p;
                ::atexit(Singleton::Destroy);
            }
        }
        return *sInstance;
    }

private:
    static void Destroy()
    {
        sDestroyed = true;
        delete sInstance;
        sInstance = NULL;
    }

private:
    static T* volatile sInstance;
    static pthread_mutex_t sLock;
    static bool sDestroyed;
};

template<typename T>
pthread_mutex_t Singleton<T>::sLock = PTHREAD_MUTEX_INITIALIZER;

template<typename T>
T* volatile Singleton<T>::sInstance = NULL;

template<typename T>
bool Singleton<T>::sDestroyed = false;

} // namespace stone
} // namespace pangu
} // namespace apsara

#endif // STONE_MEMORY_SINGLETON_H
