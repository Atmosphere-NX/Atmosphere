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
#include "htcs_manager_service_object.hpp"

namespace ams::htcs::server {

    namespace {

        static constexpr inline size_t NumServers           = 1;
        static constexpr inline size_t MaxSessions          = 63;
        static constexpr inline sm::ServiceName ServiceName = sm::ServiceName::Encode("htcs");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize   = 0x80;
            static constexpr size_t MaxDomains          = 0x10;
            static constexpr size_t MaxDomainObjects    = 100;
            static constexpr bool CanDeferInvokeRequest = false;
            static constexpr bool CanManageMitmServers  = false;
        };

        using ServerManager = sf::hipc::ServerManager<NumServers, ServerOptions, MaxSessions>;

        /* Service object. */
        ServerManager g_server_manager;

        /* Service object. */
        constinit sf::UnmanagedServiceObject<tma::IHtcsManager, ManagerServiceObject> g_htcs_service_object;

    }

    void Initialize() {
        /* Add a reference to the htcs manager. */
        htcs::impl::HtcsManagerHolder::AddReference();
    }

    void RegisterHipcServer() {
        /* Register the service. */
        R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_htcs_service_object.GetShared(), ServiceName, MaxSessions));
    }

    void LoopHipcServer() {
        /* Loop, servicing services. */
        g_server_manager.LoopProcess();
    }

}
