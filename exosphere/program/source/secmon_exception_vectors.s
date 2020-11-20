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

/* Actual Vectors for Secure Monitor. */
.global _ZN3ams6secmon16ExceptionVectorsEv
vector_base _ZN3ams6secmon16ExceptionVectorsEv

/* Current EL, SP0 */
vector_entry synch_sp0
    /* Branch to the exception handler. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    .endfunc
    .cfi_endproc
_ZN3ams6secmon26UnexpectedExceptionHandlerEv:
    #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)
    /* Jump to the debug exception handler. */
    ldr x16, =_ZN3ams6secmon16ExceptionHandlerEv
    br  x16
    #else
    /* Load the ErrorInfo scratch. */
    ldr x0, =0x1F004AC40

    /* Write ErrorInfo_UnknownAbort to it. */
    ldr w1, =0x07F00010
    str w1, [x0]

    /* Perform an error reboot. */
    b _ZN3ams6secmon11ErrorRebootEv
    #endif

vector_entry irq_sp0
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size irq_sp0

vector_entry fiq_sp0
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size fiq_sp0

vector_entry serror_sp0
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size serror_sp0

/* Current EL, SPx */
vector_entry synch_spx
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size synch_spx

vector_entry irq_spx
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size irq_spx

vector_entry fiq_spx
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size fiq_spx

vector_entry serror_spx
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size serror_spx

/* Lower EL, A64 */
vector_entry synch_a64
    /* Check whether the exception is an SMC. If it's not, take the unexpected handler. */
    stp x29, x30, [sp, #-0x10]!
    mrs x30, esr_el3
    lsr w29, w30, #26
    cmp w29, #0x17
    ldp x29, x30, [sp], #0x10
    b.ne _ZN3ams6secmon26UnexpectedExceptionHandlerEv

    /* Save x29 and x30. */
    stp x29, x30, [sp, #-0x10]!

    /* Get the current core. */
    mrs x29, mpidr_el1
    and x29, x29, #3

    /* If we're not on core 3, take the core0-2 handler. */
    cmp x29, #3
    b.ne _ZN3ams6secmon25HandleSmcExceptionCore012Ev

    /* Handle the smc. */
    bl _ZN3ams6secmon18HandleSmcExceptionEv

    /* Return. */
    ldp x29, x30, [sp], #0x10
    eret
    check_vector_size synch_a64

vector_entry irq_a64
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size irq_a64

vector_entry fiq_a64
    /* Save x18, x26-x30. */
    stp x29, x30, [sp, #-0x10]!
    stp x28, x18, [sp, #-0x10]!
    stp x26, x27, [sp, #-0x10]!

    /* Set x18 to the global data region. */
    ldr x18, =0x1F01FA000

    /* Get the current core. */
    mrs x29, mpidr_el1
    and x29, x29, #3

    /* If we're not on core 3, take the core0-2 handler. */
    cmp x29, #3
    b.ne _ZN3ams6secmon25HandleFiqExceptionCore012Ev

    /* Handle the fiq exception. */
    bl _ZN3ams6secmon18HandleFiqExceptionEv

    /* Restore registers. */
    ldp x26, x27, [sp], #0x10
    ldp x28, x18, [sp], #0x10
    ldp x29, x30, [sp], #0x10

    /* Return. */
    eret
    check_vector_size fiq_a64

vector_entry serror_a64
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    .endfunc
    .cfi_endproc
_ZN3ams6secmon25HandleSmcExceptionCore012Ev:
    /* Acquire exclusive access to the common smc stack. */
    stp x4, x5, [sp, #-0x10]!
    stp x2, x3, [sp, #-0x10]!
    stp x0, x1, [sp, #-0x10]!
    bl _ZN3ams6secmon25AcquireCommonSmcStackLockEv
    ldp x0, x1, [sp], #0x10
    ldp x2, x3, [sp], #0x10
    ldp x4, x5, [sp], #0x10

    /* Pivot to use the common smc stack. */
    mov x30, sp
    ldr x29, =0x1F01F6E80
    mov sp, x29
    stp x29, x30, [sp, #-0x10]!

    /* Handle the SMC. */
    bl _ZN3ams6secmon18HandleSmcExceptionEv

    /* Restore our core-specific stack. */
    ldp x29, x30, [sp], #0x10
    mov sp, x30

    /* Release our exclusive access to the common smc stack. */
    stp x0, x1, [sp, #-0x10]!
    bl _ZN3ams6secmon25ReleaseCommonSmcStackLockEv
    ldp x0, x1, [sp], #0x10

    /* Return. */
    ldp x29, x30, [sp], #0x10
    eret

/* Lower EL, A32 */
vector_entry synch_a32
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    check_vector_size synch_a32

vector_entry irq_a32
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    .endfunc
    .cfi_endproc
_ZN3ams6secmon18HandleSmcExceptionEv:
    /* Save registers. */
    stp x29, x30, [sp, #-0x10]!
    stp x18, x19, [sp, #-0x10]!
    stp x16, x17, [sp, #-0x10]!
    stp x14, x15, [sp, #-0x10]!
    stp x12, x13, [sp, #-0x10]!
    stp x10, x11, [sp, #-0x10]!
    stp  x8,  x9, [sp, #-0x10]!
    stp  x6,  x7, [sp, #-0x10]!
    stp  x4,  x5, [sp, #-0x10]!
    stp  x2,  x3, [sp, #-0x10]!
    stp  x0,  x1, [sp, #-0x10]!

    /* Set x18 to the global data region. */
    ldr x18, =0x1F01FA000

    /* Get esr. */
    mrs x0, esr_el3
    and x0, x0, #0xFFFF

    /* Get the function arguments. */
    mov x1, sp

    /* Invoke the smc handler. */
    bl _ZN3ams6secmon3smc9HandleSmcEiRNS1_12SmcArgumentsE

    /* Restore registers. */
    ldp  x0,  x1, [sp], #0x10
    ldp  x2,  x3, [sp], #0x10
    ldp  x4,  x5, [sp], #0x10
    ldp  x6,  x7, [sp], #0x10
    ldp  x8,  x9, [sp], #0x10
    ldp x10, x11, [sp], #0x10
    ldp x12, x13, [sp], #0x10
    ldp x14, x15, [sp], #0x10
    ldp x16, x17, [sp], #0x10
    ldp x18, x19, [sp], #0x10
    ldp x29, x30, [sp], #0x10

    ret
vector_entry fiq_a32
    /* Handle fiq from a32 the same as fiq from a64. */
    b fiq_a64
    .endfunc
    .cfi_endproc
_ZN3ams6secmon18HandleFiqExceptionEv:
    /* Save registers. */
    stp x29, x30, [sp, #-0x10]!
    stp x24, x25, [sp, #-0x10]!
    stp x22, x23, [sp, #-0x10]!
    stp x20, x21, [sp, #-0x10]!
    stp x18, x19, [sp, #-0x10]!
    stp x16, x17, [sp, #-0x10]!
    stp x14, x15, [sp, #-0x10]!
    stp x12, x13, [sp, #-0x10]!
    stp x10, x11, [sp, #-0x10]!
    stp  x8,  x9, [sp, #-0x10]!
    stp  x6,  x7, [sp, #-0x10]!
    stp  x4,  x5, [sp, #-0x10]!
    stp  x2,  x3, [sp, #-0x10]!
    stp  x0,  x1, [sp, #-0x10]!

    /* Invoke the interrupt handler. */
    bl _ZN3ams6secmon15HandleInterruptEv

    /* Restore registers. */
    ldp  x0,  x1, [sp], #0x10
    ldp  x2,  x3, [sp], #0x10
    ldp  x4,  x5, [sp], #0x10
    ldp  x6,  x7, [sp], #0x10
    ldp  x8,  x9, [sp], #0x10
    ldp x10, x11, [sp], #0x10
    ldp x12, x13, [sp], #0x10
    ldp x14, x15, [sp], #0x10
    ldp x16, x17, [sp], #0x10
    ldp x18, x19, [sp], #0x10
    ldp x20, x21, [sp], #0x10
    ldp x22, x23, [sp], #0x10
    ldp x24, x25, [sp], #0x10
    ldp x29, x30, [sp], #0x10

    ret

vector_entry serror_a32
    /* An unexpected exception was taken. */
    b _ZN3ams6secmon26UnexpectedExceptionHandlerEv
    .endfunc
    .cfi_endproc
_ZN3ams6secmon25HandleFiqExceptionCore012Ev:
   /* Acquire exclusive access to the common smc stack. */
    stp x4, x5, [sp, #-0x10]!
    stp x2, x3, [sp, #-0x10]!
    stp x0, x1, [sp, #-0x10]!
    bl _ZN3ams6secmon25AcquireCommonSmcStackLockEv
    ldp x0, x1, [sp], #0x10
    ldp x2, x3, [sp], #0x10
    ldp x4, x5, [sp], #0x10

    /* Pivot to use the common smc stack. */
    mov x30, sp
    ldr x29, =0x1F01F6E80
    mov sp, x29
    stp x29, x30, [sp, #-0x10]!

    /* Handle the fiq exception. */
    bl _ZN3ams6secmon18HandleFiqExceptionEv

    /* Restore our core-specific stack. */
    ldp x29, x30, [sp], #0x10
    mov sp, x30

    /* Release our exclusive access to the common smc stack. */
    stp x0, x1, [sp, #-0x10]!
    bl _ZN3ams6secmon25ReleaseCommonSmcStackLockEv
    ldp x0, x1, [sp], #0x10

    /* Return. */
    ldp x26, x27, [sp], #0x10
    ldp x28, x18, [sp], #0x10
    ldp x29, x30, [sp], #0x10
    eret

    /* Instantiate the literal pool for the exception vectors. */
    .ltorg
