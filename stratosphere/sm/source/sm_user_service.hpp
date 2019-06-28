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

#pragma once
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/sm.hpp>

namespace sts::sm {

    /* Service definition. */
    class UserService final : public IServiceObject {
        protected:
            /* Command IDs. */
            enum class CommandId {
                Initialize                       = 0,
                GetService                       = 1,
                RegisterService                  = 2,
                UnregisterService                = 3,

                AtmosphereInstallMitm            = 65000,
                AtmosphereUninstallMitm          = 65001,
                AtmosphereAssociatePidTidForMitm = 65002,
                AtmosphereAcknowledgeMitmSession = 65003,
                AtmosphereHasMitm                = 65004,

                AtmosphereHasService             = 65100,
            };
        private:
            u64 pid = InvalidProcessId;
            bool has_initialized = false;
        private:
            Result EnsureInitialized();
        public:
            /* Official commands. */
            virtual Result Initialize(PidDescriptor pid);
            virtual Result GetService(Out<MovedHandle> out_h, ServiceName service);
            virtual Result RegisterService(Out<MovedHandle> out_h, ServiceName service, u32 max_sessions, bool is_light);
            virtual Result UnregisterService(ServiceName service);

            /* Atmosphere commands. */
            virtual Result AtmosphereInstallMitm(Out<MovedHandle> srv_h, Out<MovedHandle> qry_h, ServiceName service);
            virtual Result AtmosphereUninstallMitm(ServiceName service);
            virtual Result AtmosphereAssociatePidTidForMitm(u64 pid, u64 tid);
            virtual Result AtmosphereAcknowledgeMitmSession(Out<u64> client_pid, Out<MovedHandle> fwd_h, ServiceName service);
            virtual Result AtmosphereHasMitm(Out<bool> out, ServiceName service);

            virtual Result AtmosphereHasService(Out<bool> out, ServiceName service);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(UserService, Initialize),
                MAKE_SERVICE_COMMAND_META(UserService, GetService),
                MAKE_SERVICE_COMMAND_META(UserService, RegisterService),
                MAKE_SERVICE_COMMAND_META(UserService, UnregisterService),

                MAKE_SERVICE_COMMAND_META(UserService, AtmosphereInstallMitm),
                MAKE_SERVICE_COMMAND_META(UserService, AtmosphereUninstallMitm),
                MAKE_SERVICE_COMMAND_META(UserService, AtmosphereAssociatePidTidForMitm),
                MAKE_SERVICE_COMMAND_META(UserService, AtmosphereAcknowledgeMitmSession),
                MAKE_SERVICE_COMMAND_META(UserService, AtmosphereHasMitm),

                MAKE_SERVICE_COMMAND_META(UserService, AtmosphereHasService),
            };
    };

}
