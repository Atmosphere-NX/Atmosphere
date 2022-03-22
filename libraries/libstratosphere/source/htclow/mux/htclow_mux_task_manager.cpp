/*
 * Copyright (c) Atmosphère-NX
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

    EventTrigger TaskManager::GetTrigger(u32 task_id) {
        /* Check pre-conditions. */
        AMS_ASSERT(0 <= task_id && task_id < MaxTaskCount);
        AMS_ASSERT(m_valid[task_id]);

        return m_tasks[task_id].event_trigger;
    }

    Result TaskManager::AllocateTask(u32 *out_task_id, impl::ChannelInternalType channel) {
        /* Find a free task. */
        u32 task_id = 0;
        for (task_id = 0; task_id < util::size(m_tasks); ++task_id) {
            if (!m_valid[task_id]) {
                break;
            }
        }

        /* Verify the task is free. */
        R_UNLESS(task_id < util::size(m_tasks), htclow::ResultOutOfTask());

        /* Mark the task as allocated. */
        m_valid[task_id] = true;

        /* Setup the task. */
        m_tasks[task_id].channel           = channel;
        m_tasks[task_id].has_event_trigger = false;
        os::InitializeEvent(std::addressof(m_tasks[task_id].event), false, os::EventClearMode_ManualClear);

        /* Return the task id. */
        *out_task_id = task_id;
        return ResultSuccess();
    }

    void TaskManager::FreeTask(u32 task_id) {
        /* Check pre-conditions. */
        AMS_ASSERT(0 <= task_id && task_id < MaxTaskCount);

        /* Invalidate the task. */
        if (m_valid[task_id]) {
            os::FinalizeEvent(std::addressof(m_tasks[task_id].event));
            m_valid[task_id] = false;
        }
    }

    void TaskManager::ConfigureConnectTask(u32 task_id) {
        /* Check pre-conditions. */
        AMS_ASSERT(0 <= task_id && task_id < MaxTaskCount);
        AMS_ASSERT(m_valid[task_id]);

        /* Set the task type. */
        m_tasks[task_id].type = TaskType_Connect;
    }

    void TaskManager::ConfigureFlushTask(u32 task_id) {
        /* Check pre-conditions. */
        AMS_ASSERT(0 <= task_id && task_id < MaxTaskCount);
        AMS_ASSERT(m_valid[task_id]);

        /* Set the task type. */
        m_tasks[task_id].type = TaskType_Flush;
    }

    void TaskManager::ConfigureReceiveTask(u32 task_id, size_t size) {
        /* Check pre-conditions. */
        AMS_ASSERT(0 <= task_id && task_id < MaxTaskCount);
        AMS_ASSERT(m_valid[task_id]);

        /* Set the task type. */
        m_tasks[task_id].type = TaskType_Receive;

        /* Set the task size. */
        m_tasks[task_id].size = size;
    }

    void TaskManager::ConfigureSendTask(u32 task_id) {
        /* Check pre-conditions. */
        AMS_ASSERT(0 <= task_id && task_id < MaxTaskCount);
        AMS_ASSERT(m_valid[task_id]);

        /* Set the task type. */
        m_tasks[task_id].type = TaskType_Send;
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
            if (m_valid[i] && m_tasks[i].channel == channel && m_tasks[i].type == TaskType_Receive && m_tasks[i].size <= size) {
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
