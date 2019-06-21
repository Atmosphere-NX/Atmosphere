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
#include "sm_types.hpp"

namespace sts::sm {

    /* Command IDs. */
    enum UserServiceCmd {
        User_Cmd_Initialize = 0,
        User_Cmd_GetService = 1,
        User_Cmd_RegisterService = 2,
        User_Cmd_UnregisterService = 3,

        User_Cmd_AtmosphereInstallMitm = 65000,
        User_Cmd_AtmosphereUninstallMitm = 65001,
        User_Cmd_AtmosphereAssociatePidTidForMitm = 65002,
        User_Cmd_AtmosphereAcknowledgeMitmSession = 65003,
    };

    /* Service definition. */
    class UserService final : public IServiceObject {
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
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MakeServiceCommandMeta<User_Cmd_Initialize, &UserService::Initialize>(),
                MakeServiceCommandMeta<User_Cmd_GetService, &UserService::GetService>(),
                MakeServiceCommandMeta<User_Cmd_RegisterService, &UserService::RegisterService>(),
                MakeServiceCommandMeta<User_Cmd_UnregisterService, &UserService::UnregisterService>(),

                MakeServiceCommandMeta<User_Cmd_AtmosphereInstallMitm, &UserService::AtmosphereInstallMitm>(),
                MakeServiceCommandMeta<User_Cmd_AtmosphereUninstallMitm, &UserService::AtmosphereUninstallMitm>(),
                MakeServiceCommandMeta<User_Cmd_AtmosphereAssociatePidTidForMitm, &UserService::AtmosphereAssociatePidTidForMitm>(),
                MakeServiceCommandMeta<User_Cmd_AtmosphereAcknowledgeMitmSession, &UserService::AtmosphereAcknowledgeMitmSession>(),
            };
    };

}
