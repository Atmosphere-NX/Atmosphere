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

#include "fpu.h"
#include "core_ctx.h"

static FpuRegisterCache TEMPORARY g_fpuRegisterCache[4] = { 0 };

// fpu_regs_load_store.s
void fpuLoadRegistersFromCache(const FpuRegisterCache *cache);
void fpuStoreRegistersToCache(FpuRegisterCache *cache);

FpuRegisterCache *fpuGetRegisterCache(void)
{
    return &g_fpuRegisterCache[currentCoreCtx->coreId];
}

FpuRegisterCache *fpuReadRegisters(void)
{
    FpuRegisterCache *cache = &g_fpuRegisterCache[currentCoreCtx->coreId];
    if (!cache->valid) {
        fpuStoreRegistersToCache(cache);
        cache->valid = true;
    }
    return cache;
}

void fpuCommitRegisters(void)
{
    FpuRegisterCache *cache = &g_fpuRegisterCache[currentCoreCtx->coreId];
    cache->dirty = true;

    // Because the caller rewrote the entire cache in the event it didn't read it before:
    cache->valid = true;
}

void fpuCleanInvalidateRegisterCache(void)
{
    FpuRegisterCache *cache = &g_fpuRegisterCache[currentCoreCtx->coreId];
    if (cache->dirty) {
        fpuLoadRegistersFromCache(cache);
        cache->dirty = false;
    }

    cache->valid = false;
}
