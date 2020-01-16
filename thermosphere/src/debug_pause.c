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

// Reminder: use these functions behind a lock

static Barrier g_debugPauseBarrier;
static atomic_uint g_debugPausePausedCoreList;
static atomic_uint g_debugPauseSingleStepCoreList;

void debugPauseSgiHandler(void)
{
    currentCoreCtx->wasPaused = true;
    barrierWait(&g_debugPauseBarrier);
}

void debugPauseWaitAndUpdateSingleStep(void)
{
    u32 coreId = currentCoreCtx->coreId;
    if (atomic_load(&g_debugPausePausedCoreList) & BIT(coreId)) {
        unmaskIrq();
        do {
            __wfe();
        } while (atomic_load(&g_debugPausePausedCoreList) & BIT(coreId));
        maskIrq();
    }

    currentCoreCtx->wasPaused = false;

    // Single-step: if inactive and requested, start single step; cancel if active and not requested
    u32 ssReqd = (atomic_load(&g_debugPauseSingleStepCoreList) & ~BIT(currentCoreCtx->coreId)) != 0;
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

    // Since we're using a debugger lock, a simple stlr should be fine...
    atomic_store(&g_debugPausePausedCoreList, coreList);

    if (coreList != BIT(currentCoreCtx->coreId)) {
        // We need to notify other cores...
        u32 otherCores = coreList & ~BIT(currentCoreCtx->coreId);
        barrierInit(&g_debugPauseBarrier, otherCores | BIT(currentCoreCtx->coreId));
        generateSgiForList(ThermosphereSgi_DebugPause, otherCores);
        barrierWait(&g_debugPauseBarrier);
    }

    if (coreList & BIT(currentCoreCtx->coreId)) {
        currentCoreCtx->wasPaused = true;
    }

    unmaskIrq();
}

void debugUnpauseCores(u32 coreList, u32 singleStepList)
{
    singleStepList &= coreList;

    // Since we're using a debugger lock, a simple stlr should be fine...
    atomic_store(&g_debugPauseSingleStepCoreList, singleStepList);
    atomic_store(&g_debugPausePausedCoreList, 0);

    __sev();
}
