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
#include "tma_service_manager.hpp"

TmaServiceManager::TmaServiceManager() {
    /* Set up queues */
    for (size_t i = 0; i < TmaServiceManager::PacketQueueDepth; i++) {
        TmaPacket *packet = nullptr;
        
        packet = new TmaPacket();
        packet->SetFreeQueue(&this->free_send_packet_queue);
        this->free_send_packet_queue.Send(reinterpret_cast<uintptr_t>(packet));
        packet = nullptr;
        
        packet = new TmaPacket();
        packet->SetFreeQueue(&this->free_recv_packet_queue);
        this->free_recv_packet_queue.Send(reinterpret_cast<uintptr_t>(packet));
        packet = nullptr;
    }
    for (size_t i = 0; i < TmaServiceManager::WorkQueueDepth; i++) {
        this->free_work_queue.Send(reinterpret_cast<uintptr_t>(new TmaWorkItem()));
    }
}

TmaServiceManager::~TmaServiceManager() {
    /* Destroy queues. */
    TmaPacket *packet = nullptr;
    while (this->free_send_packet_queue.TryReceive(reinterpret_cast<uintptr_t *>(&packet))) {
        delete packet;
        packet = nullptr;
    }
    while (this->free_recv_packet_queue.TryReceive(reinterpret_cast<uintptr_t *>(&packet))) {
        delete packet;
        packet = nullptr;
    }
    
    TmaWorkItem *work_item = nullptr;
    while (this->free_work_queue.TryReceive(reinterpret_cast<uintptr_t *>(&work_item))) {
        delete work_item;
        work_item = nullptr;
    }
    while (this->work_queue.TryReceive(reinterpret_cast<uintptr_t *>(&work_item))) {
        delete work_item;
        work_item = nullptr;
    }
}

void TmaServiceManager::Initialize() {
    this->initialized = true;
    this->work_thread.Initialize(TmaServiceManager::WorkThread, this, 0x4000, 0x26);
    this->work_thread.Start();
}

void TmaServiceManager::Finalize() {
    if (this->initialized) {
        this->initialized = false;
        if (this->connection && this->connection->IsConnected()) {
            this->connection->Disconnect();
        }
        
        /* Signal to work thread to end. */
        this->work_queue.Send(reinterpret_cast<uintptr_t>(nullptr));
        this->work_thread.Join();
        
        /* TODO: N tells services that they have no manager here. Do we want to do that? */
    }
}

void TmaServiceManager::AddWork(TmaWorkType type, TmaTask *task, TmaPacket *packet) {
    if (!this->initialized) {
        std::abort();
    }
    
    TmaWorkItem *work_item = nullptr;
    this->free_work_queue.Receive(reinterpret_cast<uintptr_t *>(&work_item));
    
    work_item->task = task;
    work_item->packet = packet;
    work_item->work_type = type;
    this->work_queue.Send(reinterpret_cast<uintptr_t>(work_item));
}

/* Packet management. */
TmaConnResult TmaServiceManager::SendPacket(TmaPacket *packet) {
    TmaConnResult res = TmaConnResult::Disconnected;
    
    if (this->connection != nullptr) {
        res = this->connection->SendPacket(packet); 
    }
    
    return res;
}


void TmaServiceManager::OnReceivePacket(TmaPacket *packet) {
    this->AddWork(TmaWorkType::ReceivePacket, nullptr, packet);
}

TmaPacket *TmaServiceManager::AllocateSendPacket() {
    if (!this->initialized) {
        std::abort();
    }
    
    TmaPacket *packet = nullptr;
    this->free_send_packet_queue.Receive(reinterpret_cast<uintptr_t *>(&packet));
    
    packet->ClearOffset();
    packet->SetBodyLength();
    
    return packet;
}

TmaPacket *TmaServiceManager::AllocateRecvPacket() {
    if (!this->initialized) {
        std::abort();
    }
    
    TmaPacket *packet = nullptr;
    this->free_recv_packet_queue.Receive(reinterpret_cast<uintptr_t *>(&packet));
    
    packet->ClearOffset();
    
    return packet;
}

void TmaServiceManager::FreePacket(TmaPacket *packet) {
    if (!this->initialized) {
        std::abort();
    }
    
    if (packet != nullptr) {
        packet->GetFreeQueue()->Send(reinterpret_cast<uintptr_t>(packet));
    }
}


/* Service/task management. */
TmaService *TmaServiceManager::GetServiceById(TmaServiceId id) {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    for (auto srv : this->services) {
        if (srv->GetServiceId() == id) {
            return srv;
        }
    }
    
    return nullptr;
}

void TmaServiceManager::AddService(TmaService *service) {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    this->services.push_back(service);
}

void TmaServiceManager::AddTask(TmaTask *task, TmaPacket *packet) {
    this->AddWork(TmaWorkType::NewTask, task, packet);
}

void TmaServiceManager::FreeTask(TmaTask *task) {
    this->AddWork(TmaWorkType::FreeTask, task, nullptr);
}

void TmaServiceManager::CancelTask(u32 task_id) {
    if (this->initialized) {
        this->task_list.Cancel(task_id);
    }
}

void TmaServiceManager::CancelTasks() {
    if (this->initialized) {
        this->task_list.CancelAll();
    }
}

u32 TmaServiceManager::GetNextTaskId() {
    while (true) {
        u32 id;
        {
            /* N only uses 16 bits for the task id. We'll use 24. */
            std::scoped_lock<HosMutex> lk(this->lock);
            id = (this->next_task_id++) & 0xFFFFFF;
        }
        
        if (this->task_list.IsIdFree(id)) {  
            return id;
        }
    }
}

/* Connection management. */
void TmaServiceManager::Tick() {
    this->AddWork(TmaWorkType::Tick, nullptr, nullptr);
}

void TmaServiceManager::SetConnection(TmaConnection *conn) {
    this->connection = conn;
}

void TmaServiceManager::OnDisconnect() {
    if (!this->initialized) {
        std::abort();
    }
    
    if (!this->GetAsleep()) {
        this->disconnect_signal.Reset();
        
        this->AddWork(TmaWorkType::Disconnect, nullptr, nullptr);
        
        /* TODO: why does N wait with a timeout of zero here? */
        this->disconnect_signal.Wait();
    }
}

void TmaServiceManager::Sleep() {
    if (!this->initialized) {
        std::abort();
    }
    
    if (!this->GetAsleep()) {
        this->wake_signal.Reset();
        this->sleep_signal.Reset();
        
        /* Tell the work thread to stall, wait for ACK. */
        this->AddWork(TmaWorkType::Sleep, nullptr, nullptr);
        this->sleep_signal.Wait();
        
        this->SetAsleep(true);
    }
}

void TmaServiceManager::Wake(TmaConnection *conn) {
    if (this->connection != nullptr) {
        std::abort();
    }
    if (this->GetAsleep()) {
        this->connection = conn;
        this->connection->SetWokenUp(true);
        this->connection->SetServiceManager(this);
        /* Tell the work thread to resume. */
        this->wake_signal.Signal();
    }
}

bool TmaServiceManager::GetConnected() const {
    return this->connection != nullptr && this->connection->IsConnected();
}

/* Work thread. */
void TmaServiceManager::WorkThread(void *_this) {
    TmaServiceManager *this_ptr = reinterpret_cast<TmaServiceManager *>(_this);
    if (!this_ptr->initialized) {
        std::abort();
    }
    
    while (true) {
        /* Receive a work item. */
        TmaWorkItem *work_item = nullptr;
        this_ptr->work_queue.Receive(reinterpret_cast<uintptr_t *>(&work_item));

        if (work_item == nullptr) {
            /* We're done. */
            this_ptr->task_list.CancelAll();
            break;
        }
        
        switch (work_item->work_type) {
            case TmaWorkType::Tick:
                /* HandleTickWork called unconditionally. */
                break;
            case TmaWorkType::NewTask:
                this_ptr->HandleNewTaskWork(work_item);
                break;
            case TmaWorkType::FreeTask:
                this_ptr->HandleFreeTaskWork(work_item);
                break;
            case TmaWorkType::ReceivePacket:
                this_ptr->HandleReceivePacketWork(work_item);
                break;
            case TmaWorkType::Disconnect:
                this_ptr->HandleDisconnectWork();
                break;
            case TmaWorkType::Sleep:
                this_ptr->HandleSleepWork();
                break;
            case TmaWorkType::None:
            default:
                std::abort();
                break;
        }
        
        this_ptr->free_work_queue.Send(reinterpret_cast<uintptr_t>(work_item));
        this_ptr->HandleTickWork();
    }
}

void TmaServiceManager::HandleNewTaskWork(TmaWorkItem *work_item) {
    this->task_list.Add(work_item->task);
    if (this->GetConnected()) {
        if (work_item->packet != nullptr) {
            this->SendPacket(work_item->packet);
        }
    } else {
        work_item->task->Cancel();
        this->FreePacket(work_item->packet);
    }
}

void TmaServiceManager::HandleFreeTaskWork(TmaWorkItem *work_item) {
    delete work_item->task;
}

void TmaServiceManager::HandleReceivePacketWork(TmaWorkItem *work_item) {
    ON_SCOPE_EXIT { this->FreePacket(work_item->packet); };
    
    /* Handle continuation packets. */
    if (work_item->packet->GetContinuation()) {
        this->task_list.ReceivePacket(work_item->packet);
        return;
    }
    
    /* Make a new task for the packet. */
    TmaService *srv = this->GetServiceById(work_item->packet->GetServiceId());
    if (srv != nullptr) {
        TmaTask *task = srv->NewTask(work_item->packet);
        if (task != nullptr) {
            this->task_list.Add(task);
        }
    }
}

void TmaServiceManager::HandleTickWork() {
    if (this->connection == nullptr) {
        std::abort();
    }
    
    /* N does this kind of manual cleanup if send isn't called. */
    /* It's pretty gross, but in lieu of a better idea... */
    bool needs_manual_cleanup = true;
    
    TmaPacket *packet = nullptr;
    
    while (this->connection != nullptr && this->free_send_packet_queue.TryReceive(reinterpret_cast<uintptr_t *>(&packet))) {
        needs_manual_cleanup = false;
        
        if (this->task_list.SendPacket(this->GetConnected(), packet)) {
            if (this->SendPacket(packet) != TmaConnResult::Success) {
                break;
            }
        } else {
            this->FreePacket(packet);
            break;
        }
    }
    
    if (needs_manual_cleanup) {
        this->task_list.CleanupDoneTasks();
    }
}

void TmaServiceManager::HandleDisconnectWork() {
    this->task_list.CancelAll();
    this->disconnect_signal.Signal();
}

void TmaServiceManager::HandleSleepWork() {
    /* Put the task list to sleep. */
    this->task_list.Sleep();
    
    /* Put services to sleep. */
    for (auto srv : this->services) {
        srv->OnSleep();
    }
    
    /* Signal to main thread that we're sleeping. */
    this->sleep_signal.Signal();
    /* Wait for us to wake up. */
    this->wake_signal.Wait();
    
    /* We're awake now... */
    this->SetAsleep(false);
    
    /* Wake up services. */
    for (auto srv : this->services) {
        srv->OnWake();
    }
    
    /* Wake up the task list. */
    this->task_list.Wake();
}
