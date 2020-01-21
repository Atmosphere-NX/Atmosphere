/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "gdb.h"

#ifndef GDB_PORT_BASE
#define GDB_PORT_BASE 4000
#endif

typedef struct GDBServer
{
    sock_server super;
    s32 referenceCount;
    Handle statusUpdated;
    Handle statusUpdateReceived;
    GDBContext ctxs[MAX_DEBUG];
} GDBServer;

Result GDB_InitializeServer(GDBServer *server);
void GDB_FinalizeServer(GDBServer *server);

void GDB_IncrementServerReferenceCount(GDBServer *server);
void GDB_DecrementServerReferenceCount(GDBServer *server);

void GDB_RunServer(GDBServer *server);

void GDB_LockAllContexts(GDBServer *server);
void GDB_UnlockAllContexts(GDBServer *server);

GDBContext *GDB_SelectAvailableContext(GDBServer *server, u16 minPort, u16 maxPort);
GDBContext *GDB_FindAllocatedContextByPid(GDBServer *server, u32 pid);

int GDB_AcceptClient(GDBContext *ctx);
int GDB_CloseClient(GDBContext *ctx);
GDBContext *GDB_GetClient(GDBServer *server, u16 port);
void GDB_ReleaseClient(GDBServer *server, GDBContext *ctx);
int GDB_DoPacket(GDBContext *ctx);
