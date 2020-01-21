/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb.h"

// We'll actually use SVC 0xFF for breakpoints :P
#define BREAKPOINT_INSTRUCTION_ARM      0xEF0000FF
#define BREAKPOINT_INSTRUCTION_THUMB    0xDFFF

u32 GDB_FindClosestBreakpointSlot(GDBContext *ctx, u32 address);
int GDB_GetBreakpointInstruction(u32 *instr, GDBContext *ctx, u32 address);
int GDB_AddBreakpoint(GDBContext *ctx, u32 address, bool thumb, bool persist);
int GDB_DisableBreakpointById(GDBContext *ctx, u32 id);
int GDB_RemoveBreakpoint(GDBContext *ctx, u32 address);
