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
    b begin_relocation_loop
    _version:
    .word 0x00000000 /* Version. */
    .word 0x00000000 /* Reserved. */

    begin_relocation_loop:

    /* Relocate ourselves if necessary */
    ldr r2, =__start__
    adr r3, _start
    cmp r2, r3
    beq _relocation_loop_end

    /* If we are relocating, we are not rebooting to ourselves. Note that. */
    ldr r0, =0x4003FFFC
    mov r1, #0x0
    str r1, [r0]

    mov r4, #0x1000
    mov r1, #0x0
    _relocation_loop:
        ldmia r3!, {r5-r12}
        stmia r2!, {r5-r12}
        add r1, r1, #0x20
        cmp r1, r4
        bne _relocation_loop

    ldr r12, =_second_relocation_start
    bx  r12

    _second_relocation_start:
    ldr r4, =__bss_start__
    sub r4, r4, r2
    mov r1, #0x0

    _second_relocation_loop:
        ldmia r3!, {r5-r12}
        stmia r2!, {r5-r12}
        add r1, r1, #0x20
        cmp r1, r4
        bne _second_relocation_loop

    ldr r12, =_relocation_loop_end
    bx  r12

    _relocation_loop_end:

    /* Set the stack pointer */
    ldr  sp, =__stack_top__
    mov  fp, #0
    bl  __program_init

    /* Set r0 to r12 to 0 (for debugging) & call main */
    .rept 13
    CLEAR_GPR_REG_ITER
    .endr
    ldr lr, =__program_exit
    ldr r0, =_version
    ldr r0, [r0]
    b   sept_main

/* No need to include this in normal programs: */
.section .chainloader.text.start, "ax", %progbits
.arm
.align 5
.global relocate_and_chainload
.type   relocate_and_chainload, %function
relocate_and_chainload:
    ldr sp, =__stack_top__
    b   relocate_and_chainload_main
