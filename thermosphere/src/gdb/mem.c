/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include <string.h>
#include "mem.h"
#include "net.h"
#include "../guest_memory.h"
#include "../pattern_utils.h"

int GDB_SendMemory(GDBContext *ctx, const char *prefix, size_t prefixLen, size_t addr, size_t len)
{
    char *buf = ctx->buffer + 1;
    char *membuf = ctx->workBuffer;

    if(prefix != NULL) {
        memmove(buf, prefix, prefixLen);
    }
    else {
        prefixLen = 0;
    }

    if(prefixLen + 2 * len > GDB_BUF_LEN) {
        // gdb shouldn't send requests which responses don't fit in a packet
        return prefix == NULL ? GDB_ReplyErrno(ctx, ENOMEM) : -1;
    }

    size_t total = guestReadMemory(addr, len, membuf);

    if (total == 0) {
        return prefix == NULL ? GDB_ReplyErrno(ctx, EFAULT) : -EFAULT;
    } else {
        GDB_EncodeHex(buf + prefixLen, membuf, total);
        return GDB_SendPacket(ctx, buf, prefixLen + 2 * total);
    }
}

int GDB_WriteMemory(GDBContext *ctx, const void *buf, uintptr_t addr, size_t len)
{
    size_t total = guestWriteMemory(addr, len, buf);
    return total == len ? GDB_ReplyOk(ctx) : GDB_ReplyErrno(ctx, EFAULT);
}

u32 GDB_SearchMemory(bool *found, GDBContext *ctx, uintptr_t addr, size_t len, const void *pattern, size_t patternLen)
{
    // Note: need to ensure GDB_WORK_BUF_LEN is at least 0x1000 bytes in size

    char *buf = ctx->workBuffer;
    size_t maxNbPages = GDB_WORK_BUF_LEN / 0x1000;
    uintptr_t curAddr = addr;

    while (curAddr < addr + len) {
        size_t nbPages;
        uintptr_t addrBase = curAddr & ~0xFFF;
        size_t addrDispl = curAddr & 0xFFF;

        for (nbPages = 0; nbPages < maxNbPages; nbPages++) {
            if (guestReadMemory(addrBase + nbPages * 0x1000, 0x1000, buf + nbPages * 0x1000) != 0x1000) {
                break;
            }
        }

        u8 *pos = NULL;
        if(addrDispl + patternLen <= 0x1000 * nbPages) {
            pos = memsearch(buf + addrDispl, pattern, 0x1000 * nbPages - addrDispl, patternLen);
        }

        if(pos != NULL) {
            *found = true;
            return addrBase + (pos - buf);
        }

        curAddr = addrBase + 0x1000;
    }

    *found = false;
    return 0;
}

GDB_DECLARE_HANDLER(ReadMemory)
{
    unsigned long lst[2];
    if (GDB_ParseHexIntegerList(lst, ctx->commandData, 2, 0) == NULL) {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    uintptr_t addr = lst[0];
    size_t len = lst[1];

    return GDB_SendMemory(ctx, NULL, 0, addr, len);
}

GDB_DECLARE_HANDLER(WriteMemory)
{
    unsigned long lst[2];
    const char *dataStart = GDB_ParseHexIntegerList(lst, ctx->commandData, 2, ':');
    if (dataStart == NULL || *dataStart != ':') {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    dataStart++;
    uintptr_t addr = lst[0];
    size_t len = lst[1];

    if(dataStart + 2 * len >= ctx->buffer + 4 + GDB_BUF_LEN) {
        // Data len field doesn't match what we got...
        return GDB_ReplyErrno(ctx, ENOMEM);
    }

    size_t n = GDB_DecodeHex(ctx->workBuffer, dataStart, len);

    if(n != len) {
        // Decoding error...
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    return GDB_WriteMemory(ctx, ctx->workBuffer, addr, len);
}

GDB_DECLARE_HANDLER(WriteMemoryRaw)
{
    unsigned long lst[2];
    const char *dataStart = GDB_ParseHexIntegerList(lst, ctx->commandData, 2, ':');
    if (dataStart == NULL || *dataStart != ':') {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    dataStart++;
    uintptr_t addr = lst[0];
    size_t len = lst[1];

    if(dataStart + 2 * len >= ctx->buffer + 4 + GDB_BUF_LEN) {
        // Data len field doesn't match what we got...
        return GDB_ReplyErrno(ctx, ENOMEM);
    }

    // Note: could be done in place in ctx->buffer...
    size_t n = GDB_UnescapeBinaryData(ctx->workBuffer, dataStart, len);

    if(n != len) {
        // Decoding error...
        return GDB_ReplyErrno(ctx, n);
    }

    return GDB_WriteMemory(ctx, ctx->workBuffer, addr, len);
}

GDB_DECLARE_QUERY_HANDLER(SearchMemory)
{
    unsigned long lst[2];
    const char *patternStart;
    size_t patternLen;
    bool found;
    u32 foundAddr;

    if (strncmp(ctx->commandData, "memory:", 7) != 0) {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    ctx->commandData += 7;
    patternStart = GDB_ParseIntegerList(lst, ctx->commandData, 2, ';', ';', 16, false);
    if (patternStart == NULL || *patternStart != ';') {
        return GDB_ReplyErrno(ctx, EILSEQ);
    }

    uintptr_t addr = lst[0];
    size_t len = lst[1];

    patternStart++;
    patternLen = ctx->commandEnd - patternStart;

    // Unescape pattern in place
    char *pattern = patternStart;
    patternLen = GDB_UnescapeBinaryData(pattern, patternStart, patternLen);

    foundAddr = GDB_SearchMemory(&found, ctx, addr, len, patternStart, patternLen);

    return found ? GDB_SendFormattedPacket(ctx, "1,%x", foundAddr) : GDB_SendPacket(ctx, "0", 1);
}
