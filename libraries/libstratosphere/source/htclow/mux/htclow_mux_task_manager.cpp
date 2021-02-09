/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "htclow_mux_task_manager.hpp"

namespace ams::htclow::mux {

    os::EventType *TaskManager::GetTaskEvent(u32 task_id) {
        /* Check pre-conditions. */
        AMS_ASSERT(0 <= task_id && task_id < MaxTaskCount);
        AMS_ASSERT(m_valid[task_id]);

        return std::addressof(m_tasks[task_id].event);
    }

    void TaskManager::NotifyDisconnect(impl::ChannelInternalType channel) {
        for (auto i = 0; i < MaxTaskCount; ++i) {
            if (m_valid[i] && m_tasks[i].channel == channel) {
                this->CompleteTask(i, EventTrigger_Disconnect);
            }
        }
    }

    void TaskManager::NotifyReceiveData(impl::ChannelInternalType channel, size_t size) {
        for (auto i = 0; i < MaxTaskCount; ++i) {
            if (m_valid[i] && m_tasks[i].channel == channel && m_tasks[i].size <= size) {
                this->CompleteTask(i, EventTrigger_ReceiveData);
            }
        }
    }

    void TaskManager::NotifySendReady() {
        for (auto i = 0; i < MaxTaskCount; ++i) {
            if (m_valid[i] && m_tasks[i].type == TaskType_Send) {
                this->CompleteTask(i, EventTrigger_SendReady);
            }
        }
    }

    void TaskManager::NotifySendBufferEmpty(impl::ChannelInternalType channel) {
        for (auto i = 0; i < MaxTaskCount; ++i) {
            if (m_valid[i] && m_tasks[i].channel == channel && m_tasks[i].type == TaskType_Flush) {
                this->CompleteTask(i, EventTrigger_SendBufferEmpty);
            }
        }
    }

    void TaskManager::NotifyConnectReady() {
        for (auto i = 0; i < MaxTaskCount; ++i) {
            if (m_valid[i] && m_tasks[i].type == TaskType_Connect) {
                this->CompleteTask(i, EventTrigger_ConnectReady);
            }
        }
    }

    void TaskManager::CompleteTask(int index, EventTrigger trigger) {
        /* Check pre-conditions. */
        AMS_ASSERT(0 <= index && index < MaxTaskCount);
        AMS_ASSERT(m_valid[index]);

        /* Complete the task. */
        if (!m_tasks[index].has_event_trigger) {
            m_tasks[index].has_event_trigger = true;
            m_tasks[index].event_trigger     = trigger;
            os::SignalEvent(std::addressof(m_tasks[index].event));
        }
    }

}
