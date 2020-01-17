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

#include "core_ctx.h"
#include "memory_map.h"

// start.s
extern uintptr_t g_initialKernelEntrypoint;

static atomic_uint g_activeCoreMask = 0;

// Prevents it from being put in BSS
CoreCtx g_coreCtxs[4] = {
    { .coreId = 0 },
    { .coreId = 1 },
    { .coreId = 2 },
    { .coreId = 3 },
};

void coreCtxInit(u32 coreId, bool isBootCore, u64 argument)
{
    size_t crashStackSize = 0x1000 / 4;
    currentCoreCtx = &g_coreCtxs[coreId];
    currentCoreCtx->isBootCore = isBootCore;
    currentCoreCtx->kernelArgument = argument;
    currentCoreCtx->crashStack = (u8 *)(MEMORY_MAP_VA_CRASH_STACKS_TOP - crashStackSize * coreId);
    if (isBootCore && currentCoreCtx->kernelEntrypoint == 0) {
        currentCoreCtx->kernelEntrypoint = g_initialKernelEntrypoint;
    }
}

void setCurrentCoreActive(void)
{
    atomic_fetch_or(&g_activeCoreMask, BIT(currentCoreCtx->coreId));
}

u32 getActiveCoreMask(void)
{
    return (u32)atomic_load(&g_activeCoreMask);
}
