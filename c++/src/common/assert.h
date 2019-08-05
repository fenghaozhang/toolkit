#ifndef _SRC_COMMON_ASSERT_H
#define _SRC_COMMON_ASSERT_H

#include <assert.h>

#include "logging.h"

#define ASSERT(x)                                       \
        do {                                            \
            if (UNLIKELY(!(x)))                         \
            {                                           \
                FlushLog(); assert(x);                  \
            }                                           \
        } while (false);

#ifdef DEBUG
#define ASSERT_DEBUG(x)     ASSERT(x)
#else
#define ASSERT_DEBUG(x)
#endif  // DEBUG

#endif  // _SRC_COMMON_ASSERT_H
