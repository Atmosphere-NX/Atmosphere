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

.section .text.start, "ax", %progbits
.arm
.align 5
.global _start
.type   _start, %function
_start:
    /* Set coldboot */
    mov r0, #0x0
    ldr r1, =0x7000E400
    str r0, [r1, #0x50]

    /* Tell pk1ldr normal reboot, no error */
    str r0, [r1, #0x1B4]
    str r0, [r1, #0x840]

    /* Cleanup SVC handler address. */
    ldr r0, =0x40004C30
    ldr r1, =0x6000F208
    str r0, [r1]

    /* Disable RCM forcefully */
    mov r0, #0x4
    ldr r1, =0x15DC
    ldr r2, =0xE020
    bl ipatch_word

    /* Patch BCT signature check */
    mov r0, #0x5
    ldr r1, =0x4AEE
    ldr r2, =0xE05B
    bl ipatch_word

    /* Patch bootloader read */
    mov r0, #0x6
    ldr r1, =0x4E88
    ldr r2, =0xE018
    bl ipatch_word

    ldr r0, =__main_phys_start__
    ldr r1, =__main_start__
    mov r2, #0x0
    ldr r3, =(__main_size__)
    copy_panic_payload:
        ldr r4, [r0, r2]
        str r4, [r1, r2]
        add r2, r2, #0x4
        cmp r2, r3
        bne copy_panic_payload



    /* Jump back to bootrom start. */
    ldr r0, =0x101010
    bx r0

    /* Unused, but forces inclusion in binary. */
    b main


.section .text.ipatch_word, "ax", %progbits
.arm
.align 5
.global ipatch_word
.type   ipatch_word, %function
ipatch_word:
    ldr r3, =0x6001dc00
    lsl r0, r0, #0x2
    lsr r1, r1, #0x1
    lsl r1, r1, #0x10
    orr r1, r1, r2
    str r1, [r3, r0]
    bx lr

.section .text.jump_to_main, "ax", %progbits
.arm
.align 5
.global jump_to_main
.type   jump_to_main, %function
jump_to_main:
    /* Insert 0x240 of NOPs, for version compatibility. */
.rept (0x240/4)
    nop
.endr
    /* Just jump to main */
    ldr sp, =__stack_top__
    b main


