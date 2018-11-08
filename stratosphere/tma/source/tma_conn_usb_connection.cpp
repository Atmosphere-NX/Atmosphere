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
#include "tma_conn_usb_connection.hpp"
#include "tma_usb_comms.hpp"

static HosThread g_SendThread, g_RecvThread;

TmaConnResult TmaUsbConnection::InitializeComms() {
    return TmaUsbComms::Initialize();
}

TmaConnResult TmaUsbConnection::FinalizeComms() {
    return TmaUsbComms::Finalize();
}

void TmaUsbConnection::ClearSendQueue() {
    uintptr_t _packet;
    while (this->send_queue.TryReceive(&_packet)) {
        TmaPacket *packet = reinterpret_cast<TmaPacket *>(_packet);
        if (packet != nullptr) {
            this->FreePacket(packet);
        }
    }
}

void TmaUsbConnection::SendThreadFunc(void *arg) {
    TmaUsbConnection *this_ptr = reinterpret_cast<TmaUsbConnection *>(arg);
    TmaConnResult res = TmaConnResult::Success;
    TmaPacket *packet = nullptr;
    
    while (res == TmaConnResult::Success) {
        /* Receive a packet from the send queue. */
        {
            uintptr_t _packet;
            this_ptr->send_queue.Receive(&_packet);
            packet = reinterpret_cast<TmaPacket *>(_packet);
        }
        
        if (packet != nullptr) {
            /* Send the packet if we're connected. */
            if (this_ptr->IsConnected()) {
                res = TmaUsbComms::SendPacket(packet);
            }
            
            this_ptr->FreePacket(packet);
            this_ptr->Tick();
        } else {
            res = TmaConnResult::Disconnected;
        }
    }
    
    this_ptr->SetConnected(false);
    this_ptr->OnDisconnected();
}

void TmaUsbConnection::RecvThreadFunc(void *arg) {
    TmaUsbConnection *this_ptr = reinterpret_cast<TmaUsbConnection *>(arg);
    TmaConnResult res = TmaConnResult::Success;
    u64 i = 0;
    this_ptr->SetConnected(true);
    
    while (res == TmaConnResult::Success) {
        if (!this_ptr->IsConnected()) {
            break;
        }
        TmaPacket *packet = this_ptr->AllocateRecvPacket();
        if (packet == nullptr) { std::abort(); }
        
        res = TmaUsbComms::ReceivePacket(packet);
        
        if (res == TmaConnResult::Success) {
            TmaPacket *send_packet = this_ptr->AllocateSendPacket();
            send_packet->Write<u64>(i++);
            this_ptr->send_queue.Send(reinterpret_cast<uintptr_t>(send_packet));
            this_ptr->FreePacket(packet);
        } else {
            this_ptr->FreePacket(packet);
        }
    }
    
    this_ptr->SetConnected(false);
    this_ptr->send_queue.Send(reinterpret_cast<uintptr_t>(nullptr));
}

void TmaUsbConnection::OnUsbStateChange(void *arg, u32 state) {
    TmaUsbConnection *this_ptr = reinterpret_cast<TmaUsbConnection *>(arg);
    switch (state) {
        case 0:
        case 6:
            this_ptr->StopThreads();
            break;
        case 5:
            this_ptr->StartThreads();
            break;
    }
}

void TmaUsbConnection::StartThreads() {
    g_SendThread.Join();
    g_RecvThread.Join();
    
    g_SendThread.Initialize(&TmaUsbConnection::SendThreadFunc, this, 0x4000, 38);
    g_RecvThread.Initialize(&TmaUsbConnection::RecvThreadFunc, this, 0x4000, 38);
    
    this->ClearSendQueue();
    g_SendThread.Start();
    g_RecvThread.Start();
}

void TmaUsbConnection::StopThreads() {
    TmaUsbComms::CancelComms();
    g_SendThread.Join();
    g_RecvThread.Join();
}

bool TmaUsbConnection::IsConnected() {
    return this->is_connected;
}

TmaConnResult TmaUsbConnection::Disconnect() {
    TmaUsbComms::SetStateChangeCallback(nullptr, nullptr);
    
    this->StopThreads();
    
    return TmaConnResult::Success;
}

TmaConnResult TmaUsbConnection::SendPacket(TmaPacket *packet) {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    if (this->IsConnected()) {
        this->send_queue.Send(reinterpret_cast<uintptr_t>(packet));
        return TmaConnResult::Success;
    } else {
        this->FreePacket(packet);
        this->Tick();
        return TmaConnResult::Disconnected;
    }
}
