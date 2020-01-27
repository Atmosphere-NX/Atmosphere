/*
 * Copyright (c) 2019 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Lots of code from:
/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "defines.h"
#include "../transport_interface.h"

typedef struct PackedGdbHioRequest
{
    char magic[4]; // "GDB\x00"
    u32 version;

    // Request
    char functionName[16+1];
    char paramFormat[8+1];

    u64 parameters[8];
    size_t stringLengths[8];

    // Return
    s64 retval;
    int gdbErrno;
    bool ctrlC;
} PackedGdbHioRequest;

enum {
    GDB_FLAG_NOACK              = BIT(0),
    GDB_FLAG_CONTINUING         = BIT(1),
    GDB_FLAG_TERMINATE          = BIT(2),
    GDB_FLAG_ATTACHED_AT_START  = BIT(3),
    GDB_FLAG_NONSTOP            = BIT(4),
};

typedef enum GDBState
{
    GDB_STATE_DISCONNECTED,
    GDB_STATE_CONNECTED,
    GDB_STATE_ATTACHED,
    GDB_STATE_DETACHING,
} GDBState;

typedef struct GDBContext {
    // No need for a lock, it's in the transport interface layer...

    TransportInterface *transportInterface;
    u32 flags;
    GDBState state;
    bool noAckSent;

    u32 currentThreadId;
    u32 selectedThreadId;
    u32 selectedThreadIdForContinuing;
    bool catchThreadEvents;
    bool processEnded, processExited;

    //DebugEventInfo latestDebugEvent; FIXME

    uintptr_t currentHioRequestTargetAddr;
    PackedGdbHioRequest currentHioRequest;

    size_t targetXmlLen;

    char *commandData, *commandEnd;
    size_t lastSentPacketSize;
    char *buffer[GDB_BUF_LEN + 4];
    char *workBuffer;
} GDBContext;

typedef int (*GDBCommandHandler)(GDBContext *ctx);

void GDB_InitializeContext(GDBContext *ctx, TransportInterfaceType ifaceType, u32 ifaceId, u32 ifaceFlags);

void GDB_AttachToContext(GDBContext *ctx);
void GDB_DetachFromContext(GDBContext *ctx);

void GDB_AcquireContext(GDBContext *ctx);
void GDB_ReleaseContext(GDBContext *ctx);
void GDB_MigrateRxIrq(GDBContext *ctx, u32 newAffinity);

GDB_DECLARE_HANDLER(Unsupported);
GDB_DECLARE_HANDLER(EnableExtendedMode);

static inline bool GDB_IsAttached(GDBContext *ctx)
{
    return ctx->state == GDB_STATE_ATTACHED;
}
