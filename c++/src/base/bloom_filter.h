#ifndef _SRC_BASE_BLOOM_FILTER_H
#define _SRC_BASE_BLOOM_FILTER_H

#include <assert.h>
#include "common2/base/bit_vector.h"
#include "common2/base/hash.h"

typedef uint32_t(*HashFuncs)(const char*, size_t);

// k: the hash functions
// m: size of bloom filter
// n: actual size of samples.
// The probability of a false positive is minimized for k = ln2 *(m / n)
// When k = 2, m = 2.88 * n is preferred.
template<typename Key>
class BloomFilter
{
public:
    static const uint32_t kMaxNumFilters = 4;
    static const uint32_t kDefaultNumFilters = 2;

    // size: The estimated scale of samples. NOT capacity of bloom filters.
    explicit BloomFilter(uint32_t size,
                         uint32_t numFilters = kDefaultNumFilters)
        : mNumFilters(numFilters),
          mSize(0)
    {
        double kLn2 = 0.69314718; // Ln(2)
        mSize = static_cast<uint32_t>((size * numFilters) / kLn2);
        assert(mNumFilters <= kMaxNumFilters);
    }

    void Put(const Key& key, BitVector& bits) const
    {
        for (uint32_t i = 0; i < mNumFilters; ++i)
        {
            bits.SetTrue(Hash(key, i));
        }
    }

    void Put(const Key& key, uint32_t* bits) const
    {
        for (uint32_t i = 0; i < mNumFilters; ++i)
        {
            BitBase::SetTrue(bits, Hash(key, i));
        }
    }

    bool Get(const Key& key, const BitVector& bits) const
    {
        for (uint32_t i = 0; i < mNumFilters; ++i)
        {
            if (!bits.Get(Hash(key, i)))
            {
                return false;
            }
        }
        return true;
    }

    bool Get(const Key& key, const uint32_t* bits) const
    {
        for (uint32_t i = 0; i < mNumFilters; ++i)
        {
            if (!BitBase::Get(bits, Hash(key, i)))
            {
                return false;
            }
        }
        return true;
    }

    inline uint32_t GetSize() const
    {
        return mSize;
    }

    inline uint32_t GetBytes() const
    {
        return (mSize + 31) / 32 * 4;
    }

    inline uint32_t GetNumFilters() const
    {
        return mNumFilters;
    }

public:
    static HashFuncs sHashFuncs[kMaxNumFilters];

private:
    uint32_t Hash(const Key& key, uint32_t index) const
    {
        return sHashFuncs[index](reinterpret_cast<const char*>(&key),
                                 sizeof(key)) % mSize;
    }

    uint32_t mNumFilters;
    uint32_t mSize;
};

template<typename Key>
HashFuncs BloomFilter<Key>::sHashFuncs[BloomFilter<Key>::kMaxNumFilters] =
    { XXHash, MurmurHash, JSHash, FNVHash };

#endif  // _SRC_BASE_BLOOM_FILTER_H
