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

#include "pm_registration.hpp"

enum ShellCmd {
    Shell_Cmd_LaunchProcess = 0,
    Shell_Cmd_TerminateProcessId = 1,
    Shell_Cmd_TerminateTitleId = 2,
    Shell_Cmd_GetProcessWaitEvent = 3,
    Shell_Cmd_GetProcessEventType = 4,
    Shell_Cmd_FinalizeExitedProcess = 5,
    Shell_Cmd_ClearProcessNotificationFlag = 6,
    Shell_Cmd_NotifyBootFinished = 7,
    Shell_Cmd_GetApplicationProcessId = 8,
    Shell_Cmd_BoostSystemMemoryResourceLimit = 9
};

enum ShellCmd_5X {
    Shell_Cmd_5X_LaunchProcess = 0,
    Shell_Cmd_5X_TerminateProcessId = 1,
    Shell_Cmd_5X_TerminateTitleId = 2,
    Shell_Cmd_5X_GetProcessWaitEvent = 3,
    Shell_Cmd_5X_GetProcessEventType = 4,
    Shell_Cmd_5X_NotifyBootFinished = 5,
    Shell_Cmd_5X_GetApplicationProcessId = 6,
    Shell_Cmd_5X_BoostSystemMemoryResourceLimit = 7,
    
    Shell_Cmd_BoostSystemThreadsResourceLimit = 8,
    Shell_Cmd_GetUnimplementedEventHandle = 9 /* TODO: Rename when Nintendo implements this. */
};

class ShellService final : public IServiceObject {    
    private:
        /* Actual commands. */
        Result LaunchProcess(Out<u64> pid, Registration::TidSid tid_sid, u32 launch_flags);
        Result TerminateProcessId(u64 pid);
        Result TerminateTitleId(u64 tid);
        void GetProcessWaitEvent(Out<CopiedHandle> event);
        void GetProcessEventType(Out<u64> type, Out<u64> pid);
        Result FinalizeExitedProcess(u64 pid);
        Result ClearProcessNotificationFlag(u64 pid);
        void NotifyBootFinished();
        Result GetApplicationProcessId(Out<u64> pid);
        Result BoostSystemMemoryResourceLimit(u64 sysmem_size);
        Result BoostSystemThreadsResourceLimit();
        Result GetUnimplementedEventHandle(Out<CopiedHandle> event);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0-4.0.0 */
            MakeServiceCommandMeta<Shell_Cmd_LaunchProcess, &ShellService::LaunchProcess, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Shell_Cmd_TerminateProcessId, &ShellService::TerminateProcessId, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Shell_Cmd_TerminateTitleId, &ShellService::TerminateTitleId, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Shell_Cmd_GetProcessWaitEvent, &ShellService::GetProcessWaitEvent, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Shell_Cmd_GetProcessEventType, &ShellService::GetProcessEventType, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Shell_Cmd_FinalizeExitedProcess, &ShellService::FinalizeExitedProcess, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Shell_Cmd_ClearProcessNotificationFlag, &ShellService::ClearProcessNotificationFlag, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Shell_Cmd_NotifyBootFinished, &ShellService::NotifyBootFinished, FirmwareVersion_Min, FirmwareVersion_400>(),
            MakeServiceCommandMeta<Shell_Cmd_GetApplicationProcessId, &ShellService::GetApplicationProcessId, FirmwareVersion_Min, FirmwareVersion_400>(),
            
            /* 4.0.0-4.0.0 */
            MakeServiceCommandMeta<Shell_Cmd_BoostSystemMemoryResourceLimit, &ShellService::BoostSystemMemoryResourceLimit, FirmwareVersion_400, FirmwareVersion_400>(),
            
            /* 5.0.0-* */
            MakeServiceCommandMeta<Shell_Cmd_5X_LaunchProcess, &ShellService::LaunchProcess, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Shell_Cmd_5X_TerminateProcessId, &ShellService::TerminateProcessId, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Shell_Cmd_5X_TerminateTitleId, &ShellService::TerminateTitleId, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Shell_Cmd_5X_GetProcessWaitEvent, &ShellService::GetProcessWaitEvent, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Shell_Cmd_5X_GetProcessEventType, &ShellService::GetProcessEventType, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Shell_Cmd_5X_NotifyBootFinished, &ShellService::NotifyBootFinished, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Shell_Cmd_5X_GetApplicationProcessId, &ShellService::GetApplicationProcessId, FirmwareVersion_500>(),
            MakeServiceCommandMeta<Shell_Cmd_5X_BoostSystemMemoryResourceLimit, &ShellService::BoostSystemMemoryResourceLimit, FirmwareVersion_500>(),
            
            /* 7.0.0-* */
            MakeServiceCommandMeta<Shell_Cmd_BoostSystemThreadsResourceLimit, &ShellService::BoostSystemThreadsResourceLimit, FirmwareVersion_700>(),
            
            /* 8.0.0-* */
            MakeServiceCommandMeta<Shell_Cmd_GetUnimplementedEventHandle, &ShellService::GetUnimplementedEventHandle, FirmwareVersion_800>(),
        };
};
