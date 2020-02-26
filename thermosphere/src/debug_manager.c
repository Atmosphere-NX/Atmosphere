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
#include <string.h>

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
    atomic_bool                 reportingEnabled;
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
    } while (!atomic_compare_exchange_weak(&g_debugManager.pausedCoreList, &readList, desiredList));

    if (remainingList & ~BIT(currentCoreCtx->coreId)) {
        // We need to notify other cores...
        u32 otherCores = remainingList & ~BIT(currentCoreCtx->coreId);
        barrierInit(&g_debugManager.pauseBarrier, otherCores | BIT(currentCoreCtx->coreId));
        generateSgiForList(ThermosphereSgi_DebugPause, otherCores);
        barrierWait(&g_debugManager.pauseBarrier);
    }

    if (remainingList & BIT(currentCoreCtx->coreId)) {
        currentCoreCtx->wasPaused = true;
    }

    __sev();
}

void debugManagerPauseSgiHandler(void)
{
    currentCoreCtx->wasPaused = true;
    barrierWait(&g_debugManager.pauseBarrier);
}

void debugManagerInit(TransportInterfaceType gdbIfaceType, u32 gdbIfaceId, u32 gdbIfaceFlags)
{
    memset(&g_debugManager, 0, sizeof(DebugManager));
    GDB_InitializeContext(&g_gdbContext, gdbIfaceType, gdbIfaceId, gdbIfaceFlags);
    GDB_MigrateRxIrq(&g_gdbContext, currentCoreCtx->coreId);
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
            GDB_TrySignalDebugEvent(&g_gdbContext, &g_debugManager.debugEventInfos[coreId]);
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

void debugManagerSetReportingEnabled(bool enabled)
{
    atomic_store(&g_debugManager.reportingEnabled, enabled);
}

bool debugManagerHasDebugEvent(u32 coreId)
{
    bool isPaused = debugManagerIsCorePaused(coreId);
    return isPaused && g_debugManager.debugEventInfos[coreId].type != DBGEVENT_NONE;
}

void debugManagerPauseCores(u32 coreList)
{
    u64 flags = maskIrq();
    debugManagerDoPauseCores(coreList);
    restoreInterruptFlags(flags);
}

void debugManagerSetSingleStepCoreList(u32 coreList)
{
    atomic_store(&g_debugManager.singleStepCoreList, coreList);
}

void debugManagerUnpauseCores(u32 coreList)
{
    FOREACH_BIT (tmp, coreId, coreList) {
        if (&g_debugManager.debugEventInfos[coreId].handled) {
            // Discard already handled debug events
            g_debugManager.debugEventInfos[coreId].type = DBGEVENT_NONE;
        }
    }

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

DebugEventInfo *debugManagerGetDebugEvent(u32 coreId)
{
    return &g_debugManager.debugEventInfos[coreId];
}

void debugManagerReportEvent(DebugEventType type, ...)
{
    u64 flags = maskIrq();
    bool reportingEnabled = atomic_load(&g_debugManager.reportingEnabled);
    if (!reportingEnabled && type != DBGEVENT_DEBUGGER_BREAK) {
        restoreInterruptFlags(flags);
        return;
    }

    u32 coreId = currentCoreCtx->coreId;

    DebugEventInfo *info = &g_debugManager.debugEventInfos[coreId];
    memset(info, 0 , sizeof(DebugEventInfo));

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

    if (reportingEnabled) {
        exceptionEnterInterruptibleHypervisorCode();
        unmaskIrq();

        GDB_TrySignalDebugEvent(&g_gdbContext, info);
    }

    restoreInterruptFlags(flags);
}

void debugManagerBreakCores(u32 coreList)
{
    u32 coreId = currentCoreCtx->coreId;
    if (coreList & ~BIT(coreId)) {
        generateSgiForList(ThermosphereSgi_ReportDebuggerBreak, coreList & ~BIT(coreId));
    }
    if ((coreList & BIT(coreId)) && !debugManagerHasDebugEvent(coreId)) {
        debugManagerReportEvent(DBGEVENT_DEBUGGER_BREAK);
    }

    // Wait for all cores
    __sevl();
    do {
        __wfe();
    } while ((atomic_load(&g_debugManager.pausedCoreList) & coreList) != coreList);
}

void debugManagerContinueCores(u32 coreList)
{
    u32 coreId = currentCoreCtx->coreId;
    if (coreList & ~BIT(coreId)) {
        generateSgiForList(ThermosphereSgi_DebuggerContinue, coreList & ~BIT(coreId));
    }
    if (coreList & BIT(coreId) && debugManagerIsCorePaused(coreId)) {
        debugManagerUnpauseCores(BIT(coreId));
    }

    // Wait for all cores
    __sevl();
    do {
        __wfe();
    } while ((atomic_load(&g_debugManager.pausedCoreList) & coreList) != 0);
}

/*     u64 mdcr = GET_SYSREG(mdcr_el2);

    // Trap Debug Exceptions, and accesses to debug registers.
    mdcr |= MDCR_EL2_TDE;

    // Implied from TDE
    mdcr |= MDCR_EL2_TDRA | MDCR_EL2_TDOSA | MDCR_EL2_TDA;

    SET_SYSREG(mdcr_el2, mdcr);
*/
