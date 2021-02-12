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
#include "htcfs_client_impl.hpp"
#include "../htclow/htclow_default_channel_config.hpp"

namespace ams::htcfs {

    namespace {

        /* TODO: Move to a header? */
        constexpr u16 RpcChannelId = 0;

        alignas(os::ThreadStackAlignment) constinit u8 g_monitor_thread_stack[os::MemoryPageSize];

        constinit u8 g_cache[32_KB];

    }

    ClientImpl::ClientImpl(htclow::HtclowManager *manager)
        : m_htclow_manager(manager),
          m_cache_manager(g_cache, sizeof(g_cache)),
          m_header_factory(),
          m_mutex(),
          m_module(htclow::ModuleId::Htcfs),
          m_rpc_channel(manager),
          m_data_channel(manager),
          m_connected(false),
          m_event(os::EventClearMode_ManualClear)
    {
        /* Start our thread. */
        this->Start();
    }

    void ClientImpl::Start() {
        /* Create our thread. */
        os::CreateThread(std::addressof(m_monitor_thread), ThreadEntry, this, g_monitor_thread_stack, sizeof(g_monitor_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcfsMonitor));

        /* Set thread name pointer. */
        os::SetThreadNamePointer(std::addressof(m_monitor_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtcfsMonitor));

        /* Start our thread. */
        os::StartThread(std::addressof(m_monitor_thread));
    }

    void ClientImpl::Cancel() {
        /* Signal our event. */
        m_event.Signal();
    }

    void ClientImpl::Wait() {
        /* Wait for and destroy our thread. */
        os::WaitThread(std::addressof(m_monitor_thread));
        os::DestroyThread(std::addressof(m_monitor_thread));
    }

    void ClientImpl::ThreadBody() {
        /* Loop forever, until we're cancelled. */
        while (!m_event.TryWait()) {
            /* Lock ourselves. */
            std::scoped_lock lk(m_mutex);

            /* Open our channel. */
            R_ABORT_UNLESS(m_rpc_channel.Open(std::addressof(m_module), RpcChannelId));

            /* Ensure that we clean up our channel when we're done. */
            ON_SCOPE_EXIT {
                m_rpc_channel.Close();
                m_cache_manager.Invalidate();
            };

            /* Set our channel config and buffers. */
            m_rpc_channel.SetConfig(htclow::DefaultChannelConfig);
            m_rpc_channel.SetReceiveBuffer(m_receive_buffer, sizeof(m_receive_buffer));
            m_rpc_channel.SetSendBuffer(m_send_buffer, sizeof(m_send_buffer));

            /* Wait for our channel state to be connectable. */
            if (this->WaitAny(htclow::ChannelState_Connectable, m_event.GetBase()) != 0) {
                return;
            }

            /* Ensure that when we're done, we reset our connected status. */
            ON_SCOPE_EXIT { m_connected = false; };

            /* Try to connect. */
            const Result conn_result = m_rpc_channel.Connect();
            if (R_FAILED(conn_result)) {
                /* DEBUG */
                R_ABORT_UNLESS(conn_result);
                continue;
            }

            /* Ensure that we manage our connection correctly. */
            auto conn_guard = SCOPE_GUARD { m_rpc_channel.Shutdown(); };

            /* Try to set up the protocol. */
            const Result setup_result = this->SetUpProtocol();
            if (R_FAILED(setup_result)) {
                R_ABORT_UNLESS(setup_result);
                continue;
            }

            /* We're properly connected now. */
            m_connected = true;
            conn_guard.Cancel();

            /* Tear down the protocol when we're done processing. */
            ON_SCOPE_EXIT { this->TearDownProtocol(); };

            /* Wait to become disconnected. */
            if (this->WaitAny(htclow::ChannelState_Disconnected, m_event.GetBase()) != 0) {
                break;
            }
        }
    }

    int ClientImpl::WaitAny(htclow::ChannelState state, os::EventType *event) {
        /* Wait. */
        int idx = 0;
        while (m_rpc_channel.GetChannelState() != state) {
            /* Get the channel state event. */
            os::EventType *channel_state_event = m_rpc_channel.GetChannelStateEvent();

            /* Perform wait with lock temporarily not held. */
            {
                m_mutex.Unlock();
                idx = os::WaitAny(channel_state_event, event);
                m_mutex.Lock();
            }

            /* If we're cancel-signalled, we're done. */
            if (idx != 0) {
                break;
            }

            /* Clear the channel state event. */
            os::ClearEvent(channel_state_event);
        }
        return idx;
    }

    Result ClientImpl::SetUpProtocol() {
        /* TODO: Actual client <-> host RPC here. */
        m_header_factory.SetVersion(1);
        return ResultSuccess();
    }

    void ClientImpl::TearDownProtocol() {
        /* Set the header factory version to zero. */
        m_header_factory.SetVersion(0);
    }

}
