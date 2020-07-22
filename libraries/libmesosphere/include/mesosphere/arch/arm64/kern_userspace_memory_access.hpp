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
#pragma once
#include <mesosphere/kern_common.hpp>

namespace ams::kern::arch::arm64 {

    void UserspaceAccessFunctionAreaBegin();

    class UserspaceAccess {
        public:
            static bool CopyMemoryFromUser(void *dst, const void *src, size_t size);
            static bool CopyMemoryFromUserAligned32Bit(void *dst, const void *src, size_t size);
            static bool CopyMemoryFromUserAligned64Bit(void *dst, const void *src, size_t size);
            static bool CopyMemoryFromUserSize32Bit(void *dst, const void *src);
            static s32  CopyStringFromUser(void *dst, const void *src, size_t size);

            static bool CopyMemoryToUser(void *dst, const void *src, size_t size);
            static bool CopyMemoryToUserAligned32Bit(void *dst, const void *src, size_t size);
            static bool CopyMemoryToUserAligned64Bit(void *dst, const void *src, size_t size);
            static bool CopyMemoryToUserSize32Bit(void *dst, const void *src);
            static s32  CopyStringToUser(void *dst, const void *src, size_t size);

            static bool ClearMemory(void *dst, size_t size);
            static bool ClearMemoryAligned32Bit(void *dst, size_t size);
            static bool ClearMemoryAligned64Bit(void *dst, size_t size);
            static bool ClearMemorySize32Bit(void *dst);

            static bool UpdateLockAtomic(u32 *out, u32 *address, u32 if_zero, u32 new_orr_mask);
            static bool UpdateIfEqualAtomic(s32 *out, s32 *address, s32 compare_value, s32 new_value);
            static bool DecrementIfLessThanAtomic(s32 *out, s32 *address, s32 compare);

            static bool StoreDataCache(uintptr_t start, uintptr_t end);
            static bool FlushDataCache(uintptr_t start, uintptr_t end);
            static bool InvalidateDataCache(uintptr_t start, uintptr_t end);
            static bool InvalidateInstructionCache(uintptr_t start, uintptr_t end);

            static bool ReadIoMemory32Bit(void *dst, const void *src, size_t size);
            static bool ReadIoMemory16Bit(void *dst, const void *src, size_t size);
            static bool ReadIoMemory8Bit(void *dst, const void *src, size_t size);
            static bool WriteIoMemory32Bit(void *dst, const void *src, size_t size);
            static bool WriteIoMemory16Bit(void *dst, const void *src, size_t size);
            static bool WriteIoMemory8Bit(void *dst, const void *src, size_t size);
    };


    void UserspaceAccessFunctionAreaEnd();

}