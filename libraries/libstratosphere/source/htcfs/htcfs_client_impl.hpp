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
#include "../htclow/htclow_manager.hpp"
#include "../htclow/htclow_channel.hpp"
#include "htcfs_cache_manager.hpp"
#include "htcfs_header_factory.hpp"
#include "../htclow/htclow_default_channel_config.hpp"

namespace ams::htcfs {

    class ClientImpl {
        public:
            static constexpr size_t MaxPacketBodySize = htclow::DefaultChannelConfig.max_packet_size - sizeof(htclow::PacketHeader);
        private:
            u8 m_receive_buffer[0x1C040];
            u8 m_send_buffer[0x1C040];
            u8 m_packet_buffer[MaxPacketBodySize + sizeof(htclow::PacketHeader)];
            htclow::HtclowManager *m_htclow_manager;
            CacheManager m_cache_manager;
            HeaderFactory m_header_factory;
            os::SdkMutex m_mutex;
            htclow::Module m_module;
            htclow::Channel m_rpc_channel;
            htclow::Channel m_data_channel;
            bool m_connected;
            os::ThreadType m_monitor_thread;
            os::Event m_event;
        private:
            static void ThreadEntry(void *arg) { static_cast<ClientImpl *>(arg)->ThreadBody(); }

            void ThreadBody();
        public:
            ClientImpl(htclow::HtclowManager *manager);

            ~ClientImpl() {
                this->Cancel();
                this->Wait();
            }
        public:
            void Start();
            void Cancel();
            void Wait();
        public:
            Result OpenDirectory(s32 *out_handle, const char *path, fs::OpenDirectoryMode mode, bool case_sensitive);
            Result CloseDirectory(s32 handle);

            Result GetEntryCount(s64 *out, s32 handle);
            Result ReadDirectory(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle);
            Result ReadDirectoryLarge(s64 *out, fs::DirectoryEntry *out_entries, size_t max_out_entries, s32 handle);
            Result GetPriorityForDirectory(s32 *out, s32 handle);
            Result SetPriorityForDirectory(s32 priority, s32 handle);
        private:
            int WaitAny(htclow::ChannelState state, os::EventType *event);

            Result SetUpProtocol();
            void TearDownProtocol();

            Result CheckResponseHeaderWithoutVersion(const Header &response, PacketType packet_type);
            Result CheckResponseHeader(const Header &response, PacketType packet_type);
            Result CheckResponseHeader(const Header &response, PacketType packet_type, s64 body_size);

            Result GetMaxProtocolVersion(s16 *out);
            Result SetProtocolVersion(s16 version);

            Result InitializeRpcChannel();

            Result SendToRpcChannel(const void *src, s64 size);
            Result ReceiveFromRpcChannel(void *dst, s64 size);

            Result SendToHtclow(const void *src, s64 size, htclow::Channel *channel);
            Result ReceiveFromHtclow(void *dst, s64 size, htclow::Channel *channel);

            Result SendRequest(const Header &request) { return this->SendRequest(request, nullptr, 0, nullptr, 0); }
            Result SendRequest(const Header &request, const void *arg1, size_t arg1_size) { return this->SendRequest(request, arg1, arg1_size, nullptr, 0); }
            Result SendRequest(const Header &request, const void *arg1, size_t arg1_size, const void *arg2, size_t arg2_size);
    };

}
