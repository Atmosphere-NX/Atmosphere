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
#include "setmitm_module.hpp"
#include "set_mitm_service.hpp"
#include "setsys_mitm_service.hpp"

namespace ams::mitm::settings {

    namespace {

        enum PortIndex {
            PortIndex_SetMitm,
            PortIndex_SetSysMitm,
            PortIndex_Count,
        };

        constexpr sm::ServiceName SetMitmServiceName = sm::ServiceName::Encode("set");
        constexpr sm::ServiceName SetSysMitmServiceName = sm::ServiceName::Encode("set:sys");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize   = 0x200;
            static constexpr size_t MaxDomains          = 0;
            static constexpr size_t MaxDomainObjects    = 0;
            static constexpr bool CanDeferInvokeRequest = false;
            static constexpr bool CanManageMitmServers  = true;
        };

        constexpr size_t MaxSessions = 60;

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
                case PortIndex_SetMitm:
                    return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<ISetMitmInterface, SetMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                case PortIndex_SetSysMitm:
                    return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<ISetSysMitmInterface, SetSysMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

    void MitmModule::ThreadFunction(void *) {
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* Create mitm servers. */
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<SetMitmService>(PortIndex_SetMitm, SetMitmServiceName)));
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<SetSysMitmService>(PortIndex_SetSysMitm, SetSysMitmServiceName)));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
