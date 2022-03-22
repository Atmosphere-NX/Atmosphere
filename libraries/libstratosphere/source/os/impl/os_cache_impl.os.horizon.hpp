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
#include <stratosphere.hpp>

namespace ams::os::impl {

    inline void FlushDataCacheImpl(const void *addr, size_t size) {
        #if defined(ATMOSPHERE_ARCH_ARM64)
        {
            /* Declare helper variables. */
            uintptr_t cache_type_register = 0;
            uintptr_t cache_line_size     = 0;
            const uintptr_t end_addr      = reinterpret_cast<uintptr_t>(addr) + size;

            /* Get the cache type register. */
            __asm__ __volatile__("mrs %[cache_type_register], ctr_el0" : [cache_type_register]"=r"(cache_type_register));

            /* Calculate cache line size. */
            cache_line_size = 4 << ((cache_type_register >> 16) & 0xF);

            /* Iterate, flushing cache lines. */
            for (uintptr_t cur = reinterpret_cast<uintptr_t>(addr) & ~(cache_line_size - 1); cur < end_addr; cur += cache_line_size) {
                __asm__ __volatile__ ("dc civac, %[cur]" :: [cur]"r"(cur));
            }

            /* Insert a memory barrier, now that memory has been flushed. */
            __asm__ __volatile__("dsb sy" ::: "memory");
        }
        #else
            const auto result = svc::FlushProcessDataCache(svc::PseudoHandle::CurrentProcess, reinterpret_cast<uintptr_t>(addr), size);
            R_ASSERT(result);
            AMS_UNUSED(result);
        #endif
    }

    inline void FlushEntireDataCacheImpl() {
        svc::FlushEntireDataCache();
    }

}