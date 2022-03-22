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
#pragma once
#include <stratosphere.hpp>

namespace ams::htclow::mux {

    constexpr inline int MaxTaskCount = 0x80;

    enum EventTrigger : u8 {
        EventTrigger_Disconnect      =  1,
        EventTrigger_ReceiveData     =  2,
        EventTrigger_SendComplete    =  4,
        EventTrigger_SendReady       =  5,
        EventTrigger_SendBufferEmpty = 10,
        EventTrigger_ConnectReady    = 11,
    };

    class TaskManager {
        private:
            enum TaskType : u8 {
                TaskType_Receive = 0,
                TaskType_Send    = 1,
                TaskType_Flush   = 6,
                TaskType_Connect = 7,
            };

            struct Task {
                impl::ChannelInternalType channel;
                os::EventType event;
                bool has_event_trigger;
                EventTrigger event_trigger;
                TaskType type;
                size_t size;
            };
        private:
            bool m_valid[MaxTaskCount];
            Task m_tasks[MaxTaskCount];
        public:
            TaskManager() : m_valid() { /* ... */ }

            Result AllocateTask(u32 *out_task_id, impl::ChannelInternalType channel);
            void FreeTask(u32 task_id);

            os::EventType *GetTaskEvent(u32 task_id);
            EventTrigger GetTrigger(u32 task_id);

            void ConfigureConnectTask(u32 task_id);
            void ConfigureFlushTask(u32 task_id);
            void ConfigureReceiveTask(u32 task_id, size_t size);
            void ConfigureSendTask(u32 task_id);

            void NotifyDisconnect(impl::ChannelInternalType channel);
            void NotifyReceiveData(impl::ChannelInternalType channel, size_t size);
            void NotifySendReady();
            void NotifySendBufferEmpty(impl::ChannelInternalType channel);
            void NotifyConnectReady();

            void CompleteTask(int index, EventTrigger trigger);
    };

}
