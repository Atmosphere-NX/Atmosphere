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
#include "htc_htcmisc_rpc_server.hpp"

namespace ams::htc::server::rpc {

    namespace {

        constexpr inline size_t ReceiveThreadStackSize = os::MemoryPageSize;

        alignas(os::ThreadStackAlignment) constinit u8 g_receive_thread_stack[ReceiveThreadStackSize];

    }

    HtcmiscRpcServer::HtcmiscRpcServer(driver::IDriver *driver, htclow::ChannelId channel)
        : m_00(0),
          m_driver(driver),
          m_channel_id(channel),
          m_receive_thread_stack(g_receive_thread_stack),
          m_cancelled(false),
          m_thread_running(false)
    {
        /* ... */
    }

}
