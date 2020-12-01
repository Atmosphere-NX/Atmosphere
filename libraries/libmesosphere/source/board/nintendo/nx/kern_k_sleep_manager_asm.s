/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

/* ams::kern::board::nintendo::nx::KSleepManager::CpuSleepHandler(uintptr_t arg, uintptr_t entry) */
.section    .sleep._ZN3ams4kern5board8nintendo2nx13KSleepManager15CpuSleepHandlerEmm, "ax", %progbits
.global     _ZN3ams4kern5board8nintendo2nx13KSleepManager15CpuSleepHandlerEmm
.type       _ZN3ams4kern5board8nintendo2nx13KSleepManager15CpuSleepHandlerEmm, %function
_ZN3ams4kern5board8nintendo2nx13KSleepManager15CpuSleepHandlerEmm:
    /* Save arguments. */
    mov     x16, x0
    mov     x17, x1

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

    /* Save cpuactlr/cpuectlr. */
    mrs     x1, cpuectlr_el1
    mrs     x2, cpuactlr_el1
    stp     x1, x2, [x0], #0x10

    /* Save ttbr0/ttbr1. */
    mrs     x1, ttbr0_el1
    mrs     x2, ttbr1_el1
    stp     x1, x2, [x0], #0x10

    /* Save tcr/mair. */
    mrs     x1, tcr_el1
    mrs     x2, mair_el1
    stp     x1, x2, [x0], #0x10

    /* Save sctlr/tpidr. */
    mrs     x1, sctlr_el1
    mrs     x2, tpidr_el1
    stp     x1, x2, [x0], #0x10

    /* Save the virtual resumption entrypoint. */
    adr     x1, 77f
    stp     x1, xzr, [x0], #0x10

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
    mov     x2, x17
    mov     x3, x16
    smc     #1
0:  b       0b

/* ams::kern::board::nintendo::nx::KSleepManager::ResumeEntry(uintptr_t arg) */
.section    .sleep._ZN3ams4kern5board8nintendo2nx13KSleepManager11ResumeEntryEm, "ax", %progbits
.global     _ZN3ams4kern5board8nintendo2nx13KSleepManager11ResumeEntryEm
.type       _ZN3ams4kern5board8nintendo2nx13KSleepManager11ResumeEntryEm, %function
_ZN3ams4kern5board8nintendo2nx13KSleepManager11ResumeEntryEm:
    /* Mask interrupts. */
    msr     daifset, #0xF

    /* Save the argument. */
    mov     x21, x0

    /* Check that we're at the correct exception level. */
    mrs     x0, currentel

    /* Check if we're EL1. */
    cmp     x0, #0x4
    b.eq    3f

    /* Check if we're EL2. */
    cmp     x0, #0x8
    b.eq    2f

1:  /* We're running at EL3. */
    b       1b

2:  /* We're running at EL2. */
    b       2b

3:  /* We're running at EL1. */

    /* Invalidate the L1 cache. */
    mov     x0, #0
    bl      _ZN3ams4kern5board8nintendo2nx13KSleepManager33InvalidateDataCacheForResumeEntryEm

    /* Get the current core id. */
    mrs     x0, mpidr_el1
    and     x0, x0, #0xFF

    /* If we're on core0, we want to invalidate the L2 cache. */
    cbnz    x0, 4f

    mov     x0, #1
    bl      _ZN3ams4kern5board8nintendo2nx13KSleepManager33InvalidateDataCacheForResumeEntryEm

4:  /* Invalidate the L1 cache. */
    mov     x0, #0
    bl      _ZN3ams4kern5board8nintendo2nx13KSleepManager33InvalidateDataCacheForResumeEntryEm

    /* Invalidate the instruction cache. */
    ic      ialluis
    dsb     sy
    isb

    /* Invalidate the entire tlb. */
    tlbi    vmalle1is
    dsb     sy
    isb

    /* Switch to sp 1. */
    msr     spsel, #1

    /* Prepare to restore the saved context. */
    mov     x0, x21

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

    /* Restore cpuactlr/cpuectlr. */
    ldp     x1, x2, [x0], #0x10
    mrs     x3, cpuectlr_el1
    cmp     x1, x3
5:  b.ne    5b
    mrs     x3, cpuactlr_el1
    cmp     x2, x3
6:  b.ne    6b

    /* Restore ttbr0/ttbr1. */
    ldp     x1, x2, [x0], #0x10
    msr     ttbr0_el1, x1
    msr     ttbr1_el1, x2

    /* Restore tcr/mair. */
    ldp     x1, x2, [x0], #0x10
    msr     tcr_el1, x1
    msr     mair_el1, x2

    /* Get sctlr, tpidr, and the entrypoint. */
    ldp     x1, x2,  [x0], #0x10
    ldp     x3, xzr, [x0], #0x10

    /* Set the global context back into x18/tpidr. */
    msr     tpidr_el1, x2
    dsb     sy
    isb

    /* Restore sctlr with the wxn bit cleared. */
    bic     x2, x1, #0x80000
    msr     sctlr_el1, x2
    dsb     sy
    isb

    /* Jump to the entrypoint. */
    br      x3

77: /* Virtual resumption entrypoint. */

    /* Restore sctlr. */
    msr     sctlr_el1, x1
    dsb     sy
    isb

    ret

/* ams::kern::board::nintendo::nx::KSleepManager::InvalidateDataCacheForResumeEntry(uintptr_t level) */
.section    .sleep._ZN3ams4kern5board8nintendo2nx13KSleepManager33InvalidateDataCacheForResumeEntryEm, "ax", %progbits
.global     _ZN3ams4kern5board8nintendo2nx13KSleepManager33InvalidateDataCacheForResumeEntryEm
.type       _ZN3ams4kern5board8nintendo2nx13KSleepManager33InvalidateDataCacheForResumeEntryEm, %function
_ZN3ams4kern5board8nintendo2nx13KSleepManager33InvalidateDataCacheForResumeEntryEm:
    /* const u64 level_sel_value = level << 1; */
    lsl     x8, x0, #1

    /* cpu::SetCsselrEl1(level_sel_value); */
    msr     csselr_el1, x8

    /* cpu::InstructionMemoryBarrier(); */
    isb

    /* CacheSizeIdAccessor ccsidr_el1; */
    mrs     x13, ccsidr_el1

    /* const int num_ways  = ccsidr_el1.GetAssociativity(); */
    ubfx    w10, w13, #3, #0xA

    /* const int line_size = ccsidr_el1.GetLineSize(); */
    and     w11, w13, #7

    /* const int num_sets  = ccsidr_el1.GetNumberOfSets(); */
    ubfx    w13, w13, #0xD, #0xF

    /* int way = 0; */
    mov     w9, wzr

    /* const u64 set_shift = static_cast<u64>(line_size + 4); */
    add     w11, w11, #4

    /* const u64 way_shift = static_cast<u64>(__builtin_clz(num_ways)); */
    clz     w12, w10


0:  /* do { */
    /*     int set = 0; */
    mov     w14, wzr

    /*      const u64 way_value = (static_cast<u64>(way) << way_shift); */
    lsl     w15, w9, w12

1:  /*     do { */

    /*         const u64 isw_value = (static_cast<u64>(set) << set_shift) | way_value | level_sel_value; */
    lsl     w16, w14, w11
    orr     w16, w16, w15
    sxtw    x16, w16
    orr     x16, x16, x8

    /*         __asm__ __volatile__("dc isw, %0" :: "r"(isw_value) : "memory"); */
    dc      isw, x16

    /*     while (set <= num_sets); */
    cmp     w13, w14
    add     w14, w14, #1
    b.ne    1b

    /* while (way <= num_ways); */
    cmp     w9, w10
    add     w9, w9, #1
    b.ne    0b

    /* cpu::EnsureInstructionConsistency(); */
    dsb     sy
    isb
    ret
