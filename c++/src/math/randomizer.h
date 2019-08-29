#ifndef _SRC_MATH_RANDOMIZER_H
#define _SRC_MATH_RANDOMIZER_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

class Randomizer
{
public:
    explicit Randomizer(uint64_t s = 0) { SetSeed(s); }
    void SetSeed(uint64_t s)
    {
        if (s == 0)
        {
            s = ::time(NULL);
        }
        srand48_r(s, &mData);
    }

    unsigned int Rand(unsigned int max = RAND_MAX)
    {
        if (max > RAND_MAX)
            max = RAND_MAX;
        return RandUInt32() % max;
    }

    uint32_t Next()
    {
        return RandUInt32();
    }

    /** Return a random boolean. */
    bool RandBool();

    /** Return a random integer uniformly distributed in [0, 2^32). */
    uint32_t RandUInt32();

    /** Return a random integer uninformly distributed in [0, 2^64). */
    uint64_t RandUInt64();

private:
    // This class is intentially to be copyable.
    struct drand48_data mData;
};

inline bool Randomizer::RandBool()
{
    uint32_t n = RandUInt32();
    return n & 0x1;
}

inline uint32_t Randomizer::RandUInt32()
{
    int64_t n = 0;
    mrand48_r(&mData, &n);  // mrand48_r() returns integer between -2^31 and 2^31.
    int32_t m = n;
    return static_cast<uint32_t>(m);
}

inline uint64_t Randomizer::RandUInt64()
{
    uint32_t hi = RandUInt32();
    uint32_t lo = RandUInt32();
    return (static_cast<uint64_t>(hi) << 32) | lo;
}

#endif  // _SRC_MATH_RANDOMIZER_H
