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
#include "htc_rpc_client.hpp"

namespace ams::htc::server::rpc {

    namespace {

        constexpr inline size_t ThreadStackSize = os::MemoryPageSize;

        alignas(os::ThreadStackAlignment) constinit u8 g_receive_thread_stack[ThreadStackSize];
        alignas(os::ThreadStackAlignment) constinit u8 g_send_thread_stack[ThreadStackSize];

        constinit os::SdkMutex g_rpc_mutex;

        constinit RpcTaskIdFreeList g_task_id_free_list;
        constinit RpcTaskTable g_task_table;

    }

    RpcClient::RpcClient(driver::IDriver *driver, htclow::ChannelId channel)
        : m_00(0),
          m_driver(driver),
          m_channel_id(channel),
          m_receive_thread_stack(g_receive_thread_stack),
          m_send_thread_stack(g_send_thread_stack),
          m_mutex(g_rpc_mutex),
          m_task_id_free_list(g_task_id_free_list),
          m_task_table(g_task_table),
          m_task_active(),
          m_task_queue(),
          m_cancelled(false),
          m_thread_running(false)
    {
        /* Initialize all events. */
        for (size_t i = 0; i < MaxRpcCount; ++i) {
            os::InitializeEvent(std::addressof(m_receive_buffer_available_events[i]), false, os::EventClearMode_AutoClear);
            os::InitializeEvent(std::addressof(m_send_buffer_available_events[i]), false, os::EventClearMode_AutoClear);
        }
    }

}
