/*
*   This file is part of Luma3DS.
*   Copyright (C) 2016-2019 Aurora Wright, TuxSH
*
*   SPDX-License-Identifier: (MIT OR GPL-2.0-or-later)
*/

#pragma once

#include "../gdb.h"
#include "../transport_interface.h"

typedef struct GDBServer {
    TransportInterface *transportIfaces[MAX_CTX];
    GDBContext ctxs[MAX_CTX];
} GDBServer;

void GDB_InitializeServer(GDBServer *server);
void GDB_FinalizeServer(GDBServer *server);

void GDB_RunServer(GDBServer *server);

void GDB_LockAllContexts(GDBServer *server);
void GDB_UnlockAllContexts(GDBServer *server);

// Currently, transport ifaces are tied to client

GDBContext *GDB_SelectAvailableContext(GDBServer *server);

int GDB_AcceptClient(GDBContext *ctx);
int GDB_CloseClient(GDBContext *ctx);
GDBContext *GDB_GetClient(GDBServer *server, TransportInterface *iface);
void GDB_ReleaseClient(GDBServer *server, GDBContext *ctx);
int GDB_DoPacket(GDBContext *ctx);
