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

.section    .crt0.text._ZN3ams10sdmmc_test5StartEv, "ax", %progbits
.align      3
.global     _ZN3ams10sdmmc_test5StartEv
_ZN3ams10sdmmc_test5StartEv:
    /* Switch to system mode, mask all interrupts, clear all flags */
    msr cpsr_cxsf, #0xDF

    /* Set the stack pointer. */
    ldr sp, =__stack_top__

    /* Set our link register to the exception handler. */
    ldr lr, =_ZN3ams10sdmmc_test16ExceptionHandlerEv

    /* Call init array functions. */
    bl __libc_init_array

    /* Invoke main. */
    b _ZN3ams10sdmmc_test4MainEv

    /* Infinite loop. */
    2: b 2b