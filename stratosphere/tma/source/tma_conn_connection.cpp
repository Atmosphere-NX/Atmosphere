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
#include "tma_service_manager.hpp"

/* Packet management. */
TmaPacket *TmaConnection::AllocateSendPacket() {
    return this->service_manager->AllocateSendPacket();
}

TmaPacket *TmaConnection::AllocateRecvPacket() {
    return this->service_manager->AllocateRecvPacket();
}

void TmaConnection::FreePacket(TmaPacket *packet) {
    this->service_manager->FreePacket(packet);
}

void TmaConnection::OnReceivePacket(TmaPacket *packet) {
    this->service_manager->OnReceivePacket(packet);
}

void TmaConnection::OnDisconnected() {
    if (!this->is_initialized) {
        std::abort();
    }
    
    if (this->service_manager != nullptr) {
        this->service_manager->OnDisconnect();
    }
    
    this->has_woken_up = false;
    this->OnConnectionEvent(ConnectionEvent::Disconnected);
}

void TmaConnection::OnConnectionEvent(ConnectionEvent event) {
    if (this->connection_event_callback != nullptr) {
        this->connection_event_callback(this->connection_event_arg, event);
    }
}

void TmaConnection::CancelTasks() {
    this->service_manager->CancelTasks();
}

void TmaConnection::Tick() {
    this->service_manager->Tick();
}
