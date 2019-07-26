/*
 * Copyright (c) 2018 Atmosphère-NX
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
    
    /* Backup current stack pointer. */
    mov r12, sp
    
    /* Set the stack pointer */
    ldr  sp, =__stack_top__
    mov  fp, #0
    
    /* Save context */
    push {r12, lr}
    
    /* Call init. */
    bl  __program_init

    /* Set r0 to r12 to 0 (for debugging) & call main */
    .rept 13
    CLEAR_GPR_REG_ITER
    .endr
    ldr r0, =__program_argc
    ldr r1, =__program_argv
    ldr r0, [r0]
    ldr r1, [r1]
    bl   main
    
    /* Save result. */
    push {r0}
    
    /* Exit manually. */
    bl  __program_exit
    
    /* Restore result. */
    pop {r0}

    /* Restore context */
    pop {r12}
    pop {lr}
    
    /* Restore previous stack pointer. */
    mov sp, r12
    
    /* Return */
    bx lr