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



.section .crt0._ZN3ams6nxboot6loader5StartEv, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot6loader5StartEv
.type   _ZN3ams6nxboot6loader5StartEv, %function
_ZN3ams6nxboot6loader5StartEv:
    /* Switch to system mode, mask all interrupts, clear all flags */
    msr cpsr_cxsf, #0xDF

    /* Relocate main program. */
    ldr r0, =_ZN3ams6nxboot6loader5StartEv
    adr r1, _ZN3ams6nxboot6loader5StartEv
    cmp r0, r1
    beq 3f

    /* Relocate first 0x100. */
    mov r4, #0x100
    0:
        ldmia r1!, {r5-r12}
        stmia r0!, {r5-r12}
        subs  r4, #0x20
        bne 0b

    /* Jump, continue relocating. */
    ldr r3, =1f
    bx r3

    1:
    ldr r4, =__loader_stub_lma__
    ldr r3, =__loader_stub_end__
    add r4, r4, r3
    ldr r3, =__loader_stub_start__
    sub r4, r4, r3
    sub r4, r4, r0
    2:
        ldmia r1!, {r5-r12}
        stmia r0!, {r5-r12}
        subs  r4, #0x20
        bne 2b

    /* Relocate loader stub. */
    3:
    ldr r2, =__loader_stub_lma__

    ldr r3, =__loader_stub_start__
    ldr r4, =__loader_stub_end__
    sub r4, r4, r3
    4:
        ldmia r2!, {r5-r12}
        stmia r3!, {r5-r12}
        subs  r4, #0x20
        bne 4b

    /* Set the stack pointer */
    ldr  sp, =0x40001000
    mov  fp, #0

    /* Generate arguments. */
    ldr r3, =program_lz4
    ldr r4, =program_lz4_end
    sub r4, r4, r3
    mov r0, r3
    mov r1, r4

    /* Jump to the loader stub. */
    ldr r3, =_ZN3ams6nxboot6loader20UncompressAndExecuteEPKvj
    bx  r3