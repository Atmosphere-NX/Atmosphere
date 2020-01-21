/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb.h"

#define GDB_XFER_HANDLER(name)                  GDB_HANDLER(Xfer##name)
#define GDB_DECLARE_XFER_HANDLER(name)          int GDB_XFER_HANDLER(name)(GDBContext *ctx, bool write, const char *annex, u32 offset, u32 length)

#define GDB_XFER_OSDATA_HANDLER(name)           GDB_XFER_HANDLER(OsData##name)
#define GDB_DECLARE_XFER_OSDATA_HANDLER(name)   int GDB_XFER_OSDATA_HANDLER(name)(GDBContext *ctx, bool write, u32 offset, u32 length)

GDB_DECLARE_XFER_HANDLER(Features);

GDB_DECLARE_XFER_OSDATA_HANDLER(CfwVersion);
GDB_DECLARE_XFER_OSDATA_HANDLER(Memory);
GDB_DECLARE_XFER_OSDATA_HANDLER(Processes);

GDB_DECLARE_XFER_HANDLER(OsData);

GDB_DECLARE_QUERY_HANDLER(Xfer);
