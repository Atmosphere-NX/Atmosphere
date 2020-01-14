/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

.macro      LDSTORE_QREGS, op
    \op     q0, q1, [x0], 0x20
    \op     q2, q3, [x0], 0x20
    \op     q4, q5, [x0], 0x20
    \op     q6, q7, [x0], 0x20
    \op     q8, q9, [x0], 0x20
    \op     q10, q11, [x0], 0x20
    \op     q12, q13, [x0], 0x20
    \op     q14, q15, [x0], 0x20
    \op     q16, q17, [x0], 0x20
    \op     q18, q19, [x0], 0x20
    \op     q20, q21, [x0], 0x20
    \op     q22, q23, [x0], 0x20
    \op     q24, q25, [x0], 0x20
    \op     q26, q27, [x0], 0x20
    \op     q28, q29, [x0], 0x20
    \op     q30, q31, [x0], 0x20
.endm

FUNCTION fpuLoadRegistersFromStorage
    dmb     sy
    LDSTORE_QREGS ldp
    ldp     x1, x2, [x0]
    msr     fpsr, x1
    msr     fpcr, x2
    dsb     sy
    isb     sy
    ret
END_FUNCTION

FUNCTION fpuStoreRegistersToStorage
    dsb     sy
    isb     sy
    LDSTORE_QREGS stp
    mrs     x1, fpsr
    mrs     x2, fpcr
    stp     x1, x2, [x0]
    dmb     sy
    ret
END_FUNCTION
