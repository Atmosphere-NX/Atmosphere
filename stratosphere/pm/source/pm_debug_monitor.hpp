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

#include "pm_registration.hpp"

enum DmntCmd {
    Dmnt_Cmd_GetUnknownStub = 0,
    Dmnt_Cmd_GetDebugProcessIds = 1,
    Dmnt_Cmd_LaunchDebugProcess = 2,
    Dmnt_Cmd_GetTitleProcessId = 3,
    Dmnt_Cmd_EnableDebugForTitleId = 4,
    Dmnt_Cmd_GetApplicationProcessId = 5,
    Dmnt_Cmd_EnableDebugForApplication = 6,
    
    Dmnt_Cmd_5X_GetDebugProcessIds = 0,
    Dmnt_Cmd_5X_LaunchDebugProcess = 1,
    Dmnt_Cmd_5X_GetTitleProcessId = 2,
    Dmnt_Cmd_5X_EnableDebugForTitleId = 3,
    Dmnt_Cmd_5X_GetApplicationProcessId = 4,
    Dmnt_Cmd_5X_EnableDebugForApplication = 5,
    
    Dmnt_Cmd_6X_DisableDebug = 6,

    Dmnt_Cmd_AtmosphereGetProcessHandle = 65000,
    Dmnt_Cmd_AtmosphereGetCurrentLimitInfo = 65001,
};

class DebugMonitorService final : public IServiceObject {
    private:
        /* Actual commands. */
        Result GetUnknownStub(Out<u32> count, OutBuffer<u8> out_buf, u64 in_unk);
        Result GetDebugProcessIds(Out<u32> count, OutBuffer<u64> out_pids);
        Result LaunchDebugProcess(u64 pid);
        Result GetTitleProcessId(Out<u64> pid, u64 tid);
        Result EnableDebugForTitleId(Out<CopiedHandle> event, u64 tid);
        Result GetApplicationProcessId(Out<u64> pid);
        Result EnableDebugForApplication(Out<CopiedHandle> event);
        Result DisableDebug(u32 which);

        /* Atmosphere commands. */
        Result AtmosphereGetProcessHandle(Out<CopiedHandle> proc_hand, u64 pid);
        Result AtmosphereGetCurrentLimitInfo(Out<u64> cur_val, Out<u64> lim_val, u32 category, u32 resource);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0-4.1.0 */
            MakeServiceCommandMeta<Dmnt_Cmd_GetUnknownStub, &DebugMonitorService::GetUnknownStub, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Dmnt_Cmd_GetDebugProcessIds, &DebugMonitorService::GetDebugProcessIds, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Dmnt_Cmd_LaunchDebugProcess, &DebugMonitorService::LaunchDebugProcess, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Dmnt_Cmd_GetTitleProcessId, &DebugMonitorService::GetTitleProcessId, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Dmnt_Cmd_EnableDebugForTitleId, &DebugMonitorService::EnableDebugForTitleId, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Dmnt_Cmd_GetApplicationProcessId, &DebugMonitorService::GetApplicationProcessId, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Dmnt_Cmd_EnableDebugForApplication, &DebugMonitorService::EnableDebugForApplication, FirmwareVersion_Min, FirmwareVersion_400>(),
            
            /* 5.0.0-* */
            MakeServiceCommandMeta<Dmnt_Cmd_5X_GetDebugProcessIds, &DebugMonitorService::GetDebugProcessIds, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Dmnt_Cmd_5X_LaunchDebugProcess, &DebugMonitorService::LaunchDebugProcess, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Dmnt_Cmd_5X_GetTitleProcessId, &DebugMonitorService::GetTitleProcessId, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Dmnt_Cmd_5X_EnableDebugForTitleId, &DebugMonitorService::EnableDebugForTitleId, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Dmnt_Cmd_5X_GetApplicationProcessId, &DebugMonitorService::GetApplicationProcessId, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Dmnt_Cmd_5X_EnableDebugForApplication, &DebugMonitorService::EnableDebugForApplication, FirmwareVersion_500>(),
            
            /* 6.0.0-* */
            MakeServiceCommandMeta<Dmnt_Cmd_6X_DisableDebug, &DebugMonitorService::DisableDebug, FirmwareVersion_600>(),
            
            /* Atmosphere extensions. */
            MakeServiceCommandMeta<Dmnt_Cmd_AtmosphereGetProcessHandle, &DebugMonitorService::AtmosphereGetProcessHandle>(),
            MakeServiceCommandMeta<Dmnt_Cmd_AtmosphereGetCurrentLimitInfo, &DebugMonitorService::AtmosphereGetCurrentLimitInfo>(),
        };
};
