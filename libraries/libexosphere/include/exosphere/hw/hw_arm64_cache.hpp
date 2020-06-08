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
#include <vapours.hpp>

namespace ams::hw::arch::arm64 {

#if defined(ATMOSPHERE_CPU_ARM_CORTEX_A57) || defined(ATMOSPHERE_CPU_ARM_CORTEX_A53)
    constexpr inline size_t InstructionCacheLineSize = 0x40;
    constexpr inline size_t DataCacheLineSize        = 0x40;
    constexpr inline size_t NumPerformanceCounters   = 6;
#else
    #error "Unknown CPU for cache line sizes"
#endif

    ALWAYS_INLINE void InvalidateEntireTlb() {
        __asm__ __volatile__("tlbi alle3is" ::: "memory");
    }

    ALWAYS_INLINE void InvalidateDataCacheLine(void *ptr) {
        __asm __volatile__("dc ivac, %[ptr]" :: [ptr]"r"(ptr) : "memory");
    }

    ALWAYS_INLINE void FlushDataCacheLine(void *ptr) {
        __asm __volatile__("dc civac, %[ptr]" :: [ptr]"r"(ptr) : "memory");
    }

    ALWAYS_INLINE void InvalidateEntireInstructionCache() {
        __asm__ __volatile__("ic iallu" ::: "memory");
    }

    ALWAYS_INLINE void InvalidateTlb(uintptr_t address) {
        const uintptr_t page_index = address / 4_KB;
        __asm__ __volatile__("tlbi vae3is, %[page_index]" :: [page_index]"r"(page_index) : "memory");
    }

    ALWAYS_INLINE void InvalidateTlbLastLevel(uintptr_t address) {
        const uintptr_t page_index = address / 4_KB;
        __asm__ __volatile__("tlbi vale3is, %[page_index]" :: [page_index]"r"(page_index) : "memory");
    }

    void FlushDataCache(const void *ptr, size_t size);

}