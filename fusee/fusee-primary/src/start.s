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

.macro CLEAR_GPR_REG_ITER
    mov r\@, #0
.endm

.section .text.start, "ax", %progbits
.arm
.align 5
.global _start
.type   _start, %function
_start:
    /* Switch to system mode, mask all interrupts, clear all flags */
    msr cpsr_cxsf, #0xDF

    /* Relocate loader stub. */
    ldr r0, =_start
    adr r1, _start
    ldr r2, =__loader_stub_lma__
    sub r2, r2, r0
    add r2, r2, r1

    ldr r3, =__loader_stub_start__
    ldr r4, =__loader_stub_end__
    sub r4, r4, r3
    _relocation_loop:
        ldmia r2!, {r5-r12}
        stmia r3!, {r5-r12}
        subs  r4, #0x20
        bne _relocation_loop

    /* Set the stack pointer */
    ldr  sp, =__stack_top__
    mov  fp, #0

    /* Generate arguments. */
    ldr r3, =fusee_primary_main_lz4
    ldr r4, =fusee_primary_main_lz4_end
    sub r4, r4, r3
    sub r3, r3, r0
    add r3, r3, r1
    mov r0, r3
    mov r1, r4

    /* Jump to the loader stub. */
    ldr r3, =load_fusee_primary_main
    bx  r3
