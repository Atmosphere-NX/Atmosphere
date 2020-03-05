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

// ams::hvisor::SafeIoCopy(void*, void const*, unsigned long)
FUNCTION _ZN3ams6hvisor10SafeIoCopyEPvPKvm
    // Caller needs to mask interrupts
    // See _synchSp0 exception handler
    msr     spsel, #0
    mov     x9, x18
    mov     x18, #0
    mov     x16, #0

    // x0-x2 parameters
    // x3: remainder
    // x4: offset

    cbz     x2, 2f
    mov     x3, x2
    mov     x4, #0

    mov     x5, #0
    // while (remainder > 0) { offset += increment; test alignment etc. } return offset;
    1:
        // Dispatcher
        add     x5, x1, x4
        add     x6, x0, x4
        orr     x7, x5, x6
        tst     x7, #3
        // ((src + off)|(dst + off)) & 3 == 0 ? remainder > 3 : eq
        ccmp    x3, #3, #0, eq
        bhi     3f
        // same thing but for 2-byte alignment
        cmp     x3, #1
        cset    w8, hi
        bics    wzr, w8, w5
        bne     4f

        // 8-bit load, if the load and/or store crashes, x16 = 1 (same thing for the other load/stores)
        ldrb    w5, [x5]
        strb    w5, [x6]
        cbnz    x16, 2f
        add     x4, x4, #1
        subs    x3, x3, #1
        bne     1b
    2:
        // Return
        msr     spsel, #1
        mov     x18, x9
        mov     x0, x4
        ret
    3:
        // 32-bit load
        ldr     w5, [x5]
        str     w5, [x6]
        cbnz    x16, 2b
        add     x4, x4, #4
        subs    x3, x3, #4
        bne     1b
        b       2b
    4:
        // 16-bit load
        ldrh    w5, [x5]
        strh    w5, [x6]
        cbnz    x16, 2b
        add     x4, x4, #2
        subs    x3, x3, #2
        bne     1b
        b       2b
END_FUNCTION
