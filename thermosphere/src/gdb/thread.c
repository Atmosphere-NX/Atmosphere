/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <stdio.h>

#include "thread.h"
#include "net.h"
#include "../core_ctx.h"

GDB_DECLARE_HANDLER(SetThreadId)
{
    if (ctx->commandData[0] == 'g') {
        if(strncmp(ctx->commandData + 1, "-1", 2) == 0) {
            return GDB_ReplyErrno(ctx, EINVAL);
        }

        unsigned long id;
        if (GDB_ParseHexIntegerList(&id, ctx->commandData + 1, 1, 0) == NULL) {
            return GDB_ReplyErrno(ctx, EILSEQ);
        }

        ctx->selectedThreadId = id;
        // TODO: change irq affinity (and remove selectedThreadId?)
        return GDB_ReplyOk(ctx);
    } else if (ctx->commandData[0] == 'c') {
        if(strncmp(ctx->commandData + 1, "-1", 2) == 0) {
            ctx->selectedThreadIdForContinuing = 0;
        } else {
            unsigned long id;
            if(GDB_ParseHexIntegerList(&id, ctx->commandData + 1, 1, 0) == NULL)
                return GDB_ReplyErrno(ctx, EILSEQ);
            ctx->selectedThreadIdForContinuing = id;
        }

        return GDB_ReplyOk(ctx);
    }
    else
        return GDB_ReplyErrno(ctx, EPERM);
}

GDB_DECLARE_HANDLER(IsThreadAlive)
{
    unsigned long threadId;

    if (GDB_ParseHexIntegerList(&threadId, ctx->commandData, 1, 0) == NULL) {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    u32 coreMask = ctx->attachedCoreList;
    return (coreMask & BIT(threadId)) != 0 ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, ESRCH);
}

GDB_DECLARE_QUERY_HANDLER(CurrentThreadId)
{
    if (ctx->currentThreadId == 0) {
        // FIXME: probably remove this variable
        ctx->currentThreadId = 1 + currentCoreCtx->coreId;
    }

    return GDB_SendFormattedPacket(ctx, "QC%x", ctx->currentThreadId);
}

GDB_DECLARE_QUERY_HANDLER(fThreadInfo)
{
    // We have made our GDB packet big enough to list all the thread ids (coreIds + 1 for each coreId)
    char *buf = ctx->buffer + 1;
    int n = 1;
    buf[0] = 'm';

    u32 coreMask = ctx->attachedCoreList;

    FOREACH_BIT (tmp, coreId, coreMask) {
        n += sprintf(buf + n, "%x,", 1 + coreId);
    }

    // Remove trailing comma
    buf[--n] = 0;

    return GDB_SendStreamData(ctx, buf, 0, n, n, true);
}

GDB_DECLARE_QUERY_HANDLER(sThreadInfo)
{
    // We have made our GDB packet big enough to list all the thread ids (coreIds + 1 for each coreId) in fThreadInfo
    // Note: we assume GDB doesn't accept notifications during the sequence transfer...
    return GDB_SendPacket(ctx, "m", 1);
}

GDB_DECLARE_QUERY_HANDLER(ThreadEvents)
{
    switch (ctx->commandData[0]) {
        case '0':
            ctx->catchThreadEvents = false;
            return GDB_ReplyOk(ctx);
        case '1':
            ctx->catchThreadEvents = true;
            return GDB_ReplyOk(ctx);
        default:
            return GDB_ReplyErrno(ctx, EILSEQ);
    }
}

GDB_DECLARE_QUERY_HANDLER(ThreadExtraInfo)
{
    unsigned long id;
    int n;

    if(GDB_ParseHexIntegerList(&id, ctx->commandData, 1, 0) == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

    n = sprintf(ctx->workBuffer, "TODO");

    return GDB_SendHexPacket(ctx, ctx->workBuffer, n);
}
