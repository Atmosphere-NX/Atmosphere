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
#include "dmnt2_debug_log.hpp"
#include "dmnt2_gdb_server.hpp"
#include "dmnt2_gdb_server_impl.hpp"

namespace ams::dmnt {

    namespace {

        constexpr size_t ServerThreadStackSize = util::AlignUp(4 * GdbPacketBufferSize + os::MemoryPageSize, os::ThreadStackAlignment);

        alignas(os::ThreadStackAlignment) constinit u8 g_server_thread_stack[ServerThreadStackSize];

        constinit os::ThreadType g_server_thread;

        constinit util::TypedStorage<GdbServerImpl> g_gdb_server;

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

                        {
                            /* Create gdb server for the socket. */
                            util::ConstructAt(g_gdb_server, client_fd);
                            ON_SCOPE_EXIT { util::DestroyAt(g_gdb_server); };

                            /* Process for the server. */
                            util::GetReference(g_gdb_server).LoopProcess();
                        }

                        /* Close the client socket. */
                        htcs::Close(client_fd);
                    }
                }
            }
        }

    }

    void InitializeGdbServer() {
        /* Create and start gdb server threads. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_server_thread), GdbServerThreadFunction, nullptr, g_server_thread_stack, sizeof(g_server_thread_stack), os::HighestThreadPriority - 1));
        os::StartThread(std::addressof(g_server_thread));
    }

}
