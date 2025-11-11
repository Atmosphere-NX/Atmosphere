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
#include "htcfs_client_impl.hpp"
#include "htcfs_result.hpp"

namespace ams::htcfs {

    namespace {

        /* TODO: Move to a header? */
        constexpr u16 RpcChannelId = 0;
        constexpr u16 DataChannelId = 1;

        alignas(os::ThreadStackAlignment) constinit u8 g_monitor_thread_stack[os::MemoryPageSize];

        constexpr size_t FileDataCacheSize = 32_KB;
        constinit u8 g_cache[FileDataCacheSize];

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
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_monitor_thread), ThreadEntry, this, g_monitor_thread_stack, sizeof(g_monitor_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtcfsMonitor)));

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
        R_SUCCEED();
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

        R_SUCCEED();
    }

    Result ClientImpl::CheckResponseHeader(const Header &response, PacketType packet_type) {
        /* Perform base checks. */
        R_TRY(this->CheckResponseHeaderWithoutVersion(response, packet_type));

        /* Check the version. */
        R_UNLESS(response.version == m_header_factory.GetVersion(), htcfs::ResultUnexpectedResponseProtocolVersion());

        R_SUCCEED();
    }

    Result ClientImpl::CheckResponseHeader(const Header &response, PacketType packet_type, s64 body_size) {
        /* Perform base checks. */
        R_TRY(this->CheckResponseHeader(response, packet_type));

        /* Check the body size. */
        R_UNLESS(response.body_size == body_size, htcfs::ResultUnexpectedResponseBodySize());

        R_SUCCEED();
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

        R_SUCCEED();
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

        R_SUCCEED();
    }

    Result ClientImpl::SendToRpcChannel(const void *src, s64 size) {
        R_RETURN(this->SendToHtclow(src, size, std::addressof(m_rpc_channel)));
    }

    Result ClientImpl::ReceiveFromRpcChannel(void *dst, s64 size) {
        R_RETURN(this->ReceiveFromHtclow(dst, size, std::addressof(m_rpc_channel)));
    }

    Result ClientImpl::ReceiveFromDataChannel(s64 size) {
        R_RETURN(m_data_channel.WaitReceive(size));
    }

    Result ClientImpl::SendToDataChannel() {
        R_RETURN(m_data_channel.Flush());
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

        R_SUCCEED();
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

        R_SUCCEED();
    }

    Result ClientImpl::InitializeRpcChannel() {
        /* Check that we're not cancelled. */
        R_UNLESS(!m_event.TryWait(), htcfs::ResultConnectionFailure());

        /* Check that we're connected. */
        R_UNLESS(m_connected, htcfs::ResultConnectionFailure());

        R_SUCCEED();
    }

    void ClientImpl::InitializeDataChannelForReceive(void *dst, size_t size) {
        /* Open the data channel. */
        R_ABORT_UNLESS(m_data_channel.Open(std::addressof(m_module), DataChannelId));

        /* Set our config. */
        constexpr htclow::ChannelConfig BulkReceiveConfig = {
            .flow_control_enabled = false,
            .handshake_enabled    = false,
            .max_packet_size      = 0x3E000,
        };
        m_data_channel.SetConfig(BulkReceiveConfig);

        /* Set receive buffer. */
        m_data_channel.SetReceiveBuffer(dst, size);

        /* Connect. */
        R_ABORT_UNLESS(m_data_channel.Connect());
    }

    void ClientImpl::InitializeDataChannelForSend(const void *src, size_t size) {
        /* Open the data channel. */
        R_ABORT_UNLESS(m_data_channel.Open(std::addressof(m_module), DataChannelId));

        /* Check that the size is valid. */
        AMS_ASSERT(util::IsIntValueRepresentable<s64>(size));

        /* Set our config. */
        constexpr htclow::ChannelConfig BulkSendConfig = {
            .flow_control_enabled = false,
            .handshake_enabled    = false,
            .max_packet_size      = 0xE020,
        };
        m_data_channel.SetConfig(BulkSendConfig);

        /* Set our send buffer. */
        m_data_channel.SetSendBufferWithData(src, size);

        /* Connect. */
        R_ABORT_UNLESS(m_data_channel.Connect());
    }

    void ClientImpl::FinalizeDataChannel() {
        /* Close our data channel. */
        m_data_channel.Close();
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

        R_SUCCEED();
    }

    Result ClientImpl::OpenFile(s32 *out_handle, const char *path, fs::OpenMode mode, bool case_sensitive) {
        /* Invalidate the cache manager. */
        m_cache_manager.Invalidate();

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeOpenFileHeader(std::addressof(request), path_len, mode, case_sensitive, FileDataCacheSize);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request, path, path_len));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type));

        /* Check the response body size. */
        R_UNLESS(response.body_size >= 0,                                      htcfs::ResultUnexpectedResponseBodySize());
        R_UNLESS(static_cast<size_t>(response.body_size) <= MaxPacketBodySize, htcfs::ResultUnexpectedResponseBodySize());

        /* Receive the response body. */
        if (response.body_size > 0) {
            R_TRY(this->ReceiveFromRpcChannel(m_packet_buffer, response.body_size));
        }

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(response.params[1]));

        /* Set our output handle. */
        *out_handle = response.params[2];

        /* If we have data to cache, cache it. */
        if (response.params[3]) {
            m_cache_manager.Record(response.params[4], m_packet_buffer, response.params[2], response.body_size);
        }

        R_SUCCEED();
    }

    Result ClientImpl::FileExists(bool *out, const char *path, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeFileExistsHeader(std::addressof(request), path_len, case_sensitive);

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

        /* Set the output. */
        *out = response.params[2] != 0;

        R_SUCCEED();
    }

    Result ClientImpl::DeleteFile(const char *path, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeDeleteFileHeader(std::addressof(request), path_len, case_sensitive);

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

        R_SUCCEED();
    }

    Result ClientImpl::RenameFile(const char *old_path, const char *new_path, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto old_path_len = std::strlen(new_path);
        const auto new_path_len = std::strlen(old_path);
        m_header_factory.MakeRenameFileHeader(std::addressof(request), old_path_len, new_path_len, case_sensitive);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request, old_path, old_path_len, new_path, new_path_len));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(response.params[1]));

        R_SUCCEED();
    }

    Result ClientImpl::GetEntryType(fs::DirectoryEntryType *out, const char *path, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeGetEntryTypeHeader(std::addressof(request), path_len, case_sensitive);

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

        /* Set the output. */
        *out = static_cast<fs::DirectoryEntryType>(response.params[2]);

        R_SUCCEED();
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

        R_SUCCEED();
    }

    Result ClientImpl::DirectoryExists(bool *out, const char *path, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeDirectoryExistsHeader(std::addressof(request), path_len, case_sensitive);

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

        /* Set the output. */
        *out = response.params[2] != 0;

        R_SUCCEED();
    }

    Result ClientImpl::CreateDirectory(const char *path, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeCreateDirectoryHeader(std::addressof(request), path_len, case_sensitive);

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

        R_SUCCEED();
    }

    Result ClientImpl::DeleteDirectory(const char *path, bool recursively, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeDeleteDirectoryHeader(std::addressof(request), path_len, recursively, case_sensitive);

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

        R_SUCCEED();
    }

    Result ClientImpl::RenameDirectory(const char *old_path, const char *new_path, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto old_path_len = std::strlen(new_path);
        const auto new_path_len = std::strlen(old_path);
        m_header_factory.MakeRenameDirectoryHeader(std::addressof(request), old_path_len, new_path_len, case_sensitive);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request, old_path, old_path_len, new_path, new_path_len));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(response.params[1]));

        R_SUCCEED();
    }

    Result ClientImpl::CreateFile(const char *path, s64 size, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeCreateFileHeader(std::addressof(request), path_len, size, case_sensitive);

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

        R_SUCCEED();
    }

    Result ClientImpl::GetFileTimeStamp(u64 *out_create, u64 *out_access, u64 *out_modify, const char *path, bool case_sensitive) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeGetFileTimeStampHeader(std::addressof(request), path_len, case_sensitive);

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

        /* Set output. */
        *out_create = static_cast<u64>(response.params[2]);
        *out_access = static_cast<u64>(response.params[3]);
        *out_modify = static_cast<u64>(response.params[4]);

        R_SUCCEED();
    }

    Result ClientImpl::GetCaseSensitivePath(char *dst, size_t dst_size, const char *path) {
        /* Sanity check the output buffer. */
        R_UNLESS(util::IsIntValueRepresentable<s64>(dst_size), htcfs::ResultInvalidArgument());
        R_UNLESS(dst_size > 0,                                 htcfs::ResultInvalidArgument());

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeGetCaseSensitivePathHeader(std::addressof(request), path_len);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request, path, path_len));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type));

        /* Check that we succeeded. */
        const auto htcfs_result = ConvertHtcfsResult(response.params[0]);
        if (R_FAILED(htcfs_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(htcfs_result);
        }

        /* Check our operation's result. */
        const auto native_result = ConvertNativeResult(response.params[1]);
        if (R_FAILED(native_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(native_result);
        }

        /* Check the body size. */
        R_UNLESS(response.body_size < static_cast<s64>(dst_size), htcfs::ResultUnexpectedResponseBodySize());

        /* Receive the response body. */
        R_TRY(this->ReceiveFromRpcChannel(dst, response.body_size));

        /* Null-terminate the output path. */
        dst[response.body_size] = '\x00';

        R_SUCCEED();
    }

    Result ClientImpl::GetDiskFreeSpace(s64 *out_free, s64 *out_total, s64 *out_total_free, const char *path) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        const auto path_len = std::strlen(path);
        m_header_factory.MakeGetDiskFreeSpaceHeader(std::addressof(request), path_len);

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

        /* Set output. */
        *out_free       = response.params[2];
        *out_total      = response.params[3];
        *out_total_free = response.params[4];

        R_SUCCEED();
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

        R_SUCCEED();
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

        R_SUCCEED();
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
        R_RETURN(this->ReceiveFromRpcChannel(out_entries, response.body_size));
    }

    Result ClientImpl::ReadDirectoryLarge(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Setup data channel. */
        const bool use_data_channel = max_out_entries > 0;
        if (use_data_channel) {
            this->InitializeDataChannelForReceive(out_entries, max_out_entries * sizeof(*out_entries));
        }
        ON_SCOPE_EXIT { if (use_data_channel) { this->FinalizeDataChannel(); } };

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeReadDirectoryLargeHeader(std::addressof(request), handle, max_out_entries, DataChannelId);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        const auto htcfs_result = ConvertHtcfsResult(response.params[0]);
        if (R_FAILED(htcfs_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(htcfs_result);
        }

        /* Check our operation's result. */
        const auto native_result = ConvertNativeResult(response.params[1]);
        if (R_FAILED(native_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(native_result);
        }

        /* Check that the number of entries read is allowable. */
        R_UNLESS(static_cast<size_t>(response.params[2]) <= max_out_entries, htcfs::ResultUnexpectedResponseBody());

        /* Read the entries, if there are any. */
        if (response.params[2] > 0) {
            R_TRY(this->ReceiveFromDataChannel(response.params[2] * sizeof(*out_entries)));
        }

        /* Set the number of output entries. */
        *out = response.params[2];

        R_SUCCEED();
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

        R_SUCCEED();
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

        R_SUCCEED();
    }

    Result ClientImpl::CloseFile(s32 handle) {
        /* Invalidate the cache. */
        m_cache_manager.Invalidate(handle);

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeCloseFileHeader(std::addressof(request), handle);

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

        R_SUCCEED();
    }

    Result ClientImpl::ReadFile(s64 *out, void *buffer, s32 handle, s64 offset, s64 buffer_size, fs::ReadOption option) {
        AMS_UNUSED(option);

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Try to read from our cache. */
        if (util::IsIntValueRepresentable<size_t>(offset) && util::IsIntValueRepresentable<size_t>(buffer_size)) {
            size_t read_size;
            if (m_cache_manager.ReadFile(std::addressof(read_size), buffer, handle, static_cast<size_t>(offset), static_cast<size_t>(buffer_size))) {
                AMS_ASSERT(util::IsIntValueRepresentable<s64>(read_size));

                *out = static_cast<s64>(read_size);
                R_SUCCEED();
            }
        }

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeReadFileHeader(std::addressof(request), handle, offset, buffer_size);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type));

        /* Check that we succeeded. */
        const auto htcfs_result = ConvertHtcfsResult(response.params[0]);
        if (R_FAILED(htcfs_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(htcfs_result);
        }

        /* Check our operation's result. */
        const auto native_result = ConvertNativeResult(response.params[1]);
        if (R_FAILED(native_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(native_result);
        }

        /* Check the body size. */
        R_UNLESS(response.body_size <= buffer_size, htcfs::ResultUnexpectedResponseBodySize());

        /* Receive the file data. */
        R_TRY(this->ReceiveFromRpcChannel(buffer, response.body_size));

        /* Set the output size. */
        *out = response.body_size;

        R_SUCCEED();
    }

    Result ClientImpl::ReadFileLarge(s64 *out, void *buffer, s32 handle, s64 offset, s64 buffer_size, fs::ReadOption option) {
        AMS_UNUSED(option);

        /* Check our buffer size. */
        R_UNLESS(util::IsIntValueRepresentable<size_t>(buffer_size), htcfs::ResultInvalidArgument());

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Setup data channel. */
        this->InitializeDataChannelForReceive(buffer, buffer_size);
        ON_SCOPE_EXIT { this->FinalizeDataChannel(); };

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeReadFileLargeHeader(std::addressof(request), handle, offset, buffer_size, DataChannelId);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        const auto htcfs_result = ConvertHtcfsResult(response.params[0]);
        if (R_FAILED(htcfs_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(htcfs_result);
        }

        /* Check our operation's result. */
        const auto native_result = ConvertNativeResult(response.params[1]);
        if (R_FAILED(native_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(native_result);
        }

        /* Check that the size read is allowable. */
        R_UNLESS(response.params[2] <= buffer_size, htcfs::ResultUnexpectedResponseBodySize());

        /* Read the entries, if there are any. */
        R_TRY(this->ReceiveFromDataChannel(response.params[2]));

        /* Set the number of output entries. */
        *out = response.params[2];

        R_SUCCEED();
    }

    Result ClientImpl::WriteFile(const void *buffer, s32 handle, s64 offset, s64 buffer_size, fs::WriteOption option) {
        /* Invalidate the cache. */
        m_cache_manager.Invalidate(handle);

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeWriteFileHeader(std::addressof(request), buffer_size, handle, option._value, offset);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request, buffer, buffer_size));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(response.params[1]));

        R_SUCCEED();
    }

    Result ClientImpl::WriteFileLarge(const void *buffer, s32 handle, s64 offset, s64 buffer_size, fs::WriteOption option) {
        /* Invalidate the cache. */
        m_cache_manager.Invalidate(handle);

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeWriteFileLargeHeader(std::addressof(request), handle, option._value, offset, buffer_size, DataChannelId);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Verify that the host reports ready to receive our data. */
        if (static_cast<HtcfsResult>(response.params[0]) != HtcfsResult::Ready) {
            R_RETURN(ConvertHtcfsResult(response.params[0]));
        }

        /* Verify that our send will be valid. */
        AMS_ASSERT(util::IsIntValueRepresentable<size_t>(buffer_size));

        /* Perform the send. */
        {
            /* Initialize data channel for our write. */
            this->InitializeDataChannelForSend(buffer, buffer_size);

            /* Ensure that we clean up our data channel. */
            ON_SCOPE_EXIT { this->FinalizeDataChannel(); };

            /* Send to our data channel. */
            R_TRY(this->SendToDataChannel());
        }

        /* Receive the large-write response. */
        Header write_resp;
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(write_resp), sizeof(write_resp)));

        /* Check the write-response header. */
        R_TRY(this->CheckResponseHeader(write_resp, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(write_resp.params[0]));

        /* Check our operation's result. */
        R_TRY(ConvertNativeResult(write_resp.params[1]));

        R_SUCCEED();
    }

    Result ClientImpl::GetFileSize(s64 *out, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Check if we have the file size cached. */
        R_SUCCEED_IF(m_cache_manager.GetFileSize(out, handle));

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeGetFileSizeHeader(std::addressof(request), handle);

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

        /* Set the output. */
        *out = response.params[2];

        R_SUCCEED();
    }

    Result ClientImpl::SetFileSize(s64 size, s32 handle) {
        /* Invalidate the cache. */
        m_cache_manager.Invalidate(handle);

        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeSetFileSizeHeader(std::addressof(request), handle, size);

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

        R_SUCCEED();
    }

    Result ClientImpl::FlushFile(s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeFlushFileHeader(std::addressof(request), handle);

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

        R_SUCCEED();
    }

    Result ClientImpl::GetPriorityForFile(s32 *out, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeGetPriorityForFileHeader(std::addressof(request), handle);

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

        R_SUCCEED();
    }

    Result ClientImpl::SetPriorityForFile(s32 priority, s32 handle) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeSetPriorityForFileHeader(std::addressof(request), handle, priority);

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type, 0));

        /* Check that we succeeded. */
        R_TRY(ConvertHtcfsResult(response.params[0]));

        R_SUCCEED();
    }

    Result ClientImpl::GetWorkingDirectory(char *dst, size_t dst_size) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeGetWorkingDirectoryHeader(std::addressof(request));

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type));

        /* Check that we succeeded. */
        const auto htcfs_result = ConvertHtcfsResult(response.params[0]);
        if (R_FAILED(htcfs_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(htcfs_result);
        }

        /* Check that the body size is valid. */
        R_UNLESS(response.body_size < static_cast<s64>(dst_size), htcfs::ResultUnexpectedResponseBodySize());

        /* Receive the response body. */
        R_TRY(this->ReceiveFromRpcChannel(dst, response.body_size));

        /* Null-terminate the response body. */
        dst[response.body_size] = '\x00';

        R_SUCCEED();
    }

    Result ClientImpl::GetWorkingDirectorySize(s32 *out) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Initialize our rpc channel. */
        R_TRY(this->InitializeRpcChannel());

        /* Create space for request and response. */
        Header request, response;

        /* Create header for the request. */
        m_header_factory.MakeGetWorkingDirectorySizeHeader(std::addressof(request));

        /* Send the request to the host. */
        R_TRY(this->SendRequest(request));

        /* Receive response from the host. */
        R_TRY(this->ReceiveFromRpcChannel(std::addressof(response), sizeof(response)));

        /* Check the response header. */
        R_TRY(this->CheckResponseHeader(response, request.packet_type));

        /* Check that we succeeded. */
        const auto htcfs_result = ConvertHtcfsResult(response.params[0]);
        if (R_FAILED(htcfs_result)) {
            R_UNLESS(response.body_size == 0, htcfs::ResultUnexpectedResponseBodySize());
            R_RETURN(htcfs_result);
        }

        /* Check that the size is representable. */
        R_UNLESS(util::IsIntValueRepresentable<s32>(response.params[1]), htcfs::ResultInvalidSize());

        /* Set the output size. */
        *out = static_cast<s32>(response.params[1]);

        R_SUCCEED();
    }

}
