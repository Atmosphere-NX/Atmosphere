/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "context.h"

bool GDB_FetchPackedHioRequest(GDBContext *ctx, u32 addr);
bool GDB_IsHioInProgress(GDBContext *ctx);
int GDB_SendCurrentHioRequest(GDBContext *ctx);

GDB_DECLARE_HANDLER(HioReply);
