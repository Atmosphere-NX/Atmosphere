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
#include "breakpoints_watchpoints_save_restore.h"
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

bool addBreakpoint(u64 addr)
{
    recursiveSpinlockLock(&g_breakpointManager.lock);

    // Reject misaligned addresses
    if (addr & 3) {
        return false;
    }

    // Breakpoint already added
    if (findBreakpoint(addr) != NULL) {
        return true;
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

    recursiveSpinlockUnlock(&g_breakpointManager.lock);

    // TODO commit & broadcast

    return true;
}

bool removeBreakpoint(u64 addr)
{
    recursiveSpinlockLock(&g_breakpointManager.lock);

    DebugRegisterPair *regs = findBreakpoint(addr);

    if (findBreakpoint(addr) == NULL) {
        recursiveSpinlockUnlock(&g_breakpointManager.lock);
        return false;
    }

    freeBreakpoint(regs - &g_breakpointManager.breakpoints[0]);
    recursiveSpinlockUnlock(&g_breakpointManager.lock);

    // TODO commit & broadcast

    return true;
}
