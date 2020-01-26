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

#include <stdatomic.h>

#include "debug_pause.h"
#include "core_ctx.h"
#include "irq.h"
#include "spinlock.h"
#include "single_step.h"

static Barrier g_debugPauseBarrier;
static ALIGN(64) atomic_uint g_debugPausePausedCoreList;
static atomic_uint g_debugPauseSingleStepCoreList; // TODO: put this variable on the same cache line as the above

static inline void debugSetThisCorePaused(void)
{
    currentCoreCtx->wasPaused = true;
}

void debugPauseSgiHandler(void)
{
    debugSetThisCorePaused();
    barrierWait(&g_debugPauseBarrier);
}

void debugPauseWaitAndUpdateSingleStep(void)
{
    u32 coreId = currentCoreCtx->coreId;
    __builtin_prefetch(&g_debugPausePausedCoreList, 0, 3);
    if (atomic_load(&g_debugPausePausedCoreList) & BIT(coreId)) {
        unmaskIrq();
        do {
            __wfe();
        } while (atomic_load(&g_debugPausePausedCoreList) & BIT(coreId));
        maskIrq();
    }

    currentCoreCtx->wasPaused = false;

    // Single-step: if inactive and requested, start single step; cancel if active and not requested
    u32 ssReqd = (atomic_load(&g_debugPauseSingleStepCoreList) & BIT(currentCoreCtx->coreId)) != 0;
    SingleStepState singleStepState = singleStepGetNextState(currentCoreCtx->guestFrame);
    if (ssReqd && singleStepState == SingleStepState_Inactive) {
        singleStepSetNextState(currentCoreCtx->guestFrame, SingleStepState_ActiveNotPending);
    } else if (!ssReqd && singleStepState != SingleStepState_Inactive) {
        singleStepSetNextState(currentCoreCtx->guestFrame, SingleStepState_Inactive);
    }
}

void debugPauseCores(u32 coreList)
{
    maskIrq();

    __builtin_prefetch(&g_debugPausePausedCoreList, 1, 3);

    u32 desiredList = coreList;
    u32 remainingList = coreList;
    u32 readList = atomic_load(&g_debugPausePausedCoreList);
    do {
        desiredList |= readList;
        remainingList &= ~readList;
    } while (atomic_compare_exchange_weak(&g_debugPausePausedCoreList, &readList, desiredList));

    if (remainingList != BIT(currentCoreCtx->coreId)) {
        // We need to notify other cores...
        u32 otherCores = remainingList & ~BIT(currentCoreCtx->coreId);
        barrierInit(&g_debugPauseBarrier, otherCores | BIT(currentCoreCtx->coreId));
        generateSgiForList(ThermosphereSgi_DebugPause, otherCores);
        barrierWait(&g_debugPauseBarrier);
    }

    if (remainingList & BIT(currentCoreCtx->coreId)) {
        debugSetThisCorePaused();
    }

    unmaskIrq();
}

void debugUnpauseCores(u32 coreList, u32 singleStepList)
{
    singleStepList &= coreList;

    __builtin_prefetch(&g_debugPausePausedCoreList, 1, 0);

    // Since we're using a debugger lock, a simple stlr should be fine...
    atomic_store(&g_debugPauseSingleStepCoreList, singleStepList);
    atomic_fetch_and(&g_debugPausePausedCoreList, ~coreList);

    __sev();
}
