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

#include <string.h>
#include "software_breakpoints.h"
#include "utils.h"
#include "guest_memory.h"
#include "core_ctx.h"

SoftwareBreakpointManager g_softwareBreakpointManager = {0};

/*
    Consider the following:
        - Breakpoints are based on VA
        - Translation tables may change
        - Translation tables may differ from core to core

    We also define sw breakpoints on invalid addresses (for one or more cores) UNPREDICTABLE.
*/

static size_t findClosestSoftwareBreakpointSlot(uintptr_t address)
{
    if(g_softwareBreakpointManager.numBreakpoints == 0 || address <= g_softwareBreakpointManager.breakpoints[0].address) {
        return 0;
    } else if(address > g_softwareBreakpointManager.breakpoints[g_softwareBreakpointManager.numBreakpoints - 1].address) {
        return g_softwareBreakpointManager.numBreakpoints;
    }

    size_t a = 0, b = g_softwareBreakpointManager.numBreakpoints - 1, m;

    do {
        m = (a + b) / 2;
        if(g_softwareBreakpointManager.breakpoints[m].address < address) {
            a = m;
        } else if(g_softwareBreakpointManager.breakpoints[m].address > address) {
            b = m;
        } else {
            return m;
        }
    } while(b - a > 1);

    return b;
}

static inline bool doApplySoftwareBreakpoint(size_t id)
{
    SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
    u32 brkInst = 0xF2000000 | bp->uid;

    size_t sz = guestReadWriteMemory(bp->address, 4, &bp->savedInstruction, &brkInst);
    bp->applied = sz == 4;
    atomic_store(&bp->triedToApplyOrRevert, true);
    return sz == 4;
}

static void applySoftwareBreakpointHandler(void *p)
{
    size_t id = *(size_t *)p;
    if (currentCoreCtx->coreId == currentCoreCtx->executedFunctionSrcCore) {
        doApplySoftwareBreakpoint(id);
        __sev();
    } else {
        SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
        while (!atomic_load(&bp->triedToApplyOrRevert)) {
            __wfe();
        }
    }
}

static bool applySoftwareBreakpoint(size_t id)
{
    if (g_softwareBreakpointManager.breakpoints[id].applied) {
        return true;
    }

    // This is okay for non-stop mode if sync isn't perfect here
    atomic_store(&g_softwareBreakpointManager.breakpoints[id].triedToApplyOrRevert, false);
    executeFunctionOnAllCores(applySoftwareBreakpointHandler, &id, true);
    atomic_signal_fence(memory_order_seq_cst);
    return g_softwareBreakpointManager.breakpoints[id].applied;
}

static inline bool doRevertSoftwareBreakpoint(size_t id)
{
    SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];

    size_t sz = guestWriteMemory(bp->address, 4, &bp->savedInstruction);
    bp->applied = sz != 4;
    atomic_store(&bp->triedToApplyOrRevert, true);
    return sz == 4;
}

static void revertSoftwareBreakpointHandler(void *p)
{
    size_t id = *(size_t *)p;
    if (currentCoreCtx->coreId == currentCoreCtx->executedFunctionSrcCore) {
        doRevertSoftwareBreakpoint(id);
        __sev();
    } else {
        SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
        while (!atomic_load(&bp->triedToApplyOrRevert)) {
            __wfe();
        }
    }
}

static bool revertSoftwareBreakpoint(size_t id)
{
    if (!g_softwareBreakpointManager.breakpoints[id].applied) {
        return true;
    }

    atomic_store(&g_softwareBreakpointManager.breakpoints[id].triedToApplyOrRevert, false);
    executeFunctionOnAllCores(revertSoftwareBreakpointHandler, &id, true);
    atomic_signal_fence(memory_order_seq_cst);
    return !g_softwareBreakpointManager.breakpoints[id].applied;
}

bool applyAllSoftwareBreakpoints(void)
{
    u64 flags = recursiveSpinlockLockMaskIrq(&g_softwareBreakpointManager.lock);
    bool ret = true;

    for (size_t i = 0; i < g_softwareBreakpointManager.numBreakpoints; i++) {
        ret = ret && doApplySoftwareBreakpoint(i); // note: no broadcast intentional
    }

    recursiveSpinlockUnlockRestoreIrq(&g_softwareBreakpointManager.lock, flags);
    return ret;
}

bool revertAllSoftwareBreakpoints(void)
{
    u64 flags = recursiveSpinlockLockMaskIrq(&g_softwareBreakpointManager.lock);
    bool ret = true;
    for (size_t i = 0; i < g_softwareBreakpointManager.numBreakpoints; i++) {
        ret = ret && doRevertSoftwareBreakpoint(i); // note: no broadcast intentional
    }

    recursiveSpinlockUnlockRestoreIrq(&g_softwareBreakpointManager.lock, flags);
    return ret;
}

int addSoftwareBreakpoint(uintptr_t addr, bool persistent)
{
    if ((addr & 3) != 0) {
        return -EINVAL;
    }

    recursiveSpinlockLock(&g_softwareBreakpointManager.lock);

    size_t id = findClosestSoftwareBreakpointSlot(addr);

    if(id != g_softwareBreakpointManager.numBreakpoints && g_softwareBreakpointManager.breakpoints[id].address == addr) {
        recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);
        return -EEXIST;
    } else if(g_softwareBreakpointManager.numBreakpoints == MAX_SW_BREAKPOINTS) {
        recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);
        return -EBUSY;
    }

    for(size_t i = g_softwareBreakpointManager.numBreakpoints; i > id && i != 0; i--) {
        g_softwareBreakpointManager.breakpoints[i] = g_softwareBreakpointManager.breakpoints[i - 1];
    }
    ++g_softwareBreakpointManager.numBreakpoints;

    SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
    bp->address = addr;
    bp->persistent = persistent;
    bp->applied = false;
    bp->uid = 0x2000 + g_softwareBreakpointManager.bpUniqueCounter++;

    int rc = applySoftwareBreakpoint(id) ? 0 : -EFAULT;
    recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);

    return rc;
}

int removeSoftwareBreakpoint(uintptr_t addr, bool keepPersistent)
{
    if ((addr & 3) != 0) {
        return -EINVAL;
    }

    recursiveSpinlockLock(&g_softwareBreakpointManager.lock);

    size_t id = findClosestSoftwareBreakpointSlot(addr);
    bool ok = true;

    if(id == g_softwareBreakpointManager.numBreakpoints || g_softwareBreakpointManager.breakpoints[id].address != addr) {
        recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);
        return -ENOENT;
    }

    SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
    if (!keepPersistent || !bp->persistent) {
        ok = revertSoftwareBreakpoint(id);
    }

    for(size_t i = id; i < g_softwareBreakpointManager.numBreakpoints - 1; i++) {
        g_softwareBreakpointManager.breakpoints[i] = g_softwareBreakpointManager.breakpoints[i + 1];
    }

    memset(&g_softwareBreakpointManager.breakpoints[--g_softwareBreakpointManager.numBreakpoints], 0, sizeof(SoftwareBreakpoint));

    recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);

    return ok ? 0 : -EFAULT;
}

int removeAllSoftwareBreakpoints(bool keepPersistent)
{
    bool ok = true;
    recursiveSpinlockLock(&g_softwareBreakpointManager.lock);

    for (size_t id = 0; id < g_softwareBreakpointManager.numBreakpoints; id++) {
        SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
        if (!keepPersistent || !bp->persistent) {
            ok = ok && revertSoftwareBreakpoint(id);
        }
    }

    g_softwareBreakpointManager.numBreakpoints = 0;
    g_softwareBreakpointManager.bpUniqueCounter = 0;
    memset(g_softwareBreakpointManager.breakpoints, 0, sizeof(g_softwareBreakpointManager.breakpoints));

    recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);

    return ok ? 0 : -EFAULT;
}
