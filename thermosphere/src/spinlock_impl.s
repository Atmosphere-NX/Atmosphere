/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "asm_macros.s"

// From Arm TF


/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

FUNCTION spinlockLock
    mov     w2, #1
    sevl
    prfm    pstl1keep, [x0]
    1:
        wfe
        2:
            ldaxr   w1, [x0]
            cbnz    w1, 1b
            stxr    w1, w2, [x0]
            cbnz    w1, 2b
    ret
END_FUNCTION

FUNCTION spinlockTryLock
    mov     x1, x0
    mov     w2, #1
    prfm    pstl1strm, [x1]
    1:
        ldaxr   w0, [x1]
        cbnz    w0, 2f
        stxr    w0, w2, [x1]
        cbnz    w0, 1b
    2:
    eor     w0, w0, #1
    ret
END_FUNCTION

FUNCTION spinlockUnlock
    stlr        wzr, [x0]
    ret
END_FUNCTION
