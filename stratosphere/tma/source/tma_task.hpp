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

enum class TmaTaskState : u32 {
    InProgress,
    Complete,
    Canceled,
};

class TmaServiceManager;

class TmaTask {
    public:
        static constexpr u32 MaxPriority = 15;
        static constexpr u32 NumPriorities = MaxPriority + 1;
    protected:
        TmaServiceManager *manager;
        u32 priority = 0;
        TmaServiceId service_id = TmaServiceId::Invalid;
        u32 task_id = 0;
        u32 command = 0;
        TmaTaskState state = TmaTaskState::InProgress;
        HosSignal signal;
        bool owned_by_task_list = true;
        bool sleep_allowed = true;
    public:
        TmaTask(TmaServiceManager *m) : manager(m) { }
        virtual ~TmaTask() { }
        
        u32 GetPriority() const { return this->priority; }
        TmaServiceId GetServiceId() const { return this->service_id; }
        u32 GetTaskId() const { return this->task_id; }
        u32 GetCommand() const { return this->command; }
        TmaTaskState GetState() const { return this->state; }
        bool GetOwnedByTaskList() const { return this->owned_by_task_list; }
        bool GetSleepAllowed() const { return this->sleep_allowed; }
        
        void SetPriority(u32 p) { this->priority = p; }
        void SetServiceId(TmaServiceId s) { this->service_id = s; }
        void SetTaskId(u32 i) { this->task_id = i; }
        void SetCommand(u32 c) { this->command = c; }
        void SetOwnedByTaskList(bool o) { this->owned_by_task_list = o; }
        void SetSleepAllowed(bool a) { this->sleep_allowed = a; }
        
        void Signal() { this->signal.Signal(); }
        void ResetSignal() { this->signal.Reset(); }
        
        void Complete();
        void Cancel();
        
        virtual void OnStart(TmaPacket *packet) = 0;
        virtual void OnReceivePacket(TmaPacket *packet) = 0;
        virtual void OnSendPacket(TmaPacket *packet) = 0;
};
