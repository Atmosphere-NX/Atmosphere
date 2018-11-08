/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include <switch.h>
#include <stratosphere.hpp>
#include "tma_conn_connection.hpp"

/* Packet management. */
TmaPacket *TmaConnection::AllocateSendPacket() {
    /* TODO: Service Manager. */
    return new TmaPacket();
}

TmaPacket *TmaConnection::AllocateRecvPacket() {
    /* TODO: Service Manager. */
    return new TmaPacket();
}

void TmaConnection::FreePacket(TmaPacket *packet) {
    /* TODO: Service Manager. */
    delete packet;
}

void TmaConnection::OnReceivePacket(TmaPacket *packet) {
    /* TODO: Service Manager. */
}

void TmaConnection::OnDisconnected() {
    if (!this->is_initialized) {
        std::abort();
    }
    
    /* TODO: Service Manager. */
    
    this->has_woken_up = false;
    this->OnConnectionEvent(ConnectionEvent::Disconnected);
}

void TmaConnection::OnConnectionEvent(ConnectionEvent event) {
    if (this->connection_event_callback != nullptr) {
        this->connection_event_callback(this->connection_event_arg, event);
    }
}

void TmaConnection::CancelTasks() {
    /* TODO: Service Manager. */
}

void TmaConnection::Tick() {
    /* TODO: Service Manager. */
}
