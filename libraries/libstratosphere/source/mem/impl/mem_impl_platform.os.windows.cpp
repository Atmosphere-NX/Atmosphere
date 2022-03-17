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
#include <stratosphere/windows.hpp>
#include <VersionHelpers.h>
#include "mem_impl_platform.hpp"

namespace ams::mem::impl {

    namespace {

        errno_t ConvertGetLastError(DWORD error, errno_t fallback = EIO) {
            /* TODO: Implement this? */
            AMS_UNUSED(error);
            return fallback;
        }

    }

    errno_t virtual_alloc(void **ptr, size_t size) {
        /* Ensure size is non-zero. */
        if (size == 0) {
            return EINVAL;
        }

        /* Allocate virtual memory. */
        if (void *mem = ::VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE); mem != nullptr) {
            *ptr = mem;
            return 0;
        } else {
            return ConvertGetLastError(::GetLastError());
        }
    }

    errno_t virtual_free(void *ptr, size_t size) {
        /* Ensure pointer/size aren't zero. */
        if (ptr == nullptr || size == 0) {
            return EINVAL;
        }

        /* Free the memory. */
        if (::VirtualFree(ptr, 0, MEM_RELEASE)) {
            return 0;
        } else {
            return ConvertGetLastError(::GetLastError());
        }
    }

    errno_t physical_alloc(void *ptr, size_t size, Prot prot) {
        /* Ensure pointer isn't zero. */
        if (ptr == nullptr) {
            return EINVAL;
        }

        /* Convert the protection. */
        DWORD fl_protect;
        switch (util::ToUnderlying(prot)) {
            case Prot_none:
                fl_protect = PAGE_NOACCESS;
                break;
            case Prot_read:
                fl_protect = PAGE_READONLY;
                break;
            case Prot_write:
            case Prot_write | Prot_read:
                fl_protect = PAGE_READWRITE;
                break;
            case Prot_exec:
                fl_protect = PAGE_EXECUTE;
                break;
            case Prot_exec | Prot_read:
                fl_protect = PAGE_EXECUTE_READ;
                break;
            case Prot_exec | Prot_write:
            case Prot_exec | Prot_write | Prot_read:
                fl_protect = PAGE_EXECUTE_READWRITE;
                break;
            default:
                return EINVAL;
        }

        if (::VirtualAlloc(ptr, size, MEM_COMMIT, fl_protect)) {
            return 0;
        } else {
            return ConvertGetLastError(::GetLastError());
        }
    }

    errno_t physical_free(void *ptr, size_t size) {
        /* Detect empty allocation. */
        const uintptr_t aligned_start = util::AlignUp(reinterpret_cast<uintptr_t>(ptr),          os::MemoryPageSize);
        const uintptr_t aligned_end   = util::AlignDown(reinterpret_cast<uintptr_t>(ptr) + size, os::MemoryPageSize);
        const size_t    aligned_size  = aligned_end - aligned_start;
        if (aligned_end <= aligned_start) {
            return 0;
        }

        if (::VirtualFree(reinterpret_cast<LPVOID>(aligned_start), aligned_size, MEM_DECOMMIT)) {
            return 0;
        } else {
            return ConvertGetLastError(::GetLastError());
        }
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
        /* TODO: CryptGenRandom? */
        os::GenerateRandomBytes(dst, dst_size);
        return 0;
    }

    errno_t epochtime(s64 *dst) {
        /* TODO: Something more sane than this. */
        auto ts   = os::ConvertToTimeSpan(os::GetSystemTick());
        *dst = (ts.GetNanoSeconds() / INT64_C(100)) + INT64_C(0x8A09F909AE60000);
        return 0;
    }

    errno_t getcpu(s32 *out) {
        AMS_FUNCTION_LOCAL_STATIC(bool, s_has_processor_ex, ::IsWindows7OrGreater());

        if (s_has_processor_ex) {
            PROCESSOR_NUMBER pnum;
            ::GetCurrentProcessorNumberEx(std::addressof(pnum));

            *out = (pnum.Group << 8) | pnum.Number;
        } else {
            *out = ::GetCurrentProcessorNumber();
        }

        return 0;
    }

}
