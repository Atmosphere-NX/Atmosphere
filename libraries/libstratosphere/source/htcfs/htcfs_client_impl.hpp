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

namespace ams::htcfs {

    class ClientImpl {
        private:
            u8 m_receive_buffer[0x1C040];
            u8 m_send_buffer[0x1C040];
            u8 m_packet_buffer[0xE020];
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
        private:
            int WaitAny(htclow::ChannelState state, os::EventType *event);

            Result SetUpProtocol();
            void TearDownProtocol();
    };

}
