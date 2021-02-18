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
#include "uartmitm_module.hpp"
#include "uart_mitm_service.hpp"
#include "uart_mitm_logger.hpp"

namespace ams::mitm::uart {

    namespace {

        enum PortIndex {
            PortIndex_Mitm,
            PortIndex_Count,
        };

        constexpr sm::ServiceName UartMitmServiceName = sm::ServiceName::Encode("uart");

        struct ServerOptions {
            static constexpr size_t PointerBufferSize = 0x1000;
            static constexpr size_t MaxDomains = 0;
            static constexpr size_t MaxDomainObjects = 0;
        };

        constexpr size_t MaxServers = 1;
        constexpr size_t MaxSessions = 10;

        class ServerManager final : public sf::hipc::ServerManager<MaxServers, ServerOptions, MaxSessions> {
            private:
                virtual Result OnNeedsToAccept(int port_index, Server *server) override;
        };

        ServerManager g_server_manager;

        Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
            /* Acknowledge the mitm session. */
            std::shared_ptr<::Service> fsrv;
            sm::MitmProcessInfo client_info;
            server->AcknowledgeMitmSession(std::addressof(fsrv), std::addressof(client_info));

            switch (port_index) {
                case PortIndex_Mitm:
                    return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<impl::IUartMitmInterface, UartMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        bool ShouldMitmUart() {
            u8 en = 0;
            if (settings::fwdbg::GetSettingsItemValue(&en, sizeof(en), "atmosphere", "enable_uart_mitm") == sizeof(en)) {
                return (en != 0);
            }
            return false;
        }
    }

    void MitmModule::ThreadFunction(void *arg) {
        /* The OpenPort cmds had the params changed with 6.x/7.x, so only support 7.x+. */
        if (hos::GetVersion() < hos::Version_7_0_0) {
            return;
        }

        /* Wait until initialization is complete. */
        mitm::WaitInitialized();

        /* Only use uart-mitm if enabled by the sys-setting. */
        if (!ShouldMitmUart()) {
            return;
        }

        /* Create mitm servers. */
        R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<UartMitmService>(PortIndex_Mitm, UartMitmServiceName)));

        mitm::uart::g_logger = std::make_shared<UartLogger>();

        /* Loop forever, servicing our services. */
        g_server_manager.LoopProcess();

        mitm::uart::g_logger.reset();
    }

}
