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
#include "dmnt_cheat_service.hpp"
#include "dmnt_cheat_manager.hpp"

/* ========================================================================================= */
/* ====================================  Meta Commands  ==================================== */
/* ========================================================================================= */

void DmntCheatService::HasCheatProcess(Out<bool> out) {
    out.SetValue(DmntCheatManager::GetHasActiveCheatProcess());
}

void DmntCheatService::GetCheatProcessEvent(Out<CopiedHandle> out_event) {
    out_event.SetValue(DmntCheatManager::GetCheatProcessEventHandle());
}

Result DmntCheatService::GetCheatProcessMetadata(Out<CheatProcessMetadata> out_metadata) {
    return DmntCheatManager::GetCheatProcessMetadata(out_metadata.GetPointer());
}

Result DmntCheatService::ForceOpenCheatProcess() {
    Result rc = DmntCheatManager::ForceOpenCheatProcess();
    
    if (R_FAILED(rc)) {
        rc = ResultDmntCheatNotAttached;
    }
    
    return rc;
}

/* ========================================================================================= */
/* ===================================  Memory Commands  =================================== */
/* ========================================================================================= */

Result DmntCheatService::GetCheatProcessMappingCount(Out<u64> out_count) {
    return DmntCheatManager::GetCheatProcessMappingCount(out_count.GetPointer());
}

Result DmntCheatService::GetCheatProcessMappings(OutBuffer<MemoryInfo> mappings, Out<u64> out_count, u64 offset) {
    if (mappings.buffer == nullptr) {
        return ResultDmntCheatNullBuffer;
    }
    
    return DmntCheatManager::GetCheatProcessMappings(mappings.buffer, mappings.num_elements, out_count.GetPointer(), offset);
}

Result DmntCheatService::ReadCheatProcessMemory(OutBuffer<u8> buffer, u64 address, u64 out_size) {
    if (buffer.buffer == nullptr) {
        return ResultDmntCheatNullBuffer;
    }
    
    u64 sz = out_size;
    if (buffer.num_elements < sz) {
        sz = buffer.num_elements;
    }
    
    return DmntCheatManager::ReadCheatProcessMemory(address, buffer.buffer, sz);
}

Result DmntCheatService::WriteCheatProcessMemory(InBuffer<u8> buffer, u64 address, u64 in_size) {
    if (buffer.buffer == nullptr) {
        return ResultDmntCheatNullBuffer;
    }
    
    u64 sz = in_size;
    if (buffer.num_elements < sz) {
        sz = buffer.num_elements;
    }
    
    return DmntCheatManager::WriteCheatProcessMemory(address, buffer.buffer, sz);
}

Result DmntCheatService::QueryCheatProcessMemory(Out<MemoryInfo> mapping, u64 address) {
    return DmntCheatManager::QueryCheatProcessMemory(mapping.GetPointer(), address);
}

/* ========================================================================================= */
/* ===================================  Cheat Commands  ==================================== */
/* ========================================================================================= */

Result DmntCheatService::GetCheatCount(Out<u64> out_count) {
    return DmntCheatManager::GetCheatCount(out_count.GetPointer());
}

Result DmntCheatService::GetCheats(OutBuffer<CheatEntry> cheats, Out<u64> out_count, u64 offset) {
    if (cheats.buffer == nullptr) {
        return ResultDmntCheatNullBuffer;
    }
    
    return DmntCheatManager::GetCheats(cheats.buffer, cheats.num_elements, out_count.GetPointer(), offset);
}

Result DmntCheatService::GetCheatById(OutBuffer<CheatEntry> cheat, u32 cheat_id) {
    if (cheat.buffer == nullptr) {
        return ResultDmntCheatNullBuffer;
    }
    
    if (cheat.num_elements < 1) {
        return ResultDmntCheatInvalidBuffer;
    }
    
    return DmntCheatManager::GetCheatById(cheat.buffer, cheat_id);
}

Result DmntCheatService::ToggleCheat(u32 cheat_id) {
    return DmntCheatManager::ToggleCheat(cheat_id);
}

Result DmntCheatService::AddCheat(InBuffer<CheatDefinition> cheat, Out<u32> out_cheat_id, bool enabled) {
    if (cheat.buffer == nullptr) {
        return ResultDmntCheatNullBuffer;
    }
    
    if (cheat.num_elements < 1) {
        return ResultDmntCheatInvalidBuffer;
    }
    
    return DmntCheatManager::AddCheat(out_cheat_id.GetPointer(), cheat.buffer, enabled);
}

Result DmntCheatService::RemoveCheat(u32 cheat_id) {
    return DmntCheatManager::RemoveCheat(cheat_id);
}

/* ========================================================================================= */
/* ===================================  Address Commands  ================================== */
/* ========================================================================================= */

Result DmntCheatService::GetFrozenAddressCount(Out<u64> out_count) {
    return DmntCheatManager::GetFrozenAddressCount(out_count.GetPointer());
}

Result DmntCheatService::GetFrozenAddresses(OutBuffer<FrozenAddressEntry> frz_addrs, Out<u64> out_count, u64 offset) {
    if (frz_addrs.buffer == nullptr) {
        return ResultDmntCheatNullBuffer;
    }
    
    return DmntCheatManager::GetFrozenAddresses(frz_addrs.buffer, frz_addrs.num_elements, out_count.GetPointer(), offset);
}

Result DmntCheatService::GetFrozenAddress(Out<FrozenAddressEntry> entry, u64 address) {
    return DmntCheatManager::GetFrozenAddress(entry.GetPointer(), address);
}

Result DmntCheatService::EnableFrozenAddress(Out<u64> out_value, u64 address, u64 width) {
    switch (width) {
        case 1:
        case 2:
        case 4:
        case 8:
            break;
        default:
            return ResultDmntCheatInvalidFreezeWidth;
    }
    
    return DmntCheatManager::EnableFrozenAddress(out_value.GetPointer(), address, width);
}

Result DmntCheatService::DisableFrozenAddress(u64 address) {
    return DmntCheatManager::DisableFrozenAddress(address);
}
