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

.section    .text._ZN3ams4kern4init6loader23SaveRegistersToTpidrEl1EPv, "ax", %progbits
.global     _ZN3ams4kern4init6loader23SaveRegistersToTpidrEl1EPv
_ZN3ams4kern4init6loader23SaveRegistersToTpidrEl1EPv:
    /* Set TPIDR_EL1 to the input register. */
    msr tpidr_el1, x0

    /* Save registers to the region specified. */
    mov x1, sp
    stp x19, x20, [x0], #0x10
    stp x21, x22, [x0], #0x10
    stp x23, x24, [x0], #0x10
    stp x25, x26, [x0], #0x10
    stp x27, x28, [x0], #0x10
    stp x29, x30, [x0], #0x10
    stp x1,  xzr, [x0], #0x10
    mov x0, #0x0
    ret

.section    .text._ZN3ams4kern4init6loader22VerifyAndClearTpidrEl1EPv, "ax", %progbits
.global     _ZN3ams4kern4init6loader22VerifyAndClearTpidrEl1EPv
_ZN3ams4kern4init6loader22VerifyAndClearTpidrEl1EPv:
    /* Get system register area from thread-specific processor id */
    mrs x1, tpidr_el1

    /* We require here that the region registers are saved is same as input. */
    cmp x0, x1
    invalid_tpidr:
        b.ne invalid_tpidr

    /* Clear TPIDR_EL1. */
    msr tpidr_el1, xzr
    ret
