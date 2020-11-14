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
#include "nsmitm_module.hpp"
#include "ns_am_mitm_service.hpp"
#include "ns_web_mitm_service.hpp"

namespace ams::mitm::ns {

    namespace {

        constexpr sm::ServiceName NsAmMitmServiceName = sm::ServiceName::Encode("ns:am");
        constexpr sm::ServiceName NsWebMitmServiceName = sm::ServiceName::Encode("ns:web");

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = 5;
        using ServerOptions = sf::hipc::DefaultServerManagerOptions;
        sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> g_server_manager;

    }

    void MitmModule::ThreadFunction(void *arg) {
        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* Create mitm servers. */
        if (hos::GetVersion() < hos::Version_3_0_0) {
            R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<impl::IAmMitmInterface, NsAmMitmService>(NsAmMitmServiceName)));
        } else {
            R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<impl::IWebMitmInterface, NsWebMitmService>(NsWebMitmServiceName)));
        }

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
