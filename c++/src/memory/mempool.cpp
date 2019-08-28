#include "src/memory/mempool.h"

MemPool::MemPool(size_t bufferSize)
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

void* MemPool::Alloc(size_t size)
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

void* MemPool::AllocAligned(size_t size)
{
    const size_t kAlign = 8;
    char* ptr = reinterpret_cast<char*>
        ((reinterpret_cast<uintptr_t>(mPtr) + kAlign - 1)
         / kAlign * kAlign);
    mBufferOffset += ptr - mPtr;
    mPtr = ptr;
    return Alloc(size);
}

size_t MemPool::GetMemoryUsage() const
{
    MemCacheStat stat;
    mMemCache.GetStats(&stat);
    return mBufferSize * stat.objCount + sizeof(*this);
}
