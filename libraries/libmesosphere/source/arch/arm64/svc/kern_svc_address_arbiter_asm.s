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

/* ams::kern::svc::CallWaitForAddress64From32() */
.section    .text._ZN3ams4kern3svc26CallWaitForAddress64From32Ev, "ax", %progbits
.global     _ZN3ams4kern3svc26CallWaitForAddress64From32Ev
.type       _ZN3ams4kern3svc26CallWaitForAddress64From32Ev, %function
_ZN3ams4kern3svc26CallWaitForAddress64From32Ev:
    /* Save LR + callee-save registers. */
    str     x30, [sp, #-16]!
    stp     x6, x7, [sp, #-16]!

    /* Gather the arguments into correct registers. */
    /* NOTE: This has to be manually implemented via asm, */
    /* in order to avoid breaking ABI with pre-19.0.0. */
    orr     x2, x2, x5, lsl#32
    orr     x3, x3, x4, lsl#32

    /* Invoke the svc handler. */
    bl      _ZN3ams4kern3svc22WaitForAddress64From32ENS_3svc7AddressENS2_15ArbitrationTypeEll

    /* Clean up registers. */
    mov     x1, xzr
    mov     x2, xzr
    mov     x3, xzr
    mov     x4, xzr
    mov     x5, xzr

    ldp     x6, x7, [sp], #0x10
    ldr     x30, [sp], #0x10
    ret

/* ams::kern::svc::CallWaitForAddress64() */
.section    .text._ZN3ams4kern3svc20CallWaitForAddress64Ev, "ax", %progbits
.global     _ZN3ams4kern3svc20CallWaitForAddress64Ev
.type       _ZN3ams4kern3svc20CallWaitForAddress64Ev, %function
_ZN3ams4kern3svc20CallWaitForAddress64Ev:
    /* Save LR + FP. */
    stp     x29, x30, [sp, #-16]!

    /* Invoke the svc handler. */
    bl      _ZN3ams4kern3svc22WaitForAddress64From32ENS_3svc7AddressENS2_15ArbitrationTypeEll

    /* Clean up registers. */
    mov     x1, xzr
    mov     x2, xzr
    mov     x3, xzr
    mov     x4, xzr
    mov     x5, xzr
    mov     x6, xzr
    mov     x7, xzr

    ldp     x29, x30, [sp], #0x10
    ret
