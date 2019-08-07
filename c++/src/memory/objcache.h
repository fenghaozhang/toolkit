#ifndef _SRC_MEMORY_OBJCACHE_H
#define _SRC_MEMORY_OBJCACHE_H

#include "src/common/macro.h"
#include "src/memory/memcache.h"

template <typename T>
class ObjectCache
{
public:
    typedef MemCacheStat ObjectCacheStat;

    explicit ObjectCache(const char* const name = NULL,
                         bool construct = false);
    ~ObjectCache();

    T* Alloc();
    void Dealloc(T* ptr);

    void Stat(ObjectCacheStat* stat);

private:
    void init(const char* const name, bool construct);
    static void constructHelper(void* obj);
    static void destructHelper(void* obj);

    MemCache mMemCache;

    DISALLOW_COPY_AND_ASSIGN(ObjectCache);
};

template <typename T>
ObjectCache<T>::ObjectCache(
        const char* const name, bool construct)
{
    init(name, construct);
}

template <typename T>
ObjectCache<T>::~ObjectCache()
{
}

template <typename T>
inline T* ObjectCache<T>::Alloc()
{
    void* ptr = mMemCache.Alloc();
    if (UNLIKELY(ptr == NULL))
    {
        return NULL;
    }
    return static_cast<T*>(ptr);
}

template <typename T>
inline void ObjectCache<T>::Dealloc(T* obj)
{
    if (UNLIKELY(obj == NULL))
    {
        return;
    }
    mMemCache.Dealloc(obj);
}

template <typename T>
inline void ObjectCache<T>::Stat(ObjectCacheStat* stat)
{
    mMemCache.Stat(stat);
}

template <typename T>
void ObjectCache<T>::init(const char* const name, bool construct)
{
    MemCache::Options options;
    if (name != NULL)
    {
        options.name = name;
    }
    if (construct)
    {
        options.ctor = &constructHelper;
        options.dtor = &destructHelper;
    }
    options.objSize = sizeof(T);
    mMemCache.Init(options);
}

template <typename T>
inline void ObjectCache<T>::constructHelper(void* ptr)
{
    new (ptr) T;
}

template <typename T>
inline void ObjectCache<T>::destructHelper(void* ptr)
{
    T* object = static_cast<T*>(ptr);
    object->~T();
}

#endif  // _SRC_MEMORY_OBJCACHE_H
