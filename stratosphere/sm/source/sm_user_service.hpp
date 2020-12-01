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
    class UserService final {
        private:
            os::ProcessId process_id = os::InvalidProcessId;
            bool has_initialized = false;
        private:
            Result EnsureInitialized();
        public:
            virtual ~UserService();
        public:
            /* Official commands. */
            Result RegisterClient(const sf::ClientProcessId &client_process_id);
            Result GetServiceHandle(sf::OutMoveHandle out_h, ServiceName service);
            Result RegisterService(sf::OutMoveHandle out_h, ServiceName service, u32 max_sessions, bool is_light);
            Result UnregisterService(ServiceName service);
            Result DetachClient(const sf::ClientProcessId &client_process_id);

            /* Atmosphere commands. */
            Result AtmosphereInstallMitm(sf::OutMoveHandle srv_h, sf::OutMoveHandle qry_h, ServiceName service);
            Result AtmosphereUninstallMitm(ServiceName service);
            Result AtmosphereAcknowledgeMitmSession(sf::Out<MitmProcessInfo> client_info, sf::OutMoveHandle fwd_h, ServiceName service);
            Result AtmosphereHasMitm(sf::Out<bool> out, ServiceName service);
            Result AtmosphereWaitMitm(ServiceName service);
            Result AtmosphereDeclareFutureMitm(ServiceName service);
            Result AtmosphereClearFutureMitm(ServiceName service);

            Result AtmosphereHasService(sf::Out<bool> out, ServiceName service);
            Result AtmosphereWaitService(ServiceName service);
    };
    static_assert(sm::impl::IsIUserInterface<UserService>);

}
