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

#include "asm_macros.s"

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

    mrs     x20, cntpct_el0

    // Set sctlr_el2 ASAP to disable mmu/caching if not already done.
    mov     x1, #0x0838
    movk    x1, #0x30C5,lsl #16
    msr     sctlr_el2, x1
    dsb     sy
    isb

    bl      cacheClearLocalDataCacheOnBoot
    cbz     x19, 1f
    bl      cacheClearSharedDataCachesOnBoot

1:
    // Get core ID
    // Ensure Aff0 is 4-1 at most (4 cores), and that Aff1, 2 and 3 are 0 (1 cluster only)
    mrs     x0, mpidr_el1
    tst     x0, #(0xFF << 32)
    bne     .
    and     x0, x0, #0x00FFFFFF        // Aff0 to 2
    cmp     x0, #4
    bhs     .

    // Set stack pointer
    adrp    x8, __stacks_top__
    lsl     x9, x0, #10
    sub     sp, x8, x9

    // Set up x18, other sysregs, BSS, MMU, etc.
    // Don't call init array to save space?
    mov     w1, w19
    bl      initSystem

    // Save x18, reserve space for exception frame
    stp     x18, xzr, [sp, #-0x10]!
    sub     sp, sp, #EXCEP_STACK_FRAME_SIZE

    dsb     sy
    isb

    mov     x0, sp
    mov     x1, x20
    bl      thermosphereMain

    dsb     sy
    isb

    // Jump to kernel
    b       _restoreAllRegisters

.pool
