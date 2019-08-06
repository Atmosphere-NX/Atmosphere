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

#include "watchpoints.h"
#include "breakpoints_watchpoints_save_restore.h"
#include "utils.h"
#include "sysreg.h"
#include "arm.h"

WatchpointManager g_watchpointManager = {0};

// Init the structure (already in BSS, so already zero-initialized) and load the registers
void initWatchpoints(void)
{
    recursiveSpinlockLock(&g_watchpointManager.lock);

    if (currentCoreCtx->isBootCore && !currentCoreCtx->warmboot) {
        size_t num = ((GET_SYSREG(id_aa64dfr0_el1) >> 20) & 0xF) + 1;
        g_watchpointManager.maxWatchpoints = (u32)num;
        g_watchpointManager.allocationBitmap = 0xFFFF;
    }

    loadWatchpointRegs(g_watchpointManager.watchpoints, g_watchpointManager.maxWatchpoints);

    recursiveSpinlockUnlock(&g_watchpointManager.lock);
}
