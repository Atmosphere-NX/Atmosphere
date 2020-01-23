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

#pragma once

#define _REENT_ONLY
#include <errno.h>

#include "breakpoints_watchpoints_common.h"
#include "spinlock.h"

/// Structure to synchronize and keep track of watchpoints
typedef struct WatchpointManager {
    RecursiveSpinlock lock;
    u32 numSplitWatchpoints;
    u32 maxWatchpoints;
    u32 maxSplitWatchpoints;
    u16 allocationBitmap;
    DebugRegisterPair splitWatchpoints[16 * 8];
} WatchpointManager;

extern WatchpointManager g_watchpointManager;

void initWatchpoints(void);
DebugControlRegister retrieveSplitWatchpointConfig(u64 addr, size_t size, WatchpointLoadStoreControl direction, bool strict);
int addWatchpoint(u64 addr, size_t size, WatchpointLoadStoreControl direction);
int removeWatchpoint(u64 addr, size_t size, WatchpointLoadStoreControl direction);
int removeAllWatchpoints(void);
