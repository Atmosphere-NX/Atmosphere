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
 
/* For some reason GAS doesn't know about it, even with .cpu cortex-a57 */
#define cpuactlr_el1 s3_1_c15_c2_0
#define cpuectlr_el1 s3_1_c15_c2_1

.section    .crt0, "ax", %progbits
.align      3
.global     _start
.type       _start, %function

_start:
    b       start
    b       start2

_initialKernelEntrypoint:
    .quad   0

start:
    mov     x19, #1
    b       _startCommon
start2:
    mov     x19, #0
_startCommon:
    // Disable interrupts, select sp_el2
    msr     daifset, 0b1111
    msr     spsel, #1

    mrs     x20, sctlr_el2
    // Get core ID
    mrs     x20, mpidr_el1
    and     x20, x20, #0xFF

    // Set tmp stack
    ldr     x8, =__stacks_top__

    /* lsl     x9, x20, #10
    sub     x8, x8, x9*/
    mov     sp, x8

    // Set up x18
    adrp    x18, g_coreCtxs
    add     x18, x18, #:lo12:g_coreCtxs
    add     x18, x18, x20, lsl #3
    stp     x18, xzr, [sp, #-0x10]!

    // Store entrypoint if first core
    cbz     x19, _store_arg
    ldr     x8, _initialKernelEntrypoint
    str     x8, [x18, #8]

_store_arg:
    str     x0, [x18, #0]

    // Set VBAR
    ldr     x8, =__vectors_start__
    msr     vbar_el2, x8

    // Make sure the regs have been set
    dsb     sy
    isb

    // Don't call init array to save space?
    // Clear BSS & call main for the first core executing this code
    cbz     x20, _jump_to_kernel
    ldr     x0, =__bss_start__
    mov     w1, #0
    ldr     x2, =__end__
    sub     x2, x2, x0
    bl      memset

    bl      main

_jump_to_kernel:
    // Jump to kernel
    mov     x8, #(0b1111 << 6 | 0b0101) // EL1h+DAIF
    msr     spsr_el2, x8

    ldp     x0, x1, [x18]
    msr     elr_el2, x1
    dsb     sy
    isb
    eret

.pool
