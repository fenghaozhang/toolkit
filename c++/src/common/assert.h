#ifndef _SRC_COMMON_MACRO_H
#define _SRC_COMMON_MACRO_H

#include <assert.h>

#include "logging.h"

#define ASSERT(x)                                       \
        do {                                            \
            if (UNLIKELY(!(x)))                         \
            {                                           \
                FlushLog(); assert(x);                  \
            }                                           \
        } while (false);

#ifdef DEBUG_MODE
#define ASSERT_DEBUG(x)     ASSERT(x)
#else
#define ASSERT_DEBUG(x)
#endif  // DEBUG_MODE

#endif  // _SRC_COMMON_MACRO_H
