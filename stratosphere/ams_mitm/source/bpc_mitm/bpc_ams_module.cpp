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
#include "bpc_ams_module.hpp"
#include "bpc_ams_service.hpp"

namespace ams::mitm::bpc_ams {

    namespace {

        constexpr sm::ServiceName AtmosphereServiceName = sm::ServiceName::Encode("bpc:ams");
        constexpr size_t          AtmosphereMaxSessions = 4;

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = AtmosphereMaxSessions;
        using ServerOptions = sf::hipc::DefaultServerManagerOptions;
        sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;

        constinit sf::UnmanagedServiceObject<bpc::impl::IAtmosphereInterface, bpc::AtmosphereService> g_ams_service_object;

    }

    void MitmModule::ThreadFunction(void *) {
        /* Create bpc:ams. */
        {
            os::NativeHandle bpcams_h;
            R_ABORT_UNLESS(svc::ManageNamedPort(&bpcams_h, AtmosphereServiceName.name, AtmosphereMaxSessions));
            g_server_manager.RegisterObjectForServer(g_ams_service_object.GetShared(), bpcams_h);
        }

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
