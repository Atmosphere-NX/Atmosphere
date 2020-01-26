/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "context.h"

#define GDB_XFER_HANDLER(name)                  GDB_HANDLER(Xfer##name)
#define GDB_DECLARE_XFER_HANDLER(name)          int GDB_XFER_HANDLER(name)(GDBContext *ctx, bool write, const char *annex, size_t offset, size_t length)

GDB_DECLARE_XFER_HANDLER(Features);

GDB_DECLARE_QUERY_HANDLER(Xfer);
