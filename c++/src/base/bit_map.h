#ifndef _SRC_BASE_BIT_VECTOR_H
#define _SRC_BASE_BIT_VECTOR_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "src/common/assert.h"
#include "src/common/macros.h"
#include "src/memory/arena.h"

class BitBase
{
public:
    static bool Get(const uint32_t* bits, size_t index)
    {
        return (bits[index >> 5] & Mask(index)) != 0;
    }

    static void Set(uint32_t* bits, size_t index)
    {
        bits[index >> 5] |= Mask(index);
    }

    static void Clear(uint32_t* bits, size_t index)
    {
        bits[index >> 5] &= ~Mask(index);
    }

    static uint32_t Mask(size_t index)
    {
        return 1U << (31 - (index & 0x1f));
    }
};

class BitMap
{
public:
    explicit BitMap(size_t capacity, Arena* arena = NULL);
    ~BitMap();

    size_t Capacity() const
    {
        return mCapacity;
    }

    bool Get(size_t index) const
    {
        ASSERT(index < mCapacity);
        return BitBase::Get(mSlices, index);
    }
    
    bool operator[](size_t index) const
    {
        ASSERT(index < mCapacity);
        return BitBase::Get(mSlices, index);
    }

    void Set(size_t index)
    {
        ASSERT(index < mCapacity);
        BitBase::Set(mSlices, index);
    }

    void Clear(size_t index)
    {
        ASSERT(index < mCapacity);
        BitBase::Clear(mSlices, index);
    }

    void Reset()
    {
        memset(mSlices, 0, mSliceCount * 32);
    }

    /** Return numbers of bit '1' */
    size_t Count() const;

private:
    typedef uint32_t SliceType;
    void initBitMap();
    static size_t countTrue(uint32_t x);
    static size_t firstFalse(uint32_t x);

    SliceType* mSlices;
    const size_t mCapacity;
    const size_t mSliceBits;
    const size_t mSliceCount;
    Arena* mArena;

    DISALLOW_COPY_AND_ASSIGN(BitMap);
};

class SparseBitMap
{
public:
    explicit SparseBitMap(size_t capacity = 0, Arena* arena = NULL);
    ~SparseBitMap();

    size_t Capacity() const;

    void Set(size_t index);

    bool Get(size_t index) const;

    bool operator[](size_t index) const;

    void Clear(size_t index);

    void Reset();

private:
    void allocMapIfNeeded(size_t index);
    BitMap* allocBitMap();
<<<<<<< HEAD
=======

>>>>>>> Add SparseBitMap
    static size_t getBitsPerMap(size_t capacity);
    static const size_t sMaxCapacity = UINT32_MAX;

    size_t mCapacity;
    size_t mBitsPerMap;
    Arena* mArena;
    std::vector<BitMap*> mMaps;

    DISALLOW_COPY_AND_ASSIGN(SparseBitMap);
};

#endif // _SRC_BASE_BIT_VECTOR_H
