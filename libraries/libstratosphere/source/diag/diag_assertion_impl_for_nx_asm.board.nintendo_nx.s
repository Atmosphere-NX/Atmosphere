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

/* ams::diag::impl::FatalErrorByResultForNx(Result value) */
.section    .text._ZN3ams4diag4impl23FatalErrorByResultForNxENS_6ResultE, "ax", %progbits
.global     _ZN3ams4diag4impl23FatalErrorByResultForNxENS_6ResultE
.type       _ZN3ams4diag4impl23FatalErrorByResultForNxENS_6ResultE, %function
.balign 0x10
_ZN3ams4diag4impl23FatalErrorByResultForNxENS_6ResultE:
    /* Save x27/x28. */
    stp     x27, x28, [sp, #-0x10]!
    stp     x0,  xzr, [sp, #-0x10]!

    /* Inline ams::diag::impl::PrepareAbort() */
    mrs     x27, tpidrro_el0
    strh    wzr, [x27, #0x100]
    ldrh    w27, [x27, #0x102]
    cbz     w27, 0f
    svc     #0x36

0:  /* Restore the value from stack. */
    ldr     x0, [sp]
    add     sp, sp, #0x10

    /* Put magic std::abort values into x27/x28. */
    mov     x28, #0xcafe
    movk    x28, #0xdead, lsl#16
    movk    x28, #0xf00d, lsl#32
    movk    x28, #0xa55a, lsl#48
    mov     x27, #8

    /* Abort */
1:
    str     x28, [x27]
    nop
    b       1b
