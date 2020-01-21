/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "gdb.h"
#include "gdb/net.h"
#include "gdb/watchpoints.h"
#include "gdb/breakpoints.h"

GDB_DECLARE_HANDLER(ToggleStopPoint)
{
    bool add = ctx->commandData[-1] == 'Z';
    u32 lst[3];

    const char *pos = GDB_ParseHexIntegerList(lst, ctx->commandData, 3, ';');
    if(pos == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);
    bool persist = *pos != 0 && strncmp(pos, ";cmds:1", 7) == 0;

    u32 kind = lst[0];
    u32 addr = lst[1];
    u32 size = lst[2];

    int res;
    static const WatchpointKind kinds[3] = { WATCHPOINT_WRITE, WATCHPOINT_READ, WATCHPOINT_READWRITE };
    switch(kind)
    {
        case 0: // software breakpoint
            if(size != 2 && size != 4)
                return GDB_ReplyEmpty(ctx);
            else
            {
                res = add ? GDB_AddBreakpoint(ctx, addr, size == 2, persist) :
                            GDB_RemoveBreakpoint(ctx, addr);
                return res == 0 ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, -res);
            }

        // Watchpoints
        case 2:
        case 3:
        case 4:
            res = add ? GDB_AddWatchpoint(ctx, addr, size, kinds[kind - 2]) :
                        GDB_RemoveWatchpoint(ctx, addr, kinds[kind - 2]);
            return res == 0 ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, -res);

        default:
            return GDB_ReplyEmpty(ctx);
    }
}
