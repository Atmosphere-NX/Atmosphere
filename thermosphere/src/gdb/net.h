/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "context.h"
#define _REENT_ONLY
#include <errno.h>

u8 GDB_ComputeChecksum(const char *packetData, size_t len);
size_t GDB_EncodeHex(char *dst, const void *src, size_t len);
size_t GDB_DecodeHex(void *dst, const char *src, size_t len);
size_t GDB_EscapeBinaryData(size_t *encodedCount, void *dst, const void *src, size_t len, size_t maxLen);
size_t GDB_UnescapeBinaryData(void *dst, const void *src, size_t len);

const char *GDB_ParseIntegerList(unsigned long *dst, const char *src, size_t nb, char sep, char lastSep, u32 base, bool allowPrefix);
const char *GDB_ParseHexIntegerList(unsigned long *dst, const char *src, size_t nb, char lastSep);

int GDB_ReceivePacket(GDBContext *ctx);
int GDB_SendPacket(GDBContext *ctx, const char *packetData, size_t len);
int GDB_SendFormattedPacket(GDBContext *ctx, const char *packetDataFmt, ...);
int GDB_SendHexPacket(GDBContext *ctx, const void *packetData, size_t len);
int GDB_SendStreamData(GDBContext *ctx, const char *streamData, size_t offset, size_t length, size_t totalSize, bool forceEmptyLast);
int GDB_ReplyEmpty(GDBContext *ctx);
int GDB_ReplyOk(GDBContext *ctx);
int GDB_ReplyErrno(GDBContext *ctx, int no);
