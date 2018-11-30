/*
 * Copyright (c) 2018 Atmosphère-NX
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

enum ManagerServiceCmd {
    Manager_Cmd_RegisterProcess = 0,
    Manager_Cmd_UnregisterProcess = 1,
    
    
    Manager_Cmd_AtmosphereEndInitDefers = 65000,
    Manager_Cmd_AtmosphereHasMitm = 65001,
};

class ManagerService final : public IServiceObject {
    private:
        /* Actual commands. */
        virtual Result RegisterProcess(u64 pid, InBuffer<u8> acid_sac, InBuffer<u8> aci0_sac);
        virtual Result UnregisterProcess(u64 pid);
        virtual void AtmosphereEndInitDefers();
        virtual void AtmosphereHasMitm(Out<bool> out, SmServiceName service);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Manager_Cmd_RegisterProcess, &ManagerService::RegisterProcess>(),
            MakeServiceCommandMeta<Manager_Cmd_UnregisterProcess, &ManagerService::UnregisterProcess>(),
            
            MakeServiceCommandMeta<Manager_Cmd_AtmosphereEndInitDefers, &ManagerService::AtmosphereEndInitDefers>(),
            MakeServiceCommandMeta<Manager_Cmd_AtmosphereHasMitm, &ManagerService::AtmosphereHasMitm>(),
        };
};
