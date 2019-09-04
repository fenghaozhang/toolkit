#include "src/cpu/cpu.h"

#include <stdio.h>

#include "src/common/macros.h"

const double DEFAULT_CPU_MHZ = 2500;
static const char* gCpuInfo = "/proc/cpuinfo";

static volatile bool gCpuMHzInited = false;
static double gCpuMHz = 0;

static inline double readCpuMHz();

uint64_t GetCpuMHz()
{
    if (LIKELY(gCpuMHzInited))
    {
        return gCpuMHz;
    }
    gCpuMHz = readCpuMHz();
    gCpuMHzInited = true;
    return gCpuMHz;
}

static inline double readCpuMHz()
{
    FILE* fp = fopen(gCpuInfo, "r");
    if (fp == NULL)
    {
        return DEFAULT_CPU_MHZ;
    }

    double cpuMHz = DEFAULT_CPU_MHZ;
    char buf[256];
    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        if (sscanf(buf, "cpu MHz     : %lf", &cpuMHz) == 1)  // NOLINT
        {
            break;
        }
    }
    fclose(fp);
    return cpuMHz;
}


