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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#else
#include <vapours.hpp>
#endif

namespace ams::dd::impl {

    void StoreDataCacheImpl(void *addr, size_t size) {
        #if defined(ATMOSPHERE_ARCH_ARM64)
            /* On aarch64, we can use cache maintenance instructions. */

            /* Get cache line size. */
            uintptr_t ctr_el0 = 0;
            __asm__ __volatile__("mrs %[ctr_el0], ctr_el0" : [ctr_el0]"=r"(ctr_el0));
            const uintptr_t cache_line_size = 4 << ((ctr_el0 >> 16) & 0xF);

            /* Invalidate the cache. */
            const uintptr_t start_addr = reinterpret_cast<uintptr_t>(addr) & ~(cache_line_size - 1);
            const uintptr_t end_addr   = reinterpret_cast<uintptr_t>(addr) + size;
            for (uintptr_t cur = start_addr; cur < end_addr; cur += cache_line_size) {
                __asm__ __volatile__("dc cvac, %[cur]" : : [cur]"r"(cur));
            }

            /* Add a memory barrier. */
            __asm__ __volatile__("dsb sy" ::: "memory");
        #elif defined(ATMOSPHERE_ARCH_X64) || defined(ATMOSPHERE_ARCH_X86)
            /* Don't do anything, cache maintenance isn't available/relevant to userland. */
            AMS_UNUSED(addr, size);
        #else
            #error "Unknown architecture for macOS dd::StoreDataCacheImpl"
        #endif
    }

    void FlushDataCacheImpl(void *addr, size_t size) {
        #if defined(ATMOSPHERE_ARCH_ARM64)
            /* On aarch64, we can use cache maintenance instructions. */

            /* Get cache line size. */
            uintptr_t ctr_el0 = 0;
            __asm__ __volatile__("mrs %[ctr_el0], ctr_el0" : [ctr_el0]"=r"(ctr_el0));
            const uintptr_t cache_line_size = 4 << ((ctr_el0 >> 16) & 0xF);

            /* Invalidate the cache. */
            const uintptr_t start_addr = reinterpret_cast<uintptr_t>(addr) & ~(cache_line_size - 1);
            const uintptr_t end_addr   = reinterpret_cast<uintptr_t>(addr) + size;
            for (uintptr_t cur = start_addr; cur < end_addr; cur += cache_line_size) {
                __asm__ __volatile__("dc civac, %[cur]" : : [cur]"r"(cur));
            }

            /* Add a memory barrier. */
            __asm__ __volatile__("dsb sy" ::: "memory");
        #elif defined(ATMOSPHERE_ARCH_X64) || defined(ATMOSPHERE_ARCH_X86)
            /* Don't do anything, cache maintenance isn't available/relevant to userland. */
            AMS_UNUSED(addr, size);
        #else
            #error "Unknown architecture for macOS dd::FlushDataCacheImpl"
        #endif
    }

    void InvalidateDataCacheImpl(void *addr, size_t size) {
        #if defined(ATMOSPHERE_ARCH_ARM64)
            /* Just perform a flush, which is clean + invalidate. */
            return FlushDataCacheImpl(addr, size);
        #elif defined(ATMOSPHERE_ARCH_X64) || defined(ATMOSPHERE_ARCH_X86)
            /* Don't do anything, cache maintenance isn't available/relevant to userland. */
            AMS_UNUSED(addr, size);
        #else
            #error "Unknown architecture for macOS dd::InvalidateDataCacheImpl"
        #endif
    }

}
