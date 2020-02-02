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

.altmacro

.macro LOAD_DBG_REG_PAIRS what, id
    ldp     x2, x3, [x0, #-0x10]!
    msr     dbg\what\()cr\id\()_el1, x2
    msr     dbg\what\()vr\id\()_el1, x3
    .if     \id != 0
        LOAD_DBG_REG_PAIRS      \what, %(\id - 1)
    .endif
.endm

// Precondition: x1 <= 16
FUNCTION loadBreakpointRegs
    // x1 = number
    dmb     ish

    adr     x16, 1f
    add     x0, x0, #(MAX_BCR * 8)
    mov     x4, #(MAX_BCR * 12)
    sub     x4, x4, x1,lsl #3
    sub     x4, x4, x1,lsl #2
    add     x16, x16, x4
    br      x16

    1:
    LOAD_DBG_REG_PAIRS b, %(MAX_BCR - 1)

    dsb     ish
    isb
    ret
END_FUNCTION

// Precondition: x1 <= 16
FUNCTION loadWatchpointRegs
    // x1 = number
    dmb     ish

    adr     x16, 1f
    add     x0, x0, #(MAX_WCR * 8)
    mov     x4, #(MAX_WCR * 12)
    sub     x4, x4, x1,lsl #3
    sub     x4, x4, x1,lsl #2
    add     x16, x16, x4
    br      x16

    1:
    LOAD_DBG_REG_PAIRS w, %(MAX_WCR - 1)

    dsb     ish
    isb
    ret
END_FUNCTION
