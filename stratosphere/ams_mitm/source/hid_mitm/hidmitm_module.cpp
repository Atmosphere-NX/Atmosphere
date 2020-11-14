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
#include "hidmitm_module.hpp"
#include "hid_mitm_service.hpp"

namespace ams::mitm::hid {

    namespace {

        constexpr sm::ServiceName MitmServiceName = sm::ServiceName::Encode("hid");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x200;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        constexpr size_t MaxServers = 1;
        sf::hipc::ServerManager<MaxServers, ServerOptions> g_server_manager;

        bool ShouldMitmHidForCompability() {
            u8 en = 0;
            if (settings::fwdbg::GetSettingsItemValue(&en, sizeof(en), "atmosphere", "enable_deprecated_hid_mitm") == sizeof(en)) {
                return (en != 0);
            }
            return false;
        }

    }

    void MitmModule::ThreadFunction(void *arg) {
        /* This is only necessary on 9.x+ */
        if (hos::GetVersion() < hos::Version_9_0_0) {
            return;
        }

        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* hid mitm was a temporary solution for compatibility. */
        /* Unless we are configured to continue doing so, don't instantiate the mitm. */
        if (!ShouldMitmHidForCompability()) {
            return;
        }

        /* Create hid mitm. */
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<IHidMitmInterface, HidMitmService>(MitmServiceName)));

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();
    }

}
