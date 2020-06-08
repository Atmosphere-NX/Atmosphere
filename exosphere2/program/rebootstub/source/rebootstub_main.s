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

.section    .text._ZN3ams10rebootstub4MainEv, "ax", %progbits
.align      3
.global     _ZN3ams10rebootstub4MainEv
_ZN3ams10rebootstub4MainEv:
    /* Get the reboot type. */
    ldr r0, =_ZN3ams10rebootstub10RebootTypeE
    ldr r0, [r0]

    /* If the reboot type is power off, perform a power off. */
    cmp r0, #0
    beq _ZN3ams10rebootstub8PowerOffEv

    /* Otherwise, clear all registers jump to the reboot payload in iram. */
    ldr r0, =0x52425430 /* RBT0 */
    mov r1, #0
    mov r2, #0
    mov r3, #0
    mov r4, #0
    mov r5, #0
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    mov r9, #0
    mov r10, #0
    mov r11, #0
    mov r12, #0
    mov r9, #0
    mov lr, #0
    ldr sp, =0x40010000
    ldr pc, =0x40010000

    /* Infinite loop. */
    1: b 1b