/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "utils.h"
#include "spinlock.h"

#define MAX_CTX           3

// 512+24 is the ideal size as IDA will try to read exactly 0x100 bytes at a time. Add 4 to this, for $#<checksum>, see below.
// IDA seems to want additional bytes as well.
// 1024 is fine enough to put all regs in the 'T' stop reply packets
#define GDB_BUF_LEN 2048

#define GDB_HANDLER(name)           GDB_Handle##name
#define GDB_QUERY_HANDLER(name)     GDB_HANDLER(Query##name)
#define GDB_VERBOSE_HANDLER(name)   GDB_HANDLER(Verbose##name)

#define GDB_DECLARE_HANDLER(name)           int GDB_HANDLER(name)(GDBContext *ctx)
#define GDB_DECLARE_QUERY_HANDLER(name)     GDB_DECLARE_HANDLER(Query##name)
#define GDB_DECLARE_VERBOSE_HANDLER(name)   GDB_DECLARE_HANDLER(Verbose##name)

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
    GDB_FLAG_SELECTED           = BIT(0),
    GDB_FLAG_USED               = BIT(1),
    GDB_FLAG_ALLOCATED_MASK     = GDB_FLAG_SELECTED | GDB_FLAG_USED,
    GDB_FLAG_EXTENDED_REMOTE    = BIT(2), // unused here
    GDB_FLAG_NOACK              = BIT(3),
    GDB_FLAG_RESTART_MASK  = GDB_FLAG_NOACK | GDB_FLAG_EXTENDED_REMOTE | GDB_FLAG_USED,
    GDB_FLAG_CONTINUING         = BIT(4),
    GDB_FLAG_TERMINATE          = BIT(5),
    GDB_FLAG_ATTACHED_AT_START  = BIT(6),
    GDB_FLAG_CREATED            = BIT(7),
};

typedef enum GDBState
{
    GDB_STATE_DISCONNECTED,
    GDB_STATE_CONNECTED,
    GDB_STATE_ATTACHED,
    GDB_STATE_DETACHING,
} GDBState;

typedef struct ThreadInfo
{
    u32 id;
    u32 tls;
} ThreadInfo;

struct GDBServer;

typedef struct GDBContext
{
    TransportInterface *transportIface;
    struct GDBServer *parent;

    RecursiveSpinlock lock;

    u32 flags;
    GDBState state;
    bool noAckSent;

    u32 currentThreadId;

    bool catchThreadEvents;
    bool processEnded, processExited;

    //DebugEventInfo latestDebugEvent; FIXME

    uintptr_t currentHioRequestTargetAddr;
    PackedGdbHioRequest currentHioRequest;

    char *commandData, *commandEnd;
    int latestSentPacketSize;
    char buffer[GDB_BUF_LEN + 4];
} GDBContext;

typedef int (*GDBCommandHandler)(GDBContext *ctx);

void GDB_InitializeContext(GDBContext *ctx);
void GDB_FinalizeContext(GDBContext *ctx);

void GDB_Attach(GDBContext *ctx);
void GDB_Detach(GDBContext *ctx);

GDB_DECLARE_HANDLER(Unsupported);
GDB_DECLARE_HANDLER(EnableExtendedMode);
