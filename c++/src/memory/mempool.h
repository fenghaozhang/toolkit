#ifndef _SRC_MEMORY_MEMPOOL_H
#define _SRC_MEMORY_MEMPOOL_H

#include "src/common/macros.h"
#include "src/memory/memcache.h"

class MemPool
{
public:
    explicit MemPool(size_t bufferSize = 4096);

    void* Alloc(size_t size);

    void* AllocAligned(size_t size);

    size_t GetMemoryUsage() const;

    template <typename T>
    T* New()
    {
        void* mem = Alloc(sizeof(T));
        return new (mem) T;
    }

    template <typename T, typename Arg1>
    T* New(const Arg1& arg1)
    {
        void* mem = Alloc(sizeof(T));
        return new (mem) T(arg1);
    }

    template <typename T, typename Arg1, typename Arg2>
    T* New(const Arg1& arg1, const Arg2& arg2)
    {
        void* mem = Alloc(sizeof(T));
        return new (mem) T(arg1, arg2);
    }

    template <typename T, typename Arg1, typename Arg2, typename Arg3>
    T* New(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
    {
        void* mem = Alloc(sizeof(T));
        return new (mem) T(arg1, arg2, arg3);
    }

    template <typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    T* New(const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
    {
        void* mem = Alloc(sizeof(T));
        return new (mem) T(arg1, arg2, arg3, arg4);
    }

private:
    char*              mPtr;
    size_t             mBufferOffset;
    size_t             mBufferSize;
    MemCache           mMemCache;

    DISALLOW_COPY_AND_ASSIGN(MemPool);
};

#endif  // _SRC_MEMORY_MEMPOOL_H
