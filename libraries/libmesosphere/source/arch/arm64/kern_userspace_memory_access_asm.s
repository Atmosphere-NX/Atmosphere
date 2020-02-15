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

/* ams::kern::arch::arm64::UserspaceMemoryAccessFunctionAreaBegin() */
.section    .text._ZN3ams4kern4arch5arm6438UserspaceMemoryAccessFunctionAreaBeginEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6438UserspaceMemoryAccessFunctionAreaBeginEv
.type       _ZN3ams4kern4arch5arm6438UserspaceMemoryAccessFunctionAreaBeginEv, %function
_ZN3ams4kern4arch5arm6438UserspaceMemoryAccessFunctionAreaBeginEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */

/*  ================ All Userspace Memory Functions after this line. ================ */

/* ams::kern::arch::arm64::StoreDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6414StoreDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6414StoreDataCacheEmm
.type       _ZN3ams4kern4arch5arm6414StoreDataCacheEmm, %function
_ZN3ams4kern4arch5arm6414StoreDataCacheEmm:
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

/* ams::kern::arch::arm64::FlushDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6414FlushDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6414FlushDataCacheEmm
.type       _ZN3ams4kern4arch5arm6414FlushDataCacheEmm, %function
_ZN3ams4kern4arch5arm6414FlushDataCacheEmm:
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

/* ams::kern::arch::arm64::InvalidateDataCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6419InvalidateDataCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6419InvalidateDataCacheEmm
.type       _ZN3ams4kern4arch5arm6419InvalidateDataCacheEmm, %function
_ZN3ams4kern4arch5arm6419InvalidateDataCacheEmm:
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

/* ams::kern::arch::arm64::InvalidateInstructionCache(uintptr_t start, uintptr_t end) */
.section    .text._ZN3ams4kern4arch5arm6426InvalidateInstructionCacheEmm, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6426InvalidateInstructionCacheEmm
.type       _ZN3ams4kern4arch5arm6426InvalidateInstructionCacheEmm, %function
_ZN3ams4kern4arch5arm6426InvalidateInstructionCacheEmm:
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

/*  ================ All Userspace Memory Functions before this line. ================ */

/* ams::kern::arch::arm64::UserspaceMemoryAccessFunctionAreaEnd() */
.section    .text._ZN3ams4kern4arch5arm6436UserspaceMemoryAccessFunctionAreaEndEv, "ax", %progbits
.global     _ZN3ams4kern4arch5arm6436UserspaceMemoryAccessFunctionAreaEndEv
.type       _ZN3ams4kern4arch5arm6436UserspaceMemoryAccessFunctionAreaEndEv, %function
_ZN3ams4kern4arch5arm6436UserspaceMemoryAccessFunctionAreaEndEv:
/* NOTE: This is not a real function, and only exists as a label for safety. */