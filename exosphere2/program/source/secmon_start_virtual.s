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

.section    .text._ZN3ams6secmon5StartEv, "ax", %progbits
.align      6
.global     _ZN3ams6secmon5StartEv
_ZN3ams6secmon5StartEv:
    /* Set SPSEL 1 stack pointer to the core 0 exception stack address. */
    msr  spsel, #1
    ldr  x20, =0x1F01F6F00
    mov  sp, x20

    /* Set SPSEL 0 stack pointer to a temporary location in volatile memory. */
    msr  spsel, #1
    ldr  x20, =0x1F01C0800
    mov  sp, x20

    /* Setup X18 to point to the global context. */
    ldr  x18, =0x1F01FA000

    /* Invoke main. */
    bl _ZN3ams6secmon4MainEv

    /* Set the stack pointer to the core 3 exception stack address. */
    ldr  x20, =0x1F01FB000
    mov  sp, x20

    /* TODO: JumpToLowerExceptionLevel */
    1: b 1b

.section    .text._ZN3ams6secmon25AcquireCommonSmcStackLockEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon25AcquireCommonSmcStackLockEv
_ZN3ams6secmon25AcquireCommonSmcStackLockEv:
    /* Get the address of the lock. */
    ldr x0, =_ZN3ams6secmon18CommonSmcStackLockE

    /* Prepare to try to take the spinlock. */
    mov w1, #1
    sevl
    prfm pstl1keep, [x0]

1:  /* Repeatedly try to take the lock. */
    wfe
    ldaxr w2, [x0]
    cbnz  w2, 1b
    stxr  w2, w1, [x0]
    cbnz  w2, 1b

    /* Return. */
    ret

.section    .text._ZN3ams6secmon25ReleaseCommonSmcStackLockEv, "ax", %progbits
.align      4
.global     _ZN3ams6secmon25ReleaseCommonSmcStackLockEv
_ZN3ams6secmon25ReleaseCommonSmcStackLockEv:
    /* Get the address of the lock. */
    ldr x0, =_ZN3ams6secmon18CommonSmcStackLockE

    /* Release the spinlock. */
    stlr wzr, [x0]

    /* Return. */
    ret

.section    .data._ZN3ams6secmon18CommonSmcStackLockE, "aw", %progbits
.global     _ZN3ams6secmon18CommonSmcStackLockE
_ZN3ams6secmon18CommonSmcStackLockE:
    /* Define storage for the global common smc stack spinlock. */
    .word 0
