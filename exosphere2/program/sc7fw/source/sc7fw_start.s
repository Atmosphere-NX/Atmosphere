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

.section    .text.start, "ax", %progbits
.align      3
.global     _start
_start:
    /* Set CPSR_cf and CPSR_cf. */
    msr cpsr_f, #0xC0
    msr cpsr_cf, #0xD3

    /* Set the stack pointer. */
    ldr sp, =0x40005000

    /* Set our link register to the exception handler. */
    ldr lr, =_ZN3ams5sc7fw16ExceptionHandlerEv

    /* Invoke main. */
    bl _ZN3ams5sc7fw4MainEv

    /* Infinite loop. */
    1: b 1b