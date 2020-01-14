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

/* Some macros taken from https://github.com/ARM-software/arm-trusted-firmware/blob/master/include/common/aarch64/asm_macros.S */
/*
 * Copyright (c) 2013-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * Declare the exception vector table, enforcing it is aligned on a
 * 2KB boundary, as required by the ARMv8 architecture.
 * Use zero bytes as the fill value to be stored in the padding bytes
 * so that it inserts illegal AArch64 instructions. This increases
 * security, robustness and potentially facilitates debugging.
 */
.macro vector_base  label, section_name=.vectors
.section \section_name, "ax"
.align 11, 0
\label:
.endm

/*
 * Create an entry in the exception vector table, enforcing it is
 * aligned on a 128-byte boundary, as required by the ARMv8 architecture.
 * Use zero bytes as the fill value to be stored in the padding bytes
 * so that it inserts illegal AArch64 instructions. This increases
 * security, robustness and potentially facilitates debugging.
 */
.macro vector_entry  label, section_name=.vectors
.cfi_sections .debug_frame
.section \section_name, "ax"
.align 7, 0
.type \label, %function
.func \label
.cfi_startproc
\label:
.endm

/*
 * This macro verifies that the given vector doesnt exceed the
 * architectural limit of 32 instructions. This is meant to be placed
 * immediately after the last instruction in the vector. It takes the
 * vector entry as the parameter
 */
.macro check_vector_size since
    .endfunc
    .cfi_endproc
    .if (. - \since) > (32 * 4)
        .error "Vector exceeds 32 instructions"
    .endif
.endm

.macro  SAVE_MOST_REGISTERS
    sub     sp, sp, #EXCEP_STACK_FRAME_SIZE

    stp     x28, x29, [sp, #-0x20]
    stp     x30, xzr, [sp, #-0x10]
    mrs     x28, far_el2
    mrs     x29, cntpct_el0
    bl      _saveMostRegisters
.endm

.macro PIVOT_STACK_FOR_CRASH
    // Note: x18 assumed uncorrupted
    // Note: replace sp_el0 with crashing sp
    str     x16, [x18, #CORECTX_SCRATCH_OFFSET]
    mov     x16, sp
    msr     sp_el0, x16
    ldr     x16, [x18, #CORECTX_CRASH_STACK_OFFSET]
    mov     sp, x16
    ldr     x16, [x18, #CORECTX_SCRATCH_OFFSET]
.endm

.equ    EXCEPTION_TYPE_HOST,            0
.equ    EXCEPTION_TYPE_GUEST,           1
.equ    EXCEPTION_TYPE_HOST_CRASH,      2

.macro  EXCEPTION_HANDLER_START name, type
vector_entry \name
    .if \type == EXCEPTION_TYPE_HOST_CRASH
        PIVOT_STACK_FOR_CRASH
    .endif

    SAVE_MOST_REGISTERS

    mov         x0, sp

    .if \type == EXCEPTION_TYPE_GUEST
        ldp     x18, xzr, [sp, #EXCEP_STACK_FRAME_SIZE]
        str     x0, [x18, #CORECTX_USER_FRAME_OFFSET]
        mov     w1, #1
    .else
        mov     w1, #0
    .endif

    bl          exceptionEntryPostprocess
.endm

.macro EXCEPTION_HANDLER_END name, type
    .if \type != EXCEPTION_TYPE_HOST_CRASH
        mov     x0, sp
        bl      exceptionReturnPreprocess
        b       _restoreAllRegisters
    .else
        b       .
    .endif
check_vector_size \name
.endm

.macro  UNKNOWN_EXCEPTION name
vector_entry \name
    bl      _unknownException
check_vector_size \name
.endm

/* Actual Vectors for Thermosphere. */
.global     g_thermosphereVectors
vector_base g_thermosphereVectors

/* Current EL, SP0 */
/* Those are unused by us, except on same-EL double-faults. */
UNKNOWN_EXCEPTION       _synchSp0

_unknownException:
    pivot_stack_for_crash
    mov     x0, x30
    adr     x1, g_thermosphereVectors + 4
    sub     x0, x0, x1
    bl      handleUnknownException
    b       .

UNKNOWN_EXCEPTION       _irqSp0

/* To save space, insert in an unused vector segment. */
_saveMostRegisters:
    stp     x0, x1, [sp, #0x00]
    stp     x2, x3, [sp, #0x10]
    stp     x4, x5, [sp, #0x20]
    stp     x6, x7, [sp, #0x30]
    stp     x8, x9, [sp, #0x40]
    stp     x10, x11, [sp, #0x50]
    stp     x12, x13, [sp, #0x60]
    stp     x14, x15, [sp, #0x70]
    stp     x16, x17, [sp, #0x80]
    stp     x18, x19, [sp, #0x90]
    stp     x20, x21, [sp, #0xA0]
    stp     x22, x23, [sp, #0xB0]
    stp     x24, x25, [sp, #0xC0]
    stp     x26, x27, [sp, #0xD0]

    mrs     x20, sp_el1
    mrs     x21, sp_el0
    mrs     x22, elr_el2
    mrs     x23, spsr_el2
    mov     x24, x28                // far_el2
    mov     x25, x29                // cntpct_el0

    // See SAVE_MOST_REGISTERS macro
    ldp     x28, x29, [sp, #-0x20]
    ldp     x19, xzr, [sp, #-0x10]

    stp     x28, x29, [sp, #0xE0]
    stp     x19, x20, [sp, #0xF0]
    stp     x21, x22, [sp, #0x100]
    stp     x23, x24, [sp, #0x110]
    stp     x25, xzr, [sp, #0x120]

    ret

UNKNOWN_EXCEPTION       _fiqSp0

/* To save space, insert in an unused vector segment. */

// Accessed by start.s
.global     _restoreAllRegisters
.type       _restoreAllRegisters, %function
_restoreAllRegisters:
    ldp     x30, x20, [sp, #0xF0]
    ldp     x21, x22, [sp, #0x100]
    ldp     x23, xzr, [sp, #0x110]

    msr     sp_el1, x20
    msr     sp_el0, x21
    msr     elr_el2, x22
    msr     spsr_el2, x23

    ldp     x0, x1, [sp, #0x00]
    ldp     x2, x3, [sp, #0x10]
    ldp     x4, x5, [sp, #0x20]
    ldp     x6, x7, [sp, #0x30]
    ldp     x8, x9, [sp, #0x40]
    ldp     x10, x11, [sp, #0x50]
    ldp     x12, x13, [sp, #0x60]
    ldp     x14, x15, [sp, #0x70]
    ldp     x16, x17, [sp, #0x80]
    ldp     x18, x19, [sp, #0x90]
    ldp     x20, x21, [sp, #0xA0]
    ldp     x22, x23, [sp, #0xB0]
    ldp     x24, x25, [sp, #0xC0]
    ldp     x26, x27, [sp, #0xD0]
    ldp     x28, x29, [sp, #0xE0]

    add     sp, sp, #EXCEP_STACK_FRAME_SIZE
    eret

UNKNOWN_EXCEPTION       _serrorSp0

/* To save space, insert in an unused vector segment. */
.global semihosting_call
.type   semihosting_call, %function
.func   semihosting_call
.cfi_startproc
semihosting_call:
    hlt     #0xF000
    ret
.cfi_endproc
.endfunc

.global doSmcIndirectCallImpl
doSmcIndirectCallImpl:
    stp     x19, x20, [sp, #-0x10]!
    mov     x19, x0

    ldp     x0, x1, [x19, #0x00]
    ldp     x2, x3, [x19, #0x10]
    ldp     x4, x5, [x19, #0x20]
    ldp     x6, x7, [x19, #0x30]

    _doSmcIndirectCallImplSmcInstruction:
    smc     #0

    stp     x0, x1, [x19, #0x00]
    stp     x2, x3, [x19, #0x10]
    stp     x4, x5, [x19, #0x20]
    stp     x6, x7, [x19, #0x30]

    ldp     x19, x20, [sp], #0x10
    ret

_doSmcIndirectCallImplEnd:
.global doSmcIndirectCallImplSmcInstructionOffset
doSmcIndirectCallImplSmcInstructionOffset:
    .word _doSmcIndirectCallImplSmcInstruction - doSmcIndirectCallImpl
.global doSmcIndirectCallImplSize
doSmcIndirectCallImplSize:
    .word _doSmcIndirectCallImplEnd - doSmcIndirectCallImpl

/* Current EL, SPx */

EXCEPTION_HANDLER_START     _synchSpx, EXCEPTION_TYPE_HOST_CRASH
    mov     x0, sp
    mrs     x1, esr_el2
    bl      handleSameElSyncException
EXCEPTION_HANDLER_END       _synchSpx, EXCEPTION_TYPE_HOST_CRASH

EXCEPTION_HANDLER_START     _irqSpx, EXCEPTION_TYPE_HOST
    mov     x0, sp
    mov     w1, wzr
    mov     w2, wzr
    bl      handleIrqException
EXCEPTION_HANDLER_END       _irqSpx, EXCEPTION_TYPE_HOST

UNKNOWN_EXCEPTION           _fiqSpx
UNKNOWN_EXCEPTION           _serrorSpx

/* Lower EL, A64 */

EXCEPTION_HANDLER_START     _synchA64, EXCEPTION_TYPE_GUEST
    mov     x0, sp
    mrs     x1, esr_el2
    bl      handleLowerElSyncException
EXCEPTION_HANDLER_END       _synchA64, EXCEPTION_TYPE_GUEST

EXCEPTION_HANDLER_START     _irqA64, EXCEPTION_TYPE_GUEST
    mov     x0, sp
    mov     w1, #1
    mov     w2, #0
    bl      handleIrqException
EXCEPTION_HANDLER_END        _irqA64, EXCEPTION_TYPE_GUEST

UNKNOWN_EXCEPTION           _fiqA64
UNKNOWN_EXCEPTION           _serrorA64

/* Lower EL, A32 */

EXCEPTION_HANDLER_START     _synchA32, EXCEPTION_TYPE_GUEST
    mov     x0, sp
    mrs     x1, esr_el2
    bl      handleLowerElSyncException
EXCEPTION_HANDLER_END       _synchA32, EXCEPTION_TYPE_GUEST

EXCEPTION_HANDLER_START     _irqA32, EXCEPTION_TYPE_GUEST
    mov     x0, sp
    mov     w1, #1
    mov     w2, #1
    bl      handleIrqException
EXCEPTION_HANDLER_END        _irqA32, EXCEPTION_TYPE_GUEST

UNKNOWN_EXCEPTION           _fiqA32
UNKNOWN_EXCEPTION           _serrorA32
