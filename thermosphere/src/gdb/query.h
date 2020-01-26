/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "context.h"

int GDB_HandleReadQuery(GDBContext *ctx);
int GDB_HandleWriteQuery(GDBContext *ctx);

GDB_DECLARE_QUERY_HANDLER(Supported);
GDB_DECLARE_QUERY_HANDLER(StartNoAckMode);
GDB_DECLARE_QUERY_HANDLER(Attached);
GDB_DECLARE_QUERY_HANDLER(CatchSyscalls);
