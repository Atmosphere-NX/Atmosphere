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

#pragma once

#include "utils.h"
#include "spinlock.h"

#define MAX_TRANSPORT_INTERFACES    4

typedef enum TransportInterfaceType {
    TRANSPORT_INTERFACE_TYPE_UART = 0,

    TRANSPORT_INTERFACE_TYPE_MAX,
} TransportInterfaceType;

struct TransportInterface;
typedef struct TransportInterface TransportInterface;
typedef size_t (*TransportInterfaceReceiveDataCallback)(TransportInterface *iface, void *ctx);
typedef void (*TransportInterfaceProcessDataCallback)(TransportInterface *iface, void *ctx, size_t dataSize);

void transportInterfaceInitLayer(void);
TransportInterface *transportInterfaceCreate(
    TransportInterfaceType type,
    u32 id,
    u32 flags,
    TransportInterfaceReceiveDataCallback receiveDataCallback,
    TransportInterfaceProcessDataCallback processDataCallback,
    void *ctx
);
void transportInterfaceAcquire(TransportInterface *iface);
void transportInterfaceRelease(TransportInterface *iface);

TransportInterface *transportInterfaceFindByIrqId(u16 irqId);
void transportInterfaceSetInterruptAffinity(TransportInterface *iface, u8 affinity);

TransportInterface *transportInterfaceIrqHandlerTopHalf(u16 irqId);
void transportInterfaceIrqHandlerBottomHalf(TransportInterface *iface);

void transportInterfaceWriteData(TransportInterface *iface, const void *buffer, size_t size);
void transportInterfaceReadData(TransportInterface *iface, void *buffer, size_t size);
size_t transportInterfaceReadDataMax(TransportInterface *iface, void *buffer, size_t maxSize);
size_t transportInterfaceReadDataUntil(TransportInterface *iface, char *buffer, size_t maxSize, char delimiter);
