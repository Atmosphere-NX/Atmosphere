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
#include "mitm_pm_module.hpp"
#include "mitm_pm_service.hpp"

namespace ams::mitm::pm {

    namespace {

        constexpr sm::ServiceName PmServiceName = sm::ServiceName::Encode("mitm:pm");
        constexpr size_t          PmMaxSessions = 1;

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = PmMaxSessions;
        using ServerOptions = sf::hipc::DefaultServerManagerOptions;
        sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;

        constinit sf::UnmanagedServiceObject<mitm::pm::impl::IPmInterface, mitm::pm::PmService> g_pm_service_object;

    }

    void MitmModule::ThreadFunction(void *) {
        /* Create bpc:ams. */
        R_ABORT_UNLESS(g_server_manager.RegisterObjectForServer(g_pm_service_object.GetShared(), PmServiceName, PmMaxSessions));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
