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

.section    .text._ZN3ams8warmboot5StartEv, "ax", %progbits
.align      3
.global     _ZN3ams8warmboot5StartEv
_ZN3ams8warmboot5StartEv:
    /* Set CPSR_cf and CPSR_cf. */
    msr cpsr_f, #0xC0
    msr cpsr_cf, #0xD3

    /* Set the stack pointer. */
    ldr sp, =__stack_top__

    /* Set our link register to the exception handler. */
    ldr lr, =_ZN3ams8warmboot16ExceptionHandlerEv

    /* Invoke main. */
    ldr r0, =_metadata
    b _ZN3ams8warmboot4MainEPKNS0_8MetadataE

    /* Infinite loop. */
    1: b 1b