/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "gdb/watchpoints.h"
#include "csvc.h"

#define _REENT_ONLY
#include <errno.h>

/*
    There are only 2 Watchpoint Register Pairs on MPCORE ARM11 CPUs,
    and only 2 Breakpoint Register Pairs with context ID capabilities (BRP4-5) as well.

    We'll reserve and use all 4 of them
*/

RecursiveLock watchpointManagerLock;

typedef struct Watchpoint
{
    u32 address;
    u32 size;
    WatchpointKind kind;
    Handle debug; // => context ID
} Watchpoint;

typedef struct WatchpointManager
{
    u32 total;
    Watchpoint watchpoints[2];
} WatchpointManager;

static WatchpointManager manager;

void GDB_ResetWatchpoints(void)
{
    static bool lockInitialized = false;
    if(!lockInitialized)
    {
        RecursiveLock_Init(&watchpointManagerLock);
        lockInitialized = true;
    }
    RecursiveLock_Lock(&watchpointManagerLock);

    svcKernelSetState(0x10003); // enable monitor mode debugging
    svcKernelSetState(0x10004, 0); // disable watchpoint 0
    svcKernelSetState(0x10004, 1); // disable watchpoint 1

    memset(&manager, 0, sizeof(WatchpointManager));

    RecursiveLock_Unlock(&watchpointManagerLock);
}

int GDB_AddWatchpoint(GDBContext *ctx, u32 address, u32 size, WatchpointKind kind)
{
    RecursiveLock_Lock(&watchpointManagerLock);

    u32 offset = address - (address & ~3);

    if(manager.total == 2)
        return -EBUSY;

    if(size == 0 || (offset + size) > 4 || kind == WATCHPOINT_DISABLED)
        return -EINVAL;

    if(GDB_GetWatchpointKind(ctx, address) != WATCHPOINT_DISABLED)
        // Disallow duplicate watchpoints: the kernel doesn't give us sufficient info to differentiate them by kind (DFSR)
        return -EINVAL;

    u32 id = manager.watchpoints[0].kind == WATCHPOINT_DISABLED ? 0 : 1;
    u32 selectMask = ((1 << size) - 1) << offset;

    u32 WCR = (1          << 20) | /* linked */
              ((4 + id)   << 16) | /* ID of the linked BRP */
              (selectMask <<  5) | /* byte address select */
              ((u32)kind  <<  3) | /* kind */
              (2          <<  1) | /* user mode only */
              (1          <<  0) ; /* enabled */

    s64 out;

    Result r = svcGetHandleInfo(&out, ctx->debug, 0x10000); // context ID

    if(R_SUCCEEDED(r))
    {
        svcKernelSetState(id == 0 ? 0x10005 : 0x10006, address, WCR, (u32)out); // set watchpoint
        Watchpoint *watchpoint = &manager.watchpoints[id];
        manager.total++;
        watchpoint->address = address;
        watchpoint->size = size;
        watchpoint->kind = kind;
        watchpoint->debug = ctx->debug;
        ctx->watchpoints[ctx->nbWatchpoints++] = address;
        RecursiveLock_Unlock(&watchpointManagerLock);
        return 0;
    }
    else
    {
        RecursiveLock_Unlock(&watchpointManagerLock);
        return -EINVAL;
    }
}

int GDB_RemoveWatchpoint(GDBContext *ctx, u32 address, WatchpointKind kind)
{
    RecursiveLock_Lock(&watchpointManagerLock);

    u32 id;
    for(id = 0; id < 2 && manager.watchpoints[id].address != address && manager.watchpoints[id].debug != ctx->debug; id++);

    if(id == 2 || (kind != WATCHPOINT_DISABLED && manager.watchpoints[id].kind != kind))
    {
        RecursiveLock_Unlock(&watchpointManagerLock);
        return -EINVAL;
    }
    else
    {
        svcKernelSetState(0x10004, id); // disable watchpoint

        memset(&manager.watchpoints[id], 0, sizeof(Watchpoint));
        manager.total--;

        if(ctx->watchpoints[0] == address)
        {
            ctx->watchpoints[0] = ctx->watchpoints[1];
            ctx->watchpoints[1] = 0;
            ctx->nbWatchpoints--;
        }
        else if(ctx->watchpoints[1] == address)
        {
            ctx->watchpoints[1] = 0;
            ctx->nbWatchpoints--;
        }

        RecursiveLock_Unlock(&watchpointManagerLock);

        return 0;
    }
}

WatchpointKind GDB_GetWatchpointKind(GDBContext *ctx, u32 address)
{
    u32 id;
    for(id = 0; id < 2 && (manager.watchpoints[id].address != address || manager.watchpoints[id].debug != ctx->debug); id++);

    return id == 2 ? WATCHPOINT_DISABLED : manager.watchpoints[id].kind;
}
