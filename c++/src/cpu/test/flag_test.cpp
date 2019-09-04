#include <gtest/gtest.h>

#include <stdio.h>
#include <string.h>

#include "src/cpu/flag.h"

TEST(CpuFlags, GetFlags)
{
    const CpuFlags& cpuFlags = GetCpuFlags();
    printf("has_sse3: %u\n", cpuFlags.has_sse3);
    printf("has_ssse3: %u\n", cpuFlags.has_ssse3);
    printf("has_cmpxchg16b: %u\n", cpuFlags.has_cmpxchg16b);
    printf("has_cmpxchg8b: %u\n", cpuFlags.has_cmpxchg8b);
    printf("has_cmov: %u\n", cpuFlags.has_cmov);
    printf("has_mmx: %u\n", cpuFlags.has_mmx);
    printf("has_sse: %u\n", cpuFlags.has_sse);
    printf("has_sse2: %u\n", cpuFlags.has_sse2);
    printf("has_lahf_lm: %u\n", cpuFlags.has_lahf_lm);
    printf("has_sse4a: %u\n", cpuFlags.has_sse4a);
    printf("has_longmode: %u\n", cpuFlags.has_longmode);
    printf("has_3dnowp: %u\n", cpuFlags.has_3dnowp);
    printf("has_3dnow: %u\n", cpuFlags.has_3dnow);
    printf("has_movbe: %u\n", cpuFlags.has_movbe);
    printf("has_sse4_1: %u\n", cpuFlags.has_sse4_1);
    printf("has_sse4_2: %u\n", cpuFlags.has_sse4_2);
    printf("has_popcnt: %u\n", cpuFlags.has_popcnt);
    printf("has_aes: %u\n", cpuFlags.has_aes);
    printf("has_avx: %u\n", cpuFlags.has_avx);
    printf("has_avx2: %u\n", cpuFlags.has_avx2);
    printf("has_pclmul: %u\n", cpuFlags.has_pclmul);
    printf("has_abm: %u\n", cpuFlags.has_abm);
    printf("has_lwp: %u\n", cpuFlags.has_lwp);
    printf("has_fma: %u\n", cpuFlags.has_fma);
    printf("has_fma4: %u\n", cpuFlags.has_fma4);
    printf("has_xop: %u\n", cpuFlags.has_xop);
    printf("has_bmi: %u\n", cpuFlags.has_bmi);
    printf("has_bmi2: %u\n", cpuFlags.has_bmi2);
    printf("has_tbm: %u\n", cpuFlags.has_tbm);
    printf("has_lzcnt: %u\n", cpuFlags.has_lzcnt);
    printf("has_hle: %u\n", cpuFlags.has_hle);
    printf("has_rtm: %u\n", cpuFlags.has_rtm);
    printf("has_sgx: %u\n", cpuFlags.has_sgx);
    printf("has_pconfig: %u\n", cpuFlags.has_pconfig);
    printf("has_wbnoinvd: %u\n", cpuFlags.has_wbnoinvd);
    printf("has_rdrnd: %u\n", cpuFlags.has_rdrnd);
    printf("has_f16c: %u\n", cpuFlags.has_f16c);
    printf("has_fsgsbase: %u\n", cpuFlags.has_fsgsbase);
    printf("has_rdseed: %u\n", cpuFlags.has_rdseed);
    printf("has_prfchw: %u\n", cpuFlags.has_prfchw);
    printf("has_adx: %u\n", cpuFlags.has_adx);
    printf("has_osxsave: %u\n", cpuFlags.has_osxsave);
    printf("has_fxsr: %u\n", cpuFlags.has_fxsr);
    printf("has_xsave: %u\n", cpuFlags.has_xsave);
    printf("has_xsaveopt: %u\n", cpuFlags.has_xsaveopt);
    printf("has_avx512er: %u\n", cpuFlags.has_avx512er);
    printf("has_avx512pf: %u\n", cpuFlags.has_avx512pf);
    printf("has_avx512cd: %u\n", cpuFlags.has_avx512cd);
    printf("has_avx512f: %u\n", cpuFlags.has_avx512f);
    printf("has_sha: %u\n", cpuFlags.has_sha);
    printf("has_prefetchwt1: %u\n", cpuFlags.has_prefetchwt1);
    printf("has_clflushopt: %u\n", cpuFlags.has_clflushopt);
    printf("has_xsavec: %u\n", cpuFlags.has_xsavec);
    printf("has_xsaves: %u\n", cpuFlags.has_xsaves);
    printf("has_avx512dq: %u\n", cpuFlags.has_avx512dq);
    printf("has_avx512bw: %u\n", cpuFlags.has_avx512bw);
    printf("has_avx512vl: %u\n", cpuFlags.has_avx512vl);
    printf("has_avx512vbmi: %u\n", cpuFlags.has_avx512vbmi);
    printf("has_avx512ifma: %u\n", cpuFlags.has_avx512ifma);
    printf("has_clwb: %u\n", cpuFlags.has_clwb);
    printf("has_mwaitx: %u\n", cpuFlags.has_mwaitx);
    printf("has_clzero: %u\n", cpuFlags.has_clzero);
    printf("has_pku: %u\n", cpuFlags.has_pku);
    printf("has_rdpid: %u\n", cpuFlags.has_rdpid);
    printf("has_avx5124fmaps: %u\n", cpuFlags.has_avx5124fmaps);
    printf("has_avx5124vnniw: %u\n", cpuFlags.has_avx5124vnniw);
    printf("has_gfni: %u\n", cpuFlags.has_gfni);
    printf("has_avx512vbmi2: %u\n", cpuFlags.has_avx512vbmi2);
    printf("has_avx512bitalg: %u\n", cpuFlags.has_avx512bitalg);
    printf("has_shstk: %u\n", cpuFlags.has_shstk);
    printf("has_avx512vnni: %u\n", cpuFlags.has_avx512vnni);
    printf("has_vaes: %u\n", cpuFlags.has_vaes);
    printf("has_vpclmulqdq: %u\n", cpuFlags.has_vpclmulqdq);
    printf("has_movdiri: %u\n", cpuFlags.has_movdiri);
    printf("has_movdir64b: %u\n", cpuFlags.has_movdir64b);
    printf("has_waitpkg: %u\n", cpuFlags.has_waitpkg);
    printf("has_cldemote: %u\n", cpuFlags.has_cldemote);
    printf("has_ptwrite: %u\n", cpuFlags.has_ptwrite);
}
