/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "../utils.h"

#include "query.h"
#include "xfer.h"
#include "thread.h"
#include "mem.h"
#include "net.h"
#include "remote_command.h"

typedef enum GDBQueryDirection {
    GDB_QUERY_DIRECTION_READ,
    GDB_QUERY_DIRECTION_WRITE
} GDBQueryDirection;

#define GDB_QUERY_HANDLER_LIST_ITEM_3(name, name2, direction)   { name, GDB_QUERY_HANDLER(name2), GDB_QUERY_DIRECTION_##direction }
#define GDB_QUERY_HANDLER_LIST_ITEM(name, direction)            GDB_QUERY_HANDLER_LIST_ITEM_3(STRINGIZE(name), name, direction)

static const struct {
    const char *name;
    GDBCommandHandler handler;
    GDBQueryDirection direction;
} gdbQueryHandlers[] =
{
    GDB_QUERY_HANDLER_LIST_ITEM(Supported, READ),
    GDB_QUERY_HANDLER_LIST_ITEM(Xfer, READ),
    GDB_QUERY_HANDLER_LIST_ITEM(StartNoAckMode, WRITE),
    GDB_QUERY_HANDLER_LIST_ITEM(Attached, READ),
    GDB_QUERY_HANDLER_LIST_ITEM(fThreadInfo, READ),
    GDB_QUERY_HANDLER_LIST_ITEM(sThreadInfo, READ),
    GDB_QUERY_HANDLER_LIST_ITEM(ThreadEvents, WRITE),
    GDB_QUERY_HANDLER_LIST_ITEM(ThreadExtraInfo, READ),
    GDB_QUERY_HANDLER_LIST_ITEM(GetTLSAddr, READ),
    GDB_QUERY_HANDLER_LIST_ITEM_3("C", CurrentThreadId, READ),
    GDB_QUERY_HANDLER_LIST_ITEM_3("Search", SearchMemory, READ),
    GDB_QUERY_HANDLER_LIST_ITEM(Rcmd, READ),
};

static int GDB_HandleQuery(GDBContext *ctx, GDBQueryDirection direction)
{
    char *nameBegin = ctx->commandData; // w/o leading 'q'/'Q'
    if(*nameBegin == 0)
        return GDB_ReplyErrno(ctx, EILSEQ);

    char *nameEnd;
    char *queryData = NULL;

    for(nameEnd = nameBegin; *nameEnd != 0 && *nameEnd != ':' && *nameEnd != ','; nameEnd++);
    if(*nameEnd != 0)
    {
        *nameEnd = 0;
        queryData = nameEnd + 1;
    }
    else
        queryData = nameEnd;

    for(u32 i = 0; i < sizeof(gdbQueryHandlers) / sizeof(gdbQueryHandlers[0]); i++)
    {
        if(strcmp(gdbQueryHandlers[i].name, nameBegin) == 0 && gdbQueryHandlers[i].direction == direction)
        {
            ctx->commandData = queryData;
            return gdbQueryHandlers[i].handler(ctx);
        }
    }

    return GDB_HandleUnsupported(ctx); // No handler found!
}

int GDB_HandleReadQuery(GDBContext *ctx)
{
    return GDB_HandleQuery(ctx, GDB_QUERY_DIRECTION_READ);
}

int GDB_HandleWriteQuery(GDBContext *ctx)
{
    return GDB_HandleQuery(ctx, GDB_QUERY_DIRECTION_WRITE);
}

GDB_DECLARE_QUERY_HANDLER(Supported)
{
    // TODO!
    return GDB_SendFormattedPacket(ctx,
        "PacketSize=%x;"
        "qXfer:features:read+;qXfer:osdata:read+;"
        "QStartNoAckMode+;QThreadEvents+"
        "vContSupported+;swbreak+;hwbreak+",

        GDB_BUF_LEN // should have been sizeof(ctx->buffer) but GDB memory functions are bugged
    );
}

GDB_DECLARE_QUERY_HANDLER(StartNoAckMode)
{
    ctx->noAckSent = true;
    return GDB_ReplyOk(ctx);
}

GDB_DECLARE_QUERY_HANDLER(Attached)
{
    return GDB_SendPacket(ctx, "1", 1);
}
