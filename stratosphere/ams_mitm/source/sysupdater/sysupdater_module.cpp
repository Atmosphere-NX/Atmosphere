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
#include "sysupdater_module.hpp"
#include "sysupdater_service.hpp"
#include "sysupdater_async_impl.hpp"

namespace ams::mitm::sysupdater {

    namespace {

        constexpr sm::ServiceName SystemUpdateServiceName = sm::ServiceName::Encode("ams:su");
        constexpr size_t          SystemUpdateMaxSessions = 1;

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = SystemUpdateMaxSessions + 3;

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 1_KB;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;

        constinit sysupdater::SystemUpdateService g_system_update_service_object;

    }

    void MitmModule::ThreadFunction(void *arg) {
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* Connect to nim. */
        sm::DoWithSession([]() { nim::InitializeForNetworkInstallManager(); });
        ON_SCOPE_EXIT { nim::FinalizeForNetworkInstallManager(); };

        /* Register ams:su. */
        R_ABORT_UNLESS((g_server_manager.RegisterServer<sysupdater::impl::ISystemUpdateInterface, sysupdater::SystemUpdateService>(SystemUpdateServiceName, SystemUpdateMaxSessions, sf::GetSharedPointerTo<sysupdater::impl::ISystemUpdateInterface>(g_system_update_service_object))));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
