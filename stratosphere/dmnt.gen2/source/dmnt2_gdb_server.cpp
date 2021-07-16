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
#include "dmnt2_gdb_packet_io.hpp"
#include "dmnt2_gdb_server.hpp"
#include "dmnt2_debug_log.hpp"

namespace ams::dmnt {

    namespace {

        constexpr size_t ServerThreadStackSize = util::AlignUp(3 * GdbPacketBufferSize + os::MemoryPageSize, os::ThreadStackAlignment);

        alignas(os::ThreadStackAlignment) constinit u8 g_server_thread_stack[ServerThreadStackSize];
        constinit os::ThreadType g_server_thread;

        constinit util::TypedStorage<HtcsSession> g_session;

        void OnClientSocketAccepted(int fd) {
            /* Create htcs session for the socket. */
            util::ConstructAt(g_session, fd);
            ON_SCOPE_EXIT { util::DestroyAt(g_session); };

            HtcsSession *session = util::GetPointer(g_session);

            /* Create packet io handler. */
            GdbPacketIo packet_io;

            /* Process packets. */
            while (session->IsValid()) {
                /* Receive a packet. */
                bool do_break = false;
                char recv_buf[GdbPacketBufferSize];
                char *packet = packet_io.ReceivePacket(std::addressof(do_break), recv_buf, sizeof(recv_buf), session);

                if (!do_break && packet != nullptr) {
                    AMS_DMNT2_GDB_LOG_DEBUG("Received Packet %s\n", packet);

                    /* TODO: Process packets. */
                    packet_io.SendPacket(std::addressof(do_break), "OK", session);
                }
            }
        }

        void GdbServerThreadFunction(void *) {
            /* Loop forever, servicing our gdb server. */
            while (true) {
                /* Get a socket. */
                int fd;
                while ((fd = htcs::Socket()) == -1) {
                    os::SleepThread(TimeSpan::FromSeconds(1));
                }

                /* Ensure we cleanup the socket when we're done with it. */
                ON_SCOPE_EXIT {
                    htcs::Close(fd);
                    os::SleepThread(TimeSpan::FromSeconds(1));
                };

                /* Create a sock addr for our server. */
                htcs::SockAddrHtcs addr;
                addr.family    = htcs::HTCS_AF_HTCS;
                addr.peer_name = htcs::GetPeerNameAny();
                std::strcpy(addr.port_name.name, "iywys@$gdb");

                /* Bind. */
                if (htcs::Bind(fd, std::addressof(addr)) == -1) {
                    continue;
                }

                /* Listen on our port. */
                while (htcs::Listen(fd, 0) == 0) {
                    /* Continue accepting clients, so long as we can. */
                    int client_fd;
                    while (true) {
                        /* Try to accept a client. */
                        if (client_fd = htcs::Accept(fd, std::addressof(addr)); client_fd < 0) {
                            break;
                        }

                        /* Handle the client. */
                        OnClientSocketAccepted(client_fd);

                        /* Close the client socket. */
                        htcs::Close(client_fd);
                    }
                }
            }
        }



    }

    void InitializeGdbServer() {
        /* Create and start gdb server thread. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_server_thread), GdbServerThreadFunction, nullptr, g_server_thread_stack, sizeof(g_server_thread_stack), os::HighestThreadPriority - 1));
        os::StartThread(std::addressof(g_server_thread));
    }

}
