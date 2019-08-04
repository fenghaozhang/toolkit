#ifndef _SRC_MATH_MATH_H
#define _SRC_MATH_MATH_H

static inline bool IsPowerOfTwo(uint64_t n)
{
    return n > 0 && !(n & (n - 1));
}

#endif  // _SRC_MATH_MATH_H
