/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb.h"

int GDB_SendMemory(GDBContext *ctx, const char *prefix, size_t prefixLen, uintptr_t addr, size_t len);
int GDB_WriteMemory(GDBContext *ctx, const void *buf, uintptr_t addr, size_t len);
u32 GDB_SearchMemory(bool *found, GDBContext *ctx, size_t addr, size_t len, const void *pattern, size_t patternLen);

GDB_DECLARE_HANDLER(ReadMemory);
GDB_DECLARE_HANDLER(WriteMemory);
GDB_DECLARE_HANDLER(WriteMemoryRaw);
GDB_DECLARE_QUERY_HANDLER(SearchMemory);
