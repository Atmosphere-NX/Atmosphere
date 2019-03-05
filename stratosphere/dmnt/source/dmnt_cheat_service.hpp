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

#include "dmnt_cheat_types.hpp"

enum DmntCheatCmd {
    /* Meta */
    DmntCheat_Cmd_HasCheatProcess = 65000,
    DmntCheat_Cmd_GetCheatProcessEvent = 65001,
    DmntCheat_Cmd_GetCheatProcessMetadata = 65002,
    DmntCheat_Cmd_ForceOpenCheatProcess = 65003,
    
    /* Interact with Memory */
    DmntCheat_Cmd_GetCheatProcessMappingCount = 65100,
    DmntCheat_Cmd_GetCheatProcessMappings = 65101,
    DmntCheat_Cmd_ReadCheatProcessMemory = 65102,
    DmntCheat_Cmd_WriteCheatProcessMemory = 65103,
    DmntCheat_Cmd_QueryCheatProcessMemory = 65104,
    
    /* Interact with Cheats */
    DmntCheat_Cmd_GetCheatCount = 65200,
    DmntCheat_Cmd_GetCheats = 65201,
    DmntCheat_Cmd_GetCheatById = 65202,
    DmntCheat_Cmd_ToggleCheat = 65203,
    DmntCheat_Cmd_AddCheat = 65204,
    DmntCheat_Cmd_RemoveCheat = 65205,
    
    /* Interact with Frozen Addresses */
    DmntCheat_Cmd_GetFrozenAddressCount = 65300,
    DmntCheat_Cmd_GetFrozenAddresses = 65301,
    DmntCheat_Cmd_GetFrozenAddress = 65302,
    DmntCheat_Cmd_EnableFrozenAddress = 65303,
    DmntCheat_Cmd_DisableFrozenAddress = 65304,
};

class DmntCheatService final : public IServiceObject {
    private:
        void HasCheatProcess(Out<bool> out);
        void GetCheatProcessEvent(Out<CopiedHandle> out_event);
        Result GetCheatProcessMetadata(Out<CheatProcessMetadata> out_metadata);
        Result ForceOpenCheatProcess();
        
        Result GetCheatProcessMappingCount(Out<u64> out_count);
        Result GetCheatProcessMappings(OutBuffer<MemoryInfo> mappings, Out<u64> out_count, u64 offset);
        Result ReadCheatProcessMemory(OutBuffer<u8> buffer, u64 address, u64 out_size);
        Result WriteCheatProcessMemory(InBuffer<u8> buffer, u64 address, u64 in_size);
        Result QueryCheatProcessMemory(Out<MemoryInfo> mapping, u64 address);
        
        Result GetCheatCount(Out<u64> out_count);
        Result GetCheats(OutBuffer<CheatEntry> cheats, Out<u64> out_count, u64 offset);
        Result GetCheatById(OutBuffer<CheatEntry> cheat, u32 cheat_id);
        Result ToggleCheat(u32 cheat_id);
        Result AddCheat(InBuffer<CheatDefinition> cheat, Out<u32> out_cheat_id, bool enabled);
        Result RemoveCheat(u32 cheat_id);
        
        Result GetFrozenAddressCount(Out<u64> out_count);
        Result GetFrozenAddresses(OutBuffer<FrozenAddressEntry> addresses, Out<u64> out_count, u64 offset);
        Result GetFrozenAddress(Out<FrozenAddressEntry> entry, u64 address);
        Result EnableFrozenAddress(Out<u64> out_value, u64 address, u64 width);
        Result DisableFrozenAddress(u64 address);

    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<DmntCheat_Cmd_HasCheatProcess, &DmntCheatService::HasCheatProcess>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_GetCheatProcessEvent, &DmntCheatService::GetCheatProcessEvent>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_GetCheatProcessMetadata, &DmntCheatService::GetCheatProcessMetadata>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_ForceOpenCheatProcess, &DmntCheatService::ForceOpenCheatProcess>(),

            MakeServiceCommandMeta<DmntCheat_Cmd_GetCheatProcessMappingCount, &DmntCheatService::GetCheatProcessMappingCount>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_GetCheatProcessMappings, &DmntCheatService::GetCheatProcessMappings>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_ReadCheatProcessMemory, &DmntCheatService::ReadCheatProcessMemory>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_WriteCheatProcessMemory, &DmntCheatService::WriteCheatProcessMemory>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_QueryCheatProcessMemory, &DmntCheatService::QueryCheatProcessMemory>(),

            MakeServiceCommandMeta<DmntCheat_Cmd_GetCheatCount, &DmntCheatService::GetCheatCount>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_GetCheats, &DmntCheatService::GetCheats>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_GetCheatById, &DmntCheatService::GetCheatById>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_ToggleCheat, &DmntCheatService::ToggleCheat>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_AddCheat, &DmntCheatService::AddCheat>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_RemoveCheat, &DmntCheatService::RemoveCheat>(),

            MakeServiceCommandMeta<DmntCheat_Cmd_GetFrozenAddressCount, &DmntCheatService::GetFrozenAddressCount>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_GetFrozenAddresses, &DmntCheatService::GetFrozenAddresses>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_GetFrozenAddress, &DmntCheatService::GetFrozenAddress>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_EnableFrozenAddress, &DmntCheatService::EnableFrozenAddress>(),
            MakeServiceCommandMeta<DmntCheat_Cmd_DisableFrozenAddress, &DmntCheatService::DisableFrozenAddress>(),
        };
};
