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
#include <mesosphere/kern_select_assembly_offsets.h>

/* ams::kern::arch::arm64::EL1IrqExceptionHandler() */
.section    .text._ZN3ams4kern4arch5arm6422EL1IrqExceptionHandlerEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6422EL1IrqExceptionHandlerEv
.type       _ZN3ams4kern4arch5arm6422EL1IrqExceptionHandlerEv, %function
_ZN3ams4kern4arch5arm6422EL1IrqExceptionHandlerEv:
    /* Save registers that need saving. */
    sub     sp,  sp, #(8 * 24)

    stp     x0,  x1,  [sp, #(8 *  0)]
    stp     x2,  x3,  [sp, #(8 *  2)]
    stp     x4,  x5,  [sp, #(8 *  4)]
    stp     x6,  x7,  [sp, #(8 *  6)]
    stp     x8,  x9,  [sp, #(8 *  8)]
    stp     x10, x11, [sp, #(8 * 10)]
    stp     x12, x13, [sp, #(8 * 12)]
    stp     x14, x15, [sp, #(8 * 14)]
    stp     x16, x17, [sp, #(8 * 16)]
    stp     x19, x20, [sp, #(8 * 18)]
    stp     x21, x30, [sp, #(8 * 20)]

    mrs     x19, sp_el0
    mrs     x20, elr_el1
    mrs     x21, spsr_el1
    mov     w21, w21

    /* Invoke KInterruptManager::HandleInterrupt(bool user_mode). */
    mov     x0, #0
    bl      _ZN3ams4kern4arch5arm6417KInterruptManager15HandleInterruptEb

    /* Restore registers that we saved. */
    msr     sp_el0, x19
    msr     elr_el1, x20
    msr     spsr_el1, x21

    ldp     x0,  x1,  [sp, #(8 *  0)]
    ldp     x2,  x3,  [sp, #(8 *  2)]
    ldp     x4,  x5,  [sp, #(8 *  4)]
    ldp     x6,  x7,  [sp, #(8 *  6)]
    ldp     x8,  x9,  [sp, #(8 *  8)]
    ldp     x10, x11, [sp, #(8 * 10)]
    ldp     x12, x13, [sp, #(8 * 12)]
    ldp     x14, x15, [sp, #(8 * 14)]
    ldp     x16, x17, [sp, #(8 * 16)]
    ldp     x19, x20, [sp, #(8 * 18)]
    ldp     x21, x30, [sp, #(8 * 20)]

    add     sp, sp, #(8 * 24)

    /* Return from the exception. */
    eret

/* ams::kern::arch::arm64::EL0IrqExceptionHandler() */
.section    .text._ZN3ams4kern4arch5arm6422EL0IrqExceptionHandlerEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6422EL0IrqExceptionHandlerEv
.type       _ZN3ams4kern4arch5arm6422EL0IrqExceptionHandlerEv, %function
_ZN3ams4kern4arch5arm6422EL0IrqExceptionHandlerEv:
    /* Save registers that need saving. */
    sub     sp,  sp, #(EXCEPTION_CONTEXT_SIZE)

    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    stp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    stp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    stp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    stp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    stp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    stp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    stp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    stp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    stp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    stp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    mrs     x20, sp_el0
    mrs     x21, elr_el1
    mrs     x22, spsr_el1
    mrs     x23, tpidr_el0
    mov     w22, w22

    stp     x30, x20, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    stp     x21, x22, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    str     x23,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    /* Invoke KInterruptManager::HandleInterrupt(bool user_mode). */
    ldr     x18, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CUR_THREAD)]
    mov     x0, #1
    bl      _ZN3ams4kern4arch5arm6417KInterruptManager15HandleInterruptEb

    /* Restore state from the context. */
    ldp     x30, x20, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    ldp     x21, x22, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    ldr     x23,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
    /* Since we're returning from an exception, set SPSR.SS so that we advance an instruction if single-stepping. */
    orr x22, x22, #(1 << 21)
    #endif

    msr     sp_el0, x20
    msr     elr_el1, x21
    msr     spsr_el1, x22
    msr     tpidr_el0, x23

    ldp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    ldp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    ldp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    ldp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    ldp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    ldp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    ldp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    ldp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    ldp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    ldp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    ldp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    ldp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    ldp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    ldp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    ldp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    add     sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    /* Return from the exception. */
    eret

/* ams::kern::arch::arm64::EL0SynchronousExceptionHandler() */
.section    .text._ZN3ams4kern4arch5arm6430EL0SynchronousExceptionHandlerEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6430EL0SynchronousExceptionHandlerEv
.type       _ZN3ams4kern4arch5arm6430EL0SynchronousExceptionHandlerEv, %function
_ZN3ams4kern4arch5arm6430EL0SynchronousExceptionHandlerEv:
    /* Save x16 and x17, so that we can use them as scratch. */
    stp     x16, x17, [sp, #-16]!

    /* Get and parse the exception syndrome register. */
    mrs     x16, esr_el1
    lsr     x17, x16, #0x1a

    /* Is this an aarch32 SVC? */
    cmp     x17, #0x11
    b.eq    2f

    /* Is this an aarch64 SVC? */
    cmp     x17, #0x15
    b.eq    3f

    /* Is this an FPU error? */
    cmp     x17, #0x7
    b.eq    4f

    /* Is this a data abort? */
    cmp     x17, #0x24
    b.eq    5f

    /* Is this an instruction abort? */
    cmp     x17, #0x20
    b.eq    5f

1:  /* The exception is not a data abort or instruction abort caused by a TLB conflict. */
    /* It is also not an SVC or an FPU exception. Handle it generically! */

    /* Restore x16 and x17. */
    ldp x16, x17, [sp], 16

    /* Create a KExceptionContext to pass to HandleException. */
    sub     sp, sp, #(EXCEPTION_CONTEXT_SIZE)
    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    stp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    stp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    stp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    stp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    stp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    stp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    stp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    stp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    stp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    stp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    mrs     x20, sp_el0
    mrs     x21, elr_el1
    mrs     x22, spsr_el1
    mrs     x23, tpidr_el0
    mov     w22, w22

    stp     x30, x20, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    stp     x21, x22, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    str     x23,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    /* Call ams::kern::arch::arm64::HandleException(ams::kern::arch::arm64::KExceptionContext *) */
    ldr     x18, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CUR_THREAD)]
    mov     x0,  sp
    bl      _ZN3ams4kern4arch5arm6415HandleExceptionEPNS2_17KExceptionContextE

    /* Restore state from the context. */
    ldp     x30, x20, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    ldp     x21, x22, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    ldr     x23,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    msr     sp_el0, x20
    msr     elr_el1, x21
    msr     spsr_el1, x22
    msr     tpidr_el0, x23

    ldp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    ldp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    ldp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    ldp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    ldp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    ldp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    ldp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    ldp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    ldp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    ldp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    ldp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    ldp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    ldp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    ldp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    ldp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    add     sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    /* Return from the exception. */
    eret

2:  /* SVC from aarch32. */
    ldp x16, x17, [sp], 16
    b _ZN3ams4kern4arch5arm6412SvcHandler32Ev

3:  /* SVC from aarch64. */
    ldp x16, x17, [sp], 16
    b _ZN3ams4kern4arch5arm6412SvcHandler64Ev

4:  /* FPU exception. */
    ldp x16, x17, [sp], 16
    b _ZN3ams4kern4arch5arm6425FpuAccessExceptionHandlerEv

5:  /* Check if there's a TLB conflict that caused the abort. */
    and     x17, x16, #0x3F
    cmp     x17, #0x30
    b.ne    1b

    /* Get the ASID in x17. */
    mrs     x17, ttbr0_el1
    and     x17, x17, #(0xFFFF << 48)

    /* Check if FAR is valid by examining the FnV bit. */
    tbnz    x16, #10, 6f

    /* FAR is valid, so we can invalidate the address it holds. */
    mrs     x16, far_el1
    lsr     x16, x16, #12
    orr     x17, x16, x17
    tlbi    vae1, x17
    b       7f

6:  /* There's a TLB conflict and FAR isn't valid. */
    /* Invalidate the entire TLB. */
    tlbi    aside1, x17

7:  /* Return from a TLB conflict. */
    /* Ensure instruction consistency. */
    dsb     ish
    isb

    /* Restore x16 and x17. */
    ldp x16, x17, [sp], 16

    /* Return from the exception. */
    eret


/* ams::kern::arch::arm64::EL1SynchronousExceptionHandler() */
.section    .text._ZN3ams4kern4arch5arm6430EL1SynchronousExceptionHandlerEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6430EL1SynchronousExceptionHandlerEv
.type       _ZN3ams4kern4arch5arm6430EL1SynchronousExceptionHandlerEv, %function
_ZN3ams4kern4arch5arm6430EL1SynchronousExceptionHandlerEv:
    /* Nintendo uses tpidr_el1 as a scratch register. */
    msr     tpidr_el1, x0

    /* Get and parse the exception syndrome register. */
    mrs     x0, esr_el1
    lsr     x0, x0, #0x1a

    /* Is this an instruction abort? */
    cmp     x0, #0x21
    b.eq    5f

    /* Is this a data abort? */
    cmp     x0, #0x25
    b.eq    5f

1:  /* The exception is not a data abort or instruction abort caused by a TLB conflict. */
    /* Load the exception stack top from otherwise "unused" virtual timer compare value. */
    mrs     x0, cntv_cval_el0

    /* Setup the stack for a generic exception handle */
    lsl     x0, x0, #8
    asr     x0, x0, #8
    sub     x0, x0, #0x20
    str     x1, [x0, #8]
    mov     x1, sp
    str     x1, [x0]
    mov     sp, x0
    ldr     x1, [x0, #8]
    mrs     x0, tpidr_el1
    str     x0, [sp, #8]
    str     x1, [sp, #16]

    /* Check again if this is a data abort from EL1. */
    mrs     x0, esr_el1
    lsr     x1, x0, #0x1a
    cmp     x1, #0x25
    b.ne    3f

    /* Data abort. Check if it was from trying to access userspace memory. */
    mrs     x1, elr_el1
    adr     x0, _ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv
    cmp     x1, x0
    b.lo    3f
    adr     x0, _ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv
    cmp     x1, x0
    b.hs    3f

    /* We aborted trying to access userspace memory. */
    /* All functions that access user memory return a boolean for whether they succeeded. */
    /* With that in mind, we can simply restore the stack pointer and return false directly. */
    ldr     x0, [sp]
    mov     sp, x0

    /* Return false. */
    mov     x0, #0x0
    msr     elr_el1, x30
    eret

3:  /* The exception wasn't an triggered by copying memory from userspace. */
    ldr     x0, [sp, #8]
    ldr     x1, [sp, #16]

    /* Create a KExceptionContext to pass to HandleException. */
    sub     sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    stp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    stp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    stp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    stp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    stp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    stp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    stp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    stp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    stp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    stp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    ldr     x20, [sp]
    mrs     x21, elr_el1
    mrs     x22, spsr_el1
    mrs     x23, tpidr_el0
    mov     w22, w22

    stp     x30, x20, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    stp     x21, x22, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    str     x23,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    /* Call ams::kern::arch::arm64::HandleException(ams::kern::arch::arm64::KExceptionContext *) */
    mov     x0,  sp
    bl      _ZN3ams4kern4arch5arm6415HandleExceptionEPNS2_17KExceptionContextE

4:  /* HandleException should never return. The best we can do is infinite loop. */
    b       4b

5:  /* Check if there's a TLB conflict that caused the abort. */
    /* NOTE: There is a Nintendo bug in this code that we correct. */
    /* Nintendo compares the low 6 bits of x0 without restoring the value. */
    /* They intend to check the DFSC/IFSC bits of esr_el1, but because they */
    /* shifted esr earlier, the check is invalid and always fails. */
    mrs     x0, esr_el1
    and     x0, x0, #0x3F
    cmp     x0, #0x30
    b.ne    1b

    /* Check if FAR is valid by examining the FnV bit. */
    /* NOTE: Nintendo again has a bug here, the same as above. */
    /* They do not refresh the value of x0, and again compare with */
    /* the relevant bit already masked out of x0. */
    mrs     x0, esr_el1
    tbnz    x0, #10, 6f

    /* FAR is valid, so we can invalidate the address it holds. */
    mrs     x0, far_el1
    lsr     x0, x0, #12
    tlbi    vaae1, x0
    b       7f

6:  /* There's a TLB conflict and FAR isn't valid. */
    /* Invalidate the entire TLB. */
    tlbi    vmalle1

7:  /* Return from a TLB conflict. */
    /* Ensure instruction consistency. */
    dsb     ish
    isb

    /* Restore x0 from scratch. */
    mrs     x0, tpidr_el1

    /* Return from the exception. */
    eret


/* ams::kern::arch::arm64::FpuAccessExceptionHandler() */
.section    .text._ZN3ams4kern4arch5arm6425FpuAccessExceptionHandlerEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6425FpuAccessExceptionHandlerEv
.type       _ZN3ams4kern4arch5arm6425FpuAccessExceptionHandlerEv, %function
_ZN3ams4kern4arch5arm6425FpuAccessExceptionHandlerEv:
    /* Save registers that need saving. */
    sub     sp,  sp, #(EXCEPTION_CONTEXT_SIZE)

    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    stp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    stp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    stp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    stp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    stp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    stp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]

    mrs     x19, sp_el0
    mrs     x20, elr_el1
    mrs     x21, spsr_el1
    mov     w21, w21

    stp     x30, x19, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    stp     x20, x21, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]

    /* Invoke the FPU context switch handler. */
    ldr     x18, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CUR_THREAD)]
    bl      _ZN3ams4kern4arch5arm6423FpuContextSwitchHandlerEv

    /* Restore registers that we saved. */
    ldp     x30, x19, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    ldp     x20, x21, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]

    msr     sp_el0, x19
    msr     elr_el1, x20
    msr     spsr_el1, x21

    ldp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    ldp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    ldp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    ldp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    ldp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    ldp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    ldp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    ldp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    ldp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    ldp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    ldp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]

    add     sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    /* Return from the exception. */
    eret

/* ams::kern::arch::arm64::EL1SystemErrorHandler() */
.section    .text._ZN3ams4kern4arch5arm6421EL1SystemErrorHandlerEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6421EL1SystemErrorHandlerEv
.type       _ZN3ams4kern4arch5arm6421EL1SystemErrorHandlerEv, %function
_ZN3ams4kern4arch5arm6421EL1SystemErrorHandlerEv:
    /* Nintendo uses tpidr_el1 as a scratch register. */
    msr     tpidr_el1, x0

    /* Load the exception stack top from otherwise "unused" virtual timer compare value. */
    mrs     x0, cntv_cval_el0

    /* Setup the stack for a generic exception handle */
    lsl     x0, x0, #8
    asr     x0, x0, #8
    sub     x0, x0, #0x20
    str     x1, [x0, #8]
    mov     x1, sp
    str     x1, [x0]
    mov     sp, x0
    ldr     x1, [x0, #8]
    mrs     x0, tpidr_el1

    /* Create a KExceptionContext to pass to HandleException. */
    sub     sp, sp, #(EXCEPTION_CONTEXT_SIZE)
    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    stp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    stp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    stp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    stp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    stp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    stp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    stp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    stp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    stp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    stp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    ldr     x20, [sp]
    mrs     x21, elr_el1
    mrs     x22, spsr_el1
    mrs     x23, tpidr_el0
    mov     w22, w22

    stp     x30, x20, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    stp     x21, x22, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    str     x23,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    /* Invoke ams::kern::arch::arm64::HandleException(ams::kern::arch::arm64::KExceptionContext *). */
    mov     x0, sp
    bl      _ZN3ams4kern4arch5arm6415HandleExceptionEPNS2_17KExceptionContextE

1:  /* HandleException should never return. The best we can do is infinite loop. */
    b       1b

/* ams::kern::arch::arm64::EL0SystemErrorHandler() */
.section    .text._ZN3ams4kern4arch5arm6421EL0SystemErrorHandlerEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6421EL0SystemErrorHandlerEv
.type       _ZN3ams4kern4arch5arm6421EL0SystemErrorHandlerEv, %function
_ZN3ams4kern4arch5arm6421EL0SystemErrorHandlerEv:
    /* Create a KExceptionContext to pass to HandleException. */
    sub     sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    stp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    stp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    stp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    stp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    stp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    stp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    stp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    stp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    stp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    stp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    mrs     x20, sp_el0
    mrs     x21, elr_el1
    mrs     x22, spsr_el1
    mrs     x23, tpidr_el0
    mov     w22, w22

    stp     x30, x20, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    stp     x21, x22, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    str     x23,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    /* Invoke ams::kern::arch::arm64::HandleException(ams::kern::arch::arm64::KExceptionContext *). */
    ldr     x18, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CUR_THREAD)]
    mov     x0, sp
    bl      _ZN3ams4kern4arch5arm6415HandleExceptionEPNS2_17KExceptionContextE

    /* Restore state from the context. */
    ldp     x30, x20, [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    ldp     x21, x22, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    ldr     x23,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    msr     sp_el0, x20
    msr     elr_el1, x21
    msr     spsr_el1, x22
    msr     tpidr_el0, x23

    ldp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    ldp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    ldp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    ldp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    ldp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    ldp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    ldp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    ldp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    ldp     x16, x17, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    ldp     x18, x19, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    ldp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    ldp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    ldp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    ldp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    ldp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    add     sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    /* Return from the exception. */
    eret

