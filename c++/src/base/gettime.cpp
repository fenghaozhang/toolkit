#include "src/base/gettime.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "src/base/cpu.h"
#include "src/common/macros.h"

static bool gUseTsc = false;

static bool IsReliableTsc()
{
    static const char* gClockSource =
        "/sys/devices/system/clocksource/clocksource0/current_clocksource";
    FILE* fp = fopen(gClockSource, "r");
    if (fp == NULL)
    {
        return false;
    }

    char buf[4];
    if (fgets(buf, sizeof(buf), fp) == NULL || strcmp(buf, "tsc"))
    {
        fclose(fp);
        return false;
    }
    fclose(fp);
    return true;
}

static const bool gIsTscReliable = IsReliableTsc();

static inline uint64_t getTimeOfDay()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

static inline uint64_t getTimeByCpuCycle()
{
    static volatile double gUsPerCycle = 0.0;

    if (UNLIKELY(gUsPerCycle == 0.0))
    {
        // TODO(allen.zfh): do something
    }
    // TODO(allen.zfh): to replace with cpu cycle calculation
    return getTimeOfDay();
}

uint64_t GetCurrentTimeInUs()
{
    if (LIKELY(gIsTscReliable && gUseTsc))
    {
        return getTimeByCpuCycle();
    }
    return getTimeOfDay();
}

