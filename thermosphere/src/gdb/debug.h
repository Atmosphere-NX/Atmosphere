/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb.h"

GDB_DECLARE_VERBOSE_HANDLER(Run);
GDB_DECLARE_HANDLER(Restart);
GDB_DECLARE_VERBOSE_HANDLER(Attach);
GDB_DECLARE_HANDLER(Detach);
GDB_DECLARE_HANDLER(Kill);
GDB_DECLARE_HANDLER(Break);
GDB_DECLARE_HANDLER(Continue);
GDB_DECLARE_VERBOSE_HANDLER(Continue);
GDB_DECLARE_HANDLER(GetStopReason);

void GDB_ContinueExecution(GDBContext *ctx);
void GDB_PreprocessDebugEvent(GDBContext *ctx, DebugEventInfo *info);
int GDB_SendStopReply(GDBContext *ctx, const DebugEventInfo *info);
int GDB_HandleDebugEvents(GDBContext *ctx);
void GDB_BreakProcessAndSinkDebugEvents(GDBContext *ctx, DebugFlags flags);
