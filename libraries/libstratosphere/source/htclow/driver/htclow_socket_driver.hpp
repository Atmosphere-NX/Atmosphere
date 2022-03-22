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
#pragma once
#include <stratosphere.hpp>
#include "htclow_i_driver.hpp"
#include "htclow_socket_discovery_manager.hpp"

namespace ams::htclow::driver {

    class SocketDriver final : public IDriver {
        private:
            mem::StandardAllocator *m_allocator;
            SocketDiscoveryManager m_discovery_manager;
            socket::SockAddrIn m_server_sockaddr;
            socket::SockAddrIn m_saved_server_sockaddr;
            socket::SockAddrIn m_saved_client_sockaddr;
            os::Event m_event;
            Result m_connect_result;
            os::SdkMutex m_mutex;
            s32 m_server_socket;
            s32 m_client_socket;
            bool m_server_socket_valid;
            bool m_client_socket_valid;
            bool m_auto_connect_reserved;
        public:
            SocketDriver(mem::StandardAllocator *allocator)
                : m_allocator(allocator), m_discovery_manager(m_allocator), m_event(os::EventClearMode_ManualClear),
                  m_connect_result(), m_mutex(), m_server_socket(), m_client_socket(), m_server_socket_valid(false),
                  m_client_socket_valid(false), m_auto_connect_reserved(false)
            {
                /* ... */
            }
        private:
            static void ConnectThreadEntry(void *arg) {
                auto * const driver = static_cast<SocketDriver *>(arg);

                driver->m_connect_result = driver->ConnectThread();
                driver->m_event.Signal();
            }

            Result ConnectThread();
        private:
            Result CreateServerSocket();
            void DestroyServerSocket();

            Result SetupClientSocket(s32 desc);

            bool IsAutoConnectReserved();
            void ReserveAutoConnect();
            void DoAutoConnect();
        public:
            virtual Result Open() override;
            virtual void Close() override;
            virtual Result Connect(os::EventType *event) override;
            virtual void Shutdown() override;
            virtual Result Send(const void *src, int src_size) override;
            virtual Result Receive(void *dst, int dst_size) override;
            virtual void CancelSendReceive() override;
            virtual void Suspend() override;
            virtual void Resume() override;
    };

}
