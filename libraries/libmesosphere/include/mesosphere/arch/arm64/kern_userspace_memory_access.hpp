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
#pragma once
#include <mesosphere/kern_common.hpp>

namespace ams::kern::arch::arm64 {

    void UserspaceAccessFunctionAreaBegin();

    class UserspaceAccess {
        private:
            class Impl {
                public:
                    static bool CopyMemoryFromUser(void *dst, const void *src, size_t size);
                    static bool CopyMemoryFromUserAligned32Bit(void *dst, const void *src, size_t size);
                    static bool CopyMemoryFromUserAligned64Bit(void *dst, const void *src, size_t size);
                    static bool CopyMemoryFromUserSize64Bit(void *dst, const void *src);
                    static bool CopyMemoryFromUserSize32Bit(void *dst, const void *src);
                    static bool CopyMemoryFromUserSize32BitWithSupervisorAccess(void *dst, const void *src);
                    static s32  CopyStringFromUser(void *dst, const void *src, size_t size);

                    static bool CopyMemoryToUser(void *dst, const void *src, size_t size);
                    static bool CopyMemoryToUserAligned32Bit(void *dst, const void *src, size_t size);
                    static bool CopyMemoryToUserAligned64Bit(void *dst, const void *src, size_t size);
                    static bool CopyMemoryToUserSize32Bit(void *dst, const void *src);
                    static s32  CopyStringToUser(void *dst, const void *src, size_t size);

                    static bool UpdateLockAtomic(u32 *out, u32 *address, u32 if_zero, u32 new_orr_mask);
                    static bool UpdateIfEqualAtomic(s32 *out, s32 *address, s32 compare_value, s32 new_value);
                    static bool DecrementIfLessThanAtomic(s32 *out, s32 *address, s32 compare);

                    static bool StoreDataCache(uintptr_t start, uintptr_t end);
                    static bool FlushDataCache(uintptr_t start, uintptr_t end);
                    static bool InvalidateDataCache(uintptr_t start, uintptr_t end);

                    static bool ReadIoMemory32Bit(void *dst, const void *src, size_t size);
                    static bool ReadIoMemory16Bit(void *dst, const void *src, size_t size);
                    static bool ReadIoMemory8Bit(void *dst, const void *src, size_t size);
                    static bool WriteIoMemory32Bit(void *dst, const void *src, size_t size);
                    static bool WriteIoMemory16Bit(void *dst, const void *src, size_t size);
                    static bool WriteIoMemory8Bit(void *dst, const void *src, size_t size);
            };
        public:
            static bool CopyMemoryFromUser(void *dst, const void *src, size_t size) {
                return Impl::CopyMemoryFromUser(dst, src, size);
            }

            static bool CopyMemoryFromUserAligned32Bit(void *dst, const void *src, size_t size) {
                return Impl::CopyMemoryFromUserAligned32Bit(dst, src, size);
            }

            static bool CopyMemoryFromUserAligned64Bit(void *dst, const void *src, size_t size) {
                return Impl::CopyMemoryFromUserAligned64Bit(dst, src, size);
            }

            static bool CopyMemoryFromUserSize64Bit(void *dst, const void *src) {
                return Impl::CopyMemoryFromUserSize64Bit(dst, src);
            }

            static bool CopyMemoryFromUserSize32Bit(void *dst, const void *src) {
                return Impl::CopyMemoryFromUserSize32Bit(dst, src);
            }


            static bool CopyMemoryFromUserSize32BitWithSupervisorAccess(void *dst, const void *src) {
                /* Check that the address is within the valid userspace range. */
                if (const uintptr_t src_uptr = reinterpret_cast<uintptr_t>(src); src_uptr < ams::svc::AddressNullGuard32Size || (src_uptr + sizeof(u32) - 1) >= ams::svc::AddressMemoryRegion39Size) {
                    return false;
                }

                return Impl::CopyMemoryFromUserSize32BitWithSupervisorAccess(dst, src);
            }

            static s32  CopyStringFromUser(void *dst, const void *src, size_t size) {
                return Impl::CopyStringFromUser(dst, src, size);
            }

            static bool CopyMemoryToUser(void *dst, const void *src, size_t size) {
                return Impl::CopyMemoryToUser(dst, src, size);
            }

            static bool CopyMemoryToUserAligned32Bit(void *dst, const void *src, size_t size) {
                return Impl::CopyMemoryToUserAligned32Bit(dst, src, size);
            }

            static bool CopyMemoryToUserAligned64Bit(void *dst, const void *src, size_t size) {
                return Impl::CopyMemoryToUserAligned64Bit(dst, src, size);
            }

            static bool CopyMemoryToUserSize32Bit(void *dst, const void *src) {
                return Impl::CopyMemoryToUserSize32Bit(dst, src);
            }

            static s32  CopyStringToUser(void *dst, const void *src, size_t size) {
                return Impl::CopyStringToUser(dst, src, size);
            }

            static bool UpdateLockAtomic(u32 *out, u32 *address, u32 if_zero, u32 new_orr_mask) {
                return Impl::UpdateLockAtomic(out, address, if_zero, new_orr_mask);
            }

            static bool UpdateIfEqualAtomic(s32 *out, s32 *address, s32 compare_value, s32 new_value) {
                return Impl::UpdateIfEqualAtomic(out, address, compare_value, new_value);
            }

            static bool DecrementIfLessThanAtomic(s32 *out, s32 *address, s32 compare) {
                return Impl::DecrementIfLessThanAtomic(out, address, compare);
            }

            static bool StoreDataCache(uintptr_t start, uintptr_t end) {
                return Impl::StoreDataCache(start, end);
            }

            static bool FlushDataCache(uintptr_t start, uintptr_t end) {
                return Impl::FlushDataCache(start, end);
            }

            static bool InvalidateDataCache(uintptr_t start, uintptr_t end) {
                return Impl::InvalidateDataCache(start, end);
            }

            static bool ReadIoMemory32Bit(void *dst, const void *src, size_t size) {
                return Impl::ReadIoMemory32Bit(dst, src, size);
            }

            static bool ReadIoMemory16Bit(void *dst, const void *src, size_t size) {
                return Impl::ReadIoMemory16Bit(dst, src, size);
            }

            static bool ReadIoMemory8Bit(void *dst, const void *src, size_t size) {
                return Impl::ReadIoMemory8Bit(dst, src, size);
            }

            static bool WriteIoMemory32Bit(void *dst, const void *src, size_t size) {
                return Impl::WriteIoMemory32Bit(dst, src, size);
            }

            static bool WriteIoMemory16Bit(void *dst, const void *src, size_t size) {
                return Impl::WriteIoMemory16Bit(dst, src, size);
            }

            static bool WriteIoMemory8Bit(void *dst, const void *src, size_t size) {
                return Impl::WriteIoMemory8Bit(dst, src, size);
            }
    };


    void UserspaceAccessFunctionAreaEnd();

}