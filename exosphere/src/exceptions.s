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

/* Actual Vectors for Exosphere. */
.global exosphere_vectors
vector_base exosphere_vectors

/* Current EL, SP0 */
.global unknown_exception
unknown_exception:
vector_entry synch_sp0
    /* Panic with color FF7700, code 10. */
    mov x0, #0x10
    movk x0, #0x07F0,lsl#16
    b panic
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
    b unknown_exception
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
    stp x29, x30, [sp, #-0x10]!
    /* Verify SMC. */
    mrs x30, esr_el3
    lsr w29, w30, #0x1A
    cmp w29, #0x17
    ldp x29, x30, [sp],#0x10
    b.ne unknown_exception
    /* Call appropriate handler. */
    stp x29, x30, [sp, #-0x10]!
    mrs x29, mpidr_el1
    and x29, x29, #0x3
    cmp x29, #0x3
    b.ne handle_core012_smc_exception
    bl handle_core3_smc_exception
    ldp x29, x30, [sp],#0x10
    eret
    check_vector_size synch_a64

vector_entry irq_a64
    b unknown_exception
    check_vector_size irq_a64

vector_entry fiq_a64
    stp x29, x30, [sp, #-0x10]!
    mrs x29, mpidr_el1
    and x29, x29, #0x3
    cmp x29, #0x3
    b.ne unknown_exception
    stp x28, x29, [sp, #-0x10]!
    stp x26, x27, [sp, #-0x10]!
    bl handle_fiq_exception
    ldp x26, x27, [sp],#0x10
    ldp x28, x29, [sp],#0x10
    ldp x29, x30, [sp],#0x10
    eret
    check_vector_size fiq_a64

vector_entry serror_a64
    b unknown_exception
    .endfunc
    .cfi_endproc
/* To save space, insert in an unused vector segment. */
.global     handle_core012_smc_exception
.type       handle_core012_smc_exception, %function
handle_core012_smc_exception:
    stp x6, x7, [sp, #-0x10]!
    stp x4, x5, [sp, #-0x10]!
    stp x2, x3, [sp, #-0x10]!
    stp x0, x1, [sp, #-0x10]!
    bl set_priv_smc_in_progress
    bl get_smc_core012_stack_address
    mov x29, x0
    ldp x0, x1, [sp],#0x10
    ldp x2, x3, [sp],#0x10
    ldp x4, x5, [sp],#0x10
    ldp x6, x7, [sp],#0x10
    mov x30, sp
    mov sp, x29
    stp x29, x30, [sp, #-0x10]!
    bl handle_core3_smc_exception
    ldp x29, x30, [sp],#0x10
    mov sp, x30
    stp x6, x7, [sp, #-0x10]!
    stp x4, x5, [sp, #-0x10]!
    stp x2, x3, [sp, #-0x10]!
    stp x0, x1, [sp, #-0x10]!
    bl clear_priv_smc_in_progress
    ldp x0, x1, [sp],#0x10
    ldp x2, x3, [sp],#0x10
    ldp x4, x5, [sp],#0x10
    ldp x6, x7, [sp],#0x10
    ldp x29, x30, [sp],#0x10
    eret

/* Lower EL, A32 */
vector_entry synch_a32
    b unknown_exception
    check_vector_size synch_a32

vector_entry irq_a32
    b unknown_exception
    check_vector_size irq_a32

vector_entry fiq_a32
    b fiq_a64
    .endfunc
    .cfi_endproc
/* To save space, insert in an unused vector segment. */
.global     handle_fiq_exception
.type       handle_fiq_exception, %function
handle_fiq_exception:
    stp x29, x30, [sp, #-0x10]!
    stp x24, x25, [sp, #-0x10]!
    stp x22, x23, [sp, #-0x10]!
    stp x20, x21, [sp, #-0x10]!
    stp x18, x19, [sp, #-0x10]!
    stp x16, x17, [sp, #-0x10]!
    stp x14, x15, [sp, #-0x10]!
    stp x12, x13, [sp, #-0x10]!
    stp x10, x11, [sp, #-0x10]!
    stp x8, x9, [sp, #-0x10]!
    stp x6, x7, [sp, #-0x10]!
    stp x4, x5, [sp, #-0x10]!
    stp x2, x3, [sp, #-0x10]!
    stp x0, x1, [sp, #-0x10]!
    bl handle_registered_interrupt
    ldp x0, x1, [sp],#0x10
    ldp x2, x3, [sp],#0x10
    ldp x4, x5, [sp],#0x10
    ldp x6, x7, [sp],#0x10
    ldp x8, x9, [sp],#0x10
    ldp x10, x11, [sp],#0x10
    ldp x12, x13, [sp],#0x10
    ldp x14, x15, [sp],#0x10
    ldp x16, x17, [sp],#0x10
    ldp x18, x19, [sp],#0x10
    ldp x20, x21, [sp],#0x10
    ldp x22, x23, [sp],#0x10
    ldp x24, x25, [sp],#0x10
    ldp x29, x30, [sp],#0x10
    ret

vector_entry serror_a32
    b unknown_exception
    .endfunc
    .cfi_endproc
/* To save space, insert in an unused vector segment. */
.global     handle_core3_smc_exception
.type       handle_core3_smc_exception, %function
handle_core3_smc_exception:
    stp x29, x30, [sp, #-0x10]!
    stp x18, x19, [sp, #-0x10]!
    stp x16, x17, [sp, #-0x10]!
    stp x14, x15, [sp, #-0x10]!
    stp x12, x13, [sp, #-0x10]!
    stp x10, x11, [sp, #-0x10]!
    stp x8, x9, [sp, #-0x10]!
    stp x6, x7, [sp, #-0x10]!
    stp x4, x5, [sp, #-0x10]!
    stp x2, x3, [sp, #-0x10]!
    stp x0, x1, [sp, #-0x10]!
    mrs x0, esr_el3
    and x0, x0, #0xFFFF
    mov x1, sp
    bl call_smc_handler
    ldp x0, x1, [sp],#0x10
    ldp x2, x3, [sp],#0x10
    ldp x4, x5, [sp],#0x10
    ldp x6, x7, [sp],#0x10
    ldp x8, x9, [sp],#0x10
    ldp x10, x11, [sp],#0x10
    ldp x12, x13, [sp],#0x10
    ldp x14, x15, [sp],#0x10
    ldp x16, x17, [sp],#0x10
    ldp x18, x19, [sp],#0x10
    ldp x29, x30, [sp],#0x10
    ret
