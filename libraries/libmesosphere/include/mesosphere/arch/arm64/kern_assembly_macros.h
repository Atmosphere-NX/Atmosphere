/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <mesosphere/arch/arm64/kern_assembly_offsets.h>

#define ENABLE_FPU(tmp)          \
    mrs     tmp, cpacr_el1;      \
    orr     tmp, tmp, #0x300000; \
    msr     cpacr_el1, tmp;      \
    isb;

#define GET_THREAD_CONTEXT_AND_RESTORE_FPCR_FPSR(ctx, xtmp1, xtmp2, wtmp1, wtmp2)        \
    add     ctx, sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_THREAD_CONTEXT); \
    ldp     wtmp1, wtmp2, [ctx, #(THREAD_CONTEXT_FPCR_FPSR)];                            \
    msr     fpcr, xtmp1;                                                                 \
    msr     fpsr, xtmp2;

#define RESTORE_FPU64_CALLEE_SAVE_REGISTERS(ctx)              \
    ldp     q8,  q9,  [ctx, #(THREAD_CONTEXT_FPU64_Q8_Q9)];   \
    ldp     q10, q11, [ctx, #(THREAD_CONTEXT_FPU64_Q10_Q11)]; \
    ldp     q12, q13, [ctx, #(THREAD_CONTEXT_FPU64_Q12_Q13)]; \
    ldp     q14, q15, [ctx, #(THREAD_CONTEXT_FPU64_Q14_Q15)];

#define RESTORE_FPU64_CALLER_SAVE_REGISTERS(tmp)                                                      \
    ldr     tmp, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CALLER_SAVE_FPU_REGISTERS)]; \
    ldp     q0,  q1,  [tmp, #(THREAD_FPU64_CONTEXT_Q0_Q1)];                                           \
    ldp     q2,  q3,  [tmp, #(THREAD_FPU64_CONTEXT_Q2_Q3)];                                           \
    ldp     q4,  q5,  [tmp, #(THREAD_FPU64_CONTEXT_Q4_Q5)];                                           \
    ldp     q6,  q7,  [tmp, #(THREAD_FPU64_CONTEXT_Q6_Q7)];                                           \
    ldp     q16, q17, [tmp, #(THREAD_FPU64_CONTEXT_Q16_Q17)];                                         \
    ldp     q18, q19, [tmp, #(THREAD_FPU64_CONTEXT_Q18_Q19)];                                         \
    ldp     q20, q21, [tmp, #(THREAD_FPU64_CONTEXT_Q20_Q21)];                                         \
    ldp     q22, q23, [tmp, #(THREAD_FPU64_CONTEXT_Q22_Q23)];                                         \
    ldp     q24, q25, [tmp, #(THREAD_FPU64_CONTEXT_Q24_Q25)];                                         \
    ldp     q26, q27, [tmp, #(THREAD_FPU64_CONTEXT_Q26_Q27)];                                         \
    ldp     q28, q29, [tmp, #(THREAD_FPU64_CONTEXT_Q28_Q29)];                                         \
    ldp     q30, q31, [tmp, #(THREAD_FPU64_CONTEXT_Q30_Q31)];

#define RESTORE_FPU64_ALL_REGISTERS(ctx, tmp) \
    RESTORE_FPU64_CALLEE_SAVE_REGISTERS(ctx) \
    RESTORE_FPU64_CALLER_SAVE_REGISTERS(tmp)

#define RESTORE_FPU32_CALLEE_SAVE_REGISTERS(ctx)            \
    ldp     q4,  q5,  [ctx, #(THREAD_CONTEXT_FPU32_Q4_Q5)]; \
    ldp     q6,  q7,  [ctx, #(THREAD_CONTEXT_FPU32_Q6_Q7)];

#define RESTORE_FPU32_CALLER_SAVE_REGISTERS(tmp)                                                      \
    ldr     tmp, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CALLER_SAVE_FPU_REGISTERS)]; \
    ldp     q0,  q1,  [tmp, #(THREAD_FPU32_CONTEXT_Q0_Q1)];                                           \
    ldp     q2,  q3,  [tmp, #(THREAD_FPU32_CONTEXT_Q2_Q3)];                                           \
    ldp     q8,  q9,  [tmp, #(THREAD_FPU32_CONTEXT_Q8_Q9)];                                           \
    ldp     q10, q11, [tmp, #(THREAD_FPU32_CONTEXT_Q10_Q11)];                                         \
    ldp     q12, q13, [tmp, #(THREAD_FPU32_CONTEXT_Q12_Q13)];                                         \
    ldp     q14, q15, [tmp, #(THREAD_FPU32_CONTEXT_Q14_Q15)];

#define RESTORE_FPU32_ALL_REGISTERS(ctx, tmp) \
    RESTORE_FPU32_CALLEE_SAVE_REGISTERS(ctx)  \
    RESTORE_FPU32_CALLER_SAVE_REGISTERS(tmp)

#define ENABLE_AND_RESTORE_FPU(ctx, xtmp1, xtmp2, wtmp1, wtmp2, label_32, label_done)         \
    ENABLE_FPU(xtmp1)                                                                         \
    GET_THREAD_CONTEXT_AND_RESTORE_FPCR_FPSR(ctx, xtmp1, xtmp2, wtmp1, wtmp2)                 \
                                                                                              \
    ldrb    wtmp1, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_EXCEPTION_FLAGS)]; \
    tbz     wtmp1, #(THREAD_EXCEPTION_FLAG_BIT_INDEX_IS_FPU_64_BIT), label_32##f;             \
                                                                                              \
    RESTORE_FPU64_ALL_REGISTERS(ctx, xtmp1)                                                   \
                                                                                              \
    b       label_done##f;                                                                    \
                                                                                              \
label_32:                                                                                     \
    RESTORE_FPU32_ALL_REGISTERS(ctx, xtmp1)                                                   \
label_done:

#define ENABLE_AND_RESTORE_FPU64(ctx, xtmp1, xtmp2, wtmp1, wtmp2)             \
    ENABLE_FPU(xtmp1)                                                           \
    GET_THREAD_CONTEXT_AND_RESTORE_FPCR_FPSR(ctx, xtmp1, xtmp2, wtmp1, wtmp2) \
    RESTORE_FPU64_ALL_REGISTERS(ctx, xtmp1)

#define ENABLE_AND_RESTORE_FPU32(ctx, xtmp1, xtmp2, wtmp1, wtmp2)             \
    ENABLE_FPU(xtmp1)                                                           \
    GET_THREAD_CONTEXT_AND_RESTORE_FPCR_FPSR(ctx, xtmp1, xtmp2, wtmp1, wtmp2) \
    RESTORE_FPU32_ALL_REGISTERS(ctx, xtmp1)

#define ERET_WITH_SPECULATION_BARRIER \
    eret;                             \
    dsb nsh;                          \
    isb
