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

#include "pm_registration.hpp"

enum DmntCmd {
    Dmnt_Cmd_GetUnknownStub = 0,
    Dmnt_Cmd_GetDebugProcessIds = 1,
    Dmnt_Cmd_LaunchDebugProcess = 2,
    Dmnt_Cmd_GetTitleProcessId = 3,
    Dmnt_Cmd_EnableDebugForTitleId = 4,
    Dmnt_Cmd_GetApplicationProcessId = 5,
    Dmnt_Cmd_EnableDebugForApplication = 6,

    Dmnt_Cmd_AtmosphereGetProcessHandle = 65000,
    Dmnt_Cmd_AtmosphereGetCurrentLimitInfo = 65001,
};

enum DmntCmd_5X {
    Dmnt_Cmd_5X_GetDebugProcessIds = 0,
    Dmnt_Cmd_5X_LaunchDebugProcess = 1,
    Dmnt_Cmd_5X_GetTitleProcessId = 2,
    Dmnt_Cmd_5X_EnableDebugForTitleId = 3,
    Dmnt_Cmd_5X_GetApplicationProcessId = 4,
    Dmnt_Cmd_5X_EnableDebugForApplication = 5,
    
    Dmnt_Cmd_6X_DisableDebug = 6,

    Dmnt_Cmd_5X_AtmosphereGetProcessHandle = 65000,
    Dmnt_Cmd_5X_AtmosphereGetCurrentLimitInfo = 65001,
};

class DebugMonitorService final : public IServiceObject {
    public:
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) override;
        Result handle_deferred() override;
        
        DebugMonitorService *clone() override {
            return new DebugMonitorService(*this);
        }
        
    private:
        /* Actual commands. */
        std::tuple<Result, u32> get_unknown_stub(u64 unknown, OutBuffer<u8> out_unknown);
        std::tuple<Result, u32> get_debug_process_ids(OutBuffer<u64> out_processes);
        std::tuple<Result> launch_debug_process(u64 pid);
        std::tuple<Result, u64> get_title_process_id(u64 tid);
        std::tuple<Result, CopiedHandle> enable_debug_for_tid(u64 tid);
        std::tuple<Result, u64> get_application_process_id();
        std::tuple<Result, CopiedHandle> enable_debug_for_application();
        std::tuple<Result> disable_debug(u32 which);

        /* Atmosphere commands. */
        std::tuple<Result, CopiedHandle> get_process_handle(u64 pid);
        std::tuple<Result, u64, u64> get_current_limit_info(u32 category, u32 resource);
};
