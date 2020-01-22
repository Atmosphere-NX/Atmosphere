/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#if 1
#include "gdb.h"
#include "gdb/net.h"
#include "gdb/server.h"

#include "gdb/debug.h"

#include "gdb/stop_point.h"

#include "breakpoints.h"
#include "software_breakpoints.h"
#include "watchpoints.h"

void GDB_InitializeContext(GDBContext *ctx)
{
    memset(ctx, 0, sizeof(GDBContext));
}

void GDB_FinalizeContext(GDBContext *ctx)
{
    (void)ctx;
}

void GDB_Attach(GDBContext *ctx)
{
    if (!(ctx->flags & GDB_FLAG_ATTACHED_AT_START)) {
        // TODO: debug pause
    }

    // TODO: move the debug traps enable here?
    // TODO: process the event

    ctx->state = GDB_STATE_ATTACHED;
}

void GDB_Detach(GDBContext *ctx)
{
    removeAllWatchpoints();
    removeAllBreakpoints();
    removeAllSoftwareBreakpoints(true);

    // Reports to gdb are prevented because of "detaching" state?

    // TODO: disable debug traps

    if(ctx->flags & GDB_FLAG_TERMINATE) {
        // TODO: redefine what it means for thermosphère, if anything.
        ctx->processEnded = true;
        ctx->processExited = false;
    }

    ctx->currentHioRequestTargetAddr = 0;
    memset(&ctx->currentHioRequest, 0, sizeof(PackedGdbHioRequest));
}

GDB_DECLARE_HANDLER(Unsupported)
{
    return GDB_ReplyEmpty(ctx);
}

GDB_DECLARE_HANDLER(EnableExtendedMode)
{
    // We don't support it for now...
    return GDB_HandleUnsupported(ctx);
}

#endif
