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
#include "watchpoints.h"
#include "breakpoints_watchpoints_load.h"
#include "utils.h"
#include "core_ctx.h"
#include "execute_function.h"
#include "debug_log.h"

static WatchpointManager g_watchpointManager = {0};

// Init the structure (already in BSS, so already zero-initialized) and load the registers
void initWatchpoints(void)
{
    recursiveSpinlockLock(&g_watchpointManager.lock);

    if (currentCoreCtx->isBootCore && !currentCoreCtx->warmboot) {
        size_t num = ((GET_SYSREG(id_aa64dfr0_el1) >> 20) & 0xF) + 1;
        g_watchpointManager.maxWatchpoints = (u32)num;
        g_watchpointManager.allocationBitmap = BIT(num) - 1;
    }

    loadWatchpointRegs(g_watchpointManager.watchpoints, g_watchpointManager.maxWatchpoints);

    recursiveSpinlockUnlock(&g_watchpointManager.lock);
}

static void commitAndBroadcastWatchpointHandler(void *p)
{
    (void)p;
    u64 flags = maskIrq();
    loadWatchpointRegs(g_watchpointManager.watchpoints, g_watchpointManager.maxWatchpoints);
    restoreInterruptFlags(flags);
}

static inline void commitAndBroadcastWatchpoints(void)
{
    __dmb();
    executeFunctionOnAllCores(commitAndBroadcastWatchpointHandler, NULL, true);
}

static DebugRegisterPair *allocateWatchpoint(void)
{
    u32 pos = __builtin_ffs(g_watchpointManager.allocationBitmap);
    if (pos == 0) {
        return NULL;
    } else {
        g_watchpointManager.allocationBitmap &= ~BIT(pos - 1);
        return &g_watchpointManager.watchpoints[pos - 1];
    }
}

static void freeWatchpoint(u32 pos)
{
    memset(&g_watchpointManager.watchpoints[pos], 0, sizeof(DebugRegisterPair));
    g_watchpointManager.allocationBitmap |= BIT(pos);
}

static inline bool isRangeMaskWatchpoint(uintptr_t addr, size_t size)
{
    // size needs to be a power of 2, at least 8 (we'll only allow 16+ though), addr needs to be aligned.
    bool ret = (size & (size - 1)) == 0 && size >= 16 && (addr & (size - 1)) == 0;
    return ret;
}

// Size = 0 means nonstrict
static DebugRegisterPair *findWatchpoint(uintptr_t addr, size_t size, WatchpointLoadStoreControl direction)
{
    u64 bitmap = ~g_watchpointManager.allocationBitmap & 0xFFFF;
    FOREACH_BIT (tmp, i, bitmap) {
        DebugRegisterPair *wp = &g_watchpointManager.watchpoints[i];

        size_t off;
        size_t sz;
        size_t nmask;
    
        if (wp->cr.mask != 0) {
            off = 0;
            sz = MASK(wp->cr.mask);
            nmask = ~sz;
        } else {
            off = __builtin_ffs(wp->cr.bas) - 1;
            sz = __builtin_popcount(wp->cr.bas);
            nmask = ~7ul;
        }

        if (size != 0) {
            // Strict watchpoint check
            if (addr == wp->vr + off && direction == wp->cr.lsc && sz == size) {
                return wp;
            }
        } else {
            // Return first wp that could have triggered the exception
            if ((addr & nmask) == wp->vr && (direction & wp->cr.lsc) != 0) {
                return wp;
            }
        }
    }

    return NULL;
}

DebugControlRegister retrieveWatchpointConfig(uintptr_t addr, WatchpointLoadStoreControl direction)
{
    recursiveSpinlockLock(&g_watchpointManager.lock);
    DebugRegisterPair *wp = findWatchpoint(addr, 0, direction);
    DebugControlRegister ret = { 0 };
    if (wp != NULL) {
        ret = wp->cr;
    }
    recursiveSpinlockUnlock(&g_watchpointManager.lock);
    return ret;
}

static inline bool checkWatchpointAddressAndSizeParams(uintptr_t addr, size_t size)
{
    if (size == 0) {
        return false;
    } else if (size > 8) {
        return isRangeMaskWatchpoint(addr, size);
    } else {
        return ((addr + size) & ~7ul) == (addr & ~7ul);
    }
}

int addWatchpoint(uintptr_t addr, size_t size, WatchpointLoadStoreControl direction)
{
    if (!checkWatchpointAddressAndSizeParams(addr, size)) {
        return -EINVAL;
    }

    recursiveSpinlockLock(&g_watchpointManager.lock);

    if (g_watchpointManager.allocationBitmap == 0) {
        recursiveSpinlockUnlock(&g_watchpointManager.lock);
        return -EBUSY;
    }

    if (findWatchpoint(addr, size, direction)) {
        recursiveSpinlockUnlock(&g_watchpointManager.lock);
        return -EEXIST;
    }

    DebugRegisterPair *wp = allocateWatchpoint();
    memset(wp, 0, sizeof(DebugRegisterPair));

    wp->cr.lsc = direction;
    if (isRangeMaskWatchpoint(addr, size)) {
        wp->vr = addr;
        wp->cr.bas = 0xFF; // TRM-mandated
        wp->cr.mask = (u32)__builtin_ffsl(size) - 1;
    } else {
        size_t off = addr & 7ull;
        size_t sz = 8 - off;
        wp->vr = addr & ~7ul;
        wp->cr.bas = MASK2(off + sz, off);
    }

    wp->cr.linked = false;

    wp->cr.hmc = DebugHmc_LowerEl;
    wp->cr.ssc = DebugSsc_NonSecure;
    wp->cr.pmc = DebugPmc_El1And0;
    wp->cr.enabled = true;

    commitAndBroadcastWatchpoints();

    recursiveSpinlockUnlock(&g_watchpointManager.lock);

    return 0;
}

int removeWatchpoint(uintptr_t addr, size_t size, WatchpointLoadStoreControl direction)
{
    if (!checkWatchpointAddressAndSizeParams(addr, size)) {
        return -EINVAL;
    }

    recursiveSpinlockLock(&g_watchpointManager.lock);

    DebugRegisterPair *wp = findWatchpoint(addr, size, direction);
    if (wp != NULL) {
        freeWatchpoint(wp - &g_watchpointManager.watchpoints[0]);
    } else {
        recursiveSpinlockUnlock(&g_watchpointManager.lock);
        return -ENOENT;
    }

    commitAndBroadcastWatchpoints();
    recursiveSpinlockUnlock(&g_watchpointManager.lock);

    return 0;
}

int removeAllWatchpoints(void)
{
    // Yeet it all

    recursiveSpinlockLock(&g_watchpointManager.lock);

    g_watchpointManager.allocationBitmap = BIT(g_watchpointManager.maxWatchpoints) - 1;
    memset(g_watchpointManager.watchpoints, 0, sizeof(g_watchpointManager.watchpoints));

    commitAndBroadcastWatchpoints();
    recursiveSpinlockUnlock(&g_watchpointManager.lock);

    return 0;
}
