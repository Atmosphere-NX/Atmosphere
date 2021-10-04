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
#include <mesosphere/kern_select_assembly_offsets.h>

/* ams::kern::arch::arm64::UserModeThreadStarter() */
.section    .text._ZN3ams4kern4arch5arm6421UserModeThreadStarterEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6421UserModeThreadStarterEv
.type       _ZN3ams4kern4arch5arm6421UserModeThreadStarterEv, %function
_ZN3ams4kern4arch5arm6421UserModeThreadStarterEv:
    /* NOTE: Stack layout on entry looks like following:                         */
    /* SP                                                                        */
    /* |                                                                         */
    /* v                                                                         */
    /* | KExceptionContext (size 0x120) | KThread::StackParameters (size 0x30) | */

    /* Clear the disable count for this thread's stack parameters. */
    strh wzr, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_DISABLE_COUNT)]

    /* Call ams::kern::arch::arm64::OnThreadStart() */
    bl _ZN3ams4kern4arch5arm6413OnThreadStartEv

    /* Restore thread state from the KExceptionContext on stack  */
    ldp x30, x19, [sp, #(EXCEPTION_CONTEXT_X30_SP)] /* x30 = lr,   x19 = sp  */
    ldp x20, x21, [sp, #(EXCEPTION_CONTEXT_PC_PSR)] /* x20 = pc,   x21 = psr */
    ldr x22,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]  /* x22 = tpidr           */

    msr sp_el0,    x19
    msr elr_el1,   x20
    msr spsr_el1,  x21
    msr tpidr_el0, x22

    ldp x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    ldp x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    ldp x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    ldp x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    ldp x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    ldp x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    ldp x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    ldp x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    ldp x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    ldp x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    ldp x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    ldp x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    ldp x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    ldp x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    ldp x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    /* Increment stack pointer above the KExceptionContext */
    add sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    /* Return to EL0 */
    eret

/* ams::kern::arch::arm64::SupervisorModeThreadStarter() */
.section    .text._ZN3ams4kern4arch5arm6427SupervisorModeThreadStarterEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6427SupervisorModeThreadStarterEv
.type       _ZN3ams4kern4arch5arm6427SupervisorModeThreadStarterEv, %function
_ZN3ams4kern4arch5arm6427SupervisorModeThreadStarterEv:
    /* NOTE: Stack layout on entry looks like following:                        */
    /* SP                                                                       */
    /* |                                                                        */
    /* v                                                                        */
    /* | u64 argument | u64 entrypoint | KThread::StackParameters (size 0x30) | */

    /* Load the argument and entrypoint. */
    ldp x0, x1, [sp], #0x10

    /* Clear the disable count for this thread's stack parameters. */
    strh wzr, [sp, #(THREAD_STACK_PARAMETERS_DISABLE_COUNT)]

    /*  Mask I bit in DAIF */
    msr daifclr, #2
    br x1

    /* This should never execute, but Nintendo includes an ERET here. */
    eret


/* ams::kern::arch::arm64::KThreadContext::RestoreFpuRegisters64(const KThreadContext &) */
.section    .text._ZN3ams4kern4arch5arm6414KThreadContext21RestoreFpuRegisters64ERKS3_, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6414KThreadContext21RestoreFpuRegisters64ERKS3_
.type       _ZN3ams4kern4arch5arm6414KThreadContext21RestoreFpuRegisters64ERKS3_, %function
_ZN3ams4kern4arch5arm6414KThreadContext21RestoreFpuRegisters64ERKS3_:
    /* Load and restore FPCR and FPSR from the context. */
    ldr x1, [x0, #(THREAD_CONTEXT_FPCR)]
    msr fpcr, x1
    ldr x1, [x0, #(THREAD_CONTEXT_FPSR)]
    msr fpsr, x1

    /* Restore the FPU registers. */
    ldp q0,  q1,  [x0, #(16 *  0 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q2,  q3,  [x0, #(16 *  2 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q4,  q5,  [x0, #(16 *  4 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q6,  q7,  [x0, #(16 *  6 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q8,  q9,  [x0, #(16 *  8 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q10, q11, [x0, #(16 * 10 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q12, q13, [x0, #(16 * 12 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q14, q15, [x0, #(16 * 14 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q16, q17, [x0, #(16 * 16 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q18, q19, [x0, #(16 * 18 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q20, q21, [x0, #(16 * 20 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q22, q23, [x0, #(16 * 22 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q24, q25, [x0, #(16 * 24 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q26, q27, [x0, #(16 * 26 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q28, q29, [x0, #(16 * 28 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q30, q31, [x0, #(16 * 30 + THREAD_CONTEXT_FPU_REGISTERS)]

    ret

/* ams::kern::arch::arm64::KThreadContext::RestoreFpuRegisters32(const KThreadContext &) */
.section    .text._ZN3ams4kern4arch5arm6414KThreadContext21RestoreFpuRegisters32ERKS3_, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6414KThreadContext21RestoreFpuRegisters32ERKS3_
.type       _ZN3ams4kern4arch5arm6414KThreadContext21RestoreFpuRegisters32ERKS3_, %function
_ZN3ams4kern4arch5arm6414KThreadContext21RestoreFpuRegisters32ERKS3_:
    /* Load and restore FPCR and FPSR from the context. */
    ldr x1, [x0, #(THREAD_CONTEXT_FPCR)]
    msr fpcr, x1
    ldr x1, [x0, #(THREAD_CONTEXT_FPSR)]
    msr fpsr, x1

    /* Restore the FPU registers. */
    ldp q0,  q1,  [x0, #(16 *  0 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q2,  q3,  [x0, #(16 *  2 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q4,  q5,  [x0, #(16 *  4 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q6,  q7,  [x0, #(16 *  6 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q8,  q9,  [x0, #(16 *  8 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q10, q11, [x0, #(16 * 10 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q12, q13, [x0, #(16 * 12 + THREAD_CONTEXT_FPU_REGISTERS)]
    ldp q14, q15, [x0, #(16 * 14 + THREAD_CONTEXT_FPU_REGISTERS)]

    ret
