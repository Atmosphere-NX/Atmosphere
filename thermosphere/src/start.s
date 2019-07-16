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

.section    .text.start, "ax", %progbits
.align      3
.global     _start
.type       _start, %function

_start:
    // Disable interrupts
    msr     daifset, 0b1111

    // Set VBAR
    ldr     x8, =__vectors_start__
    msr     vbar_el2, x8

    // Set tmp stack
    ldr     x8, =__stacks_top__
    mov     sp, x8

    // Don't call init array to save space?
    // Clear BSS
    ldr     x0, =__bss_start__
    mov     w1, #0
    ldr     x2, =__end__
    sub     x2, x2, x0
    bl      memset

    b       . // FIXME!

.pool
