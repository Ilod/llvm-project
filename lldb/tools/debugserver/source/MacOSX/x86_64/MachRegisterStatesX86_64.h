//===-- MachRegisterStatesX86_64.h --------------------------------*- C++
//-*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  Created by Sean Callanan on 3/16/11.
//
//===----------------------------------------------------------------------===//

#ifndef __MachRegisterStatesX86_64_h__
#define __MachRegisterStatesX86_64_h__

#include <inttypes.h>

#define __x86_64_THREAD_STATE 4
#define __x86_64_FLOAT_STATE 5
#define __x86_64_EXCEPTION_STATE 6
#define __x86_64_DEBUG_STATE 11
#define __x86_64_AVX_STATE 17

typedef struct {
  uint64_t __rax;
  uint64_t __rbx;
  uint64_t __rcx;
  uint64_t __rdx;
  uint64_t __rdi;
  uint64_t __rsi;
  uint64_t __rbp;
  uint64_t __rsp;
  uint64_t __r8;
  uint64_t __r9;
  uint64_t __r10;
  uint64_t __r11;
  uint64_t __r12;
  uint64_t __r13;
  uint64_t __r14;
  uint64_t __r15;
  uint64_t __rip;
  uint64_t __rflags;
  uint64_t __cs;
  uint64_t __fs;
  uint64_t __gs;
} __x86_64_thread_state_t;

typedef struct {
  uint16_t __invalid : 1;
  uint16_t __denorm : 1;
  uint16_t __zdiv : 1;
  uint16_t __ovrfl : 1;
  uint16_t __undfl : 1;
  uint16_t __precis : 1;
  uint16_t __PAD1 : 2;
  uint16_t __pc : 2;
  uint16_t __rc : 2;
  uint16_t __PAD2 : 1;
  uint16_t __PAD3 : 3;
} __x86_64_fp_control_t;

typedef struct {
  uint16_t __invalid : 1;
  uint16_t __denorm : 1;
  uint16_t __zdiv : 1;
  uint16_t __ovrfl : 1;
  uint16_t __undfl : 1;
  uint16_t __precis : 1;
  uint16_t __stkflt : 1;
  uint16_t __errsumm : 1;
  uint16_t __c0 : 1;
  uint16_t __c1 : 1;
  uint16_t __c2 : 1;
  uint16_t __tos : 3;
  uint16_t __c3 : 1;
  uint16_t __busy : 1;
} __x86_64_fp_status_t;

typedef struct {
  uint8_t __mmst_reg[10];
  uint8_t __mmst_rsrv[6];
} __x86_64_mmst_reg;

typedef struct { uint8_t __xmm_reg[16]; } __x86_64_xmm_reg;

typedef struct {
  int32_t __fpu_reserved[2];
  __x86_64_fp_control_t __fpu_fcw;
  __x86_64_fp_status_t __fpu_fsw;
  uint8_t __fpu_ftw;
  uint8_t __fpu_rsrv1;
  uint16_t __fpu_fop;
  uint32_t __fpu_ip;
  uint16_t __fpu_cs;
  uint16_t __fpu_rsrv2;
  uint32_t __fpu_dp;
  uint16_t __fpu_ds;
  uint16_t __fpu_rsrv3;
  uint32_t __fpu_mxcsr;
  uint32_t __fpu_mxcsrmask;
  __x86_64_mmst_reg __fpu_stmm0;
  __x86_64_mmst_reg __fpu_stmm1;
  __x86_64_mmst_reg __fpu_stmm2;
  __x86_64_mmst_reg __fpu_stmm3;
  __x86_64_mmst_reg __fpu_stmm4;
  __x86_64_mmst_reg __fpu_stmm5;
  __x86_64_mmst_reg __fpu_stmm6;
  __x86_64_mmst_reg __fpu_stmm7;
  __x86_64_xmm_reg __fpu_xmm0;
  __x86_64_xmm_reg __fpu_xmm1;
  __x86_64_xmm_reg __fpu_xmm2;
  __x86_64_xmm_reg __fpu_xmm3;
  __x86_64_xmm_reg __fpu_xmm4;
  __x86_64_xmm_reg __fpu_xmm5;
  __x86_64_xmm_reg __fpu_xmm6;
  __x86_64_xmm_reg __fpu_xmm7;
  __x86_64_xmm_reg __fpu_xmm8;
  __x86_64_xmm_reg __fpu_xmm9;
  __x86_64_xmm_reg __fpu_xmm10;
  __x86_64_xmm_reg __fpu_xmm11;
  __x86_64_xmm_reg __fpu_xmm12;
  __x86_64_xmm_reg __fpu_xmm13;
  __x86_64_xmm_reg __fpu_xmm14;
  __x86_64_xmm_reg __fpu_xmm15;
  uint8_t __fpu_rsrv4[6 * 16];
  int32_t __fpu_reserved1;
} __x86_64_float_state_t;

typedef struct {
  uint32_t __fpu_reserved[2];
  __x86_64_fp_control_t __fpu_fcw;
  __x86_64_fp_status_t __fpu_fsw;
  uint8_t __fpu_ftw;
  uint8_t __fpu_rsrv1;
  uint16_t __fpu_fop;
  uint32_t __fpu_ip;
  uint16_t __fpu_cs;
  uint16_t __fpu_rsrv2;
  uint32_t __fpu_dp;
  uint16_t __fpu_ds;
  uint16_t __fpu_rsrv3;
  uint32_t __fpu_mxcsr;
  uint32_t __fpu_mxcsrmask;
  __x86_64_mmst_reg __fpu_stmm0;
  __x86_64_mmst_reg __fpu_stmm1;
  __x86_64_mmst_reg __fpu_stmm2;
  __x86_64_mmst_reg __fpu_stmm3;
  __x86_64_mmst_reg __fpu_stmm4;
  __x86_64_mmst_reg __fpu_stmm5;
  __x86_64_mmst_reg __fpu_stmm6;
  __x86_64_mmst_reg __fpu_stmm7;
  __x86_64_xmm_reg __fpu_xmm0;
  __x86_64_xmm_reg __fpu_xmm1;
  __x86_64_xmm_reg __fpu_xmm2;
  __x86_64_xmm_reg __fpu_xmm3;
  __x86_64_xmm_reg __fpu_xmm4;
  __x86_64_xmm_reg __fpu_xmm5;
  __x86_64_xmm_reg __fpu_xmm6;
  __x86_64_xmm_reg __fpu_xmm7;
  __x86_64_xmm_reg __fpu_xmm8;
  __x86_64_xmm_reg __fpu_xmm9;
  __x86_64_xmm_reg __fpu_xmm10;
  __x86_64_xmm_reg __fpu_xmm11;
  __x86_64_xmm_reg __fpu_xmm12;
  __x86_64_xmm_reg __fpu_xmm13;
  __x86_64_xmm_reg __fpu_xmm14;
  __x86_64_xmm_reg __fpu_xmm15;
  uint8_t __fpu_rsrv4[6 * 16];
  uint32_t __fpu_reserved1;
  uint8_t __avx_reserved1[64];
  __x86_64_xmm_reg __fpu_ymmh0;
  __x86_64_xmm_reg __fpu_ymmh1;
  __x86_64_xmm_reg __fpu_ymmh2;
  __x86_64_xmm_reg __fpu_ymmh3;
  __x86_64_xmm_reg __fpu_ymmh4;
  __x86_64_xmm_reg __fpu_ymmh5;
  __x86_64_xmm_reg __fpu_ymmh6;
  __x86_64_xmm_reg __fpu_ymmh7;
  __x86_64_xmm_reg __fpu_ymmh8;
  __x86_64_xmm_reg __fpu_ymmh9;
  __x86_64_xmm_reg __fpu_ymmh10;
  __x86_64_xmm_reg __fpu_ymmh11;
  __x86_64_xmm_reg __fpu_ymmh12;
  __x86_64_xmm_reg __fpu_ymmh13;
  __x86_64_xmm_reg __fpu_ymmh14;
  __x86_64_xmm_reg __fpu_ymmh15;
} __x86_64_avx_state_t;

typedef struct {
  uint32_t __trapno;
  uint32_t __err;
  uint64_t __faultvaddr;
} __x86_64_exception_state_t;

typedef struct {
  uint64_t __dr0;
  uint64_t __dr1;
  uint64_t __dr2;
  uint64_t __dr3;
  uint64_t __dr4;
  uint64_t __dr5;
  uint64_t __dr6;
  uint64_t __dr7;
} __x86_64_debug_state_t;

#endif
