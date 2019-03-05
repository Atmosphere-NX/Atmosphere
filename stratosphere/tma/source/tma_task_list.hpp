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
#include <vector>

#include "tma_conn_service_ids.hpp"
#include "tma_task.hpp"

class TmaTaskList {
    private:
        mutable HosMutex lock;
        std::vector<TmaTask *> tasks[TmaTask::NumPriorities];
        std::vector<TmaTask *> sleeping_tasks[TmaTask::NumPriorities];
    private:
        void Remove(TmaTask *task);
        TmaTask *GetById(u32 task_id) const;
    public:
        TmaTaskList() { }
        virtual ~TmaTaskList() { }
        
        u32 GetNumTasks() const;
        u32 GetNumSleepingTasks() const;
        bool IsIdFree(u32 task_id) const;
        
        bool SendPacket(bool connected, TmaPacket *packet);
        bool ReceivePacket(TmaPacket *packet);
        void CleanupDoneTasks();
        void Add(TmaTask *task);
        void Cancel(u32 task_id);
        void CancelAll();
        
        void Sleep();
        void Wake();
};