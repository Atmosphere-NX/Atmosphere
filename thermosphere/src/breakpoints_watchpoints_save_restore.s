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


// Precondition: x1 <= 16
.section    .text.loadBreakpointRegs, "ax", %progbits
.type       loadBreakpointRegs, %function
.global     loadBreakpointRegs
loadBreakpointRegs:
    // x1 = number
    adr     x16, 1f
    add     x0, x0, #(16 * 8)
    mov     x4, #(16 * 12)
    sub     x4, x4, x1,lsl #3
    sub     x4, x4, x1,lsl #2
    add     x16, x16, x4
    br      x16

    1:
    .irp    count, 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
    ldp     x2, x3, [x0, #-0x10]!
    msr     dbgbcr\count\()_el1, x2
    msr     dbgbvr\count\()_el1, x3
    .endr
    dsb     sy
    isb
    ret

// Precondition: x1 <= 16
.section    .text.storeBreakpointRegs, "ax", %progbits
.type       storeBreakpointRegs, %function
.global     storBreakpointRegs
storeBreakpointRegs:
    // x1 = number
    dsb     sy
    isb

    adr     x16, 1f
    add     x0, x0, #(16 * 8)
    mov     x4, #(16 * 12)
    sub     x4, x4, x1,lsl #3
    sub     x4, x4, x1,lsl #2
    add     x16, x16, x4
    br      x16

    1:
    .irp    count, 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
    mrs     x2, dbgbcr\count\()_el1
    mrs     x3, dbgbvr\count\()_el1
    stp     x2, x3, [x0, #-0x10]!

    .endr
    ret


// Precondition: x1 <= 16
.section    .text.loadWatchpointRegs, "ax", %progbits
.type       loadWatchpointRegs, %function
.global     loadWatchpointRegs
loadWatchpointRegs:
    // x1 = number
    adr     x16, 1f
    add     x0, x0, #(16 * 8)
    mov     x4, #(16 * 12)
    sub     x4, x4, x1,lsl #3
    sub     x4, x4, x1,lsl #2
    add     x16, x16, x4
    br      x16

    1:
    .irp    count, 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
    ldp     x2, x3, [x0, #-0x10]!
    msr     dbgwcr\count\()_el1, x2
    msr     dbgwvr\count\()_el1, x3
    .endr
    dsb     sy
    isb
    ret

// Precondition: x1 <= 16
.section    .text.storeWatchpointRegs, "ax", %progbits
.type       storeWatchpointRegs, %function
.global     storWatchpointRegs
storeWatchpointRegs:
    // x1 = number

    dsb     sy
    isb

    adr     x16, 1f
    add     x0, x0, #(16 * 8)
    mov     x4, #(16 * 12)
    sub     x4, x4, x1,lsl #3
    sub     x4, x4, x1,lsl #2
    add     x16, x16, x4
    br      x16

    1:
    .irp    count, 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
    mrs     x2, dbgwcr\count\()_el1
    mrs     x3, dbgwvr\count\()_el1
    stp     x2, x3, [x0, #-0x10]!

    .endr
    ret
