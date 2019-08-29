#ifndef _SRC_COMMON_ASM_H
#define _SRC_COMMON_ASM_H

namespace asm
{

/**
 * Close instruction optimization
 */
inline void Barrier()
{
    asm volatile("": : :"memory");
}

/**
 * This function can increase spin-wait-loop performance by:
 * 1. Close instruction and pipeline reorder
 * 2. Save cpu power
 */
inline void CpuRelax()
{
    asm volatile("pause\n": : :"memory");
}

}

#endif  // _SRC_COMMON_ASM_H
