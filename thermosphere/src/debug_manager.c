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
#include <stdarg.h>

#include "debug_manager.h"
#include "core_ctx.h"
#include "irq.h"
#include "spinlock.h"
#include "single_step.h"

#include "gdb/debug.h"

GDBContext g_gdbContext = { 0 };

typedef struct DebugManager {
    DebugEventInfo              debugEventInfos[MAX_CORE];
    uintptr_t                   steppingRangeStartAddrs[MAX_CORE];
    uintptr_t                   steppingRangeEndAddrs[MAX_CORE];

    ALIGN(64) atomic_uint       pausedCoreList;
    atomic_uint                 singleStepCoreList;
    atomic_uint                 eventsSentList;
    Barrier                     pauseBarrier;
} DebugManager;

static DebugManager g_debugManager = { 0 };

static void debugManagerDoPauseCores(u32 coreList)
{
    __builtin_prefetch(&g_debugManager.pausedCoreList, 1, 0);

    u32 desiredList = coreList;
    u32 remainingList = coreList;
    u32 readList = atomic_load(&g_debugManager.pausedCoreList);
    do {
        desiredList |= readList;
        remainingList &= ~readList;
    } while (atomic_compare_exchange_weak(&g_debugManager.pausedCoreList, &readList, desiredList));

    if (remainingList != BIT(currentCoreCtx->coreId)) {
        // We need to notify other cores...
        u32 otherCores = remainingList & ~BIT(currentCoreCtx->coreId);
        barrierInit(&g_debugManager.pauseBarrier, otherCores | BIT(currentCoreCtx->coreId));
        generateSgiForList(ThermosphereSgi_DebugPause, otherCores);
        barrierWait(&g_debugManager.pauseBarrier);
    }

    if (remainingList & BIT(currentCoreCtx->coreId)) {
        currentCoreCtx->wasPaused = true;
    }
}

void debugManagerPauseSgiHandler(void)
{
    currentCoreCtx->wasPaused = true;
    barrierWait(&g_debugManager.pauseBarrier);
}

bool debugManagerHandlePause(void)
{
    u32 coreId = currentCoreCtx->coreId;
    __builtin_prefetch(&g_debugManager.pausedCoreList, 0, 3);

    if (atomic_load(&g_debugManager.pausedCoreList) & BIT(coreId)) {
        unmaskIrq();
        do {
            __wfe();
        } while (atomic_load(&g_debugManager.pausedCoreList) & BIT(coreId));
        maskIrq();

        if (!g_debugManager.debugEventInfos[coreId].handled) {
            // Do we still have an unhandled debug event?
            // TODO build
            //GDB_TrySignalDebugEvent(&g_gdbContext, &g_debugManager.debugEventInfos[coreId]);
            return false;
        }
    }

    currentCoreCtx->wasPaused = false;

    // Single-step: if inactive and requested, start single step; cancel if active and not requested
    u32 ssReqd = (atomic_load(&g_debugManager.singleStepCoreList) & BIT(currentCoreCtx->coreId)) != 0;
    SingleStepState singleStepState = singleStepGetNextState(currentCoreCtx->guestFrame);
    if (ssReqd) {
        currentCoreCtx->steppingRangeStartAddr = g_debugManager.steppingRangeStartAddrs[coreId];
        currentCoreCtx->steppingRangeEndAddr = g_debugManager.steppingRangeEndAddrs[coreId];
        if(singleStepState == SingleStepState_Inactive) {
            singleStepSetNextState(currentCoreCtx->guestFrame, SingleStepState_ActiveNotPending);
        }
    } else if (!ssReqd && singleStepState != SingleStepState_Inactive) {
        singleStepSetNextState(currentCoreCtx->guestFrame, SingleStepState_Inactive);
    }

    return true;
}

void debugManagerPauseCores(u32 coreList)
{
    u64 flags = maskIrq();
    debugManagerDoPauseCores(coreList);
    restoreInterruptFlags(flags);
}

void debugManagerUnpauseCores(u32 coreList, u32 singleStepList)
{
    singleStepList &= coreList;

    FOREACH_BIT (tmp, coreId, coreList) {
        if (&g_debugManager.debugEventInfos[coreId].handled) {
            // Discard already handled debug events
            g_debugManager.debugEventInfos[coreId].type = DBGEVENT_NONE;
        }
    }

    // Since we're using a debugger lock, a simple stlr should be fine...
    atomic_store(&g_debugManager.singleStepCoreList, singleStepList);
    atomic_fetch_and(&g_debugManager.pausedCoreList, ~coreList);

    __sev();
}

void debugManagerSetSteppingRange(u32 coreId, uintptr_t startAddr, uintptr_t endAddr)
{
    g_debugManager.steppingRangeStartAddrs[coreId] = startAddr;
    g_debugManager.steppingRangeEndAddrs[coreId] = endAddr;
}

u32 debugManagerGetPausedCoreList(void)
{
    return atomic_load(&g_debugManager.pausedCoreList);
}

const DebugEventInfo *debugManagerMarkAndGetCoreDebugEvent(u32 coreId)
{
    g_debugManager.debugEventInfos[coreId].handled = true;
    return &g_debugManager.debugEventInfos[coreId];
}

void debugManagerReportEvent(DebugEventType type, ...)
{
    u64 flags = maskIrq();
    u32 coreId = currentCoreCtx->coreId;

    DebugEventInfo *info = &g_debugManager.debugEventInfos[coreId];

    info->type = type;
    info->coreId = coreId;
    info->frame = currentCoreCtx->guestFrame;

    va_list args;
    va_start(args, type);

    switch (type) {
        case DBGEVENT_OUTPUT_STRING:
            info->outputString.address = va_arg(args, uintptr_t);
            info->outputString.size = va_arg(args, size_t);
            break;
        default:
            break;
    }

    va_end(args);

    // Now, pause ourselves and try to signal we have a debug event
    debugManagerDoPauseCores(BIT(coreId));

    exceptionEnterInterruptibleHypervisorCode();
    unmaskIrq();

    // TODO GDB_TrySignalDebugEvent(&g_gdbContext, info);

    restoreInterruptFlags(flags);
}
