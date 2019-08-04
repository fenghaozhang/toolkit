// Copyright (c) 2016, Alibaba Inc. All rights reserved.

#ifndef PANGU2_COMMON_MEMORY_OBJECT_CACHE_H
#define PANGU2_COMMON_MEMORY_OBJECT_CACHE_H

#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string>

#include "common2/memory/mem_cache.h"
#include "common2/macros.h"

namespace apsara
{
namespace pangu
{

struct ObjectCacheOptions
{
    std::string name;       // for logging
    size_t reserve;         // unit: object
    size_t reserveAtStart;  // unit: object
    size_t limit;           // unit: object
    bool reconstruct;       // if true, call constructor when Alloc()
                            // and destructor when Free()

    ObjectCacheOptions()
        : name("unnamed_object_pool"),
          reserve(16),
          reserveAtStart(0),
          limit(SIZE_MAX),
          reconstruct(true)
    {
    }
};

/**
 * A object pool with size constraints.
 *
 * This pool is used for holding long-live objects.   Short-live objects should
 * be managed by EasyPool.
 *
 * The object T must have a zero-parameter constructor.  If your constructor
 * have more constructors, we should implement your own object cache based on
 * MemCache.
 *
 * This is NOT thread-safe.
 */
template <typename T>
class ObjectCache
{
public:
    ObjectCache() : mReconstruct(true) {}

    void Init();
    void Init(const ObjectCacheOptions& options);
    T* Alloc();
    void Free(T* p);
    void GetStats(MemCacheStats* stats) const;
    bool IsEmpty() const;

private:
    static void constructHelper(void* p);
    static void destructHelper(void* p);

    MemCache mCache;
    bool mReconstruct;

    PANGU_DISALLOW_COPY_AND_ASSIGN(ObjectCache);
};

// IMPLEMENTATION
template <typename T>
void ObjectCache<T>::Init()
{
    ObjectCacheOptions defaultOptions;
    Init(defaultOptions);
}

template <typename T>
void ObjectCache<T>::Init(const ObjectCacheOptions& options)
{
    MemCacheOptions memCacheOptions;
    memCacheOptions.name = options.name;
    memCacheOptions.objectSize = sizeof(T);
    memCacheOptions.reserve = options.reserve;
    memCacheOptions.reserveAtStart = options.reserveAtStart;
    memCacheOptions.limit = options.limit;
    mReconstruct = options.reconstruct;
    if (!mReconstruct)
    {
        memCacheOptions.ctor = &ObjectCache::constructHelper;
        memCacheOptions.dtor = &ObjectCache::destructHelper;
    }
    mCache.Init(memCacheOptions);
}

template <typename T>
T* ObjectCache<T>::Alloc()
{
    void* mem = mCache.Alloc();
    if (PANGU_UNLIKELY(mem == NULL))
    {
        return NULL;
    }
    if (mReconstruct)
    {
        return new (mem) T;
    }
    return reinterpret_cast<T*>(mem);
}

template <typename T>
void ObjectCache<T>::Free(T* p)
{
    if (PANGU_UNLIKELY(p == NULL))
    {
        return;
    }
    if (mReconstruct)
    {
        p->~T();
    }
    mCache.Free(p);
}

template <typename T>
void ObjectCache<T>::GetStats(MemCacheStats* stats) const
{
    mCache.GetStats(stats);
}

template <typename T>
bool ObjectCache<T>::IsEmpty() const
{
    return mCache.IsEmpty();
}

template <typename T>
void ObjectCache<T>::constructHelper(void* p)
{
    new (p) T;
}

template <typename T>
void ObjectCache<T>::destructHelper(void* p)
{
    T* object = static_cast<T*>(p);
    object->~T();
}

}  // namespace pangu
}  // namespace apsara

#endif   // PANGU2_COMMON_MEMORY_OBJECT_CACHE_H
