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

.section .reboot.start, "ax", %progbits
.arm
.align 5
.global reboot_start
.type   reboot_start, %function
reboot_start:
    /* Switch to system mode, mask all interrupts, clear all flags */
    msr cpsr_cxsf, #0xDF

    /* Relocate reboot start to 0x4003E000. */
    ldr r0, =0x4003E000
    adr r1, reboot_start
    cmp r0, r1
    beq 1f

    mov r2, #0x1000
0:
    ldmia r1!, {r5-r12}
    stmia r0!, {r5-r12}
    subs  r2, #0x20
    bne 0b

    ldr r0, =0x4003E000
    adr r1, reboot_start
    adr r2, 1f
    sub r2, r2, r1
    add r2, r2, r0
    bx  r2

1:

    /* Restore our low iram code. */
    ldr r0, =0x40008000
    ldr r1, =0x40030000
    mov r2, #0x8000
0:
    ldmia r1!, {r5-r12}
    stmia r0!, {r5-r12}
    subs  r2, #0x20
    bne 0b

    /* Restore our upper stub code. */
    ldr r0, =0x40010000
    ldr r1, =0x4003D000
    mov r2, #0x1000
0:
    ldmia r1!, {r5-r12}
    stmia r0!, {r5-r12}
    subs  r2, #0x20
    bne 0b

    /* Jump to start. */
    ldr r0, =0x40008000
    bx  r0

0:  b 0b