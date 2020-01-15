/*
 * Copyright (c) 2019 Atmosphère-NX
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

#include "caches.h"
#include "preprocessor.h"

#define DEFINE_CACHE_RANGE_FUNC(isn, name, cache, post)\
void name(const void *addr, size_t size)\
{\
    u32 lineCacheSize = cacheGetSmallest##cache##CacheLineSize();\
    uintptr_t begin = (uintptr_t)addr & ~(lineCacheSize - 1);\
    uintptr_t end = ((uintptr_t)addr + size + lineCacheSize - 1) & ~(lineCacheSize - 1);\
    for (uintptr_t pos = begin; pos < end; pos += lineCacheSize) {\
        __asm__ __volatile__ (isn ", %0" :: "r"(pos) : "memory");\
    }\
    post;\
}

static inline ALINLINE void cacheSelectByLevel(bool instructionCache, u32 level)
{
    u32 ibit  = instructionCache ? 1 : 0;
    u32 lbits = (level & 7) << 1;
    SET_SYSREG(csselr_el1, lbits | ibit);
    __isb();
}

static inline ALINLINE void cacheInvalidateDataCacheLevel(u32 level)
{
    cacheSelectByLevel(false, level);
    u32 ccsidr = (u32)GET_SYSREG(ccsidr_el1);
    u32 numWays = 1 + ((ccsidr >> 3) & 0x3FF);
    u32 numSets = 1 + ((ccsidr >> 13) & 0x7FFF);
    u32 wayShift = __builtin_clz(numWays);
    u32 setShift = (ccsidr & 7) + 4;
    u32 lbits = (level & 7) << 1;

    for (u32 way = 0; way <= numWays; way++) {
        for (u32 set = 0; set <= numSets; set++) {
            u64 val = ((u64)way << wayShift) | ((u64)set << setShift) | lbits;
            __asm__ __volatile__ ("dc isw, %0" :: "r"(val) : "memory");
        }
    }
}

static inline ALINLINE void cacheInvalidateDataCacheLevels(u32 from, u32 to)
{
    // Let's hope it doesn't generate a stack frame...
    for (u32 level = from; level < to; level++) {
        cacheInvalidateDataCacheLevel(level);
    }

    __dsb_sy();
    __isb();
}

DEFINE_CACHE_RANGE_FUNC("dc civac", cacheCleanInvalidateDataCacheRange, Data, __dsb())
DEFINE_CACHE_RANGE_FUNC("dc cvau", cacheCleanDataCacheRangePoU, Data, __dsb())
DEFINE_CACHE_RANGE_FUNC("ic ivau", cacheInvalidateInstructionCacheRangePoU, Instruction, __dsb(); __isb())

void cacheHandleSelfModifyingCodePoU(const void *addr, size_t size)
{
    // See docs for ctr_el0.{dic, idc}. It's unclear when these bits have been added, but they're
    // RES0 if not implemented, so that's fine
    u32 ctr = (u32)GET_SYSREG(ctr_el0);
    if (!(ctr & BIT(28))) {
        cacheCleanDataCacheRangePoU(addr, size);
    }
    if (!(ctr & BIT(29))) {
        cacheInvalidateInstructionCacheRangePoU(addr, size);
    }
}

void cacheClearSharedDataCachesOnBoot(void)
{
    u32 clidr = (u32)GET_SYSREG(clidr_el1);
    u32 louis = (clidr >> 21) & 7;
    u32 loc   = (clidr >> 24) & 7;
    cacheInvalidateDataCacheLevels(louis, loc);
}

void cacheClearLocalDataCacheOnBoot(void)
{
    u32 clidr = (u32)GET_SYSREG(clidr_el1);
    u32 louis = (clidr >> 21) & 7;
    cacheInvalidateDataCacheLevels(0, louis);
}
