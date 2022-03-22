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
#include "nsmitm_module.hpp"
#include "ns_am_mitm_service.hpp"
#include "ns_web_mitm_service.hpp"

namespace ams::mitm::ns {

    namespace {

        enum PortIndex {
            PortIndex_Mitm,
            PortIndex_Count,
        };

        constexpr sm::ServiceName NsAmMitmServiceName = sm::ServiceName::Encode("ns:am");
        constexpr sm::ServiceName NsWebMitmServiceName = sm::ServiceName::Encode("ns:web");

        constexpr size_t MaxSessions = 5;

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

        ServerManager g_server_manager;

        Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
            /* Acknowledge the mitm session. */
            std::shared_ptr<::Service> fsrv;
            sm::MitmProcessInfo client_info;
            server->AcknowledgeMitmSession(std::addressof(fsrv), std::addressof(client_info));

            switch (port_index) {
                case PortIndex_Mitm:
                    if (hos::GetVersion() < hos::Version_3_0_0) {
                        return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<impl::IAmMitmInterface, NsAmMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                    } else {
                        return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<impl::IWebMitmInterface, NsWebMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                    }

                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    void MitmModule::ThreadFunction(void *) {
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* Create mitm servers. */
        if (hos::GetVersion() < hos::Version_3_0_0) {
            R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<NsAmMitmService>(PortIndex_Mitm, NsAmMitmServiceName)));
        } else {
            R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<NsWebMitmService>(PortIndex_Mitm, NsWebMitmServiceName)));
        }

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
