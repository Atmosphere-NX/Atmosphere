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

.section .text.start

/* Warmboot header. */
/* Binary size */
.word __total_size__
.rept 3
.word 0x00000000
.endr
/* RSA modulus */
.rept 0x40
.word 0xFFFFFFFF
.endr
/* Padding */
.rept 4
.word 0x00000000
.endr
/* RSA signature */
.rept 0x40
.word 0xFFFFFFFF
.endr
/* Padding */
.rept 4
.word 0x00000000
.endr
/* Relocation meta */
.word __total_size__
.word _start
.word _start
.word __executable_size__

.global _start
_start:
    b crt0

.global _metadata
_metadata:
    .ascii "WBT0"     /* Magic number */
    .word  0x00000000 /* Target firmware. */
    .word  0x00000000 /* Reserved */
    .word  0x00000000 /* Reserved */

.global crt0
.type crt0, %function
crt0:
    @ setup to call lp0_entry_main
    ldr sp, =__stack_top__
    ldr lr, =reboot
    ldr r0, =_metadata
    b lp0_entry_main
