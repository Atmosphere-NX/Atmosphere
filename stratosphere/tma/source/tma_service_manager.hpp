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

#include "tma_conn_service_ids.hpp"
#include "tma_conn_packet.hpp"
#include "tma_task.hpp"
#include "tma_service.hpp"
#include "tma_task_list.hpp"
#include "tma_conn_connection.hpp"

enum class TmaWorkType : u32 {
    None,
    NewTask,
    FreeTask,
    ReceivePacket,
    Tick,
    Disconnect,
    Sleep,
};

struct TmaWorkItem {
    TmaTask *task;
    TmaPacket *packet;
    TmaWorkType work_type;
};

class TmaServiceManager {
    public:
        static constexpr size_t PacketQueueDepth = 0x8;
        static constexpr size_t WorkQueueDepth = 0x80;
    private:
        HosMutex lock;
        bool initialized = false;
        TmaTaskList task_list;
        HosThread work_thread;
        std::vector<TmaService *> services;
        TmaConnection *connection = nullptr;
        u32 next_task_id = 0;
        
        /* Work queues. */
        HosMessageQueue free_send_packet_queue = HosMessageQueue(PacketQueueDepth);
        HosMessageQueue free_recv_packet_queue = HosMessageQueue(PacketQueueDepth);
        HosMessageQueue work_queue = HosMessageQueue(WorkQueueDepth);
        HosMessageQueue free_work_queue = HosMessageQueue(WorkQueueDepth);
        
        /* Sleep management. */
        HosSignal disconnect_signal;
        HosSignal wake_signal;
        HosSignal sleep_signal;
        bool asleep = false;
    private:
        static void WorkThread(void *arg);
        void AddWork(TmaWorkType type, TmaTask *task, TmaPacket *packet);
        void HandleNewTaskWork(TmaWorkItem *work_item);
        void HandleFreeTaskWork(TmaWorkItem *work_item);
        void HandleReceivePacketWork(TmaWorkItem *work_item);
        void HandleTickWork();
        void HandleDisconnectWork();
        void HandleSleepWork();
        
        void SetAsleep(bool s) { this->asleep = s; }
    public:
        TmaServiceManager();
        virtual ~TmaServiceManager();
        void Initialize();
        void Finalize();
        
        /* Packet management. */
        TmaConnResult SendPacket(TmaPacket *packet);
        void OnReceivePacket(TmaPacket *packet);
        TmaPacket *AllocateSendPacket();
        TmaPacket *AllocateRecvPacket();
        void FreePacket(TmaPacket *packet);
        
        /* Service/task management. */
        TmaService *GetServiceById(TmaServiceId id);
        void AddService(TmaService *service);
        void AddTask(TmaTask *task, TmaPacket *packet);
        void FreeTask(TmaTask *task);
        void CancelTask(u32 task_id);
        void CancelTasks();
        u32 GetNextTaskId();
        
        /* Connection management. */
        void Tick();
        void SetConnection(TmaConnection *conn);
        void OnDisconnect();
        void Sleep();
        void Wake(TmaConnection *conn);
        bool GetAsleep() const { return this->asleep; }
        bool GetConnected() const;
};
