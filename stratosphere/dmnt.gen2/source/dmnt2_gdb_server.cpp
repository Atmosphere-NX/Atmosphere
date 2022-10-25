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
#include "dmnt2_debug_log.hpp"
#include "dmnt2_transport_layer.hpp"
#include "dmnt2_gdb_server.hpp"
#include "dmnt2_gdb_server_impl.hpp"

namespace ams::dmnt {
// bool gen2_loop(u32 count);
    namespace {

        constexpr size_t ServerThreadStackSize = util::AlignUp(4 * GdbPacketBufferSize + os::MemoryPageSize, os::ThreadStackAlignment);
        constexpr size_t ServerThreadStackSize2 = util::AlignUp(os::MemoryPageSize, os::ThreadStackAlignment);

        alignas(os::ThreadStackAlignment) constinit u8 g_server_thread_stack[ServerThreadStackSize];
        alignas(os::ThreadStackAlignment) constinit u8 g_events_thread_stack[util::AlignUp(2 * GdbPacketBufferSize + os::MemoryPageSize, os::ThreadStackAlignment)];
        alignas(os::ThreadStackAlignment) constinit u8 g_server_thread_stack2[ServerThreadStackSize2];

        constinit os::ThreadType g_server_thread, g_server_thread2;

        constinit util::TypedStorage<GdbServerImpl> g_gdb_server;
        void GdbServerThreadFunction2(void *) {
            while (true){
                util::GetReference(g_gdb_server).gen2_loop();
                svcSleepThread(50'000'000);
                // os::SleepThread(TimeSpan::FromMilliSeconds(100));
            };
        }
        void GdbServerThreadFunction(void *) {
            /* Loop forever, servicing our gdb server. */
            while (true) {
                /* Get a socket. */
                int fd;
                while ((fd = transport::Socket()) == -1) {
                    os::SleepThread(TimeSpan::FromSeconds(1));
                }

                /* Ensure we cleanup the socket when we're done with it. */
                ON_SCOPE_EXIT {
                    transport::Close(fd);
                    os::SleepThread(TimeSpan::FromSeconds(1));
                };

                /* Bind. */
                if (transport::Bind(fd, transport::PortName_GdbServer) == -1) {
                    continue;
                }

                /* Listen on our port. */
                while (transport::Listen(fd, 0) == 0) {
                    /* Continue accepting clients, so long as we can. */
                    int client_fd;
                    while (true) {
                        {
                            util::ConstructAt(g_gdb_server, 500, g_events_thread_stack, sizeof(g_events_thread_stack));
                            util::GetReference(g_gdb_server).gen2_server_on = 2;
                            ON_SCOPE_EXIT { util::GetReference(g_gdb_server).gen2_server_on = 0; util::DestroyAt(g_gdb_server); };
                            /* Try to accept a client. */
                            if (client_fd = transport::Accept(fd); client_fd < 0) {
                                break;
                            }
                        }
                        {
                            /* Create gdb server for the socket. */
                            util::ConstructAt(g_gdb_server, client_fd, g_events_thread_stack, sizeof(g_events_thread_stack));
                            ON_SCOPE_EXIT { util::DestroyAt(g_gdb_server); };

                            /* Process for the server. */
                            util::GetReference(g_gdb_server).LoopProcess();
                        }

                        /* Close the client socket. */
                        transport::Close(client_fd);
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

    void InitializeGdbServer2() {
        /* Create and start gdb server threads. */
        R_ABORT_UNLESS(os::CreateThread(std::addressof(g_server_thread2), GdbServerThreadFunction2, nullptr, g_server_thread_stack2, sizeof(g_server_thread_stack2), os::HighestThreadPriority - 1));
        os::StartThread(std::addressof(g_server_thread2));
    }
}
