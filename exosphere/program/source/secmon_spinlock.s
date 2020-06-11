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

.section    .text._ZN3ams6secmon15AcquireSpinLockERj, "ax", %progbits
.align      4
.global     _ZN3ams6secmon15AcquireSpinLockERj
_ZN3ams6secmon15AcquireSpinLockERj:
    /* Prepare to try to take the spinlock. */
    mov w1, #1
    sevl
    prfm pstl1keep, [x0]

1:  /* Repeatedly try to take the lock. */
    wfe
    ldaxr w2, [x0]
    cbnz  w2, 1b
    stxr  w2, w1, [x0]
    cbnz  w2, 1b

    /* Return. */
    ret

.section    .text._ZN3ams6secmon15ReleaseSpinLockERj, "ax", %progbits
.align      4
.global     _ZN3ams6secmon15ReleaseSpinLockERj
_ZN3ams6secmon15ReleaseSpinLockERj:
    /* Release the spinlock. */
    stlr wzr, [x0]

    /* Return. */
    ret
