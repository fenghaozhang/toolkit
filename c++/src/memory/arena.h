#ifndef _SRC_MEMORY_ARENA_H
#define _SRC_MEMORY_ARENA_H

#include <vector>

#include "src/common/macros.h"
#include "src/memory/memcache.h"

class Arena
{
public:
    explicit Arena(size_t bufferSize = 4096)
        : mPtr(NULL),
          mBufferOffset(bufferSize),
          mBufferSize(bufferSize)
    {
        MemCache::Options options;
        options.objSize = bufferSize;
        mMemCache.Init(options);
        MemCacheStat stat;
        mMemCache.GetStats(&stat);
    }

    void* Alloc(size_t size)
    {
        if (UNLIKELY(size > mBufferSize))
        {
            return NULL;
        }
        if (UNLIKELY(size + mBufferOffset >= mBufferSize))
        {
            mPtr = static_cast<char*>(mMemCache.Alloc());
            mBufferOffset = 0;
        }
        char* ptr = mPtr;
        mPtr += size;
        mBufferOffset += size;
        return ptr;
    }

    void* AllocAligned(size_t size)
    {
        const size_t kAlign = 8;
        char* ptr = reinterpret_cast<char*>
            ((reinterpret_cast<uintptr_t>(mPtr) + kAlign - 1)
             / kAlign * kAlign);
        mBufferOffset += ptr - mPtr;
        mPtr = ptr;
        return Alloc(size);
    }

    size_t GetMemoryUsage() const
    {
        MemCacheStat stat;
        mMemCache.GetStats(&stat);
        return mBufferSize * stat.objCount + sizeof(*this);
    }

private:
    char*              mPtr;
    size_t             mBufferOffset;
    size_t             mBufferSize;
    MemCache           mMemCache;

    DISALLOW_COPY_AND_ASSIGN(Arena);
};

#endif  // _SRC_MEMORY_ARENA_H
