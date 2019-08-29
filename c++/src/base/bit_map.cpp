#include "src/base/bit_map.h"

#include <algorithm>

#include "src/common/assert.h"

BitMap::BitMap(size_t capacity, MemPool* pool)
    : mCapacity(capacity),
      mSliceBits(sizeof(SliceType) * 8),
      mSliceCount((capacity + mSliceBits - 1) / mSliceBits),
      mPool(pool)
{
    initBitMap();
}

BitMap::~BitMap()
{
    if (mPool == NULL)
    {
        delete[] mSlices;
    }
}

size_t BitMap::Count() const
{
    uint32_t total = 0;
    for (size_t i = 0; i < mSliceCount; ++i)
    {
        total += __builtin_popcount(mSlices[i]);
    }
    return total;
}

inline void BitMap::initBitMap()
{
    if (mPool != NULL)
    {
        void* mem = mPool->Alloc(mSliceCount * sizeof(SliceType));
        mSlices = new (mem) SliceType[mSliceCount];
    }
    else
    {
        mSlices = new SliceType[mSliceCount];
    }
    Reset();
}

SparseBitMap::SparseBitMap(size_t capacity, MemPool* pool)
    : mPool(pool)
{
    capacity = MIN(capacity, sMaxCapacity);
    mBitsPerMap = getBitsPerMap(capacity);
    uint32_t count = (capacity + mBitsPerMap - 1) / mBitsPerMap;
    mCapacity = count * mBitsPerMap;
    mMaps.resize(count, NULL);
    for (uint32_t i = 0; i < count; i++)
    {
        mMaps[i] = allocBitMap();
    }
}

SparseBitMap::~SparseBitMap()
{
    if (mPool == NULL)
    {
        for (size_t i = 0; i < mMaps.size(); i++)
        {
            if (mMaps[i] != NULL)
            {
                delete mMaps[i];
            }
        }
    }
}

size_t SparseBitMap::Capacity() const
{
    return mCapacity;
}

void SparseBitMap::Set(size_t index)
{
    ASSERT(index < sMaxCapacity);
    allocMapIfNeeded(index);
    size_t mapIndex = index / mBitsPerMap;
    size_t offset = index % mBitsPerMap;
    mMaps[mapIndex]->Set(offset);
}

bool SparseBitMap::Get(size_t index) const
{
    ASSERT(index < sMaxCapacity);
    size_t mapIndex = index / mBitsPerMap;
    size_t offset = index % mBitsPerMap;
    if (UNLIKELY(index >= mCapacity || mMaps[mapIndex] == NULL))
    {
        return false;
    }
    return mMaps[mapIndex]->Get(offset);
}

bool SparseBitMap::operator[](size_t index) const
{
    return Get(index);
}

void SparseBitMap::Clear(size_t index)
{
    ASSERT(index < sMaxCapacity);
    size_t mapIndex = index / mBitsPerMap;
    size_t offset = index % mBitsPerMap;
    if (LIKELY(index < mCapacity && mMaps[mapIndex] != NULL))
    {
        mMaps[mapIndex]->Clear(offset);
    }
}

void SparseBitMap::Reset()
{
    for (size_t i = 0; i < mMaps.size(); ++i)
    {
        if (mMaps[i] != NULL)
        {
            mMaps[i]->Reset();
            mMaps[i] = NULL;
        }
    }
}

inline void SparseBitMap::allocMapIfNeeded(size_t index)
{
    size_t mapIndex = index / mBitsPerMap;
    if (UNLIKELY(mapIndex >= mMaps.size()))
    {
        mMaps.resize(mapIndex + 1, NULL);
        mMaps[mapIndex] = allocBitMap();
        mCapacity = (mapIndex + 1) * mBitsPerMap;
    }
    else if (UNLIKELY(mMaps[mapIndex] == NULL))
    {
        mMaps[mapIndex] = allocBitMap();
    }
}

inline BitMap* SparseBitMap::allocBitMap()
{
    if (mPool != NULL)
    {
        return mPool->New<BitMap>(mBitsPerMap, mPool);
    }
    else
    {
        return new BitMap(mBitsPerMap, NULL);
    }
}

size_t SparseBitMap::getBitsPerMap(size_t capacity)
{
    const size_t kMinBitsPerMap = 1024;
    const size_t kMaxBitsPerMap = 1024 * 256;
    return MIN(MAX(kMinBitsPerMap, capacity/4), kMaxBitsPerMap);
}
