/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "../gdb.h"
#include "../core_ctx.h"

typedef enum DebugEventType {
    DBGEVENT_DEBUGGER_BREAK = 0,
    DBGEVENT_EXCEPTION,
    DBGEVENT_CORE_ON,
    DBGEVENT_CORE_OFF,
    DBGEVENT_EXIT,
    DBGEVENT_OUTPUT_STRING,
} DebugEventType;

typedef struct OutputStringDebugEventInfo {
    uintptr_t address;
    size_t size;
} OutputStringDebugEventInfo;

typedef struct DebugEventInfo {
    DebugEventType type;
    u32 coreId;
    ExceptionStackFrame *frame;
    union {
        OutputStringDebugEventInfo outputString;
    };
} DebugEventInfo;

GDB_DECLARE_HANDLER(Detach);
GDB_DECLARE_HANDLER(Kill);
GDB_DECLARE_HANDLER(Break);
GDB_DECLARE_HANDLER(Continue);
GDB_DECLARE_VERBOSE_HANDLER(Continue);
GDB_DECLARE_HANDLER(GetStopReason);

void GDB_ContinueExecution(GDBContext *ctx);
int GDB_SendStopReply(GDBContext *ctx, const DebugEventInfo *info);
int GDB_HandleDebugEvents(GDBContext *ctx);
void GDB_BreakProcessAndSinkDebugEvents(GDBContext *ctx, DebugFlags flags);
