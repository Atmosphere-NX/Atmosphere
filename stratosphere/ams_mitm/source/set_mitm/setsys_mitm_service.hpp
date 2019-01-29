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

#include "setsys_shim.h"

enum SetSysCmd : u32 {
    SetSysCmd_GetFirmwareVersion = 3,
    SetSysCmd_GetFirmwareVersion2 = 4,
    SetSysCmd_GetSettingsItemValueSize = 37,
    SetSysCmd_GetSettingsItemValue = 38,
    
    /* Commands for which set:sys *must* act as a passthrough. */
    /* TODO: Solve the relevant IPC detection problem. */
    SetSysCmd_GetEdid = 41,
};

class SetSysMitmService : public IMitmServiceObject {      
    public:
        SetSysMitmService(std::shared_ptr<Service> s, u64 pid) : IMitmServiceObject(s, pid) {
            /* ... */
        }
        
        static bool ShouldMitm(u64 pid, u64 tid) {
            /* Mitm everything. */
            return true;
        }
        
        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);
                    
    protected:
        /* Overridden commands. */
        Result GetFirmwareVersion(OutPointerWithServerSize<SetSysFirmwareVersion, 0x1> out);
        Result GetFirmwareVersion2(OutPointerWithServerSize<SetSysFirmwareVersion, 0x1> out);
        Result GetSettingsItemValueSize(Out<u64> out_size, InPointer<char> name, InPointer<char> key);
        Result GetSettingsItemValue(Out<u64> out_size, OutBuffer<u8> out_value, InPointer<char> name, InPointer<char> key);
        
        /* Forced passthrough commands. */
        Result GetEdid(OutPointerWithServerSize<SetSysEdid, 0x1> out);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<SetSysCmd_GetFirmwareVersion, &SetSysMitmService::GetFirmwareVersion>(),
            MakeServiceCommandMeta<SetSysCmd_GetFirmwareVersion2, &SetSysMitmService::GetFirmwareVersion2>(),
            MakeServiceCommandMeta<SetSysCmd_GetSettingsItemValueSize, &SetSysMitmService::GetSettingsItemValueSize>(),
            MakeServiceCommandMeta<SetSysCmd_GetSettingsItemValue, &SetSysMitmService::GetSettingsItemValue>(),
            
            MakeServiceCommandMeta<SetSysCmd_GetEdid, &SetSysMitmService::GetEdid>(),
        };
};
