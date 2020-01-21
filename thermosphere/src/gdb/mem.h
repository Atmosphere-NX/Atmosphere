/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb.h"

Result GDB_ReadTargetMemoryInPage(void *out, GDBContext *ctx, u32 addr, u32 len);
Result GDB_WriteTargetMemoryInPage(GDBContext *ctx, const void *in, u32 addr, u32 len);
u32 GDB_ReadTargetMemory(void *out, GDBContext *ctx, u32 addr, u32 len);
u32 GDB_WriteTargetMemory(GDBContext *ctx, const void *in, u32 addr, u32 len);

int GDB_SendMemory(GDBContext *ctx, const char *prefix, u32 prefixLen, u32 addr, u32 len);
int GDB_WriteMemory(GDBContext *ctx, const void *buf, u32 addr, u32 len);
u32 GDB_SearchMemory(bool *found, GDBContext *ctx, u32 addr, u32 len, const void *pattern, u32 patternLen);

GDB_DECLARE_HANDLER(ReadMemory);
GDB_DECLARE_HANDLER(WriteMemory);
GDB_DECLARE_HANDLER(WriteMemoryRaw);
GDB_DECLARE_QUERY_HANDLER(SearchMemory);
