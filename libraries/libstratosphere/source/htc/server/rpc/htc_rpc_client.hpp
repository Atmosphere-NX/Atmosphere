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
#pragma once
#include <stratosphere.hpp>
#include "../driver/htc_i_driver.hpp"
#include "htc_rpc_task_table.hpp"
#include "htc_rpc_task_queue.hpp"
#include "htc_rpc_task_id_free_list.hpp"

namespace ams::htc::server::rpc {

    class RpcClient {
        private:
            /* TODO: where is this value coming from, again? */
            static constexpr size_t BufferSize = 0xE400;
        private:
            u64 m_00;
            driver::IDriver *m_driver;
            htclow::ChannelId m_channel_id;
            void *m_receive_thread_stack;
            void *m_send_thread_stack;
            os::ThreadType m_receive_thread;
            os::ThreadType m_send_thread;
            os::SdkMutex &m_mutex;
            RpcTaskIdFreeList &m_task_id_free_list;
            RpcTaskTable &m_task_table;
            bool m_task_active[MaxRpcCount];
            RpcTaskQueue m_task_queue;
            bool m_cancelled;
            bool m_thread_running;
            os::EventType m_receive_buffer_available_events[MaxRpcCount];
            os::EventType m_send_buffer_available_events[MaxRpcCount];
            char m_receive_buffer[BufferSize];
            char m_send_buffer[BufferSize];
        private:
            static void ReceiveThreadEntry(void *arg) { static_cast<RpcClient *>(arg)->ReceiveThread(); }
            static void SendThreadEntry(void *arg) { static_cast<RpcClient *>(arg)->SendThread(); }

            Result ReceiveThread();
            Result SendThread();
        public:
            RpcClient(driver::IDriver *driver, htclow::ChannelId channel);
        public:
            void Open();
            void Close();

            Result Start();
            void Cancel();
            void Wait();

            int WaitAny(htclow::ChannelState state, os::EventType *event);
        private:
            Result ReceiveHeader(RpcPacket *header);
            Result ReceiveBody(char *dst, size_t size);
            Result SendRequest(const char *src, size_t size);
    };

}
