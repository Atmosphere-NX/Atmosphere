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
#include <algorithm>

#include <switch.h>
#include <stratosphere.hpp>
#include "tma_task_list.hpp"

TmaTask *TmaTaskList::GetById(u32 task_id) const {
    for (u32 i = 0 ; i < TmaTask::NumPriorities; i++) {
        for (auto task : this->tasks[i]) {
            if (task->GetTaskId() == task_id) {
                return task;
            }
        }
    }
    return nullptr;
}

u32 TmaTaskList::GetNumTasks() const {
    std::scoped_lock<HosMutex> lk(this->lock);
    u32 count = 0;
    
    for (u32 i = 0 ; i < TmaTask::NumPriorities; i++) {
        count += this->tasks[i].size();
    }
    
    return count;
}

u32 TmaTaskList::GetNumSleepingTasks() const {
    std::scoped_lock<HosMutex> lk(this->lock);
    u32 count = 0;
    
    for (u32 i = 0 ; i < TmaTask::NumPriorities; i++) {
        count += this->sleeping_tasks[i].size();
    }
    
    return count;
}

bool TmaTaskList::IsIdFree(u32 task_id) const {
    std::scoped_lock<HosMutex> lk(this->lock);
    return GetById(task_id) == nullptr;
}

bool TmaTaskList::SendPacket(bool connected, TmaPacket *packet) {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    TmaTask *target_task = nullptr;
    
    /* This loop both finds a target task, and cleans up finished tasks. */
    for (u32 i = 0; i < TmaTask::NumPriorities; i++) {
        auto it = this->tasks[i].begin();
        while (it != this->tasks[i].end()) {
            auto task = *it;
            switch (task->GetState()) {
                case TmaTaskState::InProgress:
                    it++;
                    if (target_task == nullptr && task->GetNeedsPackets()) {
                        if (connected || IsMetaService(task->GetServiceId())) {
                            target_task = task;
                        }
                    }
                    break;
                case TmaTaskState::Complete:
                case TmaTaskState::Canceled:
                    it = this->tasks[i].erase(it);
                    if (task->GetOwnedByTaskList()) {
                        delete task;
                    } else {
                        task->Signal();
                    }
                    break;
                default:
                    /* TODO: Panic to fatal? */
                    std::abort();
            }
        }
    }
    
    if (target_task) {
        /* Setup packet. */
        packet->SetContinuation(true);
        packet->SetServiceId(target_task->GetServiceId());
        packet->SetTaskId(target_task->GetTaskId());
        packet->SetCommand(target_task->GetCommand());
        packet->ClearOffset();
        
        /* Actually handle packet send. */
        target_task->OnSendPacket(packet);
    }
    
    return target_task != nullptr;
}

bool TmaTaskList::ReceivePacket(TmaPacket *packet) {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    auto task = this->GetById(packet->GetTaskId());
    if (task != nullptr) {
        task->OnReceivePacket(packet);
    }
    return task != nullptr;
}


void TmaTaskList::CleanupDoneTasks() {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    /* Clean up all tasks in Complete/Canceled state. */
    for (u32 i = 0; i < TmaTask::NumPriorities; i++) {
        auto it = this->tasks[i].begin();
        while (it != this->tasks[i].end()) {
            auto task = *it;
            switch (task->GetState()) {
                case TmaTaskState::InProgress:
                    it++;
                    break;
                case TmaTaskState::Complete:
                case TmaTaskState::Canceled:
                    it = this->tasks[i].erase(it);
                    if (task->GetOwnedByTaskList()) {
                        delete task;
                    } else {
                        task->Signal();
                    }
                    break;
                default:
                    /* TODO: Panic to fatal? */
                    std::abort();
            }
        }
    }
}

void TmaTaskList::Add(TmaTask *task) {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    this->tasks[task->GetPriority()].push_back(task);
}

void TmaTaskList::Remove(TmaTask *task) {    
    const auto priority = task->GetPriority();
    
    /* Nintendo iterates over all lists instead of just the correct one. */
    /* TODO: Is there actually any reason to do that? */
    auto ind = std::find(this->tasks[priority].begin(), this->tasks[priority].end(), task);
    if (ind != this->tasks[priority].end()) {
        this->tasks[priority].erase(ind);
        return;
    }
    
    /* TODO: Panic to fatal? */
    std::abort();
}

void TmaTaskList::Cancel(u32 task_id) {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    auto task = this->GetById(task_id);
    if (task != nullptr) {
        task->Cancel();
    }
}

void TmaTaskList::CancelAll() {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    for (u32 i = 0 ; i < TmaTask::NumPriorities; i++) {
        for (auto task : this->tasks[i]) {
            task->Cancel();
        }
    }
}

void TmaTaskList::Sleep() {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    for (u32 i = 0; i < TmaTask::NumPriorities; i++) {
        auto it = this->tasks[i].begin();
        while (it != this->tasks[i].end()) {
            auto task = *it;
            if (task->GetSleepAllowed()) {
                it = this->tasks[i].erase(it);
                this->sleeping_tasks[i].push_back(task);
            } else {
                it++;
            }
        }
    }
}

void TmaTaskList::Wake() {
    std::scoped_lock<HosMutex> lk(this->lock);
    
    for (u32 i = 0; i < TmaTask::NumPriorities; i++) {
        auto it = this->sleeping_tasks[i].begin();
        while (it != this->sleeping_tasks[i].end()) {
            auto task = *it;
            it = this->sleeping_tasks[i].erase(it);
            this->tasks[i].push_back(task);
        }
    }
}
