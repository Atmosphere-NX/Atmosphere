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
