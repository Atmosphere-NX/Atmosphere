/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "gdb/server.h"
#include "gdb/net.h"
#include "gdb/query.h"
#include "gdb/verbose.h"
#include "gdb/thread.h"
#include "gdb/debug.h"
#include "gdb/regs.h"
#include "gdb/mem.h"
#include "gdb/hio.h"
#include "gdb/watchpoints.h"
#include "gdb/breakpoints.h"
#include "gdb/stop_points.h"

void GDB_InitializeServer(GDBServer *server)
{
    for(u32 i = 0; i < sizeof(server->ctxs) / sizeof(GDBContext); i++) {
        GDB_InitializeContext(server->ctxs + i);
    }

    return 0;
}

void GDB_FinalizeServer(GDBServer *server)
{
    // Kill the "next application" context if needed
    for (u32 i = 0; i < MAX_CTX; i++) {
        if (server->ctxs[i].state != GDB_STATE_DISCONNECTED) {
            GDB_CloseClient(&server->ctxs[i]);
        }
    }
}

void GDB_RunServer(GDBServer *server)
{
    // TODO transport iface
    (void)server;
}

void GDB_LockAllContexts(GDBServer *server)
{
    for (u32 i = 0; i < MAX_CTX; i++) {
        recursiveSpinlockLock(&server->ctxs[i].lock);
    }
}

void GDB_UnlockAllContexts(GDBServer *server)
{
    for (u32 i = MAX_CTX; i > 0; i--) {
        recursiveSpinlockUnlock(&server->ctxs[i - 1].lock);
    }
}

GDBContext *GDB_SelectAvailableContext(GDBServer *server)
{
    GDBContext *ctx;

    GDB_LockAllContexts(server);

    // Get a context
    size_t id;
    for (id = 0; id < MAX_CTX && (server->ctxs[id].flags & GDB_FLAG_ALLOCATED_MASK); id++);
    ctx = id < MAX_CTX ? &server->ctxs[id] : NULL;

    GDB_UnlockAllContexts(server);
    return ctx;
}

int GDB_AcceptClient(GDBContext *ctx)
{
    recursiveSpinlockLock(&ctx->lock);
    ctx->state = GDB_STATE_CONNECTED;
    ctx->latestSentPacketSize = 0;

    /*if (ctx->flags & GDB_FLAG_SELECTED)
        r = GDB_AttachToProcess(ctx);
    */
    recursiveSpinlockUnlock(&ctx->lock);

    return 0;
}

int GDB_CloseClient(GDBContext *ctx)
{
    // currently unused
    recursiveSpinlockLock(&ctx->lock);

    if (ctx->state >= GDB_STATE_ATTACHED) {
        GDB_DetachFromProcess(ctx);
    }

    ctx->flags = 0;
    ctx->state = GDB_STATE_DISCONNECTED;

    ctx->catchThreadEvents = false;

    // memset(&ctx->latestDebugEvent, 0, sizeof(DebugEventInfo)); TODO

    recursiveSpinlockUnlock(&ctx->lock);
    return 0;
}

void GDB_ReleaseClient(GDBServer *server, GDBContext *ctx)
{
    // same thing
    (void)server;
    (void)ctx;
}

static const struct
{
    char command;
    GDBCommandHandler handler;
} gdbCommandHandlers[] =
{
    { '?', GDB_HANDLER(GetStopReason) },
    { '!', GDB_HANDLER(EnableExtendedMode) },
    { 'c', GDB_HANDLER(Continue) },
    { 'C', GDB_HANDLER(Continue) },
    { 'D', GDB_HANDLER(Detach) },
    { 'F', GDB_HANDLER(HioReply) },
    { 'g', GDB_HANDLER(ReadRegisters) },
    { 'G', GDB_HANDLER(WriteRegisters) },
    { 'H', GDB_HANDLER(SetThreadId) },
    { 'k', GDB_HANDLER(Kill) },
    { 'm', GDB_HANDLER(ReadMemory) },
    { 'M', GDB_HANDLER(WriteMemory) },
    { 'p', GDB_HANDLER(ReadRegister) },
    { 'P', GDB_HANDLER(WriteRegister) },
    { 'q', GDB_HANDLER(ReadQuery) },
    { 'Q', GDB_HANDLER(WriteQuery) },
    { 'R', GDB_HANDLER(Restart) },
    { 'T', GDB_HANDLER(IsThreadAlive) },
    { 'v', GDB_HANDLER(VerboseCommand) },
    { 'X', GDB_HANDLER(WriteMemoryRaw) },
    { 'z', GDB_HANDLER(ToggleStopPoint) },
    { 'Z', GDB_HANDLER(ToggleStopPoint) },
};

static inline GDBCommandHandler GDB_GetCommandHandler(char command)
{
    static const u32 nbHandlers = sizeof(gdbCommandHandlers) / sizeof(gdbCommandHandlers[0]);

    size_t i;
    for (i = 0; i < nbHandlers && gdbCommandHandlers[i].command != command; i++);

    return i < nbHandlers ? gdbCommandHandlers[i].handler : GDB_HANDLER(Unsupported);
}

int GDB_DoPacket(GDBContext *ctx)
{
    int ret;

    recursiveSpinlockLock(&ctx->lock);
    u32 oldFlags = ctx->flags;

    if(ctx->state == GDB_STATE_DISCONNECTED) {
        return -1;
    }

    int r = GDB_ReceivePacket(ctx);
    if (r == 0) {
        ret = 0;
    } else if (r == -1) {
        ret = -1;
    } else if (ctx->buffer[0] == '\x03') {
        GDB_HandleBreak(ctx);
        ret = 0;
    } else if (ctx->buffer[0] == '$') {
        GDBCommandHandler handler = GDB_GetCommandHandler(ctx->buffer[1]);
        ctx->commandData = ctx->buffer + 2;
        ret = handler(ctx);
    } else {
        ret = 0;
    }

    if (ctx->state == GDB_STATE_DETACHING) {
        if (ctx->flags & GDB_FLAG_EXTENDED_REMOTE) {
            ctx->state = GDB_STATE_CONNECTED;
            recursiveSpinlockUnlock(&ctx->lock);
            return ret;
        } else {
            recursiveSpinlockUnlock(&ctx->lock);
            return -1;
        }
    }

    if ((oldFlags & GDB_FLAG_CONTINUING) && !(ctx->flags & GDB_FLAG_CONTINUING)) {
        // TODO
    }
    else if (!(oldFlags & GDB_FLAG_CONTINUING) && (ctx->flags & GDB_FLAG_CONTINUING)) {
        // TODO
    }

    recursiveSpinlockUnlock(&ctx->lock);
    return ret;
}
