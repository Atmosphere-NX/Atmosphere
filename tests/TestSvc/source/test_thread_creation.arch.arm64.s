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

/* ams::test::TestThreadCreateRegistersOnFunctionEntry(void *ctx) */
.section    .text._ZN3ams4test40TestThreadCreateRegistersOnFunctionEntryEPv, "ax", %progbits
.global     _ZN3ams4test40TestThreadCreateRegistersOnFunctionEntryEPv
.type       _ZN3ams4test40TestThreadCreateRegistersOnFunctionEntryEPv, %function
_ZN3ams4test40TestThreadCreateRegistersOnFunctionEntryEPv:
    /* Save all registers to our context. */
    stp  x0,  x1, [x0, #0x00]
    stp  x2,  x3, [x0, #0x10]
    stp  x4,  x5, [x0, #0x20]
    stp  x6,  x7, [x0, #0x30]
    stp  x8,  x9, [x0, #0x40]
    stp x10, x11, [x0, #0x50]
    stp x12, x13, [x0, #0x60]
    stp x14, x15, [x0, #0x70]
    stp x16, x17, [x0, #0x80]
    stp x18, x19, [x0, #0x90]
    stp x20, x21, [x0, #0xA0]
    stp x22, x23, [x0, #0xB0]
    stp x24, x25, [x0, #0xC0]
    stp x26, x27, [x0, #0xD0]
    stp x28, x29, [x0, #0xE0]

    mov x1, sp
    stp x30, x1, [x0, #0xF0]

    /* Exit the thread. */
    svc     0xa