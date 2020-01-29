/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <string.h>

#include "verbose.h"
#include "net.h"
#include "debug.h"

static const struct {
    const char *name;
    char trailingCharacter;
    GDBCommandHandler handler;
} gdbVerboseCommandHandlers[] = {
    { "Cont?",          '\0',   GDB_VERBOSE_HANDLER(ContinueSupported) },
    { "Cont",           ';',    GDB_VERBOSE_HANDLER(Continue) },
    { "MustReplyEmpty", '\0',   GDB_HANDLER(Unsupported) },
};

GDB_DECLARE_HANDLER(VerboseCommand)
{
    char *nameBegin = ctx->commandData; // w/o leading 'v'
    if (*nameBegin == 0) {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    char *nameEnd;
    char *vData = NULL;

    for (nameEnd = nameBegin; *nameEnd != 0 && *nameEnd != ';' && *nameEnd != ':'; nameEnd++);
    char oldNameEnd = *nameEnd;
    if (*nameEnd != 0) {
        *nameEnd = 0;
        vData = nameEnd + 1;
    }

    for (size_t i = 0; i < sizeof(gdbVerboseCommandHandlers) / sizeof(gdbVerboseCommandHandlers[0]); i++) {
        if (strcmp(gdbVerboseCommandHandlers[i].name, nameBegin) == 0) {
            ctx->commandData = vData;
            if (oldNameEnd == gdbVerboseCommandHandlers[i].trailingCharacter) {
                return gdbVerboseCommandHandlers[i].handler(ctx);
            } else {
                return GDB_ReplyErrno(ctx, EILSEQ);
            }
        }
    }

    return GDB_HandleUnsupported(ctx); // No handler found!
}

GDB_DECLARE_VERBOSE_HANDLER(ContinueSupported)
{
    const char *supported = "vCont;c;C;s;S;t;r";
    return GDB_SendPacket(ctx, supported, strlen(supported));
}
