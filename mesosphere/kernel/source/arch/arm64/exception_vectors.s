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

/* Actual Vectors for Kernel. */
.global _ZN3ams4kern16ExceptionVectorsEv
vector_base _ZN3ams4kern16ExceptionVectorsEv

/* Current EL, SP0 */
vector_entry synch_sp0
    /* Just infinite loop. */
    clrex
    nop
    b synch_sp0
    check_vector_size synch_sp0

vector_entry irq_sp0
    clrex
    nop
    b irq_sp0
    check_vector_size irq_sp0

vector_entry fiq_sp0
    clrex
    nop
    b fiq_sp0
    check_vector_size fiq_sp0

vector_entry serror_sp0
    clrex
    nop
    b serror_sp0
    check_vector_size serror_sp0

/* Current EL, SPx */
vector_entry synch_spx
    clrex
    b _ZN3ams4kern4arch5arm6430EL1SynchronousExceptionHandlerEv
    check_vector_size synch_spx

vector_entry irq_spx
    b _ZN3ams4kern4arch5arm6422EL1IrqExceptionHandlerEv
    check_vector_size irq_spx

vector_entry fiq_spx
    clrex
    nop
    b fiq_spx
    check_vector_size fiq_spx

vector_entry serror_spx
    clrex
    nop
    b _ZN3ams4kern4arch5arm6421EL1SystemErrorHandlerEv
    check_vector_size serror_spx

/* Lower EL, A64 */
vector_entry synch_a64
    clrex
    b _ZN3ams4kern4arch5arm6430EL0SynchronousExceptionHandlerEv
    check_vector_size synch_a64

vector_entry irq_a64
    clrex
    b _ZN3ams4kern4arch5arm6422EL0IrqExceptionHandlerEv
    check_vector_size irq_a64

vector_entry fiq_a64
    clrex
    nop
    b fiq_a64
    check_vector_size fiq_a64

vector_entry serror_a64
    clrex
    nop
    b _ZN3ams4kern4arch5arm6421EL0SystemErrorHandlerEv
    check_vector_size serror_a64

/* Lower EL, A32 */
vector_entry synch_a32
    clrex
    b _ZN3ams4kern4arch5arm6430EL0SynchronousExceptionHandlerEv
    check_vector_size synch_a32

vector_entry irq_a32
    clrex
    b _ZN3ams4kern4arch5arm6422EL0IrqExceptionHandlerEv
    check_vector_size irq_a32

vector_entry fiq_a32
    clrex
    nop
    b fiq_a32
    check_vector_size fiq_a32

vector_entry serror_a32
    clrex
    nop
    b _ZN3ams4kern4arch5arm6421EL0SystemErrorHandlerEv
    check_vector_size serror_a32
