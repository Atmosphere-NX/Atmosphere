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
    class UserService {
        private:
            os::ProcessId m_process_id;
            bool m_initialized;
        public:
            constexpr UserService() : m_process_id{os::InvalidProcessId}, m_initialized{false} { /* ... */ }
            virtual ~UserService();
        private:
            Result EnsureInitialized();
        public:
            /* Official commands. */
            Result RegisterClient(const tipc::ClientProcessId client_process_id);
            Result GetServiceHandle(tipc::OutMoveHandle out_h, ServiceName service);
            Result RegisterService(tipc::OutMoveHandle out_h, ServiceName service, u32 max_sessions, bool is_light);
            Result UnregisterService(ServiceName service);
            Result DetachClient(const tipc::ClientProcessId client_process_id);

            /* Atmosphere commands. */
            Result AtmosphereInstallMitm(tipc::OutMoveHandle srv_h, tipc::OutMoveHandle qry_h, ServiceName service);
            Result AtmosphereUninstallMitm(ServiceName service);
            Result AtmosphereAcknowledgeMitmSession(tipc::Out<MitmProcessInfo> client_info, tipc::OutMoveHandle fwd_h, ServiceName service);
            Result AtmosphereHasMitm(tipc::Out<bool> out, ServiceName service);
            Result AtmosphereWaitMitm(ServiceName service);
            Result AtmosphereDeclareFutureMitm(ServiceName service);
            Result AtmosphereClearFutureMitm(ServiceName service);

            Result AtmosphereHasService(tipc::Out<bool> out, ServiceName service);
            Result AtmosphereWaitService(ServiceName service);
        public:
            /* Backwards compatibility layer for cmif. */
            Result ProcessDefaultServiceCommand(const svc::ipc::MessageBuffer &message_buffer);
    };
    static_assert(sm::impl::IsIUserInterface<UserService>);
    /* TODO: static assert that this is a tipc interface with default prototyping. */

}
