#ifndef _SRC_BASE_EXPONENTIAL_BACKOFF_H
#define _SRC_BASE_EXPONENTIAL_BACKOFF_H

#include <stdint.h>
#include <algorithm>

#include "common2/macros.h"

namespace apsara
{
namespace pangu
{

class ExponentialBackoff
{
public:
    inline ExponentialBackoff();

    inline void Reset(uint64_t base, uint64_t limit, uint64_t scaleFactor);

    uint64_t Next();

private:
    uint64_t mCurrent;
    uint64_t mLimit;
    uint64_t mScaleFactor;

    PANGU_DISALLOW_COPY_AND_ASSIGN(ExponentialBackoff);
};

// IMPLEMENTRATIONS
inline ExponentialBackoff::ExponentialBackoff()
    : mCurrent(0),
      mLimit(0),
      mScaleFactor(0)
{
}

inline void ExponentialBackoff::Reset(
        uint64_t base,
        uint64_t limit,
        uint64_t scaleFactor)
{
    mCurrent = base;
    mLimit = limit;
    mScaleFactor = scaleFactor;
}

uint64_t ExponentialBackoff::Next()
{
    uint64_t result = mCurrent;
    mCurrent = std::min(mCurrent * mScaleFactor, mLimit);
    return result;
}

#endif  // _SRC_BASE_EXPONENTIAL_BACKOFF_H
