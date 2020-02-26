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

#include "hvisor_cpu_caches.hpp"

#define DEFINE_CACHE_RANGE_FUNC(isn, name, cache, post)\
void name(const void *addr, size_t size)\
{\
    u32 lineCacheSize = GetSmallest##cache##CacheLineSize();\
    uintptr_t begin = reinterpret_cast<uintptr_t>(addr) & ~(lineCacheSize - 1);\
    uintptr_t end = (reinterpret_cast<uintptr_t>(addr) + size + lineCacheSize - 1) & ~(lineCacheSize - 1);\
    for (uintptr_t pos = begin; pos < end; pos += lineCacheSize) {\
        __asm__ __volatile__ (isn ", %0" :: "r"(pos) : "memory");\
    }\
    post;\
}

namespace {

    ALWAYS_INLINE void SelectCacheLevel(bool instructionCache, u32 level)
    {
        u32 ibit  = instructionCache ? 1 : 0;
        u32 lbits = (level & 7) << 1;
        THERMOSPHERE_SET_SYSREG(csselr_el1, lbits | ibit);
        ams::hvisor::cpu::isb();
    }

    [[gnu::optimize("O2")]] ALWAYS_INLINE void InvalidateDataCacheLevel(u32 level)
    {
        SelectCacheLevel(false, level);
        u32 ccsidr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(ccsidr_el1));
        u32 numWays = 1 + ((ccsidr >> 3) & 0x3FF);
        u32 numSets = 1 + ((ccsidr >> 13) & 0x7FFF);
        u32 wayShift = __builtin_clz(numWays);
        u32 setShift = (ccsidr & 7) + 4;
        u32 lbits = (level & 7) << 1;

        for (u32 way = 0; way < numWays; way++) {
            for (u32 set = 0; set < numSets; set++) {
                u64 val = ((u64)way << wayShift) | ((u64)set << setShift) | lbits;
                __asm__ __volatile__ ("dc isw, %0" :: "r"(val) : "memory");
            }
        }
    }

    ALWAYS_INLINE void CleanInvalidateDataCacheLevel(u32 level)
    {
        SelectCacheLevel(false, level);
        u32 ccsidr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(ccsidr_el1));
        u32 numWays = 1 + ((ccsidr >> 3) & 0x3FF);
        u32 numSets = 1 + ((ccsidr >> 13) & 0x7FFF);
        u32 wayShift = __builtin_clz(numWays);
        u32 setShift = (ccsidr & 7) + 4;
        u32 lbits = (level & 7) << 1;

        for (u32 way = 0; way < numWays; way++) {
            for (u32 set = 0; set < numSets; set++) {
                u64 val = ((u64)way << wayShift) | ((u64)set << setShift) | lbits;
                __asm__ __volatile__ ("dc cisw, %0" :: "r"(val) : "memory");
            }
        }
    }

    [[gnu::optimize("O2")]] ALWAYS_INLINE void InvalidateDataCacheLevels(u32 from, u32 to)
    {
        // Let's hope it doesn't generate a stack frame...
        for (u32 level = from; level < to; level++) {
            InvalidateDataCacheLevel(level);
        }

        ams::hvisor::cpu::dsbSy();
        ams::hvisor::cpu::isb();
    }

}
namespace ams::hvisor::cpu {

    DEFINE_CACHE_RANGE_FUNC("dc civac", CleanInvalidateDataCacheRange, Data, dsbSy())
    DEFINE_CACHE_RANGE_FUNC("dc cvau", CleanDataCacheRangePoU, Data, dsb())
    DEFINE_CACHE_RANGE_FUNC("ic ivau", InvalidateInstructionCacheRangePoU, Instruction, dsb(); isb())

    void HandleSelfModifyingCodePoU(const void *addr, size_t size)
    {
        // See docs for ctr_el0.{dic, idc}. It's unclear when these bits have been added, but they're
        // RES0 if not implemented, so that's fine
        u32 ctr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(ctr_el0));
        if (!(ctr & BIT(28))) {
            CleanDataCacheRangePoU(addr, size);
        }
        if (!(ctr & BIT(29))) {
            InvalidateInstructionCacheRangePoU(addr, size);
        } else {
            // Make sure we have at least a dsb/isb
            dsb();
            isb();
        }
    }

    [[gnu::optimize("O2")]] void ClearSharedDataCachesOnBoot(void)
    {
        u32 clidr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(clidr_el1));
        u32 louis = (clidr >> 21) & 7;
        u32 loc   = (clidr >> 24) & 7;
        InvalidateDataCacheLevels(louis, loc);
    }

    [[gnu::optimize("O2")]] void ClearLocalDataCacheOnBoot(void)
    {
        u32 clidr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(clidr_el1));
        u32 louis = (clidr >> 21) & 7;
        InvalidateDataCacheLevels(0, louis);
    }

    /* Ok so:
        - cache set/way ops can't really be virtualized
        - since we have only one guest OS & don't care about security (for space limitations),
        we do the following:
            - ignore all cache s/w ops applying before the Level Of Unification Inner Shareable (L1, typically).
            These clearly break coherency and should only be done once, on power on/off/suspend/resume only. And we already
            do it ourselves...
            - allow ops after the LoUIS, but do it ourselves and ignore the next (numSets*numWay - 1) requests. This is because
            we have to handle Nintendo's dodgy code (check if SetWay == 0)
            - transform all s/w cache ops into clean and invalidate
     */
    void HandleTrappedSetWayOperation(u32 val)
    {
        u32 clidr = static_cast<u32>(THERMOSPHERE_GET_SYSREG(clidr_el1));
        u32 louis = (clidr >> 21) & 7;

        u32 level = val >> 1 & 7;
        u32 setway = val >> 3; 
        if (level < louis) {
            return;
        }

        if (setway == 0) {
            CleanInvalidateDataCacheLevel(level);
            dsbSy();
            isb();
        }
    }

}
