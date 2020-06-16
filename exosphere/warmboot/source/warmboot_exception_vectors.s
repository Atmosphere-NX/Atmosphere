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

.section    .vectors, "ax", %progbits
.align      3
.global     warmboot_header
warmboot_header:
    /* TODO: If Mariko warmboothax ever happens, generate a mariko header? */
    /* Warmboot header. */
    .word __total_size__
    .rept 3
    .word 0x00000000
    .endr

    /* RSA modulus. */
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

    /* Firmware metadata. */
    .word __total_size__
    .word _reset
    .word _reset
    .word __executable_size__

.global _reset
_reset:
    b _ZN3ams8warmboot5StartEv

.global _metadata
_metadata:
    .ascii "WBT0"     /* Magic number */
    .word  0x00000000 /* Target firmware. */
    .word  0x00000000 /* Reserved */
    .word  0x00000000 /* Reserved */