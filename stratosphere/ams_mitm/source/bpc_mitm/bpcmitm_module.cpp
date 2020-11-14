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
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* On Mariko, we can't reboot to payload/do exosphere-shutdown...so there is no point in bpc.mitm. */
        if (spl::GetSocType() == spl::SocType_Mariko) {
            return;
        }

        /* Create bpc mitm. */
        const sm::ServiceName service_name = (hos::GetVersion() >= hos::Version_2_0_0) ? MitmServiceName : DeprecatedMitmServiceName;
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<impl::IBpcMitmInterface, BpcMitmService>(service_name)));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
