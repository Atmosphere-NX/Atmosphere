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
#include <stratosphere.hpp>
#include "mem_impl_platform.hpp"

namespace ams::mem::impl {

    namespace {

        constinit os::SdkMutex g_virt_mem_enabled_lock;
        constinit bool g_virt_mem_enabled_detected = false;
        constinit bool g_virt_mem_enabled = false;

        void EnsureVirtualAddressMemoryDetected() {
            if (AMS_LIKELY(g_virt_mem_enabled_detected)) {
                return;
            }

            std::scoped_lock lk(g_virt_mem_enabled_lock);

            if (AMS_UNLIKELY(g_virt_mem_enabled_detected)) {
                return;
            }

            g_virt_mem_enabled = os::IsVirtualAddressMemoryEnabled();
        }

        ALWAYS_INLINE bool IsVirtualAddressMemoryEnabled() {
            EnsureVirtualAddressMemoryDetected();
            return g_virt_mem_enabled;
        }

        ALWAYS_INLINE errno_t ConvertResult(Result result) {
            /* TODO: Actually implement this in a meaningful way. */
            if (R_FAILED(result)) {
                return EINVAL;
            }
            return 0;
        }

        ALWAYS_INLINE os::MemoryPermission ConvertToOsPermission(Prot prot) {
            static_assert(static_cast<int>(Prot_read) == static_cast<int>(os::MemoryPermission_ReadOnly));
            static_assert(static_cast<int>(Prot_write) == static_cast<int>(os::MemoryPermission_WriteOnly));
            static_assert((util::ToUnderlying(Prot_read) | util::ToUnderlying(Prot_write)) == util::ToUnderlying(os::MemoryPermission_ReadWrite));
            return static_cast<os::MemoryPermission>(prot & (Prot_read | Prot_write));
        }

    }

    errno_t virtual_alloc(void **ptr, size_t size) {
        /* Ensure size isn't too large. */
        if (size > mem::impl::MaxSize) {
            return EINVAL;
        }

        /* Allocate virtual memory. */
        uintptr_t addr;
        if (IsVirtualAddressMemoryEnabled()) {
            /* TODO: Support virtual address memory. */
            AMS_ABORT("Virtual address memory not supported yet");
        } else {
            if (auto err = ConvertResult(os::AllocateMemoryBlock(std::addressof(addr), util::AlignUp(size, os::MemoryBlockUnitSize))); err != 0) {
                return err;
            }

            os::SetMemoryPermission(addr, size, os::MemoryPermission_None);
        }

        /* Set the output pointer. */
        *ptr = reinterpret_cast<void *>(addr);

        return 0;
    }

    errno_t virtual_free(void *ptr, size_t size) {
        /* Ensure size isn't zero. */
        if (size == 0) {
            return EINVAL;
        }

        if (IsVirtualAddressMemoryEnabled()) {
            /* TODO: Support virtual address memory. */
            AMS_ABORT("Virtual address memory not supported yet");
        } else {
            os::FreeMemoryBlock(reinterpret_cast<uintptr_t>(ptr), util::AlignUp(size, os::MemoryBlockUnitSize));
        }

        return 0;
    }

    errno_t physical_alloc(void *ptr, size_t size, Prot prot) {
        /* Detect empty allocation. */
        const uintptr_t aligned_start = util::AlignDown(reinterpret_cast<uintptr_t>(ptr),      os::MemoryPageSize);
        const uintptr_t aligned_end   = util::AlignUp(reinterpret_cast<uintptr_t>(ptr) + size, os::MemoryPageSize);
        const size_t    aligned_size  = aligned_end - aligned_start;
        if (aligned_end <= aligned_start) {
            return 0;
        }

        if (IsVirtualAddressMemoryEnabled()) {
            /* TODO: Support virtual address memory. */
            AMS_ABORT("Virtual address memory not supported yet");
        } else {
            os::SetMemoryPermission(aligned_start, aligned_size, ConvertToOsPermission(prot));
        }

        return 0;
    }

    errno_t physical_free(void *ptr, size_t size) {
        /* Detect empty allocation. */
        const uintptr_t aligned_start = util::AlignDown(reinterpret_cast<uintptr_t>(ptr),      os::MemoryPageSize);
        const uintptr_t aligned_end   = util::AlignUp(reinterpret_cast<uintptr_t>(ptr) + size, os::MemoryPageSize);
        const size_t    aligned_size  = aligned_end - aligned_start;
        if (aligned_end <= aligned_start) {
            return 0;
        }

        if (IsVirtualAddressMemoryEnabled()) {
            /* TODO: Support virtual address memory. */
            AMS_ABORT("Virtual address memory not supported yet");
        } else {
            os::SetMemoryPermission(aligned_start, aligned_size, os::MemoryPermission_None);
        }

        return 0;
    }

    size_t strlcpy(char *dst, const char *src, size_t size) {
        const size_t src_size = std::strlen(src);
        if (src_size >= size) {
            if (size) {
                std::memcpy(dst, src, size - 1);
                dst[size - 1] = 0;
            }
        } else {
            std::memcpy(dst, src, src_size + 1);
        }
        return src_size;
    }

    errno_t gen_random(void *dst, size_t dst_size) {
        os::GenerateRandomBytes(dst, dst_size);
        return 0;
    }

    errno_t epochtime(s64 *dst) {
        /* TODO: What is this calc? */
        auto ts   = os::ConvertToTimeSpan(os::GetSystemTick());
        *dst = (ts.GetNanoSeconds() / INT64_C(100)) + INT64_C(0x8A09F909AE60000);
        return 0;
    }

    errno_t getcpu(s32 *out) {
        *out = os::GetCurrentCoreNumber();
        return 0;
    }

}
