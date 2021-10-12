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
#include "bpcmitm_module.hpp"
#include "bpc_mitm_service.hpp"

namespace ams::mitm::bpc {

    namespace {

        enum PortIndex {
            PortIndex_Mitm,
            PortIndex_Count,
        };

        constexpr sm::ServiceName MitmServiceName = sm::ServiceName::Encode("bpc");
        constexpr sm::ServiceName DeprecatedMitmServiceName = sm::ServiceName::Encode("bpc:c");
        constexpr size_t          MitmServiceMaxSessions = 13;

        constexpr size_t MaxSessions = MitmServiceMaxSessions;

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
                    return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<impl::IBpcMitmInterface, BpcMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    void MitmModule::ThreadFunction(void *) {
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* On Mariko, we can't reboot to payload/do exosphere-shutdown...so there is no point in bpc.mitm. */
        if (spl::GetSocType() == spl::SocType_Mariko) {
            return;
        }

        /* Create bpc mitm. */
        const sm::ServiceName service_name = (hos::GetVersion() >= hos::Version_2_0_0) ? MitmServiceName : DeprecatedMitmServiceName;

        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<BpcMitmService>(PortIndex_Mitm, service_name)));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
