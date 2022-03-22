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
#include <mesosphere/kern_build_config.hpp>
#include <mesosphere/kern_select_assembly_offsets.h>

/* ams::kern::arch::arm64::SvcHandler64() */
.section    .text._ZN3ams4kern4arch5arm6412SvcHandler64Ev, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6412SvcHandler64Ev
.type       _ZN3ams4kern4arch5arm6412SvcHandler64Ev, %function
_ZN3ams4kern4arch5arm6412SvcHandler64Ev:
    /* Create a KExceptionContext for the exception. */
    sub     sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    /* Save registers needed for ReturnFromException */
    stp     x9, x10,  [sp, #(EXCEPTION_CONTEXT_X9_X10)]
    str     x11,      [sp, #(EXCEPTION_CONTEXT_X11)]
    str     x18,      [sp, #(EXCEPTION_CONTEXT_X18)]

    mrs     x8, sp_el0
    mrs     x9, elr_el1
    mrs     x10, spsr_el1
    mrs     x11, tpidr_el0
    ldr     x18, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CUR_THREAD)]

    /* Save callee-saved registers. */
    stp     x19, x20, [sp, #(EXCEPTION_CONTEXT_X19_X20)]
    stp     x21, x22, [sp, #(EXCEPTION_CONTEXT_X21_X22)]
    stp     x23, x24, [sp, #(EXCEPTION_CONTEXT_X23_X24)]
    stp     x25, x26, [sp, #(EXCEPTION_CONTEXT_X25_X26)]
    stp     x27, x28, [sp, #(EXCEPTION_CONTEXT_X27_X28)]

    /* Save miscellaneous registers. */
    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     x29, x30, [sp, #(EXCEPTION_CONTEXT_X29_X30)]
    stp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_SP_PC)]
    stp     x10, x11, [sp, #(EXCEPTION_CONTEXT_PSR_TPIDR)]

    /* Check if the SVC index is out of range. */
    mrs     x8, esr_el1
    and     x8, x8, #0xFF
    cmp     x8, #(AMS_KERN_NUM_SUPERVISOR_CALLS)
    b.ge    3f

    /* Check the specific SVC permission bit for allowal. */
    mov     x9, sp
    add     x9, x9, x8, lsr#3
    ldrb    w9, [x9, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_SVC_PERMISSION)]
    and     x10, x8, #0x7
    lsr     x10, x9, x10
    tst     x10, #1
    b.eq    3f

    /* Check if our disable count allows us to call SVCs. */
    mrs     x10, tpidrro_el0
    ldrh    w10, [x10, #(THREAD_LOCAL_REGION_DISABLE_COUNT)]
    cbz     w10, 1f

    /* It might not, so check the stack params to see if we must not allow the SVC. */
    ldrb    w10, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_IS_PINNED)]
    cbz     w10, 3f

1:  /* We can call the SVC. */
    adr     x10, _ZN3ams4kern3svc10SvcTable64E
    ldr     x11, [x10, x8, lsl#3]
    cbz     x11, 3f

    /* Note that we're calling the SVC. */
    mov     w10, #1
    strb    w10, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_IS_CALLING_SVC)]
    strb    w8,  [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CURRENT_SVC_ID)]

    /* If we should, trace the svc entry. */
#if defined(MESOSPHERE_BUILD_FOR_TRACING)
    sub     sp, sp, #0x50
    stp     x0,  x1,  [sp, #(8 *  0)]
    stp     x2,  x3,  [sp, #(8 *  2)]
    stp     x4,  x5,  [sp, #(8 *  4)]
    stp     x6,  x7,  [sp, #(8 *  6)]
    str     x11,      [sp, #(8 *  8)]
    mov     x0, sp
    bl      _ZN3ams4kern3svc13TraceSvcEntryEPKm
    ldp     x0,  x1,  [sp, #(8 *  0)]
    ldp     x2,  x3,  [sp, #(8 *  2)]
    ldp     x4,  x5,  [sp, #(8 *  4)]
    ldp     x6,  x7,  [sp, #(8 *  6)]
    ldr     x11,      [sp, #(8 *  8)]
    add     sp, sp, #0x50
#endif

    /* Invoke the SVC handler. */
    msr     daifclr, #2
    blr     x11
    msr     daifset, #2

2:  /* We completed the SVC, and we should handle DPC. */
    /* Check the dpc flags. */
    ldrb    w8, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_DPC_FLAGS)]
    cbz     w8, 4f

    /* We have DPC to do! */
    /* Save registers and call ams::kern::KDpcManager::HandleDpc(). */
    sub     sp, sp, #0x40
    stp     x0,  x1,  [sp, #(8 *  0)]
    stp     x2,  x3,  [sp, #(8 *  2)]
    stp     x4,  x5,  [sp, #(8 *  4)]
    stp     x6,  x7,  [sp, #(8 *  6)]
    bl      _ZN3ams4kern11KDpcManager9HandleDpcEv
    ldp     x0,  x1,  [sp, #(8 *  0)]
    ldp     x2,  x3,  [sp, #(8 *  2)]
    ldp     x4,  x5,  [sp, #(8 *  4)]
    ldp     x6,  x7,  [sp, #(8 *  6)]
    add     sp, sp, #0x40
    b 2b

3:  /* Invalid SVC. */
    /* Setup the context to call into HandleException. */
    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    str          x19, [sp, #(EXCEPTION_CONTEXT_X19)]
    stp     x20, x21, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    stp     x22, x23, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    stp     x24, x25, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    stp     x26, x27, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    stp     x28, x29, [sp, #(EXCEPTION_CONTEXT_X28_X29)]

    /* Call ams::kern::arch::arm64::HandleException(ams::kern::arch::arm64::KExceptionContext *) */
    mov     x0,  sp
    bl      _ZN3ams4kern4arch5arm6415HandleExceptionEPNS2_17KExceptionContextE

    /* Restore registers. */
    ldp     x30, x8,  [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    ldp     x9,  x10, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    ldr     x11,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
    /* Since we're returning from an SVC, make sure SPSR.SS is cleared so that if we're single-stepping we break instantly on the instruction after the SVC. */
    bic     x10, x10, #(1 << 21)
    #endif

    msr     sp_el0, x8
    msr     elr_el1, x9
    msr     spsr_el1, x10
    msr     tpidr_el0, x11
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

    /* Return. */
    add     sp, sp, #(EXCEPTION_CONTEXT_SIZE)
    eret

4:  /* Return from SVC. */
    /* Clear our in-SVC note. */
    strb    wzr, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_IS_CALLING_SVC)]

    /* If we should, trace the svc exit. */
#if defined(MESOSPHERE_BUILD_FOR_TRACING)
    sub     sp, sp, #0x40
    stp     x0,  x1,  [sp, #(8 *  0)]
    stp     x2,  x3,  [sp, #(8 *  2)]
    stp     x4,  x5,  [sp, #(8 *  4)]
    stp     x6,  x7,  [sp, #(8 *  6)]
    mov     x0, sp
    bl      _ZN3ams4kern3svc12TraceSvcExitEPKm
    ldp     x0,  x1,  [sp, #(8 *  0)]
    ldp     x2,  x3,  [sp, #(8 *  2)]
    ldp     x4,  x5,  [sp, #(8 *  4)]
    ldp     x6,  x7,  [sp, #(8 *  6)]
    add     sp, sp, #0x40
#endif

    /* Restore registers. */
    ldp     x30, x8,  [sp, #(EXCEPTION_CONTEXT_X30_SP)]
    ldp     x9,  x10, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    ldr     x11,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]
    ldr     x18,      [sp, #(EXCEPTION_CONTEXT_X18)]

    #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
    /* Since we're returning from an SVC, make sure SPSR.SS is cleared so that if we're single-stepping we break instantly on the instruction after the SVC. */
    bic     x10, x10, #(1 << 21)
    #endif

    msr     sp_el0, x8
    msr     elr_el1, x9
    msr     spsr_el1, x10
    msr     tpidr_el0, x11

    /* Clear registers. */
    mov     x8,  xzr
    mov     x9,  xzr
    mov     x10, xzr
    mov     x11, xzr
    mov     x12, xzr
    mov     x13, xzr
    mov     x14, xzr
    mov     x15, xzr
    mov     x16, xzr
    mov     x17, xzr

    /* Return. */
    add     sp, sp, #(EXCEPTION_CONTEXT_SIZE)
    eret

/* ams::kern::arch::arm64::SvcHandler32() */
.section    .text._ZN3ams4kern4arch5arm6412SvcHandler32Ev, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6412SvcHandler32Ev
.type       _ZN3ams4kern4arch5arm6412SvcHandler32Ev, %function
_ZN3ams4kern4arch5arm6412SvcHandler32Ev:
    /* Ensure that our registers are 32-bit. */
    mov     w0, w0
    mov     w1, w1
    mov     w2, w2
    mov     w3, w3
    mov     w4, w4
    mov     w5, w5
    mov     w6, w6
    mov     w7, w7

    /* Create a KExceptionContext for the exception. */
    sub     sp, sp, #(EXCEPTION_CONTEXT_SIZE)

    /* Save system registers */
    mrs     x17, elr_el1
    mrs     x20, spsr_el1
    mrs     x19, tpidr_el0
    ldr     x18, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CUR_THREAD)]
    stp     x17, x20, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    str     x19,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    /* Save registers. */
    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    stp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    stp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    stp     x14, xzr, [sp, #(EXCEPTION_CONTEXT_X14_X15)]

    /* Check if the SVC index is out of range. */
    mrs     x16, esr_el1
    and     x16, x16, #0xFF
    cmp     x16, #(AMS_KERN_NUM_SUPERVISOR_CALLS)
    b.ge    3f

    /* Check the specific SVC permission bit for allowal. */
    mov     x20, sp
    add     x20, x20, x16, lsr#3
    ldrb    w20, [x20, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_SVC_PERMISSION)]
    and     x17, x16, #0x7
    lsr     x17, x20, x17
    tst     x17, #1
    b.eq    3f

    /* Check if our disable count allows us to call SVCs. */
    mrs     x15, tpidrro_el0
    ldrh    w15, [x15, #(THREAD_LOCAL_REGION_DISABLE_COUNT)]
    cbz     w15, 1f

    /* It might not, so check the stack params to see if we must not allow the SVC. */
    ldrb    w15, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_IS_PINNED)]
    cbz     w15, 3f

1:  /* We can call the SVC. */
    adr     x15, _ZN3ams4kern3svc16SvcTable64From32E
    ldr     x19, [x15, x16, lsl#3]
    cbz     x19, 3f

    /* Note that we're calling the SVC. */
    mov     w15, #1
    strb    w15, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_IS_CALLING_SVC)]
    strb    w16, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_CURRENT_SVC_ID)]

    /* If we should, trace the svc entry. */
#if defined(MESOSPHERE_BUILD_FOR_TRACING)
    sub     sp, sp, #0x50
    stp     x0,  x1,  [sp, #(8 *  0)]
    stp     x2,  x3,  [sp, #(8 *  2)]
    stp     x4,  x5,  [sp, #(8 *  4)]
    stp     x6,  x7,  [sp, #(8 *  6)]
    str     x19,      [sp, #(8 *  8)]
    mov     x0, sp
    bl      _ZN3ams4kern3svc13TraceSvcEntryEPKm
    ldp     x0,  x1,  [sp, #(8 *  0)]
    ldp     x2,  x3,  [sp, #(8 *  2)]
    ldp     x4,  x5,  [sp, #(8 *  4)]
    ldp     x6,  x7,  [sp, #(8 *  6)]
    ldr     x19,      [sp, #(8 *  8)]
    add     sp, sp, #0x50
#endif

    /* Invoke the SVC handler. */
    msr     daifclr, #2
    blr     x19
    msr     daifset, #2

2:  /* We completed the SVC, and we should handle DPC. */
    /* Check the dpc flags. */
    ldrb    w16, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_DPC_FLAGS)]
    cbz     w16, 4f

    /* We have DPC to do! */
    /* Save registers and call ams::kern::KDpcManager::HandleDpc(). */
    sub     sp, sp, #0x20
    stp     w0,  w1,  [sp, #(4 *  0)]
    stp     w2,  w3,  [sp, #(4 *  2)]
    stp     w4,  w5,  [sp, #(4 *  4)]
    stp     w6,  w7,  [sp, #(4 *  6)]
    bl      _ZN3ams4kern11KDpcManager9HandleDpcEv
    ldp     w0,  w1,  [sp, #(4 *  0)]
    ldp     w2,  w3,  [sp, #(4 *  2)]
    ldp     w4,  w5,  [sp, #(4 *  4)]
    ldp     w6,  w7,  [sp, #(4 *  6)]
    add     sp, sp, #0x20
    b 2b

3:  /* Invalid SVC. */
    /* Setup the context to call into HandleException. */
    stp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    stp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    stp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    stp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X16_X17)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X18_X19)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X20_X21)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X22_X23)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X24_X25)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X26_X27)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X28_X29)]
    stp     xzr, xzr, [sp, #(EXCEPTION_CONTEXT_X30_SP)]

    /* Call ams::kern::arch::arm64::HandleException(ams::kern::arch::arm64::KExceptionContext *) */
    mov     x0,  sp
    bl      _ZN3ams4kern4arch5arm6415HandleExceptionEPNS2_17KExceptionContextE

    /* Restore registers. */
    ldp     x17, x20, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    ldr     x19,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
    /* Since we're returning from an SVC, make sure SPSR.SS is cleared so that if we're single-stepping we break instantly on the instruction after the SVC. */
    bic     x20, x20, #(1 << 21)
    #endif

    msr     elr_el1, x17
    msr     spsr_el1, x20
    msr     tpidr_el0, x19
    ldp     x0,  x1,  [sp, #(EXCEPTION_CONTEXT_X0_X1)]
    ldp     x2,  x3,  [sp, #(EXCEPTION_CONTEXT_X2_X3)]
    ldp     x4,  x5,  [sp, #(EXCEPTION_CONTEXT_X4_X5)]
    ldp     x6,  x7,  [sp, #(EXCEPTION_CONTEXT_X6_X7)]
    ldp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    ldp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    ldp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    ldp     x14, x15, [sp, #(EXCEPTION_CONTEXT_X14_X15)]

    /* Return. */
    add     sp, sp, #(EXCEPTION_CONTEXT_SIZE)
    eret

4:  /* Return from SVC. */
    /* Clear our in-SVC note. */
    strb    wzr, [sp, #(EXCEPTION_CONTEXT_SIZE + THREAD_STACK_PARAMETERS_IS_CALLING_SVC)]

    /* If we should, trace the svc exit. */
#if defined(MESOSPHERE_BUILD_FOR_TRACING)
    sub     sp, sp, #0x40
    stp     x0,  x1,  [sp, #(8 *  0)]
    stp     x2,  x3,  [sp, #(8 *  2)]
    stp     x4,  x5,  [sp, #(8 *  4)]
    stp     x6,  x7,  [sp, #(8 *  6)]
    mov     x0, sp
    bl      _ZN3ams4kern3svc12TraceSvcExitEPKm
    ldp     x0,  x1,  [sp, #(8 *  0)]
    ldp     x2,  x3,  [sp, #(8 *  2)]
    ldp     x4,  x5,  [sp, #(8 *  4)]
    ldp     x6,  x7,  [sp, #(8 *  6)]
    add     sp, sp, #0x40
#endif

    /* Restore registers. */
    ldp     x8,  x9,  [sp, #(EXCEPTION_CONTEXT_X8_X9)]
    ldp     x10, x11, [sp, #(EXCEPTION_CONTEXT_X10_X11)]
    ldp     x12, x13, [sp, #(EXCEPTION_CONTEXT_X12_X13)]
    ldp     x14, xzr, [sp, #(EXCEPTION_CONTEXT_X14_X15)]
    ldp     x17, x20, [sp, #(EXCEPTION_CONTEXT_PC_PSR)]
    ldr     x19,      [sp, #(EXCEPTION_CONTEXT_TPIDR)]

    #if defined(MESOSPHERE_ENABLE_HARDWARE_SINGLE_STEP)
    /* Since we're returning from an SVC, make sure SPSR.SS is cleared so that if we're single-stepping we break instantly on the instruction after the SVC. */
    bic     x20, x20, #(1 << 21)
    #endif

    msr     elr_el1, x17
    msr     spsr_el1, x20
    msr     tpidr_el0, x19

    /* Return. */
    add     sp, sp, #(EXCEPTION_CONTEXT_SIZE)
    eret
