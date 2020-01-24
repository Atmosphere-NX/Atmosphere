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
#include "kern_cpu_system_registers.hpp"

namespace ams::kern::arm64::cpu {

#if defined(ATMOSPHERE_CPU_ARM_CORTEX_A57) || defined(ATMOSPHERE_CPU_ARM_CORTEX_A53)
    constexpr inline size_t InstructionCacheLineSize = 0x40;
    constexpr inline size_t DataCacheLineSize        = 0x40;
#else
    #error "Unknown CPU for cache line sizes"
#endif

#if defined(ATMOSPHERE_BOARD_NINTENDO_SWITCH)
    static constexpr size_t NumCores = 4;
#else
    #error "Unknown Board for cpu::NumCores"
#endif

    /* Helpers for managing memory state. */
    ALWAYS_INLINE void DataSynchronizationBarrier() {
        __asm__ __volatile__("dsb sy" ::: "memory");
    }

    ALWAYS_INLINE void DataSynchronizationBarrierInnerShareable() {
        __asm__ __volatile__("dsb ish" ::: "memory");
    }

    ALWAYS_INLINE void DataMemoryBarrier() {
        __asm__ __volatile__("dmb sy" ::: "memory");
    }

    ALWAYS_INLINE void InstructionMemoryBarrier() {
        __asm__ __volatile__("isb" ::: "memory");
    }

    ALWAYS_INLINE void EnsureInstructionConsistency() {
        DataSynchronizationBarrier();
        InstructionMemoryBarrier();
    }

    ALWAYS_INLINE void InvalidateEntireInstructionCache() {
        __asm__ __volatile__("ic iallu" ::: "memory");
        EnsureInstructionConsistency();
    }

    /* Cache management helpers. */
    void FlushEntireDataCacheShared();
    void FlushEntireDataCacheLocal();

    ALWAYS_INLINE void InvalidateEntireTlb() {
        __asm__ __volatile__("tlbi vmalle1is" ::: "memory");
        EnsureInstructionConsistency();
    }

}
