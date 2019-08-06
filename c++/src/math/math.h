#ifndef _SRC_MATH_MATH_H
#define _SRC_MATH_MATH_H

static inline bool IsPowerOfTwo(uint64_t n)
{
    return n > 0 && !(n & (n - 1));
}

static inline uint64_t RoundUpToPowerOfTwo(uint64_t n)
{
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    n++;
    return n;
}

#endif  // _SRC_MATH_MATH_H
