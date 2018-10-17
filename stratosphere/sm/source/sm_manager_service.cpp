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
#include "sm_manager_service.hpp"
#include "sm_registration.hpp"

Result ManagerService::dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) {
    Result rc = 0xF601;
    switch ((ManagerServiceCmd)cmd_id) {
        case Manager_Cmd_RegisterProcess:
            rc = WrapIpcCommandImpl<&ManagerService::register_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Manager_Cmd_UnregisterProcess:
            rc = WrapIpcCommandImpl<&ManagerService::unregister_process>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        case Manager_Cmd_AtmosphereEndInitDefers:
            rc = WrapIpcCommandImpl<&ManagerService::end_init_defers>(this, r, out_c, pointer_buffer, pointer_buffer_size);
            break;
        default:
            break;
    }
    return rc;
}

Result ManagerService::handle_deferred() {
    /* This service is never deferrable. */
    return 0;
}


std::tuple<Result> ManagerService::register_process(u64 pid, InBuffer<u8> acid_sac, InBuffer<u8> aci0_sac) {
    return {Registration::RegisterProcess(pid, acid_sac.buffer, acid_sac.num_elements, aci0_sac.buffer, aci0_sac.num_elements)};
}

std::tuple<Result> ManagerService::unregister_process(u64 pid) {
    return {Registration::UnregisterProcess(pid)};
}

std::tuple<Result> ManagerService::end_init_defers() {
    Registration::EndInitDefers();
    return {0};
}

