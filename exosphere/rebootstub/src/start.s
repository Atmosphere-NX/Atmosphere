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
 
.section .text.start
.align 4
.global _start
_start:
    ldr r0, reboot_type
    cmp r0, #0x0
    beq do_shutdown
    b jump_to_reboot_payload
reboot_type:
.word 0x00000001

.global jump_to_reboot_payload
.type jump_to_reboot_payload, %function
jump_to_reboot_payload:
    @ clear all registers
    ldr r0, =0x52425430 @ RBT0
    mov r1, #0x0
    mov r2, #0x0
    mov r3, #0x0
    mov r4, #0x0
    mov r5, #0x0
    mov r6, #0x0
    mov r7, #0x0
    mov r8, #0x0
    mov r9, #0x0
    mov r10, #0x0
    mov r11, #0x0
    mov r12, #0x0
    mov lr, #0x0
    ldr sp, =0x40010000
    ldr pc, =0x40010000
    


