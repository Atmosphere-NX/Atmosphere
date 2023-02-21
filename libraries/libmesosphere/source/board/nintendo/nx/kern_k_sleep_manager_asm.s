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

/* For some reason GAS doesn't know about it, even with .cpu cortex-a57 */
#define cpuactlr_el1 s3_1_c15_c2_0
#define cpuectlr_el1 s3_1_c15_c2_1

#define LOAD_IMMEDIATE_32(reg, val)                \
    mov  reg, #(((val) >> 0x00) & 0xFFFF);         \
    movk reg, #(((val) >> 0x10) & 0xFFFF), lsl#16

/* ams::kern::board::nintendo::nx::KSleepManager::CpuSleepHandler(uintptr_t arg, uintptr_t entry, uintptr_t entry_arg) */
.section    .sleep._ZN3ams4kern5board8nintendo2nx13KSleepManager15CpuSleepHandlerEmmm, "ax", %progbits
.global     _ZN3ams4kern5board8nintendo2nx13KSleepManager15CpuSleepHandlerEmmm
.type       _ZN3ams4kern5board8nintendo2nx13KSleepManager15CpuSleepHandlerEmmm, %function
_ZN3ams4kern5board8nintendo2nx13KSleepManager15CpuSleepHandlerEmmm:
    /* Save arguments. */
    mov     x16, x1
    mov     x17, x2

    /* Enable access to FPU registers. */
    mrs     x1, cpacr_el1
    orr     x1, x1, #0x100000
    msr     cpacr_el1, x1
    dsb sy
    isb

    /* Save callee-save registers. */
    stp     x18, x19, [x0], #0x10
    stp     x20, x21, [x0], #0x10
    stp     x22, x23, [x0], #0x10
    stp     x24, x25, [x0], #0x10
    stp     x26, x27, [x0], #0x10
    stp     x28, x29, [x0], #0x10
    stp     x30, xzr, [x0], #0x10

    /* Save stack pointer. */
    mov     x1, sp
    str     x1, [x0], #8

    /* Save fpcr/fpsr. */
    mrs     x1, fpcr
    mrs     x2, fpsr
    stp     w1, w2, [x0], #8

    /* Save the floating point registers. */
    stp     q0,  q1,  [x0], #0x20
    stp     q2,  q3,  [x0], #0x20
    stp     q4,  q5,  [x0], #0x20
    stp     q6,  q7,  [x0], #0x20
    stp     q8,  q9,  [x0], #0x20
    stp     q10, q11, [x0], #0x20
    stp     q12, q13, [x0], #0x20
    stp     q14, q15, [x0], #0x20
    stp     q16, q17, [x0], #0x20
    stp     q28, q19, [x0], #0x20
    stp     q20, q21, [x0], #0x20
    stp     q22, q23, [x0], #0x20
    stp     q24, q25, [x0], #0x20
    stp     q26, q27, [x0], #0x20
    stp     q28, q29, [x0], #0x20
    stp     q30, q31, [x0], #0x20

    /* Save tpidr/cntv_cval_el0. */
    mrs     x1, tpidr_el1
    mrs     x2, cntv_cval_el0
    stp     x1, x2, [x0], #0x10

    /* Get the current core id. */
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF

    /* If we're on core 0, suspend. */
    cbz     x0, 1f

    /* Otherwise, power off. */
    LOAD_IMMEDIATE_32(x0, 0x84000002)
    smc     #1
0:  b       0b

1:  /* Suspend. */
    LOAD_IMMEDIATE_32(x0, 0xC4000001)
    LOAD_IMMEDIATE_32(x1, 0x0201001B)
    mov     x2, x16
    mov     x3, x17
    smc     #1
0:  b       0b

/* ams::kern::board::nintendo::nx::KSleepManager::ResumeEntry(uintptr_t arg) */
.section    .sleep._ZN3ams4kern5board8nintendo2nx13KSleepManager11ResumeEntryEm, "ax", %progbits
.global     _ZN3ams4kern5board8nintendo2nx13KSleepManager11ResumeEntryEm
.type       _ZN3ams4kern5board8nintendo2nx13KSleepManager11ResumeEntryEm, %function
_ZN3ams4kern5board8nintendo2nx13KSleepManager11ResumeEntryEm:
    /* Enable access to FPU registers. */
    mrs     x1, cpacr_el1
    orr     x1, x1, #0x100000
    msr     cpacr_el1, x1
    dsb sy
    isb

    /* Restore callee-save registers. */
    ldp     x18, x19, [x0], #0x10
    ldp     x20, x21, [x0], #0x10
    ldp     x22, x23, [x0], #0x10
    ldp     x24, x25, [x0], #0x10
    ldp     x26, x27, [x0], #0x10
    ldp     x28, x29, [x0], #0x10
    ldp     x30, xzr, [x0], #0x10

    /* Restore stack pointer. */
    ldr     x1, [x0], #8
    mov     sp, x1

    /* Restore fpcr/fpsr. */
    ldp     w1, w2, [x0], #8
    msr     fpcr, x1
    msr     fpsr, x2

    /* Restore the floating point registers. */
    ldp     q0,  q1,  [x0], #0x20
    ldp     q2,  q3,  [x0], #0x20
    ldp     q4,  q5,  [x0], #0x20
    ldp     q6,  q7,  [x0], #0x20
    ldp     q8,  q9,  [x0], #0x20
    ldp     q10, q11, [x0], #0x20
    ldp     q12, q13, [x0], #0x20
    ldp     q14, q15, [x0], #0x20
    ldp     q16, q17, [x0], #0x20
    ldp     q28, q19, [x0], #0x20
    ldp     q20, q21, [x0], #0x20
    ldp     q22, q23, [x0], #0x20
    ldp     q24, q25, [x0], #0x20
    ldp     q26, q27, [x0], #0x20
    ldp     q28, q29, [x0], #0x20
    ldp     q30, q31, [x0], #0x20

    /* Restore tpidr/cntv_cval_el0. */
    ldp     x1, x2, [x0], #0x10
    msr     tpidr_el1, x1
    msr     cntv_cval_el0, x2
    dsb     sy
    isb

    /* Return. */
    ret
