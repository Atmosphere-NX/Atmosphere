/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <string.h>

#include "context.h"
#include "net.h"

#include "../breakpoints.h"
#include "../software_breakpoints.h"
#include "../watchpoints.h"

GDB_DECLARE_HANDLER(ToggleStopPoint)
{
    bool add = ctx->commandData[-1] == 'Z';
    unsigned long lst[3];

    const char *pos = GDB_ParseHexIntegerList(lst, ctx->commandData, 3, ';');
    if (pos == NULL) {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }
    bool persist = *pos != 0 && strncmp(pos, ";cmds:1", 7) == 0;

    // In theory we should reject leading zeroes in "kind". Oh well...
    unsigned long kind = lst[0];
    uintptr_t addr = lst[1];
    size_t size = lst[2];

    int res;
    static const WatchpointLoadStoreControl kinds[3] = {
        WatchpointLoadStoreControl_Store,
        WatchpointLoadStoreControl_Load,
        WatchpointLoadStoreControl_LoadStore,
    };

    switch(kind) {
        // Software breakpoint
        case 0: {
            if(size != 4) {
                return GDB_ReplyErrno(ctx, EINVAL);
            }
            res = add ? addSoftwareBreakpoint(addr, persist) : removeSoftwareBreakpoint(addr, false);
            return res == 0 ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, -res);
        }

        // Hardware breakpoint
        case 1: {
            if(size != 4) {
                return GDB_ReplyErrno(ctx, EINVAL);
            }
            res = add ? addBreakpoint(addr) : removeBreakpoint(addr);
            return res == 0 ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, -res);
        }

        // Watchpoints
        case 2:
        case 3:
        case 4: {
            res = add ? addWatchpoint(addr, size, kinds[kind - 2]) : removeWatchpoint(addr, size, kinds[kind - 2]);
            return res == 0 ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, -res);
        }
        default: {
            return GDB_ReplyEmpty(ctx);
        }
    }
}
