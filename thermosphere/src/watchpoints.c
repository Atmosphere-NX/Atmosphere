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
#include "sysreg.h"
#include "arm.h"
#include "debug_log.h"

WatchpointManager g_watchpointManager = {0};
static TEMPORARY DebugRegisterPair g_combinedWatchpoints[16] = {0};

static void combineAllCurrentWatchpoints(void);

// Init the structure (already in BSS, so already zero-initialized) and load the registers
void initWatchpoints(void)
{
    recursiveSpinlockLock(&g_watchpointManager.lock);

    if (currentCoreCtx->isBootCore && !currentCoreCtx->warmboot) {
        size_t num = ((GET_SYSREG(id_aa64dfr0_el1) >> 20) & 0xF) + 1;
        g_watchpointManager.maxWatchpoints = (u32)num;
        g_watchpointManager.maxSplitWatchpoints = 8 * (u32)num;
        g_watchpointManager.allocationBitmap = BIT(num) - 1;
    } else if (currentCoreCtx->isBootCore) {
        combineAllCurrentWatchpoints();
    }

    loadWatchpointRegs(g_combinedWatchpoints, g_watchpointManager.maxWatchpoints);

    recursiveSpinlockUnlock(&g_watchpointManager.lock);
}

static void commitAndBroadcastWatchpointHandler(void *p)
{
    (void)p;
    u64 flags = maskIrq();
    loadWatchpointRegs(g_combinedWatchpoints, g_watchpointManager.maxWatchpoints);
    restoreInterruptFlags(flags);
}

static inline void commitAndBroadcastWatchpoints(void)
{
    __dmb_sy();
    executeFunctionOnAllCores(commitAndBroadcastWatchpointHandler, NULL, true);
}

static DebugRegisterPair *findCombinedWatchpoint(u64 addr)
{
    addr &= ~7ull;
    u16 bitmap = ~g_watchpointManager.allocationBitmap & 0xFFFF;
    while (bitmap != 0) {
        u32 pos = __builtin_ffs(bitmap);
        if (pos == 0) {
            return NULL;
        } else {
            bitmap &= ~BIT(pos - 1);
            if (g_combinedWatchpoints[pos - 1].vr == addr) {
                return &g_combinedWatchpoints[pos - 1];
            }
        }
    }

    return NULL;
}

static DebugRegisterPair *allocateCombinedWatchpoint(u16 *bitmap)
{
    u32 pos = __builtin_ffs(*bitmap);
    if (pos == 0) {
        return NULL;
    } else {
        *bitmap &= ~BIT(pos - 1);
        return &g_combinedWatchpoints[pos - 1];
    }
}

// Precondition: not a MASK-based watchpoint
static bool checkNormalWatchpointRange(u64 addr, size_t size)
{
    u16 bitmap = g_watchpointManager.allocationBitmap;
    if (findCombinedWatchpoint(addr) == NULL) {
        if (allocateCombinedWatchpoint(&bitmap) == NULL) {
            return false;
        }
    }

    // if it overlaps...
    u64 addr2 = (addr + size) & ~7ull;

    if (addr2 != (addr & ~7ull)) {
        if (findCombinedWatchpoint(addr2) == NULL) {
            return allocateCombinedWatchpoint(&bitmap) != NULL;
        }
    }

    return true;
}

static inline bool isRangeMaskWatchpoint(u64 addr, size_t size)
{
    // size needs to be a power of 2, at least 8 (we'll only allow 16+ though), addr needs to be aligned.
    bool ret = (size & (size - 1)) == 0 && size >= 16 && (addr & (size - 1)) == 0;
    return ret;
}

static bool combineWatchpoint(const DebugRegisterPair *wp)
{
    DebugRegisterPair *wpSlot = NULL;

    wpSlot = findCombinedWatchpoint(wp->vr & ~7ull);

    // To simplify, don't allow combining for wps that use MASK (except if only perms needs to be combined)
    if (wp->cr.mask != 0 && wpSlot != NULL && (wp->cr.mask != wpSlot->cr.mask || wp->vr != wpSlot->vr)) {
        wpSlot = NULL;
    }

    if (wpSlot == NULL) {
        wpSlot = allocateCombinedWatchpoint(&g_watchpointManager.allocationBitmap);
        memset(wpSlot, 0, sizeof(DebugRegisterPair));
        wpSlot->vr = wp->vr & ~7ull;
    }

    if (wpSlot == NULL) {
        return false;
    }

    wpSlot->cr.hmc = DebugHmc_LowerEl;
    wpSlot->cr.ssc = DebugSsc_NonSecure;
    wpSlot->cr.pmc = DebugPmc_El1And0;
    wpSlot->cr.enabled = true;

    // Merge 8-byte selection mask and access permissions (possibly broadening them)
    wpSlot->cr.bas |= wp->cr.bas;
    wpSlot->cr.lsc |= wp->cr.lsc;

    return true;
}

static DebugRegisterPair *doFindSplitWatchpoint(u64 addr, size_t size, WatchpointLoadStoreControl direction, bool strict)
{
    // Note: we will use RES0 bit0_1 of wr in case of overlapping
    for (u32 i = 0; i < g_watchpointManager.numSplitWatchpoints; i++) {
        DebugRegisterPair *wp = &g_watchpointManager.splitWatchpoints[i];
        if (wp->vr & 2) {
            continue;
        }

        size_t off = 0;
        size_t sz = 0;

        if (wp->cr.mask != 0) {
            off = 0;
            sz = size;
        } else {
            off = __builtin_ffs(wp->cr.bas) - 1;
            sz = __builtin_popcount(wp->cr.bas);
            if (wp->vr & 1) {
                DebugRegisterPair *wp2 = &g_watchpointManager.splitWatchpoints[i + 1];
                sz += __builtin_popcount(wp2->cr.bas);
            }
        }

        u64 wpaddr = (wp->vr & ~7ull) + off;
        if (strict) {
            if (addr == wpaddr && direction == wp->cr.lsc && sz == size) {
                return wp;
            }
        } else if (overlaps(wpaddr, sz, addr, size) && (direction & wp->cr.lsc) != 0) {
            return wp;
        }
    }

    return NULL;
}

DebugRegisterPair *findSplitWatchpoint(u64 addr, size_t size, WatchpointLoadStoreControl direction, bool strict)
{
    recursiveSpinlockLock(&g_watchpointManager.lock);
    DebugRegisterPair *ret = doFindSplitWatchpoint(addr, size, direction, strict);
    recursiveSpinlockUnlock(&g_watchpointManager.lock);
    return ret;
}

int addWatchpoint(u64 addr, size_t size, WatchpointLoadStoreControl direction)
{
    if (size == 0) {
        return -EINVAL;
    }

    recursiveSpinlockLock(&g_watchpointManager.lock);

    if (doFindSplitWatchpoint(addr, size, direction, true)) {
        recursiveSpinlockUnlock(&g_watchpointManager.lock);
        return -EEXIST;
    }

    if (g_watchpointManager.numSplitWatchpoints == g_watchpointManager.maxSplitWatchpoints) {
        recursiveSpinlockUnlock(&g_watchpointManager.lock);
        return -EBUSY;
    }

    size_t oldNumSplitWatchpoints = g_watchpointManager.numSplitWatchpoints;
    DebugRegisterPair *wp = &g_watchpointManager.splitWatchpoints[g_watchpointManager.numSplitWatchpoints++], *wp2 = NULL;

    memset(wp, 0, sizeof(DebugRegisterPair));
    wp->cr.lsc = direction;
    if (isRangeMaskWatchpoint(addr, size)) {
        wp->vr = addr;
        wp->cr.bas = 0xFF; // TRM-mandated
        wp->cr.mask = (u32)__builtin_ffsl(size) - 1;
        if (!combineWatchpoint(wp)) {
            g_watchpointManager.numSplitWatchpoints = oldNumSplitWatchpoints;
            recursiveSpinlockUnlock(&g_watchpointManager.lock);
            return -EBUSY;
        }
    } else if (size <= 9) {
        // Normal one or 2 up-to-9-bytes wp(s) (ie. never exceeeds two combined wp)
        // Note: we will use RES0 bit0_1 of wr in case of overlapping
        if (!checkNormalWatchpointRange(addr, size)) {
            g_watchpointManager.numSplitWatchpoints = oldNumSplitWatchpoints;
            recursiveSpinlockUnlock(&g_watchpointManager.lock);
            return -EINVAL;
        }

        u64 addr2 = (addr + size) & ~7ull;
        size_t off1 = addr & 7ull;
        size_t size1 = (addr != addr2) ? 8 - off1 : size;
        size_t size2 = size - size1;
        wp->vr = addr & ~7ull;
        wp->cr.bas = MASK2(off1 + size1, off1);

        if (size2 != 0) {
            if (g_watchpointManager.numSplitWatchpoints == g_watchpointManager.maxSplitWatchpoints) {
                g_watchpointManager.numSplitWatchpoints = oldNumSplitWatchpoints;
                recursiveSpinlockUnlock(&g_watchpointManager.lock);
                return false;
            }
            wp2 = &g_watchpointManager.splitWatchpoints[g_watchpointManager.numSplitWatchpoints++];
            wp2->cr.lsc = direction;
            wp2->cr.bas = MASK2(size2, 0);

            // Note: we will use RES0 bit0_1 of wr in case of overlapping
            wp->vr |= 1;
            wp2->vr = addr2 | 2;
        }

        if (!combineWatchpoint(wp) || (size2 != 0 && !combineWatchpoint(wp2))) {
            g_watchpointManager.numSplitWatchpoints = oldNumSplitWatchpoints;
            recursiveSpinlockUnlock(&g_watchpointManager.lock);
            return -EBUSY;
        }
    } else {
        recursiveSpinlockUnlock(&g_watchpointManager.lock);
        return -EINVAL;
    }

    commitAndBroadcastWatchpoints();

    recursiveSpinlockUnlock(&g_watchpointManager.lock);

    return 0;
}

static void combineAllCurrentWatchpoints(void)
{
    memset(g_combinedWatchpoints, 0, sizeof(g_combinedWatchpoints));
    g_watchpointManager.allocationBitmap = BIT(g_watchpointManager.maxWatchpoints) - 1;
    for (u32 i = 0; i < g_watchpointManager.numSplitWatchpoints; i++) {
        combineWatchpoint(&g_watchpointManager.splitWatchpoints[i]);
    }
}

int removeWatchpoint(u64 addr, size_t size, WatchpointLoadStoreControl direction)
{
    if (size == 0) {
        return -EINVAL;
    }

    recursiveSpinlockLock(&g_watchpointManager.lock);

    DebugRegisterPair *wp = doFindSplitWatchpoint(addr, size, direction, true);
    if (wp != NULL) {
        size_t pos = wp - &g_watchpointManager.splitWatchpoints[0];
        size_t num = (wp->vr & 1) ? 2 : 1;
        for (size_t i = pos + num; i < g_watchpointManager.numSplitWatchpoints; i ++) {
            g_watchpointManager.splitWatchpoints[i - num] = g_watchpointManager.splitWatchpoints[i];
        }
        g_watchpointManager.numSplitWatchpoints -= num;
        combineAllCurrentWatchpoints();
    } else {
        DEBUG("watchpoint not found 0x%016llx, size %llu, direction %d\n", addr, size, direction);
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
    g_watchpointManager.numSplitWatchpoints = 0;
    memset(g_watchpointManager.splitWatchpoints, 0, sizeof(g_watchpointManager.splitWatchpoints));
    memset(g_combinedWatchpoints, 0, sizeof(g_combinedWatchpoints));

    commitAndBroadcastWatchpoints();

    recursiveSpinlockUnlock(&g_watchpointManager.lock);

    return 0;
}
