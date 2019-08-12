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
#include "breakpoints.h"
#include "breakpoints_watchpoints_load.h"
#include "utils.h"
#include "sysreg.h"
#include "arm.h"

BreakpointManager g_breakpointManager = {0};

// Init the structure (already in BSS, so already zero-initialized) and load the registers
void initBreakpoints(void)
{
    recursiveSpinlockLock(&g_breakpointManager.lock);

    if (currentCoreCtx->isBootCore && !currentCoreCtx->warmboot) {
        size_t num = ((GET_SYSREG(id_aa64dfr0_el1) >> 12) & 0xF) + 1;
        g_breakpointManager.maxBreakpoints = (u32)num;
        g_breakpointManager.allocationBitmap = BIT(num) - 1;
    }

    loadBreakpointRegs(g_breakpointManager.breakpoints, g_breakpointManager.maxBreakpoints);

    recursiveSpinlockUnlock(&g_breakpointManager.lock);
}

static void commitAndBroadcastBreakpointHandler(void *p)
{
    (void)p;
    u64 flags = maskIrq();
    loadBreakpointRegs(g_breakpointManager.breakpoints, g_breakpointManager.maxBreakpoints);
    restoreInterruptFlags(flags);
}

static inline void commitAndBroadcastBreakpoints(void)
{
    executeFunctionOnAllCores(commitAndBroadcastBreakpointHandler, NULL, true);
}

static DebugRegisterPair *allocateBreakpoint(void)
{
    u32 pos = __builtin_ffs(g_breakpointManager.allocationBitmap);
    if (pos == 0) {
        return NULL;
    } else {
        g_breakpointManager.allocationBitmap &= ~BIT(pos - 1);
        return &g_breakpointManager.breakpoints[pos - 1];
    }
}

static void freeBreakpoint(u32 pos)
{
    memset(&g_breakpointManager.breakpoints[pos], 0, sizeof(DebugRegisterPair));
    g_breakpointManager.allocationBitmap |= BIT(pos);
}

static DebugRegisterPair *findBreakpoint(u64 addr)
{
    u16 bitmap = ~g_breakpointManager.allocationBitmap & 0xFFFF;
    while (bitmap != 0) {
        u32 pos = __builtin_ffs(bitmap);
        if (pos == 0) {
            return NULL;
        } else {
            bitmap &= ~BIT(pos - 1);
            if (g_breakpointManager.breakpoints[pos - 1].vr == addr) {
                return &g_breakpointManager.breakpoints[pos - 1];
            }
        }
    }

    return NULL;
}

// Note: A32/T32/T16 support intentionnally left out
// Note: addresses are supposed to be well-formed regarding the sign extension bits

int addBreakpoint(u64 addr)
{
    recursiveSpinlockLock(&g_breakpointManager.lock);

    // Reject misaligned addresses
    if (addr & 3) {
        recursiveSpinlockUnlock(&g_breakpointManager.lock);
        return -EINVAL;
    }

    // Breakpoint already added
    if (findBreakpoint(addr) != NULL) {
        recursiveSpinlockUnlock(&g_breakpointManager.lock);
        return -EEXIST;
    }

    DebugRegisterPair *regs = allocateBreakpoint();
    regs->cr.bt = BreakpointType_AddressMatch;
    regs->cr.linked = false;

    // NS EL1&0 only
    regs->cr.hmc = DebugHmc_LowerEl;
    regs->cr.ssc = DebugSsc_NonSecure;
    regs->cr.pmc = DebugPmc_El1And0;

    regs->cr.bas = 0xF;
    regs->cr.enabled = true;

    regs->vr = addr;

    commitAndBroadcastBreakpoints();

    recursiveSpinlockUnlock(&g_breakpointManager.lock);

    return 0;
}

int removeBreakpoint(u64 addr)
{
    recursiveSpinlockLock(&g_breakpointManager.lock);

    DebugRegisterPair *regs = findBreakpoint(addr);

    if (findBreakpoint(addr) == NULL) {
        recursiveSpinlockUnlock(&g_breakpointManager.lock);
        return -ENOENT;
    }

    freeBreakpoint(regs - &g_breakpointManager.breakpoints[0]);

    commitAndBroadcastBreakpoints();
    recursiveSpinlockUnlock(&g_breakpointManager.lock);

    return 0;
}

int removeAllBreakpoints(void)
{
    recursiveSpinlockLock(&g_breakpointManager.lock);
    g_breakpointManager.allocationBitmap = BIT(g_breakpointManager.maxBreakpoints) - 1;
    memset(g_breakpointManager.breakpoints, 0, sizeof(g_breakpointManager.breakpoints));

    commitAndBroadcastBreakpoints();

    recursiveSpinlockUnlock(&g_breakpointManager.lock);

    return 0;
}