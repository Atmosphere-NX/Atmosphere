/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <string.h>

#include "gdb/hio.h"
#include "gdb/net.h"
#include "gdb/mem.h"
#include "gdb/debug.h"
#include "fmt.h"

bool GDB_FetchPackedHioRequest(GDBContext *ctx, u32 addr)
{
    u32 total = GDB_ReadTargetMemory(&ctx->currentHioRequest, ctx, addr, sizeof(PackedGdbHioRequest));
    if (total != sizeof(PackedGdbHioRequest) || memcmp(&ctx->currentHioRequest.magic, "GDB\x00", 4) != 0)
    {
        memset(&ctx->currentHioRequest, 0, sizeof(PackedGdbHioRequest));
        ctx->currentHioRequestTargetAddr = 0;
        return false;
    }
    else
    {
        ctx->currentHioRequestTargetAddr = addr;
        return true;
    }
}

bool GDB_IsHioInProgress(GDBContext *ctx)
{
    return ctx->currentHioRequestTargetAddr != 0;
}

int GDB_SendCurrentHioRequest(GDBContext *ctx)
{
    char buf[256+1];
    char tmp[32+1];
    u32 nStr = 0;

    sprintf(buf, "F%s", ctx->currentHioRequest.functionName);

    for (u32 i = 0; i < 8 && ctx->currentHioRequest.paramFormat[i] != 0; i++)
    {
        switch (ctx->currentHioRequest.paramFormat[i])
        {
            case 'i':
            case 'I':
            case 'p':
                sprintf(tmp, ",%lx", (u32)ctx->currentHioRequest.parameters[i]);
                break;
            case 'l':
            case 'L':
                sprintf(tmp, ",%llx", ctx->currentHioRequest.parameters[i]);
                break;
            case 's':
                sprintf(tmp, ",%lx/%x", (u32)ctx->currentHioRequest.parameters[i], ctx->currentHioRequest.stringLengths[nStr++]);
                break;
            default:
                tmp[0] = 0;
                break;
        }
        strcat(buf, tmp);
    }

    return GDB_SendPacket(ctx, buf, strlen(buf));
}

GDB_DECLARE_HANDLER(HioReply)
{
    if (!GDB_IsHioInProgress(ctx))
        return GDB_ReplyErrno(ctx, EPERM);
    
    // Reply in the form of Fretcode,errno,Ctrl-C flag;call-specific attachment
    // "Call specific attachement" is always empty, though.

    const char *pos = ctx->commandData;
    u64 retval;

    if (*pos == 0 || *pos == ',')
        return GDB_ReplyErrno(ctx, EILSEQ);
    else if (*pos == '-')
    {
        pos++;
        ctx->currentHioRequest.retval = -1ll;
    }
    else if (*pos == '+')
    {
        pos++;
        ctx->currentHioRequest.retval = 1;
    }
    else
        ctx->currentHioRequest.retval = 1;

    pos = GDB_ParseHexIntegerList64(&retval, pos, 1, ',');

    if (pos == NULL)
        return GDB_ReplyErrno(ctx, EILSEQ);

    ctx->currentHioRequest.retval *= retval;
    ctx->currentHioRequest.gdbErrno = 0;
    ctx->currentHioRequest.ctrlC = false;

    if (*pos != 0)
    {
        u32 errno_;
        // GDB protocol technically allows errno to have a +/- prefix but this will never happen.
        pos = GDB_ParseHexIntegerList(&errno_, ++pos, 1, ',');
        ctx->currentHioRequest.gdbErrno = (int)errno_;
        if (pos == NULL)
            return GDB_ReplyErrno(ctx, EILSEQ);

        if (*pos != 0)
        {
            if (*pos != 'C')
                return GDB_ReplyErrno(ctx, EILSEQ);

            ctx->currentHioRequest.ctrlC = true;
        }
    }

    memset(ctx->currentHioRequest.paramFormat, 0, sizeof(ctx->currentHioRequest.paramFormat));

    u32 total = GDB_WriteTargetMemory(ctx, &ctx->currentHioRequest, ctx->currentHioRequestTargetAddr, sizeof(PackedGdbHioRequest));

    memset(&ctx->currentHioRequest, 0, sizeof(PackedGdbHioRequest));
    ctx->currentHioRequestTargetAddr = 0;

    GDB_ContinueExecution(ctx);
    return total == sizeof(PackedGdbHioRequest) ? 0 : GDB_ReplyErrno(ctx, EFAULT);
}
