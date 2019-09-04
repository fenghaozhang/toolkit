#include "src/base/gettime.h"

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "src/common/macros.h"
#include "src/cpu/cpu.h"
#include "src/sync/micro_lock.h"

#define USECS_PER_SEC       1000000ULL
#define UPDATE_TSC_INTERVAL 20000000000LL
#define INIT_TSC_INTERVAL   2000000000LL

static const char* gClockSource =
    "/sys/devices/system/clocksource/clocksource0/current_clocksource";
static double gUsPerCycle = 0.0;
static MicroLock gLock;

static bool IsReliableTsc()
{
    bool ret = false;
    FILE* file = fopen(gClockSource, "r");
    if (file != NULL)
    {
        char line[4];
        if (fgets(line, sizeof(line), file) != NULL
                && strcmp(line, "tsc") == 0)
        {
            ret = true;
        }
        fclose(file);
    }
    return ret;
}

static void InitReferenceTime(
        uint64_t* lastUs,
        uint64_t* lastCycles,
        uint64_t us,
        uint64_t cycles)
{
    if (*lastCycles == 0)
    {
        *lastCycles = cycles;
        *lastUs = us;
    }
    else
    {
        uint64_t diff = cycles - *lastCycles;
        if (diff > INIT_TSC_INTERVAL && gUsPerCycle == 0.0)
        {
            ScopedLock<MicroLock> lock(gLock);
            if (gUsPerCycle == 0.0)
            {
                gUsPerCycle = (us - *lastUs) / static_cast<double>(diff);
            }
        }
    }
}

static uint64_t GetTimeOfDay()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * USECS_PER_SEC + tv.tv_usec;
}

static void HandleFallback(uint64_t* us)
{
    static __thread uint64_t tls_ref_last_returned_us;

    if (UNLIKELY(tls_ref_last_returned_us > *us))
    {
        *us = tls_ref_last_returned_us;
    }
    else
    {
        tls_ref_last_returned_us = *us;
    }
}

static bool gIsReliable = IsReliableTsc();

uint64_t GetCurrentTimeInUs()
{
    static __thread uint64_t tls_ref_last_cycle;
    static __thread uint64_t tls_ref_last_us;

    // If tsc not relaible, use GTOD instead.
    if (UNLIKELY(!gIsReliable))
    {
        return GetTimeOfDay();
    }

    if (UNLIKELY(tls_ref_last_us == 0 || gUsPerCycle == 0.0))
    {
        uint64_t cycles = GetCpuCycles();
        uint64_t retus = GetTimeOfDay();
        InitReferenceTime(&tls_ref_last_us, &tls_ref_last_cycle, retus, cycles);
        return retus;
    }

    // On 2.xGHz, UPDATE_TSC_INTERVAL cycles is about 10s.
    // Since TSCCycle2Ns has ~1/10M deviation,we have at most
    // 1us offset against GTOD every 10s. It makes sence to
    // reset tls_ref_last_us and tls_ref_last_cycle every
    // UPDATE_TSC_INTERVAL cycles.
    uint64_t diff = GetCpuCycles() - tls_ref_last_cycle;
    uint64_t retus;
    if (diff > UPDATE_TSC_INTERVAL)
    {
        // slow path
        tls_ref_last_cycle = GetCpuCycles();
        retus = GetTimeOfDay();
        HandleFallback(&retus);
        tls_ref_last_us = retus;
    }
    else
    {
        double offset = diff * gUsPerCycle;
        retus = tls_ref_last_us + static_cast<uint64_t>(offset);
        HandleFallback(&retus);
    }
    return retus;
}
