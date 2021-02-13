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

namespace ams::htcfs {

    namespace {

        /* TODO: Move to a header? */
        constexpr u16 RpcChannelId = 0;

        alignas(os::ThreadStackAlignment) constinit u8 g_monitor_thread_stack[os::MemoryPageSize];

        constinit u8 g_cache[32_KB];

        ALWAYS_INLINE Result ConvertNativeResult(s64 value) {
            return result::impl::MakeResult(value);
        }

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

    Result ClientImpl::CheckResponseHeader(const Header &response, PacketType packet_type) {
        /* Perform base checks. */
        R_TRY(this->CheckResponseHeaderWithoutVersion(response, packet_type));

        /* Check the version. */
        R_UNLESS(response.version == m_header_factory.GetVersion(), htcfs::ResultUnexpectedResponseProtocolVersion());

        return ResultSuccess();
    }

    Result ClientImpl::CheckResponseHeader(const Header &response, PacketType packet_type, s64 body_size) {
        /* Perform base checks. */
        R_TRY(this->CheckResponseHeader(response, packet_type));

        /* Check the body size. */
        R_UNLESS(response.body_size == body_size, htcfs::ResultUnexpectedResponseBodySize());

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

    Result ClientImpl::InitializeRpcChannel() {
        /* Check that we're not cancelled. */
        R_UNLESS(!m_event.TryWait(), htcfs::ResultConnectionFailure());

        /* Check that we're connected. */
        R_UNLESS(m_connected, htcfs::ResultConnectionFailure());

        return ResultSuccess();
    }

    Result ClientImpl::SendRequest(const Header &request, const void *arg1, size_t arg1_size, const void *arg2, size_t arg2_size) {
        /* Try to perform an optimized send. */
        if (sizeof(request) + arg1_size + arg2_size < sizeof(m_packet_buffer)) {
            /* Setup our packet buffer. */
            std::memcpy(m_packet_buffer, std::addressof(request), sizeof(request));
            if (arg1_size > 0) {
                std::memcpy(m_packet_buffer + sizeof(request), arg1, arg1_size);
            }
            if (arg2_size > 0) {
                std::memcpy(m_packet_buffer + sizeof(request) + arg1_size, arg2, arg2_size);
            }

            /* Send the request. */
            R_TRY(this->SendToRpcChannel(m_packet_buffer, sizeof(request) + arg1_size + arg2_size));
        } else {
            /* We can't perform a single optimized send, so perform three separate sends. */
            R_TRY(this->SendToRpcChannel(std::addressof(request), sizeof(request)));

            if (arg1_size > 0) {
                R_TRY(this->SendToRpcChannel(arg1, arg1_size));
            }

            if (arg2_size > 0) {
                R_TRY(this->SendToRpcChannel(arg2, arg2_size));
            }
        }

        return ResultSuccess();
    }

    Result ClientImpl::OpenDirectory(s32 *out_handle, const char *path, fs::OpenDirectoryMode mode, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeOpenDirectoryHeader(std::addressof(request), path_len, mode, case_sensitive);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request, path, path_len));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(response.params[1]));

        /* Set the output handle. */
        *out_handle = static_cast<s32>(response.params[2]);

        return ResultSuccess();
    }

    Result ClientImpl::CloseDirectory(s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeCloseDirectoryHeader(std::addressof(request), handle);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(response.params[1]));

        return ResultSuccess();
    }

    Result ClientImpl::GetEntryCount(s64 *out, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeGetEntryCountHeader(std::addressof(request), handle);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(response.params[1]));

        /* Set the output count. */
        *out = response.params[2];

        return ResultSuccess();
    }

    Result ClientImpl::ReadDirectory(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeReadDirectoryHeader(std::addressof(request), handle, max_out_entries);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(response.params[1]));

        /* Check that the response body size is expected. */
        R_UNLESS(static_cast<size_t>(response.body_size) == max_out_entries * sizeof(*out_entries), htcfs::ResultUnexpectedResponseBody());

        /* Check that the number of entries read is allowable. */
        R_UNLESS(static_cast<size_t>(response.params[2]) <= max_out_entries, htcfs::ResultUnexpectedResponseBody());

        /* Receive the entries. */
        *out = response.params[2];
        return this->ReceiveFromRpcChannel(out_entries, response.body_size);
    }

    Result ClientImpl::ReadDirectoryLarge(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeReadDirectoryLargeHeader(std::addressof(request), handle, max_out_entries);

        AMS_ABORT("TODO: Data channel setup/teardown");
    }

    Result ClientImpl::GetPriorityForDirectory(s32 *out, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeGetPriorityForDirectoryHeader(std::addressof(request), handle);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Set the output. */
        *out = static_cast<s32>(response.params[1]);

        return ResultSuccess();
    }

    Result ClientImpl::SetPriorityForDirectory(s32 priority, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeSetPriorityForDirectoryHeader(std::addressof(request), handle, priority);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        return ResultSuccess();
    }

}
