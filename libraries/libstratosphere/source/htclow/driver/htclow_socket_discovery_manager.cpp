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
#include "htclow_socket_discovery_manager.hpp"
#include "htclow_socket_discovery_util.hpp"

namespace ams::htclow::driver {

    namespace {

        constexpr inline u32 BeaconQueryServiceId = 0xB48F5C51;

    }

    void SocketDiscoveryManager::OnDriverOpen() {
        /* Create our socket. */
        m_socket = socket::SocketExempt(socket::Family::Af_Inet, socket::Type::Sock_Dgram, socket::Protocol::IpProto_Udp);
        AMS_ABORT_UNLESS(m_socket != -1);

        /* Mark driver open. */
        m_driver_closed = false;

        /* Create our thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(m_discovery_thread), ThreadEntry, this, m_thread_stack, os::MemoryPageSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtclowDiscovery)));

        /* Set our thread name. */
        os::SetThreadNamePointer(std::addressof(m_discovery_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtclowDiscovery));

        /* Start our thread. */
        os::StartThread(std::addressof(m_discovery_thread));
    }

    void SocketDiscoveryManager::OnDriverClose() {
        /* Mark driver closed. */
        m_driver_closed = true;

        /* Shutdown our socket. */
        socket::Shutdown(m_socket, socket::ShutdownMethod::Shut_RdWr);

        /* Close our socket. */
        socket::Close(m_socket);

        /* Destroy our thread. */
        os::WaitThread(std::addressof(m_discovery_thread));
        os::DestroyThread(std::addressof(m_discovery_thread));
    }

    void SocketDiscoveryManager::OnSocketAcceptBegin(u16 port) {
        AMS_UNUSED(port);
    }

    void SocketDiscoveryManager::OnSocketAcceptEnd() {
        /* ... */
    }

    void SocketDiscoveryManager::ThreadFunc() {
        for (this->DoDiscovery(); !m_driver_closed; this->DoDiscovery()) {
            /* Check if the driver is closed five times. */
            for (size_t i = 0; i < 5; ++i) {
                os::SleepThread(TimeSpan::FromSeconds(1));
                if (m_driver_closed) {
                    return;
                }
            }
        }
    }

    Result SocketDiscoveryManager::DoDiscovery() {
        /* Ensure we close our socket if we fail. */
        auto socket_guard = SCOPE_GUARD { socket::Close(m_socket); };

        /* Create sockaddr for our socket. */
        const socket::SockAddrIn sockaddr = {
            .sin_len    = 0,
            .sin_family = socket::Family::Af_Inet,
            .sin_port   = socket::InetHtons(20181),
            .sin_addr   = { socket::InetHtonl(0) },
        };

        /* Bind our socket. */
        const auto bind_res = socket::Bind(m_socket, reinterpret_cast<const socket::SockAddr *>(std::addressof(sockaddr)), sizeof(sockaddr));
        R_UNLESS(bind_res != 0, htclow::ResultSocketBindError());

        /* Loop processing beacon queries. */
        while (true) {
            /* Receive a tmipc query header. */
            TmipcHeader header;
            socket::SockAddr recv_sockaddr;
            socket::SockLenT recv_sockaddr_len = sizeof(recv_sockaddr);
            const auto recv_res = socket::RecvFrom(m_socket, std::addressof(header), sizeof(header), socket::MsgFlag::MsgFlag_None, std::addressof(recv_sockaddr), std::addressof(recv_sockaddr_len));

            /* Check that our receive was valid. */
            R_UNLESS(recv_res >= 0,                              htclow::ResultSocketReceiveFromError());
            R_UNLESS(recv_sockaddr_len == sizeof(recv_sockaddr), htclow::ResultSocketReceiveFromError());

            /* Check we received a packet header. */
            if (recv_res != sizeof(header)) {
                continue;
            }

            /* Check that we received a correctly versioned BeaconQuery packet. */
            /* NOTE: Nintendo checks this *after* the following receive, but this seems saner. */
            if (header.version != TmipcVersion || header.service_id != BeaconQueryServiceId) {
                continue;
            }

            /* Receive the packet body, if there is one. */
            char packet_data[0x120];

            /* NOTE: Nintendo does not check this... */
            if (header.data_len > sizeof(packet_data)) {
                continue;
            }

            if (header.data_len > 0) {
                const auto body_res = socket::RecvFrom(m_socket, packet_data, header.data_len, socket::MsgFlag::MsgFlag_None, std::addressof(recv_sockaddr), std::addressof(recv_sockaddr_len));
                R_UNLESS(body_res >= 0, htclow::ResultSocketReceiveFromError());
                R_UNLESS(recv_sockaddr_len == sizeof(recv_sockaddr), htclow::ResultSocketReceiveFromError());

                if (body_res != header.data_len) {
                    continue;
                }
            }

            /* Make our beacon response packet. */
            const auto len = MakeBeaconResponsePacket(packet_data, sizeof(packet_data));

            /* Send the beacon response data. */
            const auto send_res = socket::SendTo(m_socket, packet_data, len, socket::MsgFlag::MsgFlag_None, std::addressof(recv_sockaddr), sizeof(recv_sockaddr));
            R_UNLESS(send_res >= 0, htclow::ResultSocketSendToError());
        }

        /* This can never happen, as the above loop should be infinite, but completion logic is here for posterity. */
        socket_guard.Cancel();
        return ResultSuccess();
    }

}
