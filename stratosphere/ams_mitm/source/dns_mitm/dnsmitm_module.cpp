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
#include "../amsmitm_initialization.hpp"
#include "dnsmitm_module.hpp"
#include "dnsmitm_debug.hpp"
#include "dnsmitm_resolver_impl.hpp"
#include "dnsmitm_host_redirection.hpp"

namespace ams::mitm::socket::resolver {

    namespace {

        enum PortIndex {
            PortIndex_Mitm,
            PortIndex_Count,
        };

        constexpr sm::ServiceName DnsMitmServiceName = sm::ServiceName::Encode("sfdnsres");

        constexpr size_t MaxSessions = 30;

        struct ServerOptions {
            static constexpr size_t PointerBufferSize   = sf::hipc::DefaultServerManagerOptions::PointerBufferSize;
            static constexpr size_t MaxDomains          = sf::hipc::DefaultServerManagerOptions::MaxDomains;
            static constexpr size_t MaxDomainObjects    = sf::hipc::DefaultServerManagerOptions::MaxDomainObjects;
            static constexpr bool CanDeferInvokeRequest = sf::hipc::DefaultServerManagerOptions::CanDeferInvokeRequest;
            static constexpr bool CanManageMitmServers  = true;
        };

        class ServerManager final : public sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions> {
            private:
                virtual Result OnNeedsToAccept(int port_index, Server *server) override;
        };

        alignas(os::MemoryPageSize) constinit u8 g_resolver_allocator_buffer[16_KB];

        ServerManager g_server_manager;

        Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
            /* Acknowledge the mitm session. */
            std::shared_ptr<::Service> fsrv;
            sm::MitmProcessInfo client_info;
            server->AcknowledgeMitmSession(std::addressof(fsrv), std::addressof(client_info));

            switch (port_index) {
                case PortIndex_Mitm:
                    return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<IResolver, ResolverImpl>(decltype(fsrv)(fsrv), client_info), fsrv);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        constexpr size_t TotalThreads = 8;
        static_assert(TotalThreads >= 1, "TotalThreads");
        constexpr size_t NumExtraThreads = TotalThreads - 1;
        constexpr size_t ThreadStackSize = mitm::ModuleTraits<socket::resolver::MitmModule>::StackSize;
        alignas(os::MemoryPageSize) u8 g_extra_thread_stacks[NumExtraThreads][ThreadStackSize];

        os::ThreadType g_extra_threads[NumExtraThreads];

        void LoopServerThread(void *) {
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

        bool ShouldMitmDns() {
            u8 en = 0;
            if (settings::fwdbg::GetSettingsItemValue(std::addressof(en), sizeof(en), "atmosphere", "enable_dns_mitm") == sizeof(en)) {
                return (en != 0);
            }
            return false;
        }

        bool ShouldEnableDebugLog() {
            u8 en = 0;
            if (settings::fwdbg::GetSettingsItemValue(std::addressof(en), sizeof(en), "atmosphere", "enable_dns_mitm_debug_log") == sizeof(en)) {
                return (en != 0);
            }
            return false;
        }

    }

    void MitmModule::ThreadFunction(void *) {
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* If we shouldn't mitm dns, don't do anything at all. */
        if (!ShouldMitmDns()) {
            return;
        }

        /* Initialize the socket allocator. */
        ams::socket::InitializeAllocatorForInternal(g_resolver_allocator_buffer, sizeof(g_resolver_allocator_buffer));

        /* Initialize debug. */
        resolver::InitializeDebug(ShouldEnableDebugLog());

        /* Initialize redirection map. */
        resolver::InitializeResolverRedirections();

        /* Create mitm servers. */
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<ResolverImpl>(PortIndex_Mitm, DnsMitmServiceName)));

        /* Loop forever, servicing our services. */
        ProcessForServerOnAllThreads();
    }

}
