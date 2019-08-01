#ifndef _SRC_BASE_CPU_H
#define _SRC_BASE_CPU_H

#include <stdint.h>

static inline uint64_t GetCpuCycles()
{
    uint32_t low, high;
    __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high));
    return (static_cast<uint64_t>(high) << 32) | low;
}

uint64_t GetCpuMHz();

#endif  // _SRC_BASE_CPU_H
