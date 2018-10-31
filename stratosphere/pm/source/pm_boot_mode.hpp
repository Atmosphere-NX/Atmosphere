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

enum BootModeCmd {
    BootMode_Cmd_GetBootMode = 0,
    BootMode_Cmd_SetMaintenanceBoot = 1
};

class BootModeService final : public IServiceObject {        
    private:
        /* Actual commands. */
        void GetBootMode(Out<u32> out);
        void SetMaintenanceBoot();
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<BootMode_Cmd_GetBootMode, &BootModeService::GetBootMode>(),
            MakeServiceCommandMeta<BootMode_Cmd_SetMaintenanceBoot, &BootModeService::SetMaintenanceBoot>(),
        };
};
