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
#include "htcfs_result.hpp"
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
        /* Get the maximum supported protocol on the host side. */
        s16 max_host_protocol;
        R_TRY(this->GetMaxProtocolVersion(std::addressof(max_host_protocol)));

        /* Verify that the host protocol is >= 0. */
        R_UNLESS(max_host_protocol >= 0, htcfs::ResultUnsupportedProtocolVersion());

        /* Inform the host what protocol we're using. */
        const auto use_version = std::min(MaxProtocolVersion, max_host_protocol);
        R_TRY(this->SetProtocolVersion(use_version));

        /* Set the version in our header factory. */
        m_header_factory.SetVersion(use_version);
        return ResultSuccess();
    }

    void ClientImpl::TearDownProtocol() {
        /* Set the header factory version to zero. */
        m_header_factory.SetVersion(0);
    }

    Result ClientImpl::CheckResponseHeaderWithoutVersion(const Header &response, PacketType packet_type) {
        /* Check the protocol. */
        R_UNLESS(response.protocol == HtcfsProtocol, htcfs::ResultUnexpectedResponseProtocolId());

        /* Check the packet category. */
        R_UNLESS(response.packet_category == PacketCategory::Response, htcfs::ResultUnexpectedResponsePacketCategory());

        /* Check the type. */
        R_UNLESS(response.packet_type == packet_type, htcfs::ResultUnexpectedResponsePacketType());

        return ResultSuccess();
    }

    Result ClientImpl::GetMaxProtocolVersion(s16 *out) {
        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeGetMaxProtocolVersionHeader(std::addressof(request));

        /* Send the request to the host. */
        R_TRY(this->SendToRpcChannel(std::addressof(request), sizeof(request)));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeaderWithoutVersion(response, request.packet_type));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Set the maximum protocol version. */
        *out = response.params[1];

        return ResultSuccess();
    }

    Result ClientImpl::SetProtocolVersion(s16 version) {
        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeSetProtocolVersionHeader(std::addressof(request), version);

        /* Send the request to the host. */
        R_TRY(this->SendToRpcChannel(std::addressof(request), sizeof(request)));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeaderWithoutVersion(response, request.packet_type));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        return ResultSuccess();
    }

    Result ClientImpl::SendToRpcChannel(const void *src, s64 size) {
        return this->SendToHtclow(src, size, std::addressof(m_rpc_channel));
    }

    Result ClientImpl::ReceiveFromRpcChannel(void *dst, s64 size) {
        return this->ReceiveFromHtclow(dst, size, std::addressof(m_rpc_channel));
    }

    Result ClientImpl::SendToHtclow(const void *src, s64 size, htclow::Channel *channel) {
        /* Check size. */
        R_UNLESS(size >= 0, htcfs::ResultInvalidArgument());

        /* Iteratively send. */
        s64 sent;
        for (s64 total = 0; total < size; total += sent) {
            /* Send the current batch of data. */
            R_TRY(channel->Send(std::addressof(sent), static_cast<const u8 *>(src) + total, size - total));

            /* Check that we sent the right amount. */
            R_UNLESS(sent == size - total, htcfs::ResultHtclowChannelClosed());
        }

        /* Flush. */
        R_TRY(channel->Flush());

        return ResultSuccess();
    }

    Result ClientImpl::ReceiveFromHtclow(void *dst, s64 size, htclow::Channel *channel) {
        /* Check size. */
        R_UNLESS(size >= 0, htcfs::ResultInvalidArgument());

        /* Iteratively receive. */
        s64 received;
        for (s64 total = 0; total < size; total += received) {
            /* Receive the current batch of data. */
            R_TRY(channel->Receive(std::addressof(received), static_cast<u8 *>(dst) + total, size - total, htclow::ReceiveOption_ReceiveAllData));

            /* Check that we received the right amount. */
            R_UNLESS(received == size - total, htcfs::ResultHtclowChannelClosed());
        }

        /* Flush. */
        R_TRY(channel->Flush());

        return ResultSuccess();
    }

}
