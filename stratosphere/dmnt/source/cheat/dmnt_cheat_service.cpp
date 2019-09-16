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

#include <switch.h>
#include "dmnt_cheat_service.hpp"
#include "impl/dmnt_cheat_api.hpp"

namespace sts::dmnt::cheat {

    /* ========================================================================================= */
    /* ====================================  Meta Commands  ==================================== */
    /* ========================================================================================= */

    void CheatService::HasCheatProcess(Out<bool> out) {
        out.SetValue(dmnt::cheat::impl::GetHasActiveCheatProcess());
    }

    void CheatService::GetCheatProcessEvent(Out<CopiedHandle> out_event) {
        out_event.SetValue(dmnt::cheat::impl::GetCheatProcessEventHandle());
    }

    Result CheatService::GetCheatProcessMetadata(Out<CheatProcessMetadata> out_metadata) {
        return dmnt::cheat::impl::GetCheatProcessMetadata(out_metadata.GetPointer());
    }

    Result CheatService::ForceOpenCheatProcess() {
        if (R_FAILED(dmnt::cheat::impl::ForceOpenCheatProcess())) {
            return ResultDmntCheatNotAttached;
        }

        return ResultSuccess;
    }

    /* ========================================================================================= */
    /* ===================================  Memory Commands  =================================== */
    /* ========================================================================================= */

    Result CheatService::GetCheatProcessMappingCount(Out<u64> out_count) {
        return dmnt::cheat::impl::GetCheatProcessMappingCount(out_count.GetPointer());
    }

    Result CheatService::GetCheatProcessMappings(OutBuffer<MemoryInfo> mappings, Out<u64> out_count, u64 offset) {
        if (mappings.buffer == nullptr) {
            return ResultDmntCheatNullBuffer;
        }

        return dmnt::cheat::impl::GetCheatProcessMappings(mappings.buffer, mappings.num_elements, out_count.GetPointer(), offset);
    }

    Result CheatService::ReadCheatProcessMemory(OutBuffer<u8> buffer, u64 address, u64 out_size) {
        if (buffer.buffer == nullptr) {
            return ResultDmntCheatNullBuffer;
        }

        return dmnt::cheat::impl::ReadCheatProcessMemory(address, buffer.buffer, std::min(out_size, buffer.num_elements));
    }

    Result CheatService::WriteCheatProcessMemory(InBuffer<u8> buffer, u64 address, u64 in_size) {
        if (buffer.buffer == nullptr) {
            return ResultDmntCheatNullBuffer;
        }

        return dmnt::cheat::impl::WriteCheatProcessMemory(address, buffer.buffer, std::min(in_size, buffer.num_elements));
    }

    Result CheatService::QueryCheatProcessMemory(Out<MemoryInfo> mapping, u64 address) {
        return dmnt::cheat::impl::QueryCheatProcessMemory(mapping.GetPointer(), address);
    }

    /* ========================================================================================= */
    /* ===================================  Cheat Commands  ==================================== */
    /* ========================================================================================= */

    Result CheatService::GetCheatCount(Out<u64> out_count) {
        return dmnt::cheat::impl::GetCheatCount(out_count.GetPointer());
    }

    Result CheatService::GetCheats(OutBuffer<CheatEntry> cheats, Out<u64> out_count, u64 offset) {
        if (cheats.buffer == nullptr) {
            return ResultDmntCheatNullBuffer;
        }

        return dmnt::cheat::impl::GetCheats(cheats.buffer, cheats.num_elements, out_count.GetPointer(), offset);
    }

    Result CheatService::GetCheatById(OutBuffer<CheatEntry> cheat, u32 cheat_id) {
        if (cheat.buffer == nullptr) {
            return ResultDmntCheatNullBuffer;
        }

        if (cheat.num_elements < 1) {
            return ResultDmntCheatInvalidBuffer;
        }

        return dmnt::cheat::impl::GetCheatById(cheat.buffer, cheat_id);
    }

    Result CheatService::ToggleCheat(u32 cheat_id) {
        return dmnt::cheat::impl::ToggleCheat(cheat_id);
    }

    Result CheatService::AddCheat(InBuffer<CheatDefinition> cheat, Out<u32> out_cheat_id, bool enabled) {
        if (cheat.buffer == nullptr) {
            return ResultDmntCheatNullBuffer;
        }

        if (cheat.num_elements < 1) {
            return ResultDmntCheatInvalidBuffer;
        }

        return dmnt::cheat::impl::AddCheat(out_cheat_id.GetPointer(), cheat.buffer, enabled);
    }

    Result CheatService::RemoveCheat(u32 cheat_id) {
        return dmnt::cheat::impl::RemoveCheat(cheat_id);
    }

    /* ========================================================================================= */
    /* ===================================  Address Commands  ================================== */
    /* ========================================================================================= */

    Result CheatService::GetFrozenAddressCount(Out<u64> out_count) {
        return dmnt::cheat::impl::GetFrozenAddressCount(out_count.GetPointer());
    }

    Result CheatService::GetFrozenAddresses(OutBuffer<FrozenAddressEntry> frz_addrs, Out<u64> out_count, u64 offset) {
        if (frz_addrs.buffer == nullptr) {
            return ResultDmntCheatNullBuffer;
        }

        return dmnt::cheat::impl::GetFrozenAddresses(frz_addrs.buffer, frz_addrs.num_elements, out_count.GetPointer(), offset);
    }

    Result CheatService::GetFrozenAddress(Out<FrozenAddressEntry> entry, u64 address) {
        return dmnt::cheat::impl::GetFrozenAddress(entry.GetPointer(), address);
    }

    Result CheatService::EnableFrozenAddress(Out<u64> out_value, u64 address, u64 width) {
        switch (width) {
            case 1:
            case 2:
            case 4:
            case 8:
                break;
            default:
                return ResultDmntCheatInvalidFreezeWidth;
        }

        return dmnt::cheat::impl::EnableFrozenAddress(out_value.GetPointer(), address, width);
    }

    Result CheatService::DisableFrozenAddress(u64 address) {
        return dmnt::cheat::impl::DisableFrozenAddress(address);
    }

}
