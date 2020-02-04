/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb_context.hpp"
#include "../core_ctx.h"
#include "../debug_manager.h"

int GDB_SendStopReply(GDBContext *ctx, const DebugEventInfo *info, bool asNotification);
int GDB_TrySignalDebugEvent(GDBContext *ctx, DebugEventInfo *info);

void GDB_BreakAllCores(GDBContext *ctx);
