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

enum DebugMonitorServiceCmd {
    Dmnt_Cmd_AddTitleToLaunchQueue = 0,
    Dmnt_Cmd_ClearLaunchQueue = 1,
    Dmnt_Cmd_GetProcessModuleInfo = 2
};

class DebugMonitorService final : public IServiceObject {        
    private:
        /* Actual commands. */
        Result AddTitleToLaunchQueue(u64 tid, InPointer<char> args, u32 args_size);
        void ClearLaunchQueue();
        Result GetProcessModuleInfo(Out<u32> count, OutPointerWithClientSize<LoaderModuleInfo> out, u64 pid);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Dmnt_Cmd_AddTitleToLaunchQueue, &DebugMonitorService::AddTitleToLaunchQueue>(),
            MakeServiceCommandMeta<Dmnt_Cmd_ClearLaunchQueue, &DebugMonitorService::ClearLaunchQueue>(),
            MakeServiceCommandMeta<Dmnt_Cmd_GetProcessModuleInfo, &DebugMonitorService::GetProcessModuleInfo>(),
        };
};
