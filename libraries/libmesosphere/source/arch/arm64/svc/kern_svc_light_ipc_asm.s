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

/* ams::kern::svc::CallSendSyncRequestLight64() */
.section    .text._ZN3ams4kern3svc26CallSendSyncRequestLight64Ev, "ax", %progbits
.global     _ZN3ams4kern3svc26CallSendSyncRequestLight64Ev
.type       _ZN3ams4kern3svc26CallSendSyncRequestLight64Ev, %function
_ZN3ams4kern3svc26CallSendSyncRequestLight64Ev:
    /* Allocate space for the light ipc data. */
    sub     sp, sp, #(4 * 8)

    /* Store the light ipc data. */
    stp     w1, w2, [sp, #(4 * 0)]
    stp     w3, w4, [sp, #(4 * 2)]
    stp     w5, w6, [sp, #(4 * 4)]
    str     w7,     [sp, #(4 * 6)]

    /* Invoke the svc handler. */
    mov     x1, sp
    stp     x29, x30, [sp, #-16]!
    bl      _ZN3ams4kern3svc22SendSyncRequestLight64EjPj
    ldp     x29, x30, [sp], #16

    /* Load the light ipc data. */
    ldp     w1, w2, [sp, #(4 * 0)]
    ldp     w3, w4, [sp, #(4 * 2)]
    ldp     w5, w6, [sp, #(4 * 4)]
    ldr     w7,     [sp, #(4 * 6)]

    /* Free the stack space for the light ipc data. */
    add     sp, sp, #(4 * 8)

    ret

/* ams::kern::svc::CallSendSyncRequestLight64From32() */
.section    .text._ZN3ams4kern3svc32CallSendSyncRequestLight64From32Ev, "ax", %progbits
.global     _ZN3ams4kern3svc32CallSendSyncRequestLight64From32Ev
.type       _ZN3ams4kern3svc32CallSendSyncRequestLight64From32Ev, %function
_ZN3ams4kern3svc32CallSendSyncRequestLight64From32Ev:
    /* Load x4-x7 from where the svc handler stores them. */
    ldp     x4, x5, [sp, #(8 * 0)]
    ldp     x6, x7, [sp, #(8 * 2)]

    /* Allocate space for the light ipc data. */
    sub     sp, sp, #(4 * 8)

    /* Store the light ipc data. */
    stp     w1, w2, [sp, #(4 * 0)]
    stp     w3, w4, [sp, #(4 * 2)]
    stp     w5, w6, [sp, #(4 * 4)]
    str     w7,     [sp, #(4 * 6)]

    /* Invoke the svc handler. */
    mov     x1, sp
    stp     x29, x30, [sp, #-16]!
    bl      _ZN3ams4kern3svc28SendSyncRequestLight64From32EjPj
    ldp     x29, x30, [sp], #16

    /* Load the light ipc data. */
    ldp     w1, w2, [sp, #(4 * 0)]
    ldp     w3, w4, [sp, #(4 * 2)]
    ldp     w5, w6, [sp, #(4 * 4)]
    ldr     w7,     [sp, #(4 * 6)]

    /* Free the stack space for the light ipc data. */
    add     sp, sp, #(4 * 8)

    /* Save x4-x7 to where the svc handler stores them. */
    stp     x4, x5, [sp, #(8 * 0)]
    stp     x6, x7, [sp, #(8 * 2)]

    ret


/* ams::kern::svc::CallReplyAndReceiveLight64() */
.section    .text._ZN3ams4kern3svc26CallReplyAndReceiveLight64Ev, "ax", %progbits
.global     _ZN3ams4kern3svc26CallReplyAndReceiveLight64Ev
.type       _ZN3ams4kern3svc26CallReplyAndReceiveLight64Ev, %function
_ZN3ams4kern3svc26CallReplyAndReceiveLight64Ev:
    /* Allocate space for the light ipc data. */
    sub     sp, sp, #(4 * 8)

    /* Store the light ipc data. */
    stp     w1, w2, [sp, #(4 * 0)]
    stp     w3, w4, [sp, #(4 * 2)]
    stp     w5, w6, [sp, #(4 * 4)]
    str     w7,     [sp, #(4 * 6)]

    /* Invoke the svc handler. */
    mov     x1, sp
    stp     x29, x30, [sp, #-16]!
    bl      _ZN3ams4kern3svc22ReplyAndReceiveLight64EjPj
    ldp     x29, x30, [sp], #16

    /* Load the light ipc data. */
    ldp     w1, w2, [sp, #(4 * 0)]
    ldp     w3, w4, [sp, #(4 * 2)]
    ldp     w5, w6, [sp, #(4 * 4)]
    ldr     w7,     [sp, #(4 * 6)]

    /* Free the stack space for the light ipc data. */
    add     sp, sp, #(4 * 8)

    ret

/* ams::kern::svc::CallReplyAndReceiveLight64From32() */
.section    .text._ZN3ams4kern3svc32CallReplyAndReceiveLight64From32Ev, "ax", %progbits
.global     _ZN3ams4kern3svc32CallReplyAndReceiveLight64From32Ev
.type       _ZN3ams4kern3svc32CallReplyAndReceiveLight64From32Ev, %function
_ZN3ams4kern3svc32CallReplyAndReceiveLight64From32Ev:
    /* Load x4-x7 from where the svc handler stores them. */
    ldp     x4, x5, [sp, #(8 * 0)]
    ldp     x6, x7, [sp, #(8 * 2)]

    /* Allocate space for the light ipc data. */
    sub     sp, sp, #(4 * 8)

    /* Store the light ipc data. */
    stp     w1, w2, [sp, #(4 * 0)]
    stp     w3, w4, [sp, #(4 * 2)]
    stp     w5, w6, [sp, #(4 * 4)]
    str     w7,     [sp, #(4 * 6)]

    /* Invoke the svc handler. */
    mov     x1, sp
    stp     x29, x30, [sp, #-16]!
    bl      _ZN3ams4kern3svc28ReplyAndReceiveLight64From32EjPj
    ldp     x29, x30, [sp], #16

    /* Load the light ipc data. */
    ldp     w1, w2, [sp, #(4 * 0)]
    ldp     w3, w4, [sp, #(4 * 2)]
    ldp     w5, w6, [sp, #(4 * 4)]
    ldr     w7,     [sp, #(4 * 6)]

    /* Free the stack space for the light ipc data. */
    add     sp, sp, #(4 * 8)

    /* Save x4-x7 to where the svc handler stores them. */
    stp     x4, x5, [sp, #(8 * 0)]
    stp     x6, x7, [sp, #(8 * 2)]

    ret
