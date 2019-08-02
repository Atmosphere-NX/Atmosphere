/*
 * Copyright (c) 2019 Atmosphère-NX
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

.section    .crt0, "ax", %progbits
.align      3
.global     _start
.type       _start, %function

_start:
    b       start
    b       start2

.global g_initialKernelEntrypoint
g_initialKernelEntrypoint:
    .quad   0

start:
    mov     x19, #1
    b       _startCommon
.global start2
.type   start2, %function
start2:
    mov     x19, xzr
_startCommon:
    // Disable interrupts, select sp_el2
    msr     daifset, 0b1111
    msr     spsel, #1

    // Set VBAR
    adrp    x8, __vectors_start__
    add     x8, x8, #:lo12:__vectors_start__
    msr     vbar_el2, x8

    // Set system to sane defaults, aarch64 for el1
    mov     x4, #0x0838
    movk    x4, #0xC5, lsl #16
    orr     x1, x4, #0x30000000
    mov     x2, #(1 << 31)
    mov     x3, #0xFFFFFFFF

    msr     sctlr_el2, x1
    msr     hcr_el2, x2
    msr     dacr32_el2, x3
    msr     sctlr_el1, x4

    dsb     sy
    isb

    mov     x2, x0

    // Get core ID
    // Ensure Aff0 is 4-1 at most (4 cores), and that Aff1, 2 and 3 are 0 (1 cluster only)
    mrs     x0, mpidr_el1
    tst     x0, #(0xFF << 32)
    bne     .
    and     x0, x0, #0x00FFFFFF        // Aff0 to 2
    cmp     x0, #4
    bhs     .

    // Set tmp stack (__stacks_top__ is aligned)
    adrp    x8, __stacks_top__
    lsl     x9, x0, #10
    sub     sp, x8, x9

    // Set up x18
    mov     w1, w19
    bl      coreCtxInit
    stp     x18, xzr, [sp, #-0x10]!

    // Don't call init array to save space?
    // Clear BSS & call main for the first core executing this code
    cbz     x19, _enable_mmu
    adrp    x0, __bss_start__
    add     x0, x0, #:lo12:__bss_start__
    mov     w1, wzr
    adrp    x2, __end__
    add     x2, x2, #:lo12:__end__
    sub     x2, x2, x0
    bl      memset

_enable_mmu:

    // Enable EL2 address translation and caches
    bl      configureMemoryMapEnableMmu
    // Enable EL1 Stage2 intermediate physical address translation
    bl      configureMemoryMapEnableStage2

    dsb     sy
    isb

    bl      main

    // Jump to kernel
    mov     x8, #(0b1111 << 6 | 0b0101) // EL1h+DAIF
    msr     spsr_el2, x8

    ldp     x0, x1, [x18]
    msr     elr_el2, x1
    dsb     sy
    isb
    eret

.pool
