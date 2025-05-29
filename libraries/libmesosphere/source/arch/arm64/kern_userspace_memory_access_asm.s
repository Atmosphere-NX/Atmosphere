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

/* ams::kern::arch::arm64::UserspaceAccessFunctionAreaBegin() */
.section    .text._ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv
.type       _ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */

/*  ================ All Userspace Access Functions after this line. ================ */

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryFromUser(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18CopyMemoryFromUserEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18CopyMemoryFromUserEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18CopyMemoryFromUserEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18CopyMemoryFromUserEPvPKvm:
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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryFromUserAligned32Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned32BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned32BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned32BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned32BitEPvPKvm:
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
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned32BitEPvPKvm

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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryFromUserAligned64Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned64BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned64BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned64BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned64BitEPvPKvm:
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
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl30CopyMemoryFromUserAligned64BitEPvPKvm

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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryFromUserSize64Bit(void *dst, const void *src) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl27CopyMemoryFromUserSize64BitEPvPKv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl27CopyMemoryFromUserSize64BitEPvPKv
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl27CopyMemoryFromUserSize64BitEPvPKv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl27CopyMemoryFromUserSize64BitEPvPKv:
    /* Just load and store a u64. */
    ldtr    x2, [x1]
    str     x2, [x0]

    /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryFromUserSize32Bit(void *dst, const void *src) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl27CopyMemoryFromUserSize32BitEPvPKv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl27CopyMemoryFromUserSize32BitEPvPKv
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl27CopyMemoryFromUserSize32BitEPvPKv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl27CopyMemoryFromUserSize32BitEPvPKv:
    /* Just load and store a u32. */
    ldtr    w2, [x1]
    str     w2, [x0]

    /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryFromUserSize32BitWithSupervisorAccess(void *dst, const void *src) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl47CopyMemoryFromUserSize32BitWithSupervisorAccessEPvPKv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl47CopyMemoryFromUserSize32BitWithSupervisorAccessEPvPKv
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl47CopyMemoryFromUserSize32BitWithSupervisorAccessEPvPKv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl47CopyMemoryFromUserSize32BitWithSupervisorAccessEPvPKv:
    /* Just load and store a u32. */
    /* NOTE: This is done with supervisor access permissions. */
    ldr     w2, [x1]
    str     w2, [x0]

    /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyStringFromUser(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18CopyStringFromUserEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18CopyStringFromUserEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18CopyStringFromUserEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18CopyStringFromUserEPvPKvm:
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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryToUser(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16CopyMemoryToUserEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16CopyMemoryToUserEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16CopyMemoryToUserEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16CopyMemoryToUserEPvPKvm:
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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryToUserAligned32Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned32BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned32BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned32BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned32BitEPvPKvm:
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
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned32BitEPvPKvm

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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryToUserAligned64Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned64BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned64BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned64BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned64BitEPvPKvm:
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
    b       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl28CopyMemoryToUserAligned64BitEPvPKvm

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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyMemoryToUserSize32Bit(void *dst, const void *src) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25CopyMemoryToUserSize32BitEPvPKv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25CopyMemoryToUserSize32BitEPvPKv
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25CopyMemoryToUserSize32BitEPvPKv, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25CopyMemoryToUserSize32BitEPvPKv:
    /* Just load and store a u32. */
    ldr     w2, [x1]
    sttr    w2, [x0]

    /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::Impl::CopyStringToUser(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16CopyStringToUserEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16CopyStringToUserEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16CopyStringToUserEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16CopyStringToUserEPvPKvm:
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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::UpdateLockAtomic(u32 *out, u32 *address, u32 if_zero, u32 new_orr_mask) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16UpdateLockAtomicEPjS5_jj, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16UpdateLockAtomicEPjS5_jj
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16UpdateLockAtomicEPjS5_jj, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16UpdateLockAtomicEPjS5_jj:
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
    cbnz    w6, _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16UpdateLockAtomicEPjS5_jj

    /* We're done. */
    str     w4, [x0]
    mov     x0, #1
    ret


/* ams::kern::arch::arm64::UserspaceAccess::Impl::UpdateIfEqualAtomic(s32 *out, s32 *address, s32 compare_value, s32 new_value) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19UpdateIfEqualAtomicEPiS5_ii, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19UpdateIfEqualAtomicEPiS5_ii
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19UpdateIfEqualAtomicEPiS5_ii, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19UpdateIfEqualAtomicEPiS5_ii:
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
    cbnz    w5, _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19UpdateIfEqualAtomicEPiS5_ii

2:  /* We're done. */
    str     w4, [x0]
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::Impl::DecrementIfLessThanAtomic(s32 *out, s32 *address, s32 compare) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25DecrementIfLessThanAtomicEPiS5_i, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25DecrementIfLessThanAtomicEPiS5_i
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25DecrementIfLessThanAtomicEPiS5_i, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25DecrementIfLessThanAtomicEPiS5_i:
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
    cbnz    w5, _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl25DecrementIfLessThanAtomicEPiS5_i

2:  /* We're done. */
    str     w3, [x0]
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::Impl::StoreDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl14StoreDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl14StoreDataCacheEmm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl14StoreDataCacheEmm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl14StoreDataCacheEmm:
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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::FlushDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl14FlushDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl14FlushDataCacheEmm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl14FlushDataCacheEmm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl14FlushDataCacheEmm:
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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::InvalidateDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19InvalidateDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19InvalidateDataCacheEmm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19InvalidateDataCacheEmm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl19InvalidateDataCacheEmm:
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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::ReadIoMemory32Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17ReadIoMemory32BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17ReadIoMemory32BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17ReadIoMemory32BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17ReadIoMemory32BitEPvPKvm:
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

    /* Prepare return address for read failure. */
    adr     x10, 4f

1:  /* Set our return address so that on read failure we continue as though we read -1. */
    mov     x30, x10

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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::ReadIoMemory16Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17ReadIoMemory16BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17ReadIoMemory16BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17ReadIoMemory16BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17ReadIoMemory16BitEPvPKvm:
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

    /* Prepare return address for read failure. */
    adr     x10, 4f

1:  /* Set our return address so that on read failure we continue as though we read -1. */
    mov     x30, x10

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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::ReadIoMemory8Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16ReadIoMemory8BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16ReadIoMemory8BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16ReadIoMemory8BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl16ReadIoMemory8BitEPvPKvm:
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

    /* Prepare return address for read failure. */
    adr     x10, 4f

1:  /* Set our return address so that on read failure we continue as though we read -1. */
    mov     x30, x10

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

/* ams::kern::arch::arm64::UserspaceAccess::Impl::WriteIoMemory32Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18WriteIoMemory32BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18WriteIoMemory32BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18WriteIoMemory32BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18WriteIoMemory32BitEPvPKvm:
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

    /* Prepare return address for failure. */
    adr     x10, 2f

1:  /* Read the word from normal memory. */
    ldtr    w9, [x5]

    /* Set our return address so that on failure we continue. */
    mov     x30, x10

    /* Write the word to io. */
    sttr    w9, [x5]
    dsb     sy

2:  /* Continue. */
    mov     x30, x8

    /* Advance. */
    add     x4, x4, #4
    add     x5, x5, #4
    cmp     x5, x7
    b.ne    1b

3:  /* We're done! */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::Impl::WriteIoMemory16Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18WriteIoMemory16BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18WriteIoMemory16BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18WriteIoMemory16BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl18WriteIoMemory16BitEPvPKvm:
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

    /* Prepare return address for failure. */
    adr     x10, 2f

1:  /* Read the word from normal memory. */
    ldtrh   w9, [x5]

    /* Set our return address so that on failure we continue. */
    mov     x30, x10

    /* Write the word to io. */
    sttrh   w9, [x5]
    dsb     sy

2:  /* Continue. */
    mov     x30, x8

    /* Advance. */
    add     x4, x4, #2
    add     x5, x5, #2
    cmp     x5, x7
    b.ne    1b

3:  /* We're done! */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::Impl::WriteIoMemory8Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17WriteIoMemory8BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17WriteIoMemory8BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17WriteIoMemory8BitEPvPKvm, %function
.balign 0x10
_ZN3ams4kern4arch5arm6415UserspaceAccess4Impl17WriteIoMemory8BitEPvPKvm:
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

    /* Prepare return address for failure. */
    adr     x10, 2f

1:  /* Read the word from normal memory. */
    ldtrb   w9, [x5]

    /* Set our return address so that on failure we continue. */
    mov     x30, x10

    /* Write the word to io. */
    sttrb   w9, [x5]
    dsb     sy

2:  /* Continue. */
    mov     x30, x8

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