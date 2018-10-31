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

enum ShellServiceCmd {
    Shell_Cmd_AddTitleToLaunchQueue = 0,
    Shell_Cmd_ClearLaunchQueue = 1,

    Shell_Cmd_AtmosphereSetExternalContentSource = 65000,
};

class ShellService final : public IServiceObject {
    private:
        /* Actual commands. */
        Result AddTitleToLaunchQueue(u64 tid, InPointer<char> args, u32 args_size);
        void ClearLaunchQueue();
        
        /* Atmosphere commands. */
        Result SetExternalContentSource(Out<MovedHandle> out, u64 tid);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<Shell_Cmd_AddTitleToLaunchQueue, &ShellService::AddTitleToLaunchQueue>(),
            MakeServiceCommandMeta<Shell_Cmd_ClearLaunchQueue, &ShellService::ClearLaunchQueue>(),
            MakeServiceCommandMeta<Shell_Cmd_AtmosphereSetExternalContentSource, &ShellService::SetExternalContentSource>(),
        };
};
