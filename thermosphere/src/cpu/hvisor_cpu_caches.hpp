/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "hvisor_cpu_instructions.hpp"
#include "hvisor_cpu_sysreg_general.hpp"

namespace ams::hvisor::cpu {

    static inline u32 GetInstructionCachePolicy(void)
    {
        u32 ctr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(ctr_el0));
        return (ctr >> 14) & 3;
    }

    static inline u32 GetSmallestInstructionCacheLineSize(void)
    {
        u32 ctr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(ctr_el0));
        u32 shift = ctr & 0xF;
        // "log2 of the number of words"...
        return 4 << shift;
    }

    static inline u32 GetSmallestDataCacheLineSize(void)
    {
        u32 ctr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(ctr_el0));
        u32 shift = (ctr >> 16) & 0xF;
        // "log2 of the number of words"...
        return 4 << shift;
    }

    static inline void InvalidateInstructionCache(void)
    {
        __asm__ __volatile__ ("ic ialluis" ::: "memory");
        cpu::isb();
    }

    static inline void InvalidateInstructionCacheLocal(void)
    {
        __asm__ __volatile__ ("ic iallu" ::: "memory");
        cpu::isb();
    }

    void CleanInvalidateDataCacheRange(const void *addr, size_t size);
    void CleanDataCacheRangePoU(const void *addr, size_t size);

    void InvalidateInstructionCacheRangePoU(const void *addr, size_t size);

    void HandleSelfModifyingCodePoU(const void *addr, size_t size);

    void ClearSharedDataCachesOnBoot(void);
    void ClearLocalDataCacheOnBoot(void);

    // Dunno where else to put that
    void HandleTrappedSetWayOperation(u32 val);

}
