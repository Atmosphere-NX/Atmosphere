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

/* ams::kern::arch::arm64::UserspaceAccessFunctionAreaBegin() */
.section    .text._ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv
.type       _ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */

/*  ================ All Userspace Access Functions after this line. ================ */

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryFromUser(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess18CopyMemoryFromUserEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess18CopyMemoryFromUserEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess18CopyMemoryFromUserEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess18CopyMemoryFromUserEPvPKvm:
    /* Check if there's anything to copy. */
    cmp     x2, #0
    b.eq    2f

    /* Keep track of the last address. */
    add     x3, x1, x2

1:  /* We're copying memory byte-by-byte. */
    ldtrb   w2, [x1]
    strb    w2, [x0], #1
    add     x1, x1, #1
    cmp     x1, x3
    b.ne    1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryFromUserAligned32Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned32BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned32BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned32BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned32BitEPvPKvm:
    /* Check if there are 0x40 bytes to copy */
    cmp     x2, #0x3F
    b.ls    1f
    ldtr    x4,  [x1, #0x00]
    ldtr    x5,  [x1, #0x08]
    ldtr    x6,  [x1, #0x10]
    ldtr    x7,  [x1, #0x18]
    ldtr    x8,  [x1, #0x20]
    ldtr    x9,  [x1, #0x28]
    ldtr    x10, [x1, #0x30]
    ldtr    x11, [x1, #0x38]
    stp     x4,  x5,  [x0, #0x00]
    stp     x6,  x7,  [x0, #0x10]
    stp     x8,  x9,  [x0, #0x20]
    stp     x10, x11, [x0, #0x30]
    add     x0, x0, #0x40
    add     x1, x1, #0x40
    sub     x2, x2, #0x40
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned32BitEPvPKvm

1:  /* We have less than 0x40 bytes to copy. */
    cmp     x2, #0
    b.eq    2f
    ldtr    w4, [x1]
    str     w4, [x0], #4
    add     x1, x1, #4
    sub     x2, x2, #4
    b       1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryFromUserAligned64Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned64BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned64BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned64BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned64BitEPvPKvm:
    /* Check if there are 0x40 bytes to copy */
    cmp     x2, #0x3F
    b.ls    1f
    ldtr    x4,  [x1, #0x00]
    ldtr    x5,  [x1, #0x08]
    ldtr    x6,  [x1, #0x10]
    ldtr    x7,  [x1, #0x18]
    ldtr    x8,  [x1, #0x20]
    ldtr    x9,  [x1, #0x28]
    ldtr    x10, [x1, #0x30]
    ldtr    x11, [x1, #0x38]
    stp     x4,  x5,  [x0, #0x00]
    stp     x6,  x7,  [x0, #0x10]
    stp     x8,  x9,  [x0, #0x20]
    stp     x10, x11, [x0, #0x30]
    add     x0, x0, #0x40
    add     x1, x1, #0x40
    sub     x2, x2, #0x40
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess30CopyMemoryFromUserAligned64BitEPvPKvm

1:  /* We have less than 0x40 bytes to copy. */
    cmp     x2, #0
    b.eq    2f
    ldtr    x4, [x1]
    str     x4, [x0], #8
    add     x1, x1, #8
    sub     x2, x2, #8
    b       1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryFromUserSize32Bit(void *dst, const void *src) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess27CopyMemoryFromUserSize32BitEPvPKv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess27CopyMemoryFromUserSize32BitEPvPKv
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess27CopyMemoryFromUserSize32BitEPvPKv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess27CopyMemoryFromUserSize32BitEPvPKv:
    /* Just load and store a u32. */
    ldtr    w2, [x1]
    str     w2, [x0]

    /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyStringFromUser(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess18CopyStringFromUserEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess18CopyStringFromUserEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess18CopyStringFromUserEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess18CopyStringFromUserEPvPKvm:
    /* Check if there's anything to copy. */
    cmp     x2, #0
    b.eq    3f

    /* Keep track of the start address and last address. */
    mov     x4, x1
    add     x3, x1, x2

1:  /* We're copying memory byte-by-byte. */
    ldtrb   w2, [x1]
    strb    w2, [x0], #1
    add     x1, x1, #1

    /* If we read a null terminator, we're done. */
    cmp     w2, #0
    b.eq    2f

    /* Check if we're done. */
    cmp     x1, x3
    b.ne    1b

2:  /* We're done, and we copied some amount of data from the string. */
    sub     x0, x1, x4
    ret

3:  /* We're done, and there was no string data. */
    mov     x0, #0
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryToUser(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess16CopyMemoryToUserEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess16CopyMemoryToUserEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess16CopyMemoryToUserEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess16CopyMemoryToUserEPvPKvm:
    /* Check if there's anything to copy. */
    cmp     x2, #0
    b.eq    2f

    /* Keep track of the last address. */
    add     x3, x1, x2

1:  /* We're copying memory byte-by-byte. */
    ldrb    w2, [x1], #1
    sttrb   w2, [x0]
    add     x0, x0, #1
    cmp     x1, x3
    b.ne    1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryToUserAligned32Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned32BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned32BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned32BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned32BitEPvPKvm:
    /* Check if there are 0x40 bytes to copy */
    cmp     x2, #0x3F
    b.ls    1f
    ldp     x4,  x5,  [x1, #0x00]
    ldp     x6,  x7,  [x1, #0x10]
    ldp     x8,  x9,  [x1, #0x20]
    ldp     x10, x11, [x1, #0x30]
    sttr    x4,  [x0, #0x00]
    sttr    x5,  [x0, #0x08]
    sttr    x6,  [x0, #0x10]
    sttr    x7,  [x0, #0x18]
    sttr    x8,  [x0, #0x20]
    sttr    x9,  [x0, #0x28]
    sttr    x10, [x0, #0x30]
    sttr    x11, [x0, #0x38]
    add     x0, x0, #0x40
    add     x1, x1, #0x40
    sub     x2, x2, #0x40
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned32BitEPvPKvm

1:  /* We have less than 0x40 bytes to copy. */
    cmp     x2, #0
    b.eq    2f
    ldr     w4, [x1], #4
    sttr    w4, [x0]
    add     x0, x0, #4
    sub     x2, x2, #4
    b       1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryToUserAligned64Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned64BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned64BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned64BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned64BitEPvPKvm:
    /* Check if there are 0x40 bytes to copy */
    cmp     x2, #0x3F
    b.ls    1f
    ldp     x4,  x5,  [x1, #0x00]
    ldp     x6,  x7,  [x1, #0x10]
    ldp     x8,  x9,  [x1, #0x20]
    ldp     x10, x11, [x1, #0x30]
    sttr    x4,  [x0, #0x00]
    sttr    x5,  [x0, #0x08]
    sttr    x6,  [x0, #0x10]
    sttr    x7,  [x0, #0x18]
    sttr    x8,  [x0, #0x20]
    sttr    x9,  [x0, #0x28]
    sttr    x10, [x0, #0x30]
    sttr    x11, [x0, #0x38]
    add     x0, x0, #0x40
    add     x1, x1, #0x40
    sub     x2, x2, #0x40
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned64BitEPvPKvm

1:  /* We have less than 0x40 bytes to copy. */
    cmp     x2, #0
    b.eq    2f
    ldr     x4, [x1], #8
    sttr    x4, [x0]
    add     x0, x0, #8
    sub     x2, x2, #8
    b       1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryToUserSize32Bit(void *dst, const void *src) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess25CopyMemoryToUserSize32BitEPvPKv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess25CopyMemoryToUserSize32BitEPvPKv
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess25CopyMemoryToUserSize32BitEPvPKv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess25CopyMemoryToUserSize32BitEPvPKv:
    /* Just load and store a u32. */
    ldr     w2, [x1]
    sttr    w2, [x0]

    /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::CopyStringToUser(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess16CopyStringToUserEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess16CopyStringToUserEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess16CopyStringToUserEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess16CopyStringToUserEPvPKvm:
    /* Check if there's anything to copy. */
    cmp     x2, #0
    b.eq    3f

    /* Keep track of the start address and last address. */
    mov     x4, x1
    add     x3, x1, x2

1:  /* We're copying memory byte-by-byte. */
    ldrb    w2, [x1], #1
    sttrb   w2, [x0]
    add     x0, x0, #1

    /* If we read a null terminator, we're done. */
    cmp     w2, #0
    b.eq    2f

    /* Check if we're done. */
    cmp     x1, x3
    b.ne    1b

2:  /* We're done, and we copied some amount of data from the string. */
    sub     x0, x1, x4
    ret

3:  /* We're done, and there was no string data. */
    mov     x0, #0
    ret

/* ams::kern::arch::arm64::UserspaceAccess::ClearMemory(void *dst, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess11ClearMemoryEPvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess11ClearMemoryEPvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess11ClearMemoryEPvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess11ClearMemoryEPvm:
    /* Check if there's anything to clear. */
    cmp     x1, #0
    b.eq    2f

    /* Keep track of the last address. */
    add     x2, x0, x1

1:  /* We're copying memory byte-by-byte. */
    sttrb   wzr, [x0]
    add     x0, x0, #1
    cmp     x0, x2
    b.ne    1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::ClearMemoryAligned32Bit(void *dst, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned32BitEPvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned32BitEPvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned32BitEPvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned32BitEPvm:
    /* Check if there are 0x40 bytes to clear. */
    cmp     x1, #0x3F
    b.ls    2f
    sttr    xzr, [x0, #0x00]
    sttr    xzr, [x0, #0x08]
    sttr    xzr, [x0, #0x10]
    sttr    xzr, [x0, #0x18]
    sttr    xzr, [x0, #0x20]
    sttr    xzr, [x0, #0x28]
    sttr    xzr, [x0, #0x30]
    sttr    xzr, [x0, #0x38]
    add     x0, x0, #0x40
    sub     x1, x1, #0x40
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned32BitEPvm

1:  /* We have less than 0x40 bytes to clear. */
    cmp     x1, #0
    b.eq    2f
    sttr    wzr, [x0]
    add     x0, x0, #4
    sub     x1, x1, #4
    b       1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::ClearMemoryAligned64Bit(void *dst, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned64BitEPvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned64BitEPvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned64BitEPvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned64BitEPvm:
    /* Check if there are 0x40 bytes to clear. */
    cmp     x1, #0x3F
    b.ls    2f
    sttr    xzr, [x0, #0x00]
    sttr    xzr, [x0, #0x08]
    sttr    xzr, [x0, #0x10]
    sttr    xzr, [x0, #0x18]
    sttr    xzr, [x0, #0x20]
    sttr    xzr, [x0, #0x28]
    sttr    xzr, [x0, #0x30]
    sttr    xzr, [x0, #0x38]
    add     x0, x0, #0x40
    sub     x1, x1, #0x40
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess23ClearMemoryAligned64BitEPvm

1:  /* We have less than 0x40 bytes to clear. */
    cmp     x1, #0
    b.eq    2f
    sttr    xzr, [x0]
    add     x0, x0, #8
    sub     x1, x1, #8
    b       1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::ClearMemorySize32Bit(void *dst) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess20ClearMemorySize32BitEPv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess20ClearMemorySize32BitEPv
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess20ClearMemorySize32BitEPv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess20ClearMemorySize32BitEPv:
    /* Just store a zero. */
    sttr    wzr, [x0]

    /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::UpdateLockAtomic(u32 *out, u32 *address, u32 if_zero, u32 new_orr_mask) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess16UpdateLockAtomicEPjS4_jj, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess16UpdateLockAtomicEPjS4_jj
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess16UpdateLockAtomicEPjS4_jj, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess16UpdateLockAtomicEPjS4_jj:
    /* Load the value from the address. */
    ldaxr   w4, [x1]

    /* Orr in the new mask. */
    orr     w5, w4, w3

    /* If the value is zero, use the if_zero value, otherwise use the newly orr'd value. */
    cmp     w4, wzr
    csel    w5, w2, w5, eq

    /* Try to store. */
    stlxr   w6, w5, [x1]

    /* If we failed to store, try again. */
    cbnz    w6, _ZN3ams4kern4arch5arm6415UserspaceAccess16UpdateLockAtomicEPjS4_jj

    /* We're done. */
    str     w4, [x0]
    mov     x0, #1
    ret


/* ams::kern::arch::arm64::UserspaceAccess::UpdateIfEqualAtomic(s32 *out, s32 *address, s32 compare_value, s32 new_value) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess19UpdateIfEqualAtomicEPiS4_ii, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess19UpdateIfEqualAtomicEPiS4_ii
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess19UpdateIfEqualAtomicEPiS4_ii, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess19UpdateIfEqualAtomicEPiS4_ii:
    /* Load the value from the address. */
    ldaxr   w4, [x1]

    /* Compare it to the desired one. */
    cmp     w4, w2

    /* If equal, we want to try to write the new value. */
    b.eq    1f

    /* Otherwise, clear our exclusive hold and finish. */
    clrex
    b       2f

1:  /* Try to store. */
    stlxr   w5, w3, [x1]

    /* If we failed to store, try again. */
    cbnz    w5, _ZN3ams4kern4arch5arm6415UserspaceAccess19UpdateIfEqualAtomicEPiS4_ii

2:  /* We're done. */
    str     w4, [x0]
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::DecrementIfLessThanAtomic(s32 *out, s32 *address, s32 compare) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess25DecrementIfLessThanAtomicEPiS4_i, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess25DecrementIfLessThanAtomicEPiS4_i
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess25DecrementIfLessThanAtomicEPiS4_i, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess25DecrementIfLessThanAtomicEPiS4_i:
    /* Load the value from the address. */
    ldaxr   w3, [x1]

    /* Compare it to the desired one. */
    cmp     w3, w2

    /* If less than, we want to try to decrement. */
    b.lt    1f

    /* Otherwise, clear our exclusive hold and finish. */
    clrex
    b       2f

1:  /* Decrement and try to store. */
    sub     w4, w3, #1
    stlxr   w5, w4, [x1]

    /* If we failed to store, try again. */
    cbnz    w5, _ZN3ams4kern4arch5arm6415UserspaceAccess25DecrementIfLessThanAtomicEPiS4_i

2:  /* We're done. */
    str     w3, [x0]
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::StoreDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess14StoreDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess14StoreDataCacheEmm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess14StoreDataCacheEmm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess14StoreDataCacheEmm:
    /* Check if we have any work to do. */
    cmp     x1, x0
    b.eq    2f

1:  /* Loop, storing each cache line. */
    dc      cvac, x0
    add     x0, x0, #0x40
    cmp     x1, x0
    b.ne    1b

2:  /* We're done! */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::FlushDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess14FlushDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess14FlushDataCacheEmm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess14FlushDataCacheEmm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess14FlushDataCacheEmm:
    /* Check if we have any work to do. */
    cmp     x1, x0
    b.eq    2f

1:  /* Loop, flushing each cache line. */
    dc      civac, x0
    add     x0, x0, #0x40
    cmp     x1, x0
    b.ne    1b

2:  /* We're done! */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::InvalidateDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess19InvalidateDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess19InvalidateDataCacheEmm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess19InvalidateDataCacheEmm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess19InvalidateDataCacheEmm:
    /* Check if we have any work to do. */
    cmp     x1, x0
    b.eq    2f

1:  /* Loop, invalidating each cache line. */
    dc      ivac, x0
    add     x0, x0, #0x40
    cmp     x1, x0
    b.ne    1b

2:  /* We're done! */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::InvalidateInstructionCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess26InvalidateInstructionCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess26InvalidateInstructionCacheEmm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess26InvalidateInstructionCacheEmm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess26InvalidateInstructionCacheEmm:
    /* Check if we have any work to do. */
    cmp     x1, x0
    b.eq    2f

1:  /* Loop, invalidating each cache line. */
    ic      ivau, x0
    add     x0, x0, #0x40
    cmp     x1, x0
    b.ne    1b

2:  /* We're done! */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::ReadIoMemory32Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess17ReadIoMemory32BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess17ReadIoMemory32BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess17ReadIoMemory32BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess17ReadIoMemory32BitEPvPKvm:
    /* Check if we have any work to do. */
    cmp     x2, #0
    b.eq    3f

    /* Save variables in temporary registers. */
    mov     x4, x0
    mov     x5, x1
    mov     x6, x2
    add     x7, x5, x6

    /* Save our return address. */
    mov     x8, x30

1:  /* Set our return address so that on read failure we continue as though we read -1. */
    adr     x30, 4f

    /* Read the word from io. */
    ldtr    w9, [x5]
    dsb     sy
    nop

2:  /* Restore our return address. */
    mov     x30, x8

    /* Write the value we read. */
    sttr    w9, [x4]

    /* Advance. */
    add     x4, x4, #4
    add     x5, x5, #4
    cmp     x5, x7
    b.ne    1b

3:  /* We're done! */
    mov     x0, #1
    ret

4:  /* We failed to read a value, so continue as though we read -1. */
    mov     w9, #0xFFFFFFFF
    b       2b

/* ams::kern::arch::arm64::UserspaceAccess::ReadIoMemory16Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess17ReadIoMemory16BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess17ReadIoMemory16BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess17ReadIoMemory16BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess17ReadIoMemory16BitEPvPKvm:
    /* Check if we have any work to do. */
    cmp     x2, #0
    b.eq    3f

    /* Save variables in temporary registers. */
    mov     x4, x0
    mov     x5, x1
    mov     x6, x2
    add     x7, x5, x6

    /* Save our return address. */
    mov     x8, x30

1:  /* Set our return address so that on read failure we continue as though we read -1. */
    adr     x30, 4f

    /* Read the word from io. */
    ldtrh   w9, [x5]
    dsb     sy
    nop

2:  /* Restore our return address. */
    mov     x30, x8

    /* Write the value we read. */
    sttrh   w9, [x4]

    /* Advance. */
    add     x4, x4, #2
    add     x5, x5, #2
    cmp     x5, x7
    b.ne    1b

3:  /* We're done! */
    mov     x0, #1
    ret

4:  /* We failed to read a value, so continue as though we read -1. */
    mov     w9, #0xFFFFFFFF
    b       2b

/* ams::kern::arch::arm64::UserspaceAccess::ReadIoMemory8Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess16ReadIoMemory8BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess16ReadIoMemory8BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess16ReadIoMemory8BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess16ReadIoMemory8BitEPvPKvm:
    /* Check if we have any work to do. */
    cmp     x2, #0
    b.eq    3f

    /* Save variables in temporary registers. */
    mov     x4, x0
    mov     x5, x1
    mov     x6, x2
    add     x7, x5, x6

    /* Save our return address. */
    mov     x8, x30

1:  /* Set our return address so that on read failure we continue as though we read -1. */
    adr     x30, 4f

    /* Read the word from io. */
    ldtrb   w9, [x5]
    dsb     sy
    nop

2:  /* Restore our return address. */
    mov     x30, x8

    /* Write the value we read. */
    sttrb   w9, [x4]

    /* Advance. */
    add     x4, x4, #1
    add     x5, x5, #1
    cmp     x5, x7
    b.ne    1b

3:  /* We're done! */
    mov     x0, #1
    ret

4:  /* We failed to read a value, so continue as though we read -1. */
    mov     w9, #0xFFFFFFFF
    b       2b

/* ams::kern::arch::arm64::UserspaceAccess::WriteIoMemory32Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess18WriteIoMemory32BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess18WriteIoMemory32BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess18WriteIoMemory32BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess18WriteIoMemory32BitEPvPKvm:
    /* Check if we have any work to do. */
    cmp     x2, #0
    b.eq    3f

    /* Save variables in temporary registers. */
    mov     x4, x0
    mov     x5, x1
    mov     x6, x2
    add     x7, x5, x6

    /* Save our return address. */
    mov     x8, x30

1:  /* Read the word from normal memory. */
    mov     x30, x8
    ldtr    w9, [x5]

    /* Set our return address so that on read failure we continue. */
    adr     x30, 2f

    /* Write the word to io. */
    sttr    w9, [x5]
    dsb     sy

2:  /* Continue. */
    nop

    /* Advance. */
    add     x4, x4, #4
    add     x5, x5, #4
    cmp     x5, x7
    b.ne    1b

3:  /* We're done! */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::WriteIoMemory16Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess18WriteIoMemory16BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess18WriteIoMemory16BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess18WriteIoMemory16BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess18WriteIoMemory16BitEPvPKvm:
    /* Check if we have any work to do. */
    cmp     x2, #0
    b.eq    3f

    /* Save variables in temporary registers. */
    mov     x4, x0
    mov     x5, x1
    mov     x6, x2
    add     x7, x5, x6

    /* Save our return address. */
    mov     x8, x30

1:  /* Read the word from normal memory. */
    mov     x30, x8
    ldtrh   w9, [x5]

    /* Set our return address so that on read failure we continue. */
    adr     x30, 2f

    /* Write the word to io. */
    sttrh   w9, [x5]
    dsb     sy

2:  /* Continue. */
    nop

    /* Advance. */
    add     x4, x4, #2
    add     x5, x5, #2
    cmp     x5, x7
    b.ne    1b

3:  /* We're done! */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::WriteIoMemory8Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess17WriteIoMemory8BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess17WriteIoMemory8BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess17WriteIoMemory8BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess17WriteIoMemory8BitEPvPKvm:
    /* Check if we have any work to do. */
    cmp     x2, #0
    b.eq    3f

    /* Save variables in temporary registers. */
    mov     x4, x0
    mov     x5, x1
    mov     x6, x2
    add     x7, x5, x6

    /* Save our return address. */
    mov     x8, x30

1:  /* Read the word from normal memory. */
    mov     x30, x8
    ldtrb   w9, [x5]

    /* Set our return address so that on read failure we continue. */
    adr     x30, 2f

    /* Write the word to io. */
    sttrb   w9, [x5]
    dsb     sy

2:  /* Continue. */
    nop

    /* Advance. */
    add     x4, x4, #1
    add     x5, x5, #1
    cmp     x5, x7
    b.ne    1b

3:  /* We're done! */
    mov     x0, #1
    ret

/*  ================ All Userspace Access Functions before this line. ================ */

/* ams::kern::arch::arm64::UserspaceAccessFunctionAreaEnd() */
.section    .text._ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv
.type       _ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */