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
#include "memlet_service.hpp"

namespace ams {

    namespace memlet {

        namespace {

            using ServerOptions = sf::hipc::DefaultServerManagerOptions;

            constexpr sm::ServiceName MemServiceName = sm::ServiceName::Encode("memlet");
            constexpr size_t          MemMaxSessions = 1;

            /* memlet. */
            constexpr size_t NumServers  = 1;
            constexpr size_t NumSessions = MemMaxSessions;

            sf::hipc::ServerManager<NumServers, ServerOptions, NumSessions> g_server_manager;

            constinit sf::UnmanagedServiceObject<memlet::impl::IService, memlet::Service> g_mem_service_object;

            void InitializeAndLoopIpcServer() {
                /* Create services. */
                R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_mem_service_object.GetShared(), MemServiceName, MemMaxSessions));

                /* Loop forever, servicing our services. */
                g_server_manager.LoopProcess();
            }

        }

    }

    namespace init {

        void InitializeSystemModule() {
            /* Initialize our connection to sm. */
            R_ABORT_UNLESS(sm::Initialize());

            /* Verify that we can sanely execute. */
            ams::CheckApiVersion();
        }

        void FinalizeSystemModule() { /* ... */ }

        void Startup() { /* ... */ }

    }

    void Main() {
        /* Set thread name. */
        os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(memlet, Main));
        AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(memlet, Main));

        /* Initialize and service our ipc service. */
        memlet::InitializeAndLoopIpcServer();
    }

}
