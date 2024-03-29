#ifndef _SRC_COMMON_ASSERT_H
#define _SRC_COMMON_ASSERT_H

#include <assert.h>

#define ASSERT(x)                                       \
        do {                                            \
            if (UNLIKELY(!(x)))                         \
            {                                           \
                assert(x);                              \
            }                                           \
        } while (false);

#ifdef __DEBUG__
#define ASSERT_DEBUG(x)     ASSERT(x)
#else
#define ASSERT_DEBUG(x)
#endif  // __DEBUG__

template<bool> struct CompileTimeAssert;
template<> struct CompileTimeAssert<true> {};
#define STATIC_ASSERT(e) (CompileTimeAssert <(e) != 0>())

#endif  // _SRC_COMMON_ASSERT_H
