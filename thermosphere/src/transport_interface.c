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

#include "transport_interface.h"
#include "platform/uart.h"
#include "core_ctx.h"
#include "irq.h"

#define FOREACH_LINK(link, list)\
    for (TransportInterfaceLink *link = transportInterfaceListGetFirstLink(list); link != transportInterfaceListGetEndLink(list); link = link->next)

typedef struct TransportInterfaceLink {
    struct TransportInterfaceLink *prev, *next;
} TransportInterfaceLink;

struct TransportInterface {
    TransportInterfaceLink link;
    TransportInterfaceReceiveDataCallback receiveDataCallback;
    TransportInterfaceProcessDataCallback processDataCallback;
    void *ctx;
    RecursiveSpinlock lock;
    u32 id;
    u32 flags;
    TransportInterfaceType type;
    // ignored ReadWriteDirection interruptDirection;
};

typedef struct TransportInterfaceList {
    TransportInterfaceLink link;
} TransportInterfaceList;

static RecursiveSpinlock g_transportInterfaceLayerLock;
static TransportInterface g_transportInterfaceStorage[MAX_TRANSPORT_INTERFACES];

static TransportInterfaceList g_transportInterfaceList, g_transportInterfaceFreeList;

static inline TransportInterfaceLink *transportInterfaceListGetLastLink(TransportInterfaceList *list)
{
    return list->link.prev;
}

static inline TransportInterfaceLink *transportInterfaceListGetFirstLink(TransportInterfaceList *list)
{
    return list->link.next;
}

static inline TransportInterfaceLink *transportInterfaceListGetEndLink(TransportInterfaceList *list)
{
    return &list->link;
}

static inline bool transportInterfaceListIsEmpty(TransportInterfaceList *list)
{
    return transportInterfaceListGetFirstLink(list) == transportInterfaceListGetEndLink(list);
}

static inline TransportInterface *transportInterfaceGetLinkParent(TransportInterfaceLink *link)
{
    // Static aliasing rules say the first member of a struct has the same address as the struct itself
    return (TransportInterface *)link;
}

static inline void transportInterfaceListInit(TransportInterfaceList *list)
{
    list->link.prev = transportInterfaceListGetEndLink(list);
    list->link.next = transportInterfaceListGetEndLink(list);
}

static inline void transportInterfaceListInsertAfter(TransportInterfaceLink *pos, TransportInterfaceLink *nd)
{
    // if pos is last & list is empty, ->next writes to first, etc.
    pos->next->prev = nd;
    nd->prev = pos;
    nd->next = pos->next;
    pos->next = nd;
}

static inline void transportInterfaceListInsertBefore(TransportInterfaceLink *pos, TransportInterfaceLink *nd)
{
    pos->prev->next = nd;
    nd->prev = pos->prev;
    nd->next = pos;
    pos->prev = nd;
}

static inline void transportInterfaceListRemove(const TransportInterfaceLink *nd)
{
    nd->prev->next = nd->next;
    nd->next->prev = nd->prev;
}

void transportInterfaceInitLayer(void)
{
    transportInterfaceListInit(&g_transportInterfaceList);
    transportInterfaceListInit(&g_transportInterfaceFreeList);

    for (u32 i = 0; i < MAX_TRANSPORT_INTERFACES; i++) {
        transportInterfaceListInsertAfter(transportInterfaceListGetLastLink(&g_transportInterfaceFreeList), &g_transportInterfaceStorage[i].link);
    }
}

TransportInterface *transportInterfaceCreate(
    TransportInterfaceType type,
    u32 id,
    u32 flags,
    TransportInterfaceReceiveDataCallback receiveDataCallback,
    TransportInterfaceProcessDataCallback processDataCallback,
    void *ctx
)
{
    u64 irqFlags = recursiveSpinlockLockMaskIrq(&g_transportInterfaceLayerLock);
    if (transportInterfaceListIsEmpty(&g_transportInterfaceFreeList)) {
        PANIC("transportInterfaceCreateAndInit: resource exhaustion\n");
    }

    TransportInterface *iface;

    FOREACH_LINK (link, &g_transportInterfaceList) {
        iface = transportInterfaceGetLinkParent(link);
        if (iface->type == type && iface->id == id) {
            PANIC("transportInterfaceCreateAndInit: device already registered\n");
        }
    }

    iface = transportInterfaceGetLinkParent(transportInterfaceListGetFirstLink(&g_transportInterfaceFreeList));

    iface->type = type;
    iface->id = id;
    iface->receiveDataCallback = receiveDataCallback;
    iface->processDataCallback = processDataCallback;
    iface->ctx = ctx;

    switch (type) {
        case TRANSPORT_INTERFACE_TYPE_UART: {
            UartDevice dev = (UartDevice)id;
            uartInit(dev, BAUD_115200, flags);
            uartSetInterruptStatus(id, DIRECTION_READ, iface->receiveDataCallback != NULL);
            break;
        }
        default:
            break;
    }

    transportInterfaceListRemove(&iface->link);
    transportInterfaceListInsertAfter(transportInterfaceListGetLastLink(&g_transportInterfaceList), &iface->link);
    recursiveSpinlockUnlockRestoreIrq(&g_transportInterfaceLayerLock, irqFlags);

    return iface;
}

void transportInterfaceAcquire(TransportInterface *iface)
{
    // Get the lock, prevent the interrupt from being pending if there's incoming data
    recursiveSpinlockLock(&iface->lock);

    switch (iface->type) {
        case TRANSPORT_INTERFACE_TYPE_UART: {
            UartDevice dev = (UartDevice)iface->id;
            uartSetInterruptStatus(dev, DIRECTION_READ, false);
            break;
        }
        default:
            break;
    }
}

void transportInterfaceRelease(TransportInterface *iface)
{
    // See transportInterfaceAcquire
    switch (iface->type) {
        case TRANSPORT_INTERFACE_TYPE_UART: {
            UartDevice dev = (UartDevice)iface->id;
            uartSetInterruptStatus(dev, DIRECTION_READ, true);
            break;
        }
        default:
            break;
    }

    recursiveSpinlockUnlock(&iface->lock);
}

TransportInterface *transportInterfaceFindByIrqId(u16 irqId)
{
    u64 irqFlags = recursiveSpinlockLockMaskIrq(&g_transportInterfaceLayerLock);

    TransportInterface *ret = NULL;
    FOREACH_LINK (link, &g_transportInterfaceList) {
        TransportInterface *iface = transportInterfaceGetLinkParent(link);
        if (iface->type == TRANSPORT_INTERFACE_TYPE_UART && uartGetIrqId((UartDevice)iface->id) == irqId) {
            ret = iface;
            break;
        }
    }

    recursiveSpinlockUnlockRestoreIrq(&g_transportInterfaceLayerLock, irqFlags);
    return ret;
}

void transportInterfaceSetInterruptAffinity(TransportInterface *iface, u8 affinity)
{
    u64 irqFlags = recursiveSpinlockLockMaskIrq(&iface->lock);
    switch (iface->type) {
        case TRANSPORT_INTERFACE_TYPE_UART: {
            UartDevice dev = (UartDevice)iface->id;
            irqSetAffinity(uartGetIrqId(dev), affinity);
            break;
        }
        default:
            break;
    }
    recursiveSpinlockUnlockRestoreIrq(&iface->lock, irqFlags);
}

TransportInterface *transportInterfaceIrqHandlerTopHalf(u16 irqId)
{
    TransportInterface *iface = transportInterfaceFindByIrqId(irqId);
    if (iface == NULL) {
        PANIC("transportInterfaceLayerIrqHandlerTop: irq id %x not found!\n", irqId);
    }

    transportInterfaceAcquire(iface);

    // Interrupt should have gone from active-and-pending to only active (on the GIC)
    return iface;
}

void transportInterfaceIrqHandlerBottomHalf(TransportInterface *iface)
{
    size_t sz = iface->receiveDataCallback(iface, iface->ctx);
    if (sz > 0 && iface->processDataCallback != NULL) {
        iface->processDataCallback(iface, iface->ctx, sz);
    }

    transportInterfaceRelease(iface);
}

void transportInterfaceWriteData(TransportInterface *iface, const void *buffer, size_t size)
{
    switch (iface->type) {
        case TRANSPORT_INTERFACE_TYPE_UART: {
            UartDevice dev = (UartDevice)iface->id;
            uartWriteData(dev, buffer, size);
            break;
        }
        default:
            break;
    }
}

void transportInterfaceReadData(TransportInterface *iface, void *buffer, size_t size)
{
    switch (iface->type) {
        case TRANSPORT_INTERFACE_TYPE_UART: {
            UartDevice dev = (UartDevice)iface->id;
            uartReadData(dev, buffer, size);
            break;
        }
        default:
            break;
    }
}

size_t transportInterfaceReadDataMax(TransportInterface *iface, void *buffer, size_t maxSize)
{
    switch (iface->type) {
        case TRANSPORT_INTERFACE_TYPE_UART: {
            UartDevice dev = (UartDevice)iface->id;
            return uartReadDataMax(dev, buffer, maxSize);
        }
        default:
            return 0;
            break;
    }
}

size_t transportInterfaceReadDataUntil(TransportInterface *iface, char *buffer, size_t maxSize, char delimiter)
{
    switch (iface->type) {
        case TRANSPORT_INTERFACE_TYPE_UART: {
            UartDevice dev = (UartDevice)iface->id;
            return uartReadDataUntil(dev, buffer, maxSize, delimiter);
        }
        default:
            return 0;
            break;
    }
}
