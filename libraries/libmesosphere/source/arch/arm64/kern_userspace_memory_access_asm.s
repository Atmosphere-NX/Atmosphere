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
_ZN3ams4kern4arch5arm6432UserspaceAccessFunctionAreaBeginEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */

/*  ================ All Userspace Access Functions after this line. ================ */

/* ams::kern::arch::arm64::UserspaceAccess::CopyMemoryToUserAligned64Bit(void *dst, const void *src, size_t size) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned64BitEPvPKvm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned64BitEPvPKvm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess28CopyMemoryToUserAligned64BitEPvPKvm, %function
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
    cmp     x2, #0x0
    b.eq    2f
    ldr     x4, [x1], #0x8
    sttr    x4, [x0]
    add     x0, x0, #0x8
    sub     x2, x2, #0x8
    b       1b

2:  /* We're done. */
    mov     x0, #1
    ret

/* ams::kern::arch::arm64::UserspaceAccess::StoreDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6415UserspaceAccess14StoreDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6415UserspaceAccess14StoreDataCacheEmm
.type       _ZN3ams4kern4arch5arm6415UserspaceAccess14StoreDataCacheEmm, %function
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

/*  ================ All Userspace Access Functions before this line. ================ */

/* ams::kern::arch::arm64::UserspaceAccessFunctionAreaEnd() */
.section    .text._ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv
.type       _ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv, %function
_ZN3ams4kern4arch5arm6430UserspaceAccessFunctionAreaEndEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */