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

.section    .crt0.text.start, "ax", %progbits
.align      6
.global     _start
_start:
    /* Set the stack pointer to a temporary location. */
    ldr x20, =0x1F00FC000
    mov sp, x20

    /* Save any arguments we may have. */
    stp x0, x1, [sp, #-16]!

    /* Initialize all memory to expected state. */
    ldr x0, =__bss_start__
    ldr x1, =__bss_end__
    bl _ZN3ams6secmon5fatal10InitializeEmm

    /* Restore any arguments we may have. */
    ldp x0, x1, [sp], #16

    /* Jump to the fatal program. */
    ldr x16, =_ZN3ams6secmon5fatal4MainEv
    br x16
