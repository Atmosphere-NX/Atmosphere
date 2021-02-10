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

    }

    RpcClient::RpcClient(driver::IDriver *driver, htclow::ChannelId channel)
        : m_00(0),
          m_driver(driver),
          m_channel_id(channel),
          m_receive_thread_stack(g_receive_thread_stack),
          m_send_thread_stack(g_send_thread_stack),
          m_mutex(g_rpc_mutex),
          m_cancelled(false),
          m_thread_running(false)
    {
        /* Initialize all events. */
        /* TODO: MaxTaskCount? */
        for (size_t i = 0; i < util::size(m_5F8_events); ++i) {
            os::InitializeEvent(std::addressof(m_5F8_events[i]), false, os::EventClearMode_AutoClear);
            os::InitializeEvent(std::addressof(m_1138_events[i]), false, os::EventClearMode_AutoClear);
        }

        /* TODO: Clear all of m_3C0 array to zero. */
    }

}
