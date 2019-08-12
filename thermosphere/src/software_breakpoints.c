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
#include "arm.h"

SoftwareBreakpointManager g_softwareBreakpointManager = {0};

/*
    Consider the following:
        - Breakpoints are based on VA
        - Translation tables may change
        - Translation tables may differ from core to core

    We also define sw breakpoints on invalid addresses (for one or more cores) UNPREDICTABLE.
*/

static size_t findClosestSoftwareBreakpointSlot(u64 address)
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
    if (bp->applied) {
        return true;
    }

    u32 brkInst = 0xF2000000 | bp->uid;

    if (readEl1Memory(&bp->savedInstruction, bp->address, 4) && writeEl1Memory(bp->address, &brkInst, 4)) {
        bp->applied = true;
        return true;
    }

    return false;
}

static void applySoftwareBreakpointHandler(void *p)
{
    u64 flags = maskIrq();
    doApplySoftwareBreakpoint(*(size_t *)p);
    restoreInterruptFlags(flags);
}

static void applySoftwareBreakpoint(size_t id)
{
    executeFunctionOnAllCores(applySoftwareBreakpointHandler, &id, true);
}

static inline bool doRevertSoftwareBreakpoint(size_t id)
{
    SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
    if (!bp->applied) {
        return true;
    }

    if (writeEl1Memory(bp->address, &bp->savedInstruction, 4)) {
        bp->applied = false;
        return true;
    }

    return false;
}

static void revertSoftwareBreakpointHandler(void *p)
{
    u64 flags = maskIrq();
    doRevertSoftwareBreakpoint(*(size_t *)p);
    restoreInterruptFlags(flags);
}

static void revertSoftwareBreakpoint(size_t id)
{
    executeFunctionOnAllCores(revertSoftwareBreakpointHandler, &id, true);
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

int addSoftwareBreakpoint(u64 addr, bool persistent)
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

    applySoftwareBreakpoint(id);
    // Note: no way to handle breakpoint failing to apply on 1+ core but not all, we need to assume operation succeeds
    recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);

    return 0;
}

int removeSoftwareBreakpoint(u64 addr, bool keepPersistent)
{
    if ((addr & 3) != 0) {
        return -EINVAL;
    }

    recursiveSpinlockLock(&g_softwareBreakpointManager.lock);

    size_t id = findClosestSoftwareBreakpointSlot(addr);

    if(id == g_softwareBreakpointManager.numBreakpoints || g_softwareBreakpointManager.breakpoints[id].address != addr) {
        recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);
        return -ENOENT;
    }

    SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
    if (!keepPersistent || !bp->persistent) {
        revertSoftwareBreakpoint(id);
        // Note: no way to handle breakpoint failing to revert on 1+ core but not all, we need to assume operation succeeds
    }

    for(size_t i = id; i < g_softwareBreakpointManager.numBreakpoints - 1; i++) {
        g_softwareBreakpointManager.breakpoints[i] = g_softwareBreakpointManager.breakpoints[i + 1];
    }

    memset(&g_softwareBreakpointManager.breakpoints[--g_softwareBreakpointManager.numBreakpoints], 0, sizeof(SoftwareBreakpoint));

    recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);

    return 0;
}

int removeAllSoftwareBreakpoints(bool keepPersistent)
{
    int ret = 0;
    recursiveSpinlockLock(&g_softwareBreakpointManager.lock);

    for (size_t id = 0; id < g_softwareBreakpointManager.numBreakpoints; id++) {
        SoftwareBreakpoint *bp = &g_softwareBreakpointManager.breakpoints[id];
        if (!keepPersistent || !bp->persistent) {
            revertSoftwareBreakpoint(id);
            // Note: no way to handle breakpoint failing to revert on 1+ core but not all, we need to assume operation succeeds
        }
    }

    g_softwareBreakpointManager.numBreakpoints = 0;
    g_softwareBreakpointManager.bpUniqueCounter = 0;
    memset(g_softwareBreakpointManager.breakpoints, 0, sizeof(g_softwareBreakpointManager.breakpoints));

    recursiveSpinlockUnlock(&g_softwareBreakpointManager.lock);

    return ret;
}