/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <stratosphere.hpp>
#include "dmnt_cheat_service.hpp"
#include "impl/dmnt_cheat_api.hpp"

namespace ams::dmnt::cheat {

    /* ========================================================================================= */
    /* ====================================  Meta Commands  ==================================== */
    /* ========================================================================================= */

    void CheatService::HasCheatProcess(sf::Out<bool> out) {
        out.SetValue(dmnt::cheat::impl::GetHasActiveCheatProcess());
    }

    void CheatService::GetCheatProcessEvent(sf::OutCopyHandle out_event) {
        out_event.SetValue(dmnt::cheat::impl::GetCheatProcessEventHandle());
    }

    Result CheatService::GetCheatProcessMetadata(sf::Out<CheatProcessMetadata> out_metadata) {
        return dmnt::cheat::impl::GetCheatProcessMetadata(out_metadata.GetPointer());
    }

    Result CheatService::ForceOpenCheatProcess() {
        R_UNLESS(R_SUCCEEDED(dmnt::cheat::impl::ForceOpenCheatProcess()), ResultCheatNotAttached());
        return ResultSuccess();
    }

    Result CheatService::PauseCheatProcess() {
        return dmnt::cheat::impl::PauseCheatProcess();
    }

    Result CheatService::ResumeCheatProcess() {
        return dmnt::cheat::impl::ResumeCheatProcess();
    }

    /* ========================================================================================= */
    /* ===================================  Memory Commands  =================================== */
    /* ========================================================================================= */

    Result CheatService::GetCheatProcessMappingCount(sf::Out<u64> out_count) {
        return dmnt::cheat::impl::GetCheatProcessMappingCount(out_count.GetPointer());
    }

    Result CheatService::GetCheatProcessMappings(const sf::OutArray<MemoryInfo> &mappings, sf::Out<u64> out_count, u64 offset) {
        R_UNLESS(mappings.GetPointer() != nullptr, ResultCheatNullBuffer());
        return dmnt::cheat::impl::GetCheatProcessMappings(mappings.GetPointer(), mappings.GetSize(), out_count.GetPointer(), offset);
    }

    Result CheatService::ReadCheatProcessMemory(const sf::OutBuffer &buffer, u64 address, u64 out_size) {
        R_UNLESS(buffer.GetPointer() != nullptr, ResultCheatNullBuffer());
        return dmnt::cheat::impl::ReadCheatProcessMemory(address, buffer.GetPointer(), std::min(out_size, buffer.GetSize()));
    }

    Result CheatService::WriteCheatProcessMemory(const sf::InBuffer &buffer, u64 address, u64 in_size) {
        R_UNLESS(buffer.GetPointer() != nullptr, ResultCheatNullBuffer());
        return dmnt::cheat::impl::WriteCheatProcessMemory(address, buffer.GetPointer(), std::min(in_size, buffer.GetSize()));
    }

    Result CheatService::QueryCheatProcessMemory(sf::Out<MemoryInfo> mapping, u64 address) {
        return dmnt::cheat::impl::QueryCheatProcessMemory(mapping.GetPointer(), address);
    }

    /* ========================================================================================= */
    /* ===================================  Cheat Commands  ==================================== */
    /* ========================================================================================= */

    Result CheatService::GetCheatCount(sf::Out<u64> out_count) {
        return dmnt::cheat::impl::GetCheatCount(out_count.GetPointer());
    }

    Result CheatService::GetCheats(const sf::OutArray<CheatEntry> &cheats, sf::Out<u64> out_count, u64 offset) {
        R_UNLESS(cheats.GetPointer() != nullptr, ResultCheatNullBuffer());
        return dmnt::cheat::impl::GetCheats(cheats.GetPointer(), cheats.GetSize(), out_count.GetPointer(), offset);
    }

    Result CheatService::GetCheatById(sf::Out<CheatEntry> cheat, u32 cheat_id) {
        return dmnt::cheat::impl::GetCheatById(cheat.GetPointer(), cheat_id);
    }

    Result CheatService::ToggleCheat(u32 cheat_id) {
        return dmnt::cheat::impl::ToggleCheat(cheat_id);
    }

    Result CheatService::AddCheat(const CheatDefinition &cheat, sf::Out<u32> out_cheat_id, bool enabled) {
        return dmnt::cheat::impl::AddCheat(out_cheat_id.GetPointer(), cheat, enabled);
    }

    Result CheatService::RemoveCheat(u32 cheat_id) {
        return dmnt::cheat::impl::RemoveCheat(cheat_id);
    }

    Result CheatService::ReadStaticRegister(sf::Out<u64> out, u8 which) {
        return dmnt::cheat::impl::ReadStaticRegister(out.GetPointer(), which);
    }

    Result CheatService::WriteStaticRegister(u8 which, u64 value) {
        return dmnt::cheat::impl::WriteStaticRegister(which, value);
    }

    Result CheatService::ResetStaticRegisters() {
        return dmnt::cheat::impl::ResetStaticRegisters();
    }

    /* ========================================================================================= */
    /* ===================================  Address Commands  ================================== */
    /* ========================================================================================= */

    Result CheatService::GetFrozenAddressCount(sf::Out<u64> out_count) {
        return dmnt::cheat::impl::GetFrozenAddressCount(out_count.GetPointer());
    }

    Result CheatService::GetFrozenAddresses(const sf::OutArray<FrozenAddressEntry> &addresses, sf::Out<u64> out_count, u64 offset) {
        R_UNLESS(addresses.GetPointer() != nullptr, ResultCheatNullBuffer());
        return dmnt::cheat::impl::GetFrozenAddresses(addresses.GetPointer(), addresses.GetSize(), out_count.GetPointer(), offset);
    }

    Result CheatService::GetFrozenAddress(sf::Out<FrozenAddressEntry> entry, u64 address) {
        return dmnt::cheat::impl::GetFrozenAddress(entry.GetPointer(), address);
    }

    Result CheatService::EnableFrozenAddress(sf::Out<u64> out_value, u64 address, u64 width) {
        /* Width needs to be a power of two <= 8. */
        R_UNLESS(width > 0,                  ResultFrozenAddressInvalidWidth());
        R_UNLESS(width <= sizeof(u64),       ResultFrozenAddressInvalidWidth());
        R_UNLESS((width & (width - 1)) == 0, ResultFrozenAddressInvalidWidth());
        return dmnt::cheat::impl::EnableFrozenAddress(out_value.GetPointer(), address, width);
    }

    Result CheatService::DisableFrozenAddress(u64 address) {
        return dmnt::cheat::impl::DisableFrozenAddress(address);
    }

}
