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

#pragma once

#include "utils.h"
#include "sysreg.h"

static inline u32 cacheGetSmallestInstructionCacheLineSize(void)
{
    u32 ctr = (u32)GET_SYSREG(ctr_el0);
    u32 shift = ctr & 0xF;
    // "log2 of the number of words"...
    return 4 << shift;
}

static inline u32 cacheGetSmallestDataCacheLineSize(void)
{
    u32 ctr = (u32)GET_SYSREG(ctr_el0);
    u32 shift = (ctr >> 16) & 0xF;
    // "log2 of the number of words"...
    return 4 << shift;
}

static inline void cacheInvalidateInstructionCache(void)
{
    __asm__ __volatile__ ("ic ialluis" ::: "memory");
    __isb();
}

static inline void cacheInvalidateInstructionCacheLocal(void)
{
    __asm__ __volatile__ ("ic iallu" ::: "memory");
    __isb();
}

void cacheCleanInvalidateDataCacheRange(const void *addr, size_t size);
void cacheCleanDataCacheRangePoU(const void *addr, size_t size);

void cacheInvalidateInstructionCacheRangePoU(const void *addr, size_t size);

void cacheHandleSelfModifyingCodePoU(const void *addr, size_t size);

void cacheClearSharedDataCachesOnBoot(void);
void cacheClearLocalDataCacheOnBoot(void);

void cacheHandleTrappedSetWayOperation(bool invalidateOnly);
