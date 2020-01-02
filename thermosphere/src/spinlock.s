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

// From Arm TF


/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

.global  spinlockLock
.type   spinlockLock, %function
.func   spinlockLock
.cfi_startproc
spinlockLock:
    mov     w2, #1
    sevl
    l1:
        wfe
        l2:
            ldaxr       w1, [x0]
            cbnz        w1, l1
            stxr        w1, w2, [x0]
            cbnz        w1, l2
    ret
.endfunc
.cfi_endproc

.global spinlockUnlock
.type   spinlockUnlock, %function
.func   spinlockUnlock
.cfi_startproc
spinlockUnlock:
    stlr    wzr, [x0]
    sev
    ret
.endfunc
.cfi_endproc