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

/* Actual Vectors for KernelLdr. */
.global kernelldr_vectors
vector_base kernelldr_vectors

/* Current EL, SP0 */
.global unknown_exception
unknown_exception:
vector_entry synch_sp0
    /* Just infinite loop. */
    b unknown_exception
    check_vector_size synch_sp0

vector_entry irq_sp0
    b unknown_exception
    check_vector_size irq_sp0

vector_entry fiq_sp0
    b unknown_exception
    check_vector_size fiq_sp0

vector_entry serror_sp0
    b unknown_exception
    check_vector_size serror_sp0

/* Current EL, SPx */
vector_entry synch_spx
    b restore_tpidr_el1
    check_vector_size synch_spx

vector_entry irq_spx
    b unknown_exception
    check_vector_size irq_spx

vector_entry fiq_spx
    b unknown_exception
    check_vector_size fiq_spx

vector_entry serror_spx
    b unknown_exception
    check_vector_size serror_spx

/* Lower EL, A64 */
vector_entry synch_a64
    b unknown_exception
    check_vector_size synch_a64

vector_entry irq_a64
    b unknown_exception
    check_vector_size irq_a64

vector_entry fiq_a64
    b unknown_exception
    check_vector_size fiq_a64

vector_entry serror_a64
    b unknown_exception
    check_vector_size serror_a64

/* Lower EL, A32 */
vector_entry synch_a32
    b unknown_exception
    check_vector_size synch_a32

vector_entry irq_a32
    b unknown_exception
    check_vector_size irq_a32

vector_entry fiq_a32
    b unknown_exception
    check_vector_size fiq_a32

vector_entry serror_a32
    b unknown_exception
    .endfunc
    .cfi_endproc
/* To save space, insert in an unused vector segment. */
.global     restore_tpidr_el1
.type       restore_tpidr_el1, %function
restore_tpidr_el1:
    mrs x0, tpidr_el1
    /* Make sure that TPIDR_EL1 can be dereferenced. */
    invalid_tpidr:
        cbz x0, invalid_tpidr
    /* Restore saved registers. */
    ldp x19, x20, [x0], #0x10
    ldp x21, x22, [x0], #0x10
    ldp x23, x24, [x0], #0x10
    ldp x25, x26, [x0], #0x10
    ldp x27, x28, [x0], #0x10
    ldp x29, x30, [x0], #0x10
    ldp x1,  xzr, [x0], #0x10
    mov sp, x1
    mov x0, #0x1
    ret