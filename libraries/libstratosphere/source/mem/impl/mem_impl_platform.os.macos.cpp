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
#include <sys/mman.h>
#include "mem_impl_platform.hpp"

#if defined(ATMOSPHERE_ARCH_X64)
#include <cpuid.h>
#endif

namespace ams::mem::impl {

    errno_t virtual_alloc(void **ptr, size_t size) {
        /* Ensure size is non-zero. */
        if (size == 0) {
            return EINVAL;
        }

        /* Allocate virtual memory. */
        if (const auto address = ::mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); address != MAP_FAILED) {
            *ptr = reinterpret_cast<void *>(address);
            return 0;
        } else {
            return errno;
        }
    }

    errno_t virtual_free(void *ptr, size_t size) {
        /* Ensure pointer/size aren't zero. */
        if (ptr == nullptr || size == 0) {
            return EINVAL;
        }

        /* Free the memory. */
        if (::munmap(ptr, size) == 0) {
            return 0;
        } else {
            return errno;
        }
    }

    errno_t physical_alloc(void *ptr, size_t size, Prot prot) {
        /* Ensure pointer isn't zero. */
        if (ptr == nullptr) {
            return EINVAL;
        }

        /* Convert the protection. */
        int native_prot;
        switch (util::ToUnderlying(prot)) {
            case Prot_none:
                native_prot = PROT_NONE;
                break;
            case Prot_read:
                native_prot = PROT_READ;
                break;
            case Prot_write:
            case Prot_write | Prot_read:
                native_prot = PROT_READ | PROT_WRITE;
                break;
            case Prot_exec:
                native_prot = PROT_EXEC;
                break;
            case Prot_exec | Prot_read:
                native_prot = PROT_READ | PROT_EXEC;
                break;
            case Prot_exec | Prot_write:
            case Prot_exec | Prot_write | Prot_read:
                native_prot = PROT_READ | PROT_WRITE | PROT_EXEC;
                break;
            default:
                return EINVAL;
        }

        if (::mprotect(ptr, size, native_prot)) {
            return 0;
        } else {
            return errno;
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

        if (::mprotect(reinterpret_cast<void *>(aligned_start), aligned_size, PROT_NONE)) {
            return 0;
        } else {
            return errno;
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
        if (__builtin_available(macOS 11.0, *)) {
            /* On macOS 11.0+, we can use the exposed API they added. */
            size_t cpu_number = 0;
            const auto res = pthread_cpu_number_np(std::addressof(cpu_number));
            AMS_ASSERT(res == 0);
            AMS_UNUSED(res);

            *out = static_cast<s32>(cpu_number);
            return 0;
        } else {
            #if defined(ATMOSPHERE_ARCH_X64)
            {
                uint32_t cpu_info[4];
                __cpuid_count(1, 0, cpu_info[0], cpu_info[1], cpu_info[2], cpu_info[3]);

                s32 core;
                if ((cpu_info[3] & (1 << 9)) == 0) {
                    core = 0;
                } else {
                    core = static_cast<s32>(cpu_info[1] >> 24);
                }

                *out = core;
                return 0;
            }
            #elif defined(ATMOSPHERE_ARCH_ARM64)
            {
                /* Read from TSD, per https://github.com/apple-oss-distributions/xnu/blob/e6231be02a03711ca404e5121a151b24afbff733/libsyscall/os/tsd.h#L68 */
                uint64_t p;
                __asm__ __volatile__ ("mrs %0, TPIDR_EL0" : "=r" (p));
                *out = static_cast<s32>(p & 0xF);
                return 0;
            }
            #else
            AMS_ABORT("Unknown architecture to get current cpu on fallback path on macOS");
            #endif
        }
    }

}
