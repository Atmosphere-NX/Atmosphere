/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "bpcmitm_module.hpp"
#include "bpc_mitm_service.hpp"

namespace ams::mitm::bpc {

    namespace {

        constexpr sm::ServiceName MitmServiceName = sm::ServiceName::Encode("bpc");
        constexpr sm::ServiceName DeprecatedMitmServiceName = sm::ServiceName::Encode("bpc:c");
        constexpr size_t          MitmServiceMaxSessions = 13;

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = MitmServiceMaxSessions;
        using ServerOptions = sf::hipc::DefaultServerManagerOptions;
        sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;

    }

    void MitmModule::ThreadFunction(void *arg) {
        /* Create bpc mitm. */
        const sm::ServiceName service_name = (hos::GetVersion() >= hos::Version_200) ? MitmServiceName : DeprecatedMitmServiceName;
        R_ASSERT(g_server_manager.RegisterMitmServer<BpcMitmService>(service_name));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
