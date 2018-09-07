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
#include <stratosphere/iserviceobject.hpp>

enum BootModeCmd {
    BootMode_Cmd_GetBootMode = 0,
    BootMode_Cmd_SetMaintenanceBoot = 1
};

class BootModeService final : public IServiceObject {
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override;
        Result handle_deferred() override;
        
        BootModeService *clone() override {
            return new BootModeService(*this);
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result, bool> get_boot_mode();
        std::tuple<Result> set_maintenance_boot();
};
