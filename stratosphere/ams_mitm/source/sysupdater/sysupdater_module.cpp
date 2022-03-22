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
#include "sysupdater_module.hpp"
#include "sysupdater_service.hpp"
#include "sysupdater_async_impl.hpp"

namespace ams::mitm::sysupdater {

    namespace {

        enum PortIndex {
            PortIndex_Sysupdater,
            PortIndex_Count,
        };

        constexpr sm::ServiceName SystemUpdateServiceName = sm::ServiceName::Encode("ams:su");
        constexpr size_t          SystemUpdateMaxSessions = 1;

        constexpr size_t MaxSessions = SystemUpdateMaxSessions + 3;

        struct ServerOptions {
            static constexpr size_t PointerBufferSize   = 1_KB;
            static constexpr size_t MaxDomains          = 0;
            static constexpr size_t MaxDomainObjects    = 0;
            static constexpr bool CanDeferInvokeRequest = false;
            static constexpr bool CanManageMitmServers  = false;
        };

        sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions> g_server_manager;

        constinit sf::UnmanagedServiceObject<sysupdater::impl::ISystemUpdateInterface, sysupdater::SystemUpdateService> g_system_update_service_object;

    }

    void MitmModule::ThreadFunction(void *) {
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* Connect to nim. */
        nim::InitializeForNetworkInstallManager();
        ON_SCOPE_EXIT { nim::FinalizeForNetworkInstallManager(); };

        /* Register ams:su. */
        R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_system_update_service_object.GetShared(), SystemUpdateServiceName, SystemUpdateMaxSessions));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
