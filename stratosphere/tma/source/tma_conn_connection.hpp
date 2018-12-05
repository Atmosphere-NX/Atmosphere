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
 
#pragma once
#include <switch.h>
#include <stratosphere.hpp>

#include "tma_conn_result.hpp"
#include "tma_conn_packet.hpp"

enum class ConnectionEvent : u32 {
    Connected,
    Disconnected
};


class TmaServiceManager;

class TmaConnection {
    protected:
        HosMutex lock;
        void (*connection_event_callback)(void *, ConnectionEvent) = nullptr;
        void *connection_event_arg = nullptr;
        bool has_woken_up = false;
        bool is_initialized = false;
        TmaServiceManager *service_manager = nullptr;
    protected:
        void OnReceivePacket(TmaPacket *packet);
        void OnDisconnected();
        void OnConnectionEvent(ConnectionEvent event);
        void CancelTasks();
        void Tick();
    public:
        /* Setup */
        TmaConnection() { }
        virtual ~TmaConnection() { }
        
        void Initialize() { 
            if (this->is_initialized) { 
                std::abort();
            }
            this->is_initialized = true;
        }
        
        void SetConnectionEventCallback(void (*callback)(void *, ConnectionEvent), void *arg) {
            this->connection_event_callback = callback;
            this->connection_event_arg = arg;
        }
        
        void Finalize() {
            if (this->is_initialized) {
                this->StopListening();
                if (this->IsConnected()) {
                    this->Disconnect();
                }
                this->is_initialized = false;
            }
        }
        
        void SetServiceManager(TmaServiceManager *manager) { this->service_manager = manager; }
        
        /* Packet management. */
        TmaPacket *AllocateSendPacket();
        TmaPacket *AllocateRecvPacket();
        void FreePacket(TmaPacket *packet);
        
        /* Sleep management. */
        bool HasWokenUp() const { return this->has_woken_up; }
        void SetWokenUp(bool woke) { this->has_woken_up = woke; }

        /* For sub-interfaces to implement, connection management. */
        virtual void StartListening() { }
        virtual void StopListening() { }
        virtual bool IsConnected() = 0;
        virtual TmaConnResult Disconnect() = 0;
        virtual TmaConnResult SendPacket(TmaPacket *packet) = 0;
};