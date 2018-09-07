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
 
#include <switch.h>
#include <stratosphere.hpp>
#include "pm_boot_mode.hpp"

static bool g_is_maintenance_boot = false;

Result BootModeService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
    
    switch ((BootModeCmd)cmd_id) {
        case BootMode_Cmd_GetBootMode:
            rc = WrapIpcCommandImpl<&BootModeService::get_boot_mode>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case BootMode_Cmd_SetMaintenanceBoot:
            rc = WrapIpcCommandImpl<&BootModeService::set_maintenance_boot>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    
    return rc;
}

Result BootModeService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}

std::tuple<Result, bool> BootModeService::get_boot_mode() {
    return {0, g_is_maintenance_boot};
}

std::tuple<Result> BootModeService::set_maintenance_boot() {
    g_is_maintenance_boot = true;
    return {0};
}
