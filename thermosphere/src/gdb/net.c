/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#include "net.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "../pattern_utils.h"

u8 GDB_ComputeChecksum(const char *packetData, size_t len)
{
    unsigned long cksum = 0;
    for(size_t i = 0; i < len; i++) {
        cksum += packetData[i];
    }

    return (u8)cksum;
}

size_t GDB_EncodeHex(char *dst, const void *src, size_t len)
{
    static const char *alphabet = "0123456789abcdef";
    const u8 *src8 = (const u8 *)src;

    for (size_t i = 0; i < len; i++) {
        dst[2 * i] = alphabet[(src8[i] & 0xf0) >> 4];
        dst[2 * i + 1] = alphabet[src8[i] & 0x0f];
    }

    return 2 * len;
}

static inline u32 GDB_DecodeHexDigit(char src, bool *ok)
{
    *ok = true;
    switch (src) {
        case '0' ... '9': return  0 + (src - '0');
        case 'a' ... 'f': return 10 + (src - 'a');
        case 'A' ... 'F': return 10 + (src - 'A');
        default:
            *ok = false;
            return 0;
    }
}

size_t GDB_DecodeHex(void *dst, const char *src, size_t len) {
    size_t i = 0;
    u8 *dst8 = (u8 *)dst;
    for (i = 0; i < len && src[2 * i] != 0 && src[2 * i + 1] != 0; i++) {
        bool ok1, ok2;
        dst8[i]  = GDB_DecodeHexDigit(src[2 * i], &ok1) << 4;
        dst8[i] |= GDB_DecodeHexDigit(src[2 * i + 1], &ok2);
        if (!ok1 || !ok2) {
            return i;
        }
    }

    return i;
}

size_t GDB_EscapeBinaryData(size_t *encodedCount, void *dst, const void *src, size_t len, size_t maxLen)
{
    u8 *dst8 = (u8 *)dst;
    const u8 *src8 = (const u8 *)src;

    maxLen = maxLen >= len ? len : maxLen;

    while ((uintptr_t)dst8 < (uintptr_t)dst + maxLen) {
        if (*src8 == '$' || *src8 == '#' || *src8 == '}' || *src8 == '*') {
            if ((uintptr_t)dst8 + 1 >= (uintptr_t)dst + maxLen) {
                break;
            }
            *dst8++ = '}';
            *dst8++ = *src8++ ^ 0x20;
        }
        else {
            *dst8++ = *src8++;
        }
    }

    *encodedCount = dst8 - (u8 *)dst;
    return src8 - (u8 *)src;
}

size_t GDB_UnescapeBinaryData(void *dst, const void *src, size_t len)
{
    u8 *dst8 = (u8 *)dst;
    const u8 *src8 = (const u8 *)src;

    while ((uintptr_t)src8 < (uintptr_t)src + len) {
        if (*src8 == '}') {
            src8++;
            *dst8++ = *src8++ ^ 0x20;
        } else {
            *dst8++ = *src8++;
        }
    }

    return dst8 - (u8 *)dst;
}

const char *GDB_ParseIntegerList(unsigned long *dst, const char *src, size_t nb, char sep, char lastSep, u32 base, bool allowPrefix)
{
    const char *pos = src;
    const char *endpos;
    bool ok;

    for (size_t i = 0; i < nb; i++) {
        unsigned long n = xstrtoul(pos, (char **)&endpos, (int) base, allowPrefix, &ok);
        if(!ok || endpos == pos) {
            return NULL;
        }

        if (i != nb - 1) {
            if (*endpos != sep) {
                return NULL;
            }
            pos = endpos + 1;
        } else {
            if (*endpos != lastSep && *endpos != 0) {
                return NULL;
            }
            pos = endpos;
        }

        dst[i] = n;
    }

    return pos;
}

const char *GDB_ParseHexIntegerList(unsigned long *dst, const char *src, size_t nb, char lastSep)
{
    return GDB_ParseIntegerList(dst, src, nb, ',', lastSep, 16, false);
}

static int GDB_SendNackIfPossible(GDBContext *ctx) {
    if (ctx->flags & GDB_FLAG_NOACK) {
        return -1;
    } else {
        char hdr = '-';
        transportInterfaceWriteData(ctx->transportInterface, &hdr, 1);
        return 1;
    }
}

int GDB_ReceivePacket(GDBContext *ctx)
{
    char hdr;
    bool ctrlC = false;
    TransportInterface *iface = ctx->transportInterface;

    // Read the first character...
    transportInterfaceReadData(iface, &hdr, 1);

    // Check if the ack/nack packets are not allowed
    if ((hdr == '+' || hdr == '-') && (ctx->flags & GDB_FLAG_NOACK) != 0) {
        DEBUG("Received a packed with an invalid header from GDB, hdr=%c\n", hdr);
        return -1;
    } 

    switch (hdr) {
        case '+':
            // Ack, don't do anything else (if allowed)
            return 0;
        case '-':
            // Nack, return the previous packet
            transportInterfaceWriteData(iface, ctx->buffer, ctx->lastSentPacketSize);
            return ctx->lastSentPacketSize;
        case '$':
            // Normal packet, handled below
            break;
        case '\x03':
            // Normal packet (Control-C), handled below
            ctrlC = true;
            break;
        default:
            // Oops, send a nack
            DEBUG("Received a packed with an invalid header from GDB, hdr=%c\n", hdr);
            return GDB_SendNackIfPossible(ctx);
    }

    // We didn't get a nack past this point, read the remaining data if any

    ctx->buffer[0] = hdr;
    if (ctrlC) {
        // Will never normally happen, but ok
        if (ctx->state < GDB_STATE_ATTACHED) {
            DEBUG("Received connection from GDB, now attaching...\n");
            GDB_AttachToContext(ctx);
            ctx->state = GDB_STATE_ATTACHED;
        }
        return 1;
    }

    size_t delimPos = transportInterfaceReadDataUntil(iface, ctx->buffer + 1, 4 + GDB_BUF_LEN - 1, '#');
    if (ctx->buffer[delimPos] != '#') {
        // The packet is malformed, send a nack
        return GDB_SendNackIfPossible(ctx);
    }

    // Read the checksum
    size_t checksumPos = delimPos + 1;
    u8 checksum;
    transportInterfaceReadData(iface, ctx->buffer + checksumPos, 2);

    if (GDB_DecodeHex(&checksum, ctx->buffer + checksumPos, 1) != 1) {
        // Malformed checksum
        return GDB_SendNackIfPossible(ctx);
    } else if (GDB_ComputeChecksum(ctx->buffer + 1, delimPos - 1) != checksum) {
        // Invalid checksum
        return GDB_SendNackIfPossible(ctx);
    }

    // Ok, send ack (if possible)
    if (!(ctx->flags & GDB_FLAG_NOACK)) {
        hdr = '+';
        transportInterfaceWriteData(iface, &hdr, 1);
    }

    // State transitions...
    if (ctx->state < GDB_STATE_ATTACHED) {
        DEBUG("Received connection from GDB, now attaching...\n");
        GDB_AttachToContext(ctx);
        ctx->state = GDB_STATE_ATTACHED;
    }

    if (ctx->noAckSent) {
        ctx->flags |= GDB_FLAG_NOACK;
        ctx->noAckSent = false;
    }

    // Set helper attributes, change '#' to NUL
    ctx->commandEnd = delimPos;
    ctx->buffer[delimPos] = '\0';

    return (int)(delimPos + 2);
}

static int GDB_DoSendPacket(GDBContext *ctx, size_t len)
{
    transportInterfaceWriteData(ctx->transportInterface, ctx->buffer, len);
    ctx->lastSentPacketSize = len;
    return (int)len;
}

int GDB_SendPacket(GDBContext *ctx, const char *packetData, size_t len)
{
    if (packetData != ctx->buffer + 1) {
        memmove(ctx->buffer + 1, packetData, len);
    }

    ctx->buffer[0] = '$';

    char *checksumLoc = ctx->buffer + len + 1;
    *checksumLoc++ = '#';

    hexItoa(GDB_ComputeChecksum(ctx->buffer + 1, len), checksumLoc, 2, false);
    return GDB_DoSendPacket(ctx, 4 + len);
}

int GDB_SendFormattedPacket(GDBContext *ctx, const char *packetDataFmt, ...)
{
    va_list args;

    va_start(args, packetDataFmt);
    int n = vsprintf(ctx->buffer + 1, packetDataFmt, args);
    va_end(args);

    if (n < 0) {
        return -1;
    }

    ctx->buffer[0] = '$';
    char *checksumLoc = ctx->buffer + n + 1;
    *checksumLoc++ = '#';

    hexItoa(GDB_ComputeChecksum(ctx->buffer + 1, n), checksumLoc, 2, false);
    return GDB_DoSendPacket(ctx, 4 + n);
}

int GDB_SendHexPacket(GDBContext *ctx, const void *packetData, size_t len)
{
    if(4 + 2 * len > GDB_BUF_LEN)
        return -1;

    ctx->buffer[0] = '$';
    GDB_EncodeHex(ctx->buffer + 1, packetData, len);

    char *checksumLoc = ctx->buffer + 2 * len + 1;
    *checksumLoc++ = '#';

    hexItoa(GDB_ComputeChecksum(ctx->buffer + 1, 2 * len), checksumLoc, 2, false);
    return GDB_DoSendPacket(ctx, 4 + 2 * len);
}

int GDB_SendNotificationPacket(GDBContext *ctx, const char *packetData, size_t len)
{
    if (packetData != ctx->buffer + 1) {
        memmove(ctx->buffer + 1, packetData, len);
    }

    ctx->buffer[0] = '%';

    char *checksumLoc = ctx->buffer + len + 1;
    *checksumLoc++ = '#';

    hexItoa(GDB_ComputeChecksum(ctx->buffer + 1, len), checksumLoc, 2, false);
    return GDB_DoSendPacket(ctx, 4 + len);
}

int GDB_SendStreamData(GDBContext *ctx, const char *streamData, size_t offset, size_t length, size_t totalSize, bool forceEmptyLast)
{
    // GDB_BUF_LEN does not include the usual %#<1-byte checksum>
    if(length > GDB_BUF_LEN - 1) {
        length = GDB_BUF_LEN - 1;
    }

    char letter;

    if ((forceEmptyLast && offset >= totalSize) || (!forceEmptyLast && offset + length >= totalSize)) {
        length = offset >= totalSize ? 0 : totalSize - offset;
        letter = 'l';
    } else {
        letter = 'm';
    }

    // Note: ctx->buffer[0] = '$'
    if (streamData + offset != ctx->buffer + 2) {
        memmove(ctx->buffer + 2, streamData + offset, length);
    }
    ctx->buffer[1] = letter; 
    return GDB_SendPacket(ctx, ctx->buffer + 1, 1 + length);
}

int GDB_ReplyEmpty(GDBContext *ctx)
{
    return GDB_SendPacket(ctx, "", 0);
}

int GDB_ReplyOk(GDBContext *ctx)
{
    return GDB_SendPacket(ctx, "OK", 2);
}

int GDB_ReplyErrno(GDBContext *ctx, int no)
{
    char buf[] = "E01";
    hexItoa(no & 0xFF, buf + 1, 2, false);
    return GDB_SendPacket(ctx, buf, 3);
}
