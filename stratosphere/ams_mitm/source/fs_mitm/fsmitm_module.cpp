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
#include "fsmitm_module.hpp"
#include "fs_mitm_service.hpp"

namespace ams::mitm::fs {

    namespace {

        constexpr sm::ServiceName MitmServiceName = sm::ServiceName::Encode("fsp-srv");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x800;
            static constexpr size_t MaxDomains = 0x40;
            static constexpr size_t MaxDomainObjects = 0x4000;
        };

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = 61;
        sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;


        constexpr size_t TotalThreads = 5;
        static_assert(TotalThreads >= 1, "TotalThreads");
        constexpr size_t NumExtraThreads = TotalThreads - 1;
        constexpr size_t ThreadStackSize = mitm::ModuleTraits<fs::MitmModule>::StackSize;
        alignas(os::MemoryPageSize) u8 g_extra_thread_stacks[NumExtraThreads][ThreadStackSize];

        os::ThreadType g_extra_threads[NumExtraThreads];

        void LoopServerThread(void *arg) {
            /* Loop forever, servicing our services. */
            g_server_manager.LoopProcess();
        }

        void ProcessForServerOnAllThreads() {
            /* Initialize threads. */
            if constexpr (NumExtraThreads > 0) {
                const s32 priority = os::GetThreadCurrentPriority(os::GetCurrentThread());
                for (size_t i = 0; i < NumExtraThreads; i++) {
                    R_ABORT_UNLESS(os::CreateThread(g_extra_threads + i, LoopServerThread, nullptr, g_extra_thread_stacks[i], ThreadStackSize, priority));
                }
            }

            /* Start extra threads. */
            if constexpr (NumExtraThreads > 0) {
                for (size_t i = 0; i < NumExtraThreads; i++) {
                    os::StartThread(g_extra_threads + i);
                }
            }

            /* Loop this thread. */
            LoopServerThread(nullptr);

            /* Wait for extra threads to finish. */
            if constexpr (NumExtraThreads > 0) {
                for (size_t i = 0; i < NumExtraThreads; i++) {
                    os::WaitThread(g_extra_threads + i);
                }
            }
        }

    }

    void MitmModule::ThreadFunction(void *arg) {
        /* Create fs mitm. */
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<IFsMitmInterface, FsMitmService>(MitmServiceName)));

        /* Process for the server. */
        ProcessForServerOnAllThreads();
    }

}
