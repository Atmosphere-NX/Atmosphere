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

/* ams::kern::arch::arm64::cpu::SynchronizeAllCoresImpl(int *sync_var, int num_cores) */
.section    .text._ZN3ams4kern4arch5arm643cpu23SynchronizeAllCoresImplEPii, "ax", %progbits
.global     _ZN3ams4kern4arch5arm643cpu23SynchronizeAllCoresImplEPii
.type       _ZN3ams4kern4arch5arm643cpu23SynchronizeAllCoresImplEPii, %function
_ZN3ams4kern4arch5arm643cpu23SynchronizeAllCoresImplEPii:
    /* Loop until the sync var is less than num cores. */
    sevl
1:
    wfe
    ldaxr w2, [x0]
    cmp   w2, w1
    b.gt  1b

    /* Increment the sync var. */
2:
    ldaxr w2, [x0]
    add   w3, w2, #1
    stlxr w4, w3, [x0]
    cbnz  w4, 2b

    /* Loop until the sync var matches our ticket. */
    add   w3, w2, w1
    sevl
3:
    wfe
    ldaxr w2, [x0]
    cmp   w2, w3
    b.ne  3b

    /* Check if the ticket is the last. */
    sub w2, w1, #1
    add w2, w2, w1
    cmp   w3, w2
    b.eq 5f

    /* Our ticket is not the last one. Increment. */
4:
    ldaxr w2, [x0]
    add   w3, w2, #1
    stlxr w4, w3, [x0]
    cbnz  w4, 4b
    ret

    /* Our ticket is the last one. */
5:
    stlr wzr, [x0]
    ret


/* ams::kern::arch::arm64::cpu::ClearPageToZero(void *) */
.section    .text._ZN3ams4kern4arch5arm643cpu19ClearPageToZeroImplEPv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm643cpu19ClearPageToZeroImplEPv
.type       _ZN3ams4kern4arch5arm643cpu19ClearPageToZeroImplEPv, %function
_ZN3ams4kern4arch5arm643cpu19ClearPageToZeroImplEPv:
    /* Efficiently clear the page using dc zva. */
    dc      zva, x0
    add     x8, x0, #0x040
    dc      zva, x8
    add     x8, x0, #0x080
    dc      zva, x8
    add     x8, x0, #0x0c0
    dc      zva, x8
    add     x8, x0, #0x100
    dc      zva, x8
    add     x8, x0, #0x140
    dc      zva, x8
    add     x8, x0, #0x180
    dc      zva, x8
    add     x8, x0, #0x1c0
    dc      zva, x8
    add     x8, x0, #0x200
    dc      zva, x8
    add     x8, x0, #0x240
    dc      zva, x8
    add     x8, x0, #0x280
    dc      zva, x8
    add     x8, x0, #0x2c0
    dc      zva, x8
    add     x8, x0, #0x300
    dc      zva, x8
    add     x8, x0, #0x340
    dc      zva, x8
    add     x8, x0, #0x380
    dc      zva, x8
    add     x8, x0, #0x3c0
    dc      zva, x8
    add     x8, x0, #0x400
    dc      zva, x8
    add     x8, x0, #0x440
    dc      zva, x8
    add     x8, x0, #0x480
    dc      zva, x8
    add     x8, x0, #0x4c0
    dc      zva, x8
    add     x8, x0, #0x500
    dc      zva, x8
    add     x8, x0, #0x540
    dc      zva, x8
    add     x8, x0, #0x580
    dc      zva, x8
    add     x8, x0, #0x5c0
    dc      zva, x8
    add     x8, x0, #0x600
    dc      zva, x8
    add     x8, x0, #0x640
    dc      zva, x8
    add     x8, x0, #0x680
    dc      zva, x8
    add     x8, x0, #0x6c0
    dc      zva, x8
    add     x8, x0, #0x700
    dc      zva, x8
    add     x8, x0, #0x740
    dc      zva, x8
    add     x8, x0, #0x780
    dc      zva, x8
    add     x8, x0, #0x7c0
    dc      zva, x8
    add     x8, x0, #0x800
    dc      zva, x8
    add     x8, x0, #0x840
    dc      zva, x8
    add     x8, x0, #0x880
    dc      zva, x8
    add     x8, x0, #0x8c0
    dc      zva, x8
    add     x8, x0, #0x900
    dc      zva, x8
    add     x8, x0, #0x940
    dc      zva, x8
    add     x8, x0, #0x980
    dc      zva, x8
    add     x8, x0, #0x9c0
    dc      zva, x8
    add     x8, x0, #0xa00
    dc      zva, x8
    add     x8, x0, #0xa40
    dc      zva, x8
    add     x8, x0, #0xa80
    dc      zva, x8
    add     x8, x0, #0xac0
    dc      zva, x8
    add     x8, x0, #0xb00
    dc      zva, x8
    add     x8, x0, #0xb40
    dc      zva, x8
    add     x8, x0, #0xb80
    dc      zva, x8
    add     x8, x0, #0xbc0
    dc      zva, x8
    add     x8, x0, #0xc00
    dc      zva, x8
    add     x8, x0, #0xc40
    dc      zva, x8
    add     x8, x0, #0xc80
    dc      zva, x8
    add     x8, x0, #0xcc0
    dc      zva, x8
    add     x8, x0, #0xd00
    dc      zva, x8
    add     x8, x0, #0xd40
    dc      zva, x8
    add     x8, x0, #0xd80
    dc      zva, x8
    add     x8, x0, #0xdc0
    dc      zva, x8
    add     x8, x0, #0xe00
    dc      zva, x8
    add     x8, x0, #0xe40
    dc      zva, x8
    add     x8, x0, #0xe80
    dc      zva, x8
    add     x8, x0, #0xec0
    dc      zva, x8
    add     x8, x0, #0xf00
    dc      zva, x8
    add     x8, x0, #0xf40
    dc      zva, x8
    add     x8, x0, #0xf80
    dc      zva, x8
    add     x8, x0, #0xfc0
    dc      zva, x8
    ret