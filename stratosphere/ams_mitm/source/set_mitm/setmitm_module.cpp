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
#include "../amsmitm_initialization.hpp"
#include "setmitm_module.hpp"
#include "set_mitm_service.hpp"
#include "setsys_mitm_service.hpp"

namespace ams::mitm::settings {

    namespace {

        constexpr sm::ServiceName SetMitmServiceName = sm::ServiceName::Encode("set");
        constexpr sm::ServiceName SetSysMitmServiceName = sm::ServiceName::Encode("set:sys");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x100;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        constexpr size_t MaxServers = 2;
        constexpr size_t MaxSessions = 60;
        sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;

    }

    void MitmModule::ThreadFunction(void *arg) {
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* Create mitm servers. */
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<ISetMitmInterface, SetMitmService>(SetMitmServiceName)));
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<ISetSysMitmInterface, SetSysMitmService>(SetSysMitmServiceName)));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
