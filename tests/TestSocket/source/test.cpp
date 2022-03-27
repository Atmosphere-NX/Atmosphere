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

namespace ams {

    namespace {

        constinit u8 g_socket_config_memory[2_MB];

        alignas(os::MemoryPageSize) constinit u8 g_server_thread_stack[16_KB];

        constexpr const u8 TestMessage[0x10] = {
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
        };

        void TestServerThread(void *arg) {
            os::EventType *server_ready_event = reinterpret_cast<os::EventType *>(arg);

            s32 listen_fd = socket::Socket(socket::Family::Af_Inet, socket::Type::Sock_Stream, socket::Protocol::IpProto_Ip);
            AMS_ABORT_UNLESS(listen_fd >= 0);
            printf("[Server]: Listen fd=%d\n", static_cast<int>(listen_fd));

            socket::SockAddrIn s_addr = {};
            s_addr.sin_family      = socket::Family::Af_Inet;
            s_addr.sin_addr.s_addr = socket::InAddr_Any;
            s_addr.sin_port        = socket::InetHtons(23337);

            /* Bind. */
            const auto bind_res = socket::Bind(listen_fd, reinterpret_cast<socket::SockAddr *>(std::addressof(s_addr)), sizeof(s_addr));
            printf("[Server]: Bind=%d\n", static_cast<int>(bind_res));
            AMS_ABORT_UNLESS(bind_res == 0);

            /* Listen. */
            const auto listen_res = socket::Listen(listen_fd, 1);
            printf("[Server]: Listen=%d\n", static_cast<int>(listen_res));
            AMS_ABORT_UNLESS(listen_res >= 0);

            printf("[Server]: Ready\n");
            os::SignalEvent(server_ready_event);

            /* Accept. */
            s32 conn_fd = socket::Accept(listen_fd, nullptr, nullptr);
            AMS_ABORT_UNLESS(conn_fd >= 0);
            printf("[Server]: Conn fd=%d\n", conn_fd);

            /* Receive. */
            u8 received[sizeof(TestMessage)] = {};
            AMS_ABORT_UNLESS(socket::Recv(conn_fd, received, sizeof(received), socket::MsgFlag::Msg_None) == sizeof(received));
            printf("[Server]: Received\n");

            AMS_ABORT_UNLESS(std::memcmp(received, TestMessage, sizeof(TestMessage)) == 0);

            /* Calculate hash. */
            u8 hash[crypto::Sha256Generator::HashSize];
            crypto::GenerateSha256(hash, sizeof(hash), received, sizeof(received));

            /* Send hash. */
            AMS_ABORT_UNLESS(socket::Send(conn_fd, hash, sizeof(hash), socket::MsgFlag::Msg_None) == sizeof(hash));
            printf("[Server]: Sent\n");

            /* Close sockets. */
            AMS_ABORT_UNLESS(socket::Close(conn_fd) == 0);
            AMS_ABORT_UNLESS(socket::Close(listen_fd) == 0);
            printf("[Server]: Closed\n");
        }

    }

    void Main() {
        auto cfg = socket::SystemConfigDefault(g_socket_config_memory, sizeof(g_socket_config_memory) / 2, sizeof(g_socket_config_memory) / 2);
        R_ABORT_UNLESS(socket::Initialize(cfg));
        {
            /* Set up for the server thread. */
            os::EventType server_ready_event;
            os::InitializeEvent(std::addressof(server_ready_event), false, os::EventClearMode_AutoClear);
            ON_SCOPE_EXIT { os::FinalizeEvent(std::addressof(server_ready_event)); };

            /* Wait for the server thread to be ready */
            os::ThreadType server_thread;
            R_ABORT_UNLESS(os::CreateThread(std::addressof(server_thread), TestServerThread, std::addressof(server_ready_event), g_server_thread_stack, sizeof(g_server_thread_stack), os::DefaultThreadPriority));
            os::SetThreadNamePointer(std::addressof(server_thread), "ServerThread");
            os::StartThread(std::addressof(server_thread));

            /* Wait for the server thread to be ready. */
            os::WaitEvent(std::addressof(server_ready_event));

            {
                /* Create socket. */
                s32 conn_fd = socket::Socket(socket::Family::Af_Inet, socket::Type::Sock_Stream, socket::Protocol::IpProto_Ip);
                AMS_ABORT_UNLESS(conn_fd >= 0);
                printf("[Client]: Conn fd=%d\n", static_cast<int>(conn_fd));

                socket::SockAddrIn s_addr = {};
                s_addr.sin_family      = socket::Family::Af_Inet;
                s_addr.sin_addr.s_addr = socket::InAddr_Loopback;
                s_addr.sin_port        = socket::InetHtons(23337);

                /* Connect. */
                const auto connect_res = socket::Connect(conn_fd, reinterpret_cast<socket::SockAddr *>(std::addressof(s_addr)), sizeof(s_addr));
                printf("[Client]: Connect=%d, last_err=%d\n", connect_res, static_cast<int>(socket::GetLastError()));
                AMS_ABORT_UNLESS(connect_res == 0);

                /* Send test. */
                AMS_ABORT_UNLESS(socket::Send(conn_fd, TestMessage, sizeof(TestMessage), socket::MsgFlag::Msg_None) == sizeof(TestMessage));
                printf("[Client]: Sent\n");

                /* Receive. */
                u8 received[crypto::Sha256Generator::HashSize] = {};
                AMS_ABORT_UNLESS(socket::Recv(conn_fd, received, sizeof(received), socket::MsgFlag::Msg_None) == sizeof(received));
                printf("[Client]: Received\n");

                /* Calculate hash. */
                u8 hash[crypto::Sha256Generator::HashSize];
                crypto::GenerateSha256(hash, sizeof(hash), TestMessage, sizeof(TestMessage));

                AMS_ABORT_UNLESS(std::memcmp(received, hash, sizeof(hash)) == 0);

                /* Close sockets. */
                AMS_ABORT_UNLESS(socket::Close(conn_fd) == 0);
                printf("[Client]: Closed\n");
            }

            /* Wait for the server thread to complete. */
            os::WaitThread(std::addressof(server_thread));
        }
        printf("Successfully performed socket test!\n");

        socket::Finalize();
    }

}