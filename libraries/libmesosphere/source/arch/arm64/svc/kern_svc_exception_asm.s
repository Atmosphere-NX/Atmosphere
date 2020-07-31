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

/* ams::kern::svc::CallReturnFromException64(Result result) */
.section    .text._ZN3ams4kern3svc25CallReturnFromException64Ev, "ax", %progbits
.global     _ZN3ams4kern3svc25CallReturnFromException64Ev
.type       _ZN3ams4kern3svc25CallReturnFromException64Ev, %function
_ZN3ams4kern3svc25CallReturnFromException64Ev:
    /* Save registers the SVC entry handler didn't. */
    stp     x12, x13, [sp, #(8 * 12)]
    stp     x14, x15, [sp, #(8 * 14)]
    stp     x16, x17, [sp, #(8 * 16)]
    str     x19,      [sp, #(8 * 19)]
    stp     x20, x21, [sp, #(8 * 20)]
    stp     x22, x23, [sp, #(8 * 22)]
    stp     x24, x25, [sp, #(8 * 24)]
    stp     x26, x26, [sp, #(8 * 26)]
    stp     x28, x29, [sp, #(8 * 28)]

    /* Call ams::kern::arch::arm64::ReturnFromException(result). */
    bl      _ZN3ams4kern4arch5arm6419ReturnFromExceptionENS_6ResultE

0:  /* We should never reach this point. */
    b       0b

/* ams::kern::svc::CallReturnFromException64From32(Result result) */
.section    .text._ZN3ams4kern3svc31CallReturnFromException64From32Ev, "ax", %progbits
.global     _ZN3ams4kern3svc31CallReturnFromException64From32Ev
.type       _ZN3ams4kern3svc31CallReturnFromException64From32Ev, %function
_ZN3ams4kern3svc31CallReturnFromException64From32Ev:
    /* Save registers the SVC entry handler didn't. */
    /* ... */

    /* Call ams::kern::arch::arm64::ReturnFromException(result). */
    bl      _ZN3ams4kern4arch5arm6419ReturnFromExceptionENS_6ResultE

0:  /* We should never reach this point. */
    b       0b


/* ams::kern::svc::RestoreContext(uintptr_t sp) */
.section    .text._ZN3ams4kern3svc14RestoreContextEm, "ax", %progbits
.global     _ZN3ams4kern3svc14RestoreContextEm
.type       _ZN3ams4kern3svc14RestoreContextEm, %function
_ZN3ams4kern3svc14RestoreContextEm:
    /* Set the stack pointer, set daif. */
    mov     sp, x0
    msr     daifset, #2

0:  /* We should handle DPC. */
    /* Check the dpc flags. */
    ldrb    w8, [sp, #(0x120 + 0x10)]
    cbz     w8, 1f

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
    b 0b

1:  /* We're done with DPC, and should return from the svc. */
    /* Clear our in-SVC note. */
    strb    wzr, [sp, #(0x120 + 0x12)]

    /* Restore registers. */
    ldp     x30, x8,  [sp, #(8 * 30)]
    ldp     x9,  x10, [sp, #(8 * 32)]
    ldr     x11,      [sp, #(8 * 34)]
    msr     sp_el0, x8
    msr     elr_el1, x9
    msr     spsr_el1, x10
    msr     tpidr_el0, x11
    ldp     x0,  x1,  [sp, #(8 *  0)]
    ldp     x2,  x3,  [sp, #(8 *  2)]
    ldp     x4,  x5,  [sp, #(8 *  4)]
    ldp     x6,  x7,  [sp, #(8 *  6)]
    ldp     x8,  x9,  [sp, #(8 *  8)]
    ldp     x10, x11, [sp, #(8 * 10)]
    ldp     x12, x13, [sp, #(8 * 12)]
    ldp     x14, x15, [sp, #(8 * 14)]
    ldp     x16, x17, [sp, #(8 * 16)]
    ldp     x18, x19, [sp, #(8 * 18)]
    ldp     x20, x21, [sp, #(8 * 20)]
    ldp     x22, x23, [sp, #(8 * 22)]
    ldp     x24, x25, [sp, #(8 * 24)]
    ldp     x26, x27, [sp, #(8 * 26)]
    ldp     x28, x29, [sp, #(8 * 28)]

    /* Return. */
    add     sp, sp, #0x120
    eret
