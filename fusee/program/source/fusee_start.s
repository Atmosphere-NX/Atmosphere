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

.section .crt0._ZN3ams6nxboot5StartEv, "ax", %progbits
.arm
.align 5
.global _ZN3ams6nxboot5StartEv
.type   _ZN3ams6nxboot5StartEv, %function
_ZN3ams6nxboot5StartEv:
    /* Setup all registers as = 0. */
    msr cpsr_f, #0xC0
    mov r0, #0
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r4, #0
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    mov r9, #0
    mov r10, #0
    mov r11, #0
    mov r12, #0

    /* Setup all modes as pointing to our exception handler. */
    msr cpsr_cf, #0xDF
    ldr sp, =0x40001000
    ldr lr, =_ZN3ams6nxboot16ExceptionHandlerEv

    msr cpsr_cf, #0xD2
    ldr sp, =0x40001000
    ldr lr, =_ZN3ams6nxboot16ExceptionHandlerEv

    msr cpsr_cf, #0xD1
    ldr sp, =0x40001000
    ldr lr, =_ZN3ams6nxboot16ExceptionHandlerEv

    msr cpsr_cf, #0xD7
    ldr sp, =0x40001000
    ldr lr, =_ZN3ams6nxboot16ExceptionHandlerEv

    msr cpsr_cf, #0xDB
    ldr sp, =0x40001000
    ldr lr, =_ZN3ams6nxboot16ExceptionHandlerEv

    msr cpsr_cf, #0xD3
    ldr sp, =0x40001000
    ldr lr, =_ZN3ams6nxboot16ExceptionHandlerEv

    /* Perform runtime initialization. */
    bl _ZN3ams6nxboot4crt010InitializeEv

    /* Perform nx boot procedure. */
    bl _ZN3ams6nxboot4MainEv
