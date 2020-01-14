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

#include "debug_pause.h"
#include "core_ctx.h"
#include "irq.h"
#include "spinlock.h"

// Reminder: use these functions behind a lock

static Barrier g_debugPauseBarrier;
static RecursiveSpinlock g_debugPauseContinueLocks[4];

void debugPauseSgiTopHalf(void)
{
    barrierWait(&g_debugPauseBarrier);
}

void debugPauseSgiBottomHalf(void)
{
    recursiveSpinlockLock(&g_debugPauseContinueLocks[currentCoreCtx->coreId]);
    maskIrq(); // <- unlikely race condition here? If it happens, it shouldn't happen more than once/should be fine anyway
    recursiveSpinlockUnlock(&g_debugPauseContinueLocks[currentCoreCtx->coreId]);
}

void debugPauseCores(u32 coreList)
{
    coreList &= ~BIT(currentCoreCtx->coreId);

    barrierInit(&g_debugPauseBarrier, coreList | BIT(currentCoreCtx->coreId));
    FOREACH_BIT (tmp, core, coreList) {
        recursiveSpinlockLock(&g_debugPauseContinueLocks[core]);
    }

    generateSgiForList(ThermosphereSgi_DebugPause, coreList);
    barrierWait(&g_debugPauseBarrier);
}

void debugUnpauseCores(u32 coreList)
{
    coreList &= ~BIT(currentCoreCtx->coreId);

    FOREACH_BIT (tmp, core, coreList) {
        recursiveSpinlockUnlock(&g_debugPauseContinueLocks[core]);
    }
}
