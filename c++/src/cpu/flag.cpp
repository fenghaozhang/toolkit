#include "src/cpu/flag.h"

#include <string.h>

#ifndef __cpuid
#define __cpuid(level, a, b, c, d)                    \
    __asm__ ("cpuid\n\t"                              \
             : "=a" (a), "=b" (b), "=c" (c), "=d" (d) \
             : "0" (level))
#endif  // __cpuid

#ifndef __cpuid_count
#define __cpuid_count(level, count, a, b, c, d)       \
    __asm__ ("cpuid\n\t"                              \
             : "=a" (a), "=b" (b), "=c" (c), "=d" (d) \
             : "0" (level), "2" (count))
#endif  // __cpuid_count

/* Return highest supported input value for cpuid instruction.  ext can
   be either 0x0 or 0x80000000 to return highest supported value for
   basic or extended cpuid information.  Function returns 0 if cpuid
   is not supported or whatever cpuid returns in eax register.  If sig
   pointer is non-null, then first four bytes of the signature
   (as found in ebx register) are returned in location pointed by sig.  */
static inline unsigned int __get_cpuid_max (unsigned int __ext, unsigned int *__sig)
{
    unsigned int __eax, __ebx, __ecx, __edx;

#ifndef __x86_64__
  /* See if we can use cpuid.  On AMD64 we always can.  */
#if __GNUC__ >= 3
    __asm__ ("pushf{l|d}\n\t"
             "pushf{l|d}\n\t"
             "pop{l}\t%0\n\t"
             "mov{l}\t{%0, %1|%1, %0}\n\t"
             "xor{l}\t{%2, %0|%0, %2}\n\t"
             "push{l}\t%0\n\t"
             "popf{l|d}\n\t"
             "pushf{l|d}\n\t"
             "pop{l}\t%0\n\t"
             "popf{l|d}\n\t"
             : "=&r" (__eax), "=&r" (__ebx)
             : "i" (0x00200000));
#else
/* Host GCCs older than 3.0 weren't supporting Intel asm syntax
   nor alternatives in i386 code.  */
    __asm__ ("pushfl\n\t"
             "pushfl\n\t"
             "popl\t%0\n\t"
             "movl\t%0, %1\n\t"
             "xorl\t%2, %0\n\t"
             "pushl\t%0\n\t"
             "popfl\n\t"
             "pushfl\n\t"
             "popl\t%0\n\t"
             "popfl\n\t"
             : "=&r" (__eax), "=&r" (__ebx)
             : "i" (0x00200000));
#endif

    if (!((__eax ^ __ebx) & 0x00200000))
    {
        return 0;
    }
#endif

    /* Host supports cpuid.  Return highest supported cpuid input value.  */
    __cpuid (__ext, __eax, __ebx, __ecx, __edx);

    if (__sig)
    {
        *__sig = __ebx;
    }

    return __eax;
}

// See https://github.com/gcc-mirror/gcc/blob/master/gcc/config/i386/driver-i386.c
void _get_local_cpu_flags(CpuFlags* flags)
{
#define bit_SSE3                (1 << 0)
#define bit_PCLMUL              (1 << 1)
#define bit_LZCNT               (1 << 5)
#define bit_SSSE3               (1 << 9)
#define bit_FMA                 (1 << 12)
#define bit_CMPXCHG16B          (1 << 13)
#define bit_SSE4_1              (1 << 19)
#define bit_SSE4_2              (1 << 20)
#define bit_MOVBE               (1 << 22)
#define bit_POPCNT              (1 << 23)
#define bit_AES                 (1 << 25)
#define bit_XSAVE               (1 << 26)
#define bit_OSXSAVE             (1 << 27)
#define bit_AVX                 (1 << 28)
#define bit_F16C                (1 << 29)
#define bit_RDRND               (1 << 30)
#define bit_CMPXCHG8B           (1 << 8)
#define bit_CMOV                (1 << 15)
#define bit_MMX                 (1 << 23)
#define bit_FXSAVE              (1 << 24)
#define bit_SSE                 (1 << 25)
#define bit_SSE2                (1 << 26)
#define bit_LAHF_LM             (1 << 0)
#define bit_ABM                 (1 << 5)
#define bit_SSE4a               (1 << 6)
#define bit_PRFCHW              (1 << 8)
#define bit_XOP                 (1 << 11)
#define bit_LWP                 (1 << 15)
#define bit_FMA4                (1 << 16)
#define bit_TBM                 (1 << 21)
#define bit_MWAITX              (1 << 29)
#define bit_MMXEXT              (1 << 22)
#define bit_LM                  (1 << 29)
#define bit_3DNOWP              (1 << 30)
#define bit_3DNOW               (1u << 31)
#define bit_CLZERO              (1 << 0)
#define bit_WBNOINVD            (1 << 9)
#define bit_FSGSBASE            (1 << 0)
#define bit_SGX                 (1 << 2)
#define bit_BMI                 (1 << 3)
#define bit_HLE                 (1 << 4)
#define bit_AVX2                (1 << 5)
#define bit_BMI2                (1 << 8)
#define bit_RTM                 (1 << 11)
#define bit_MPX                 (1 << 14)
#define bit_AVX512F             (1 << 16)
#define bit_AVX512DQ            (1 << 17)
#define bit_RDSEED              (1 << 18)
#define bit_ADX                 (1 << 19)
#define bit_AVX512IFMA          (1 << 21)
#define bit_CLFLUSHOPT          (1 << 23)
#define bit_CLWB                (1 << 24)
#define bit_AVX512PF            (1 << 26)
#define bit_AVX512ER            (1 << 27)
#define bit_AVX512CD            (1 << 28)
#define bit_SHA                 (1 << 29)
#define bit_AVX512BW            (1 << 30)
#define bit_AVX512VL            (1u << 31)
#define bit_PREFETCHWT1         (1 << 0)
#define bit_AVX512VBMI          (1 << 1)
#define bit_PKU                 (1 << 3)
#define bit_OSPKE               (1 << 4)
#define bit_WAITPKG             (1 << 5)
#define bit_AVX512VBMI2         (1 << 6)
#define bit_SHSTK               (1 << 7)
#define bit_GFNI                (1 << 8)
#define bit_VAES                (1 << 9)
#define bit_AVX512VNNI          (1 << 11)
#define bit_VPCLMULQDQ          (1 << 10)
#define bit_AVX512BITALG        (1 << 12)
#define bit_AVX512VPOPCNTDQ     (1 << 14)
#define bit_RDPID               (1 << 22)
#define bit_MOVDIRI             (1 << 27)
#define bit_MOVDIR64B           (1 << 28)
#define bit_CLDEMOTE            (1 << 25)
#define bit_AVX5124VNNIW        (1 << 2)
#define bit_AVX5124FMAPS        (1 << 3)
#define bit_IBT                 (1 << 20)
#define bit_PCONFIG             (1 << 18)
#define bit_BNDREGS             (1 << 3)
#define bit_BNDCSR              (1 << 4)
#define bit_XSAVEOPT            (1 << 0)
#define bit_XSAVEC              (1 << 1)
#define bit_XSAVES              (1 << 3)
#define bit_PTWRITE             (1 << 4)

    unsigned int eax, ebx, ecx, edx;
    unsigned int max_level, ext_level;
    unsigned int vendor;
    memset(flags, 0, sizeof(*flags));
    max_level = __get_cpuid_max (0, &vendor);
    if (max_level < 1)
    {
        return;
    }
    __cpuid (1, eax, ebx, ecx, edx);

#define TEST_BIT(x, bit)           \
    ((x) & (bit)) ? 1 : 0

    flags->has_sse3 = TEST_BIT(ecx, bit_SSE3);
    flags->has_ssse3 = TEST_BIT(ecx, bit_SSSE3);
    flags->has_sse4_1 = TEST_BIT(ecx, bit_SSE4_1);
    flags->has_sse4_2 = TEST_BIT(ecx, bit_SSE4_2);
    flags->has_avx = TEST_BIT(ecx, bit_AVX);
    flags->has_osxsave = TEST_BIT(ecx, bit_OSXSAVE);
    flags->has_cmpxchg16b = TEST_BIT(ecx, bit_CMPXCHG16B);
    flags->has_movbe = TEST_BIT(ecx, bit_MOVBE);
    flags->has_popcnt = TEST_BIT(ecx, bit_POPCNT);
    flags->has_aes = TEST_BIT(ecx, bit_AES);
    flags->has_pclmul = TEST_BIT(ecx, bit_PCLMUL);
    flags->has_fma = TEST_BIT(ecx, bit_FMA);
    flags->has_f16c = TEST_BIT(ecx, bit_F16C);
    flags->has_rdrnd = TEST_BIT(ecx, bit_RDRND);
    flags->has_xsave = TEST_BIT(ecx, bit_XSAVE);
    flags->has_cmpxchg8b = TEST_BIT(edx, bit_CMPXCHG8B);
    flags->has_cmov = TEST_BIT(edx, bit_CMOV);
    flags->has_mmx = TEST_BIT(edx, bit_MMX);
    flags->has_fxsr = TEST_BIT(edx, bit_FXSAVE);
    flags->has_sse = TEST_BIT(edx, bit_SSE);
    flags->has_sse2 = TEST_BIT(edx, bit_SSE2);

    if (max_level >= 7)
    {
        __cpuid_count (7, 0, eax, ebx, ecx, edx);

        flags->has_bmi = TEST_BIT(ebx, bit_BMI);
        flags->has_sgx = TEST_BIT(ebx, bit_SGX);
        flags->has_hle = TEST_BIT(ebx, bit_HLE);
        flags->has_rtm = TEST_BIT(ebx, bit_RTM);
        flags->has_avx2 = TEST_BIT(ebx, bit_AVX2);
        flags->has_bmi2 = TEST_BIT(ebx, bit_BMI2);
        flags->has_fsgsbase = TEST_BIT(ebx, bit_FSGSBASE);
        flags->has_rdseed = TEST_BIT(ebx, bit_RDSEED);
        flags->has_adx = TEST_BIT(ebx, bit_ADX);
        flags->has_avx512f = TEST_BIT(ebx, bit_AVX512F);
        flags->has_avx512er = TEST_BIT(ebx, bit_AVX512ER);
        flags->has_avx512pf = TEST_BIT(ebx, bit_AVX512PF);
        flags->has_avx512cd = TEST_BIT(ebx, bit_AVX512CD);
        flags->has_sha = TEST_BIT(ebx, bit_SHA);
        flags->has_clflushopt = TEST_BIT(ebx, bit_CLFLUSHOPT);
        flags->has_clwb = TEST_BIT(ebx, bit_CLWB);
        flags->has_avx512dq = TEST_BIT(ebx, bit_AVX512DQ);
        flags->has_avx512bw = TEST_BIT(ebx, bit_AVX512BW);
        flags->has_avx512vl = TEST_BIT(ebx, bit_AVX512VL);
        flags->has_avx512ifma = TEST_BIT(ebx, bit_AVX512IFMA);

        flags->has_prefetchwt1 = TEST_BIT(ecx, bit_PREFETCHWT1);
        flags->has_avx512vbmi = TEST_BIT(ecx, bit_AVX512VBMI);
        flags->has_pku = TEST_BIT(ecx, bit_OSPKE);
        flags->has_avx512vbmi2 = TEST_BIT(ecx, bit_AVX512VBMI2);
        flags->has_avx512vnni = TEST_BIT(ecx, bit_AVX512VNNI);
        flags->has_rdpid = TEST_BIT(ecx, bit_RDPID);
        flags->has_gfni = TEST_BIT(ecx, bit_GFNI);
        flags->has_vaes = TEST_BIT(ecx, bit_VAES);
        flags->has_vpclmulqdq = TEST_BIT(ecx, bit_VPCLMULQDQ);
        flags->has_avx512bitalg = TEST_BIT(ecx, bit_AVX512BITALG);
        flags->has_movdiri = TEST_BIT(ecx, bit_MOVDIRI);
        flags->has_movdir64b = TEST_BIT(ecx, bit_MOVDIR64B);
        flags->has_cldemote = TEST_BIT(ecx, bit_CLDEMOTE);

        flags->has_avx5124vnniw = TEST_BIT(edx, bit_AVX5124VNNIW);
        flags->has_avx5124fmaps = TEST_BIT(edx, bit_AVX5124FMAPS);

        flags->has_shstk = TEST_BIT(ecx, bit_SHSTK);
        flags->has_pconfig = TEST_BIT(edx, bit_PCONFIG);
        flags->has_waitpkg = TEST_BIT(ecx, bit_WAITPKG);
    }

    if (max_level >= 13)
    {
        __cpuid_count (13, 1, eax, ebx, ecx, edx);

        flags->has_xsaveopt = TEST_BIT(eax, bit_XSAVEOPT);
        flags->has_xsavec = TEST_BIT(eax, bit_XSAVEC);
        flags->has_xsaves = TEST_BIT(eax, bit_XSAVES);
    }

    if (max_level >= 0x14)
    {
        __cpuid_count (0x14, 0, eax, ebx, ecx, edx);

        flags->has_ptwrite = TEST_BIT(ebx, bit_PTWRITE);
    }

    /* Check cpuid level of extended features.  */
    __cpuid (0x80000000, ext_level, ebx, ecx, edx);

    if (ext_level >= 0x80000001)
    {
        __cpuid (0x80000001, eax, ebx, ecx, edx);

        flags->has_lahf_lm = TEST_BIT(ecx, bit_LAHF_LM);
        flags->has_sse4a = TEST_BIT(ecx, bit_SSE4a);
        flags->has_abm = TEST_BIT(ecx, bit_ABM);
        flags->has_lwp = TEST_BIT(ecx, bit_LWP);
        flags->has_fma4 = TEST_BIT(ecx, bit_FMA4);
        flags->has_xop = TEST_BIT(ecx, bit_XOP);
        flags->has_tbm = TEST_BIT(ecx, bit_TBM);
        flags->has_lzcnt = TEST_BIT(ecx, bit_LZCNT);
        flags->has_prfchw = TEST_BIT(ecx, bit_PRFCHW);

        flags->has_longmode = TEST_BIT(edx, bit_LM);
        flags->has_3dnowp = TEST_BIT(edx, bit_3DNOWP);
        flags->has_3dnow = TEST_BIT(edx, bit_3DNOW);
        flags->has_mwaitx = TEST_BIT(ecx, bit_MWAITX);
    }

    if (ext_level >= 0x80000008)
    {
        __cpuid (0x80000008, eax, ebx, ecx, edx);
        flags->has_clzero = TEST_BIT(ebx, bit_CLZERO);
        flags->has_wbnoinvd = TEST_BIT(ebx, bit_WBNOINVD);
    }

#undef TEST_BIT

    /* Get XCR_XFEATURE_ENABLED_MASK register with xgetbv.  */
#define XCR_XFEATURE_ENABLED_MASK    0x0
#define XSTATE_FP                    0x1
#define XSTATE_SSE                   0x2
#define XSTATE_YMM                   0x4
#define XSTATE_OPMASK                0x20
#define XSTATE_ZMM                   0x40
#define XSTATE_HI_ZMM                0x80

#define XCR_AVX_ENABLED_MASK \
    (XSTATE_SSE | XSTATE_YMM)
#define XCR_AVX512F_ENABLED_MASK \
    (XSTATE_SSE | XSTATE_YMM | XSTATE_OPMASK | XSTATE_ZMM | XSTATE_HI_ZMM)

    if (flags->has_osxsave)
    {
        asm (".byte 0x0f; .byte 0x01; .byte 0xd0"
                : "=a" (eax), "=d" (edx)
                : "c" (XCR_XFEATURE_ENABLED_MASK));
    }
    else
    {
        eax = 0;
    }

    /* Check if AVX registers are supported.  */
    if ((eax & XCR_AVX_ENABLED_MASK) != XCR_AVX_ENABLED_MASK)
    {
        flags->has_avx = 0;
        flags->has_avx2 = 0;
        flags->has_fma = 0;
        flags->has_fma4 = 0;
        flags->has_f16c = 0;
        flags->has_xop = 0;
        flags->has_xsave = 0;
        flags->has_xsaveopt = 0;
        flags->has_xsaves = 0;
        flags->has_xsavec = 0;
    }

    /* Check if AVX512F registers are supported.  */
    if ((eax & XCR_AVX512F_ENABLED_MASK) != XCR_AVX512F_ENABLED_MASK)
    {
        flags->has_avx512f = 0;
        flags->has_avx512er = 0;
        flags->has_avx512pf = 0;
        flags->has_avx512cd = 0;
        flags->has_avx512dq = 0;
        flags->has_avx512bw = 0;
        flags->has_avx512vl = 0;
    }
}

class InitCpuFlags
{
public:
    InitCpuFlags(CpuFlags* flags)
    {
        _get_local_cpu_flags(flags);
    }
};

CpuFlags gCpuFlags;
static InitCpuFlags initializer(&gCpuFlags);

const CpuFlags& GetCpuFlags()
{
    return gCpuFlags;
}
