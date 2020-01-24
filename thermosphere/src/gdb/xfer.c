/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <string.h>
#include <stdio.h>

#include "../utils.h"

#include "xfer.h"
#include "net.h"


struct {
    const char *name;
    int (*handler)(GDBContext *ctx, bool write, const char *annex, size_t offset, size_t length);
} xferCommandHandlers[] = {
    { "features", GDB_XFER_HANDLER(Features) },
};

static void GDB_GenerateTargetXml(char *buf)
{
    int pos;
    const char *hdr = "<?xml version=\"1.0\"?><!DOCTYPE feature SYSTEM \"gdb-target.dtd\">";
    const char *cpuDescBegin = "<feature name=\"org.gnu.gdb.aarch64.core\">";
    const char *cpuDescEnd =
    "<reg name=\"sp\" bitsize=\"64\" type=\"data_ptr\"/><reg name=\"pc\""
    "bitsize=\"64\" type=\"code_ptr\"/><reg name=\"cpsr\" bitsize=\"32\"/></feature>";

    const char *fpuDescBegin =
    "<feature name=\"org.gnu.gdb.aarch64.fpu\"><vector id=\"v2d\" type=\"ieee_double\" count=\"2\"/>"
    "<vector id=\"v2u\" type=\"uint64\" count=\"2\"/><vector id=\"v2i\" type=\"int64\" count=\"2\"/>"
    "<vector id=\"v4f\" type=\"ieee_single\" count=\"4\"/><vector id=\"v4u\" type=\"uint32\" count=\"4\"/>"
    "<vector id=\"v4i\" type=\"int32\" count=\"4\"/><vector id=\"v8u\" type=\"uint16\" count=\"8\"/>"
    "<vector id=\"v8i\" type=\"int16\" count=\"8\"/><vector id=\"v16u\" type=\"uint8\" count=\"16\"/>"
    "<vector id=\"v16i\" type=\"int8\" count=\"16\"/><vector id=\"v1u\" type=\"uint128\" count=\"1\"/>"
    "<vector id=\"v1i\" type=\"int128\" count=\"1\"/><union id=\"vnd\"><field name=\"f\" type=\"v2d\"/>"
    "<field name=\"u\" type=\"v2u\"/><field name=\"s\" type=\"v2i\"/></union><union id=\"vns\">"
    "<field name=\"f\" type=\"v4f\"/><field name=\"u\" type=\"v4u\"/><field name=\"s\" type=\"v4i\"/></union>"
    "<union id=\"vnh\"><field name=\"u\" type=\"v8u\"/><field name=\"s\" type=\"v8i\"/></union><union id=\"vnb\">"
    "<field name=\"u\" type=\"v16u\"/><field name=\"s\" type=\"v16i\"/></union><union id=\"vnq\">"
    "<field name=\"u\" type=\"v1u\"/><field name=\"s\" type=\"v1i\"/></union><union id=\"aarch64v\">"
    "<field name=\"d\" type=\"vnd\"/><field name=\"s\" type=\"vns\"/><field name=\"h\" type=\"vnh\"/>"
    "<field name=\"b\" type=\"vnb\"/><field name=\"q\" type=\"vnq\"/></union>";

    const char *fpuDescEnd = "<reg name=\"fpsr\" bitsize=\"32\"/>\r\n  <reg name=\"fpcr\" bitsize=\"32\"/>\r\n</feature>";
    const char *footer = "</target>";

    strcpy(buf, hdr);

    // CPU registers
    strcat(buf, cpuDescBegin);
    pos = (int)strlen(buf);
    for (u32 i = 0; i < 31; i++) {
        pos += sprintf(buf + pos, "<reg name=\"x%u\" bitsize=\"64\"/>", i);
    }
    strcat(buf, cpuDescEnd);

    strcat(buf, fpuDescBegin);
    pos = (int)strlen(buf);
    for (u32 i = i; i < 32; i++) {
        pos += sprintf(buf + pos, "<reg name=\"v%u\" bitsize=\"128\" type=\"aarch64v\"/>", i);
    }
    strcat(buf, fpuDescEnd);

    strcat(buf, footer);
}

GDB_DECLARE_XFER_HANDLER(Features)
{
    if(strcmp(annex, "target.xml") != 0 || write) {
        return GDB_ReplyEmpty(ctx);
    }

    // Generate the target xml on-demand
    // This is a bit whack, we rightfully assume that GDB won't sent any other command during the stream transfer
    if (ctx->targetXmlLen == 0) {
        GDB_GenerateTargetXml(ctx->workBuffer);
        ctx->targetXmlLen = strlen(ctx->workBuffer);
    }

    int n = GDB_SendStreamData(ctx, ctx->workBuffer, offset, length, ctx->targetXmlLen, false);

    // Transfer ended
    if(offset + length >= ctx->targetXmlLen) {
        ctx->targetXmlLen = 0;
    }

    return n;
}

GDB_DECLARE_QUERY_HANDLER(Xfer)
{
    const char *objectStart = ctx->commandData;
    char *objectEnd = (char*)strchr(objectStart, ':');
    if (objectEnd == NULL) {
        return -1;
    }
    *objectEnd = 0;

    char *opStart = objectEnd + 1;
    char *opEnd = (char*)strchr(opStart, ':');
    if(opEnd == NULL) {
        return -1;
    }
    *opEnd = 0;

    char *annexStart = opEnd + 1;
    char *annexEnd = (char*)strchr(annexStart, ':');
    if(annexEnd == NULL) {
        return -1;
    }
    *annexEnd = 0;

    const char *offStart = annexEnd + 1;
    size_t offset, length;

    bool write;
    const char *pos;
    if (strcmp(opStart, "read") == 0) {
        unsigned int lst[2];
        if(GDB_ParseHexIntegerList(lst, offStart, 2, 0) == NULL) {
            return GDB_ReplyErrno(ctx, EILSEQ);
        }

        offset = lst[0];
        length = lst[1];
        write = false;
    } else if (strcmp(opStart, "write") == 0) {
        pos = GDB_ParseHexIntegerList(&offset, offStart, 1, ':');
        if (pos == NULL || *pos++ != ':') {
            return GDB_ReplyErrno(ctx, EILSEQ);
        }

        size_t len = strlen(pos);
        if (len == 0 || (len % 2) != 0) {
            return GDB_ReplyErrno(ctx, EILSEQ);
        }
        length = len / 2;
        write = true;
    } else {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    for (size_t i = 0; i < sizeof(xferCommandHandlers) / sizeof(xferCommandHandlers[0]); i++) {
        if (strcmp(objectStart, xferCommandHandlers[i].name) == 0) {
            if(write) {
                ctx->commandData = (char *)pos;
            }

            return xferCommandHandlers[i].handler(ctx, write, annexStart, offset, length);
        }
    }

    return GDB_HandleUnsupported(ctx);
}
