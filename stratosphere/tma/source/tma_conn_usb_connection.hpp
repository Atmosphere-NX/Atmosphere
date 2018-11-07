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

#include "tma_conn_connection.hpp"
#include "tma_usb_comms.hpp"

class TmaUsbConnection : public TmaConnection {
    private:
        HosMessageQueue send_queue = HosMessageQueue(64);
        std::atomic<bool> is_connected = false;
    private:
        static void SendThreadFunc(void *arg);
        static void RecvThreadFunc(void *arg);
        static void OnUsbStateChange(void *this_ptr, u32 state);
        TmaConnResult SendQueryReply(TmaPacket *packet);
        void ClearSendQueue();
        void StartThreads();
        void StopThreads();
        void SetConnected(bool c) { this->is_connected = c; }
    public:
        static TmaConnResult InitializeComms();
        static TmaConnResult FinalizeComms();
        
        TmaUsbConnection() { 
            TmaUsbComms::SetStateChangeCallback(&TmaUsbConnection::OnUsbStateChange, this);
        }
        
        virtual ~TmaUsbConnection() {
            this->Disconnect(); 
        }
        
        virtual bool IsConnected() override;
        virtual TmaConnResult Disconnect() override;
        virtual TmaConnResult SendPacket(TmaPacket *packet) override;
};