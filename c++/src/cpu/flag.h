#ifndef _SRC_CPU_CPU_FLAGS_H
#define _SRC_CPU_CPU_FLAGS_H

#include <stdint.h>

struct CpuFlags
{
    uint8_t has_sse3 : 1;
    uint8_t has_ssse3 : 1;
    uint8_t has_cmpxchg16b : 1;
    uint8_t has_cmpxchg8b : 1;
    uint8_t has_cmov : 1;
    uint8_t has_mmx : 1;
    uint8_t has_sse : 1;
    uint8_t has_sse2 : 1;
    uint8_t has_lahf_lm : 1;
    uint8_t has_sse4a : 1;
    uint8_t has_longmode : 1;
    uint8_t has_3dnowp : 1;
    uint8_t has_3dnow : 1;
    uint8_t has_movbe : 1;
    uint8_t has_sse4_1 : 1;
    uint8_t has_sse4_2 : 1;
    uint8_t has_popcnt : 1;
    uint8_t has_aes : 1;
    uint8_t has_avx : 1;
    uint8_t has_avx2 : 1;
    uint8_t has_pclmul : 1;
    uint8_t has_abm : 1;
    uint8_t has_lwp : 1;
    uint8_t has_fma : 1;
    uint8_t has_fma4 : 1;
    uint8_t has_xop : 1;
    uint8_t has_bmi : 1;
    uint8_t has_bmi2 : 1;
    uint8_t has_tbm : 1;
    uint8_t has_lzcnt : 1;
    uint8_t has_hle : 1;
    uint8_t has_rtm : 1;
    uint8_t has_sgx : 1;
    uint8_t has_pconfig : 1;
    uint8_t has_wbnoinvd : 1;
    uint8_t has_rdrnd : 1;
    uint8_t has_f16c : 1;
    uint8_t has_fsgsbase : 1;
    uint8_t has_rdseed : 1;
    uint8_t has_prfchw : 1;
    uint8_t has_adx : 1;
    uint8_t has_osxsave : 1;
    uint8_t has_fxsr : 1;
    uint8_t has_xsave : 1;
    uint8_t has_xsaveopt : 1;
    uint8_t has_avx512er : 1;
    uint8_t has_avx512pf : 1;
    uint8_t has_avx512cd : 1;
    uint8_t has_avx512f : 1;
    uint8_t has_sha : 1;
    uint8_t has_prefetchwt1 : 1;
    uint8_t has_clflushopt : 1;
    uint8_t has_xsavec : 1;
    uint8_t has_xsaves : 1;
    uint8_t has_avx512dq : 1;
    uint8_t has_avx512bw : 1;
    uint8_t has_avx512vl : 1;
    uint8_t has_avx512vbmi : 1;
    uint8_t has_avx512ifma : 1;
    uint8_t has_clwb : 1;
    uint8_t has_mwaitx : 1;
    uint8_t has_clzero : 1;
    uint8_t has_pku : 1;
    uint8_t has_rdpid : 1;
    uint8_t has_avx5124fmaps : 1;
    uint8_t has_avx5124vnniw : 1;
    uint8_t has_gfni : 1;
    uint8_t has_avx512vbmi2 : 1;
    uint8_t has_avx512bitalg : 1;
    uint8_t has_shstk : 1;
    uint8_t has_avx512vnni : 1;
    uint8_t has_vaes : 1;
    uint8_t has_vpclmulqdq : 1;
    uint8_t has_movdiri : 1;
    uint8_t has_movdir64b : 1;
    uint8_t has_waitpkg : 1;
    uint8_t has_cldemote : 1;
    uint8_t has_ptwrite : 1;
};

const CpuFlags& GetCpuFlags();

#endif  // _SRC_CPU_CPU_FLAGS_H
