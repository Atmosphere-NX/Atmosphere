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
#include "htclow_socket_driver.hpp"
#include "htclow_socket_discovery_util.hpp"

namespace ams::htclow::driver {

    Result SocketDriver::ConnectThread() {
        /* Do auto connect, if we should. */
        if (m_auto_connect_reserved) {
            this->DoAutoConnect();
        }

        /* Get the socket's name. */
        socket::SockAddrIn sockaddr;
        socket::SockLenT sockaddr_len = sizeof(sockaddr);
        R_UNLESS(socket::GetSockName(m_server_socket, reinterpret_cast<socket::SockAddr *>(std::addressof(sockaddr)), std::addressof(sockaddr_len)) == 0, htclow::ResultSocketGetSockNameError());

        /* Accept. */
        m_discovery_manager.OnSocketAcceptBegin(sockaddr.sin_port);

        sockaddr_len = sizeof(m_server_sockaddr);
        const auto client_desc = socket::Accept(m_server_socket, reinterpret_cast<socket::SockAddr *>(std::addressof(m_server_sockaddr)), std::addressof(sockaddr_len));

        m_discovery_manager.OnSocketAcceptEnd();

        /* Check accept result. */
        R_UNLESS(client_desc >= 0, htclow::ResultSocketAcceptError());

        /* Setup client socket. */
        R_TRY(this->SetupClientSocket(client_desc));

        return ResultSuccess();
    }

    Result SocketDriver::CreateServerSocket() {
        /* Check that we don't have a server socket. */
        AMS_ASSERT(!m_server_socket_valid);

        /* Create the socket. */
        const auto desc = socket::SocketExempt(socket::Family::Af_Inet, socket::Type::Sock_Stream, socket::Protocol::IpProto_Tcp);
        R_UNLESS(desc != -1, htclow::ResultSocketSocketExemptError());

        /* Be sure that we close the socket if we don't succeed. */
        auto socket_guard = SCOPE_GUARD { socket::Close(desc); };

        /* Create sockaddr for our socket. */
        const socket::SockAddrIn sockaddr = {
            .sin_len    = 0,
            .sin_family = socket::Family::Af_Inet,
            .sin_port   = socket::InetHtons(20180),
            .sin_addr   = { socket::InetHtonl(0) },
        };

        /* Enable local address reuse. */
        {
            u32 enable = 1;
            const auto res = socket::SetSockOpt(desc, socket::Level::Sol_Socket, socket::Option::So_ReuseAddr, std::addressof(enable), sizeof(enable));
            AMS_ABORT_UNLESS(res == 0);
        }

        /* Bind the socket. */
        const auto bind_res = socket::Bind(desc, reinterpret_cast<const socket::SockAddr *>(std::addressof(sockaddr)), sizeof(sockaddr));
        R_UNLESS(bind_res == 0, htclow::ResultSocketBindError());

        /* Listen on the socket. */
        const auto listen_res = socket::Listen(desc, 1);
        R_UNLESS(listen_res == 0, htclow::ResultSocketListenError());

        /* We succeeded. */
        socket_guard.Cancel();
        m_server_socket       = desc;
        m_server_socket_valid = true;

        return ResultSuccess();
    }

    void SocketDriver::DestroyServerSocket() {
        if (m_server_socket_valid) {
            socket::Shutdown(m_server_socket, socket::ShutdownMethod::Shut_RdWr);
            socket::Close(m_server_socket);
            m_server_socket_valid = false;
        }
    }

    Result SocketDriver::SetupClientSocket(s32 desc) {
        std::scoped_lock lk(m_mutex);

        /* Check that we don't have a client socket. */
        AMS_ASSERT(!m_client_socket_valid);

        /* Be sure that we close the socket if we don't succeed. */
        auto socket_guard = SCOPE_GUARD { socket::Close(desc); };

        /* Enable debug logging for the socket. */
        u32 debug = 1;
        const auto res = socket::SetSockOpt(desc, socket::Level::Sol_Tcp, socket::Option::So_Debug, std::addressof(debug), sizeof(debug));
        R_UNLESS(res >= 0, htclow::ResultSocketSetSockOptError());

        /* We succeeded. */
        socket_guard.Cancel();
        m_client_socket       = desc;
        m_client_socket_valid = true;

        return ResultSuccess();
    }

    bool SocketDriver::IsAutoConnectReserved() {
        return m_auto_connect_reserved;
    }

    void SocketDriver::ReserveAutoConnect() {
        std::scoped_lock lk(m_mutex);

        if (m_client_socket_valid) {
            /* Save our client sockaddr. */
            socket::SockLenT sockaddr_len = sizeof(m_saved_client_sockaddr);
            if (socket::GetSockName(m_server_socket, reinterpret_cast<socket::SockAddr *>(std::addressof(m_saved_client_sockaddr)), std::addressof(sockaddr_len)) != 0) {
                return;
            }

            /* Save our server sockaddr. */
            m_saved_server_sockaddr = m_server_sockaddr;

            /* Mark auto-connect reserved. */
            m_auto_connect_reserved = true;
        }
    }

    void SocketDriver::DoAutoConnect() {
        /* Clear auto-connect reserved. */
        m_auto_connect_reserved = false;

        /* Create udb socket. */
        const auto desc = socket::SocketExempt(socket::Family::Af_Inet, socket::Type::Sock_Dgram, socket::Protocol::IpProto_Udp);
        if (desc == -1) {
            return;
        }

        /* Clean up the desc when we're done. */
        ON_SCOPE_EXIT { socket::Close(desc); };

        /* Create auto-connect packet. */
        char auto_connect_packet[0x120];
        s32 len;
        {
            const socket::SockAddrIn sockaddr = {
                .sin_family = socket::Family::Af_Inet,
                .sin_port   = m_saved_client_sockaddr.sin_port,
                .sin_addr   = m_saved_client_sockaddr.sin_addr,
            };

            len = htclow::driver::MakeAutoConnectIpv4RequestPacket(auto_connect_packet, sizeof(auto_connect_packet), sockaddr);
        }

        /* Send the auto-connect packet to the host on port 20181. */
        const socket::SockAddrIn sockaddr = {
            .sin_family = socket::Family::Af_Inet,
            .sin_port   = socket::InetHtons(20181),
            .sin_addr   = m_saved_server_sockaddr.sin_addr,
        };

        /* Send the auto-connect packet. */
        socket::SendTo(desc, auto_connect_packet, len, socket::MsgFlag::MsgFlag_None, reinterpret_cast<const socket::SockAddr *>(std::addressof(sockaddr)), sizeof(sockaddr));
    }

    Result SocketDriver::Open() {
        m_discovery_manager.OnDriverOpen();
        return ResultSuccess();
    }

    void SocketDriver::Close() {
        m_discovery_manager.OnDriverClose();
    }

    Result SocketDriver::Connect(os::EventType *event) {
        /* Allocate a temporary thread stack. */
        void *stack = m_allocator->Allocate(os::MemoryPageSize, os::ThreadStackAlignment);
        ON_SCOPE_EXIT { m_allocator->Free(stack); };

        /* Try to create a server socket. */
        R_TRY(this->CreateServerSocket());

        /* Prepare to run our connect thread. */
        m_event.Clear();

        /* Run our connect thread. */
        {
            /* Create the thread. */
            os::ThreadType connect_thread;
            R_ABORT_UNLESS(os::CreateThread(std::addressof(connect_thread), ConnectThreadEntry, this, stack, os::MemoryPageSize, AMS_GET_SYSTEM_THREAD_PRIORITY(htc, HtclowTcpServer)));

            /* Set the thread's name. */
            os::SetThreadNamePointer(std::addressof(connect_thread), AMS_GET_SYSTEM_THREAD_NAME(htc, HtclowTcpServer));

            /* Start the thread. */
            os::StartThread(std::addressof(connect_thread));

            /* Check if we should cancel the connection. */
            if (os::WaitAny(event, m_event.GetBase()) == 0) {
                this->DestroyServerSocket();
            }

            /* Wait for the connect thread to finish. */
            os::WaitThread(std::addressof(connect_thread));

            /* Destroy the connection thread. */
            os::DestroyThread(std::addressof(connect_thread));

            /* Destroy the server socket. */
            this->DestroyServerSocket();
        }

        /* Return our connection result. */
        return m_connect_result;
    }

    void SocketDriver::Shutdown() {
        std::scoped_lock lk(m_mutex);

        /* Shut down our client socket, if we need to. */
        if (m_client_socket_valid) {
            socket::Shutdown(m_client_socket, socket::ShutdownMethod::Shut_RdWr);
            socket::Close(m_client_socket);
            m_client_socket_valid = 0;
        }
    }

    Result SocketDriver::Send(const void *src, int src_size) {
        /* Check the input size. */
        R_UNLESS(src_size >= 0, htclow::ResultInvalidArgument());

        /* Repeatedly send data until it's all sent. */
        ssize_t cur_sent;
        for (ssize_t sent = 0; sent < src_size; sent += cur_sent) {
            cur_sent = socket::Send(m_client_socket, static_cast<const u8 *>(src) + sent, src_size - sent, socket::MsgFlag::MsgFlag_None);
            R_UNLESS(cur_sent > 0, htclow::ResultSocketSendError());
        }

        return ResultSuccess();
    }

    Result SocketDriver::Receive(void *dst, int dst_size) {
        /* Check the input size. */
        R_UNLESS(dst_size >= 0, htclow::ResultInvalidArgument());

        /* Repeatedly receive data until it's all sent. */
        ssize_t cur_recv;
        for (ssize_t received = 0; received < dst_size; received += cur_recv) {
            cur_recv = socket::Recv(m_client_socket, static_cast<u8 *>(dst) + received, dst_size - received, socket::MsgFlag::MsgFlag_None);
            R_UNLESS(cur_recv > 0, htclow::ResultSocketReceiveError());
        }

        return ResultSuccess();
    }

    void SocketDriver::CancelSendReceive() {
        this->Shutdown();
    }

    void SocketDriver::Suspend() {
        this->ReserveAutoConnect();
    }

    void SocketDriver::Resume() {
        /* ... */
    }

}
