/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb.h"
#define _REENT_ONLY
#include <errno.h>

u8 GDB_ComputeChecksum(const char *packetData, u32 len);
void GDB_EncodeHex(char *dst, const void *src, u32 len);
u32 GDB_DecodeHex(void *dst, const char *src, u32 len);
u32 GDB_EscapeBinaryData(u32 *encodedCount, void *dst, const void *src, u32 len, u32 maxLen);
u32 GDB_UnescapeBinaryData(void *dst, const void *src, u32 len);
const char *GDB_ParseIntegerList(u32 *dst, const char *src, u32 nb, char sep, char lastSep, u32 base, bool allowPrefix);
const char *GDB_ParseHexIntegerList(u32 *dst, const char *src, u32 nb, char lastSep);
const char *GDB_ParseIntegerList64(u64 *dst, const char *src, u32 nb, char sep, char lastSep, u32 base, bool allowPrefix);
const char *GDB_ParseHexIntegerList64(u64 *dst, const char *src, u32 nb, char lastSep);
int GDB_ReceivePacket(GDBContext *ctx);
int GDB_SendPacket(GDBContext *ctx, const char *packetData, u32 len);
int GDB_SendFormattedPacket(GDBContext *ctx, const char *packetDataFmt, ...);
int GDB_SendHexPacket(GDBContext *ctx, const void *packetData, u32 len);
int GDB_SendStreamData(GDBContext *ctx, const char *streamData, u32 offset, u32 length, u32 totalSize, bool forceEmptyLast);
int GDB_SendDebugString(GDBContext *ctx, const char *fmt, ...); // unsecure
int GDB_ReplyEmpty(GDBContext *ctx);
int GDB_ReplyOk(GDBContext *ctx);
int GDB_ReplyErrno(GDBContext *ctx, int no);
