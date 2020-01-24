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

#pragma once
#include <stratosphere.hpp>

namespace ams::sm {

    /* Service definition. */
    class UserService final : public sf::IServiceObject {
        protected:
            /* Command IDs. */
            enum class CommandId {
                Initialize                       = 0,
                GetService                       = 1,
                RegisterService                  = 2,
                UnregisterService                = 3,

                AtmosphereInstallMitm            = 65000,
                AtmosphereUninstallMitm          = 65001,
                /* Deprecated: AtmosphereAssociatePidTidForMitm = 65002 */
                AtmosphereAcknowledgeMitmSession = 65003,
                AtmosphereHasMitm                = 65004,
                AtmosphereWaitMitm               = 65005,
                AtmosphereDeclareFutureMitm      = 65006,

                AtmosphereHasService             = 65100,
                AtmosphereWaitService            = 65101,
            };
        private:
            os::ProcessId process_id = os::InvalidProcessId;
            bool has_initialized = false;
        private:
            Result EnsureInitialized();
        public:
            /* Official commands. */
            virtual Result Initialize(const sf::ClientProcessId &client_process_id);
            virtual Result GetService(sf::OutMoveHandle out_h, ServiceName service);
            virtual Result RegisterService(sf::OutMoveHandle out_h, ServiceName service, u32 max_sessions, bool is_light);
            virtual Result UnregisterService(ServiceName service);

            /* Atmosphere commands. */
            virtual Result AtmosphereInstallMitm(sf::OutMoveHandle srv_h, sf::OutMoveHandle qry_h, ServiceName service);
            virtual Result AtmosphereUninstallMitm(ServiceName service);
            virtual Result AtmosphereAcknowledgeMitmSession(sf::Out<MitmProcessInfo> client_info, sf::OutMoveHandle fwd_h, ServiceName service);
            virtual Result AtmosphereHasMitm(sf::Out<bool> out, ServiceName service);
            virtual Result AtmosphereWaitMitm(ServiceName service);
            virtual Result AtmosphereDeclareFutureMitm(ServiceName service);

            virtual Result AtmosphereHasService(sf::Out<bool> out, ServiceName service);
            virtual Result AtmosphereWaitService(ServiceName service);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(Initialize),
                MAKE_SERVICE_COMMAND_META(GetService),
                MAKE_SERVICE_COMMAND_META(RegisterService),
                MAKE_SERVICE_COMMAND_META(UnregisterService),

                MAKE_SERVICE_COMMAND_META(AtmosphereInstallMitm),
                MAKE_SERVICE_COMMAND_META(AtmosphereUninstallMitm),
                MAKE_SERVICE_COMMAND_META(AtmosphereAcknowledgeMitmSession),
                MAKE_SERVICE_COMMAND_META(AtmosphereHasMitm),
                MAKE_SERVICE_COMMAND_META(AtmosphereWaitMitm),
                MAKE_SERVICE_COMMAND_META(AtmosphereDeclareFutureMitm),

                MAKE_SERVICE_COMMAND_META(AtmosphereHasService),
                MAKE_SERVICE_COMMAND_META(AtmosphereWaitService),
            };
    };

}
