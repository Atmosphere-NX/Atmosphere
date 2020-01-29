/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "context.h"
#include "../core_ctx.h"
#include "../debug_manager.h"

void GDB_ContinueExecution(GDBContext *ctx);
int GDB_SendStopReply(GDBContext *ctx, const DebugEventInfo *info, bool asNotification);
int GDB_TrySignalDebugEvent(GDBContext *ctx, DebugEventInfo *info);

GDB_DECLARE_VERBOSE_HANDLER(Stopped);

GDB_DECLARE_HANDLER(Detach);
GDB_DECLARE_HANDLER(Kill);
GDB_DECLARE_VERBOSE_HANDLER(CtrlC);
GDB_DECLARE_HANDLER(ContinueOrStepDeprecated);
GDB_DECLARE_VERBOSE_HANDLER(Continue);
GDB_DECLARE_HANDLER(GetStopReason);

//void GDB_BreakProcessAndSinkDebugEvents(GDBContext *ctx, DebugFlags flags);
