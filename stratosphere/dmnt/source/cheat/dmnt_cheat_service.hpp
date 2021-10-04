/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>

/* TODO: In libstratosphere, eventually? */
#define AMS_DMNT_I_CHEAT_INTERFACE_INTERFACE_INFO(C, H)                                                                                                                                                        \
    AMS_SF_METHOD_INFO(C, H, 65000, void,   HasCheatProcess,             (sf::Out<bool> out),                                                                                  (out))                          \
    AMS_SF_METHOD_INFO(C, H, 65001, void,   GetCheatProcessEvent,        (sf::OutCopyHandle out_event),                                                                        (out_event))                    \
    AMS_SF_METHOD_INFO(C, H, 65002, Result, GetCheatProcessMetadata,     (sf::Out<dmnt::cheat::CheatProcessMetadata> out_metadata),                                            (out_metadata))                 \
    AMS_SF_METHOD_INFO(C, H, 65003, Result, ForceOpenCheatProcess,       (),                                                                                                   ())                             \
    AMS_SF_METHOD_INFO(C, H, 65004, Result, PauseCheatProcess,           (),                                                                                                   ())                             \
    AMS_SF_METHOD_INFO(C, H, 65005, Result, ResumeCheatProcess,          (),                                                                                                   ())                             \
    AMS_SF_METHOD_INFO(C, H, 65006, Result, ForceCloseCheatProcess,      (),                                                                                                   ())                             \
    AMS_SF_METHOD_INFO(C, H, 65100, Result, GetCheatProcessMappingCount, (sf::Out<u64> out_count),                                                                             (out_count))                    \
    AMS_SF_METHOD_INFO(C, H, 65101, Result, GetCheatProcessMappings,     (const sf::OutArray<svc::MemoryInfo> &mappings, sf::Out<u64> out_count, u64 offset),                  (mappings, out_count, offset))  \
    AMS_SF_METHOD_INFO(C, H, 65102, Result, ReadCheatProcessMemory,      (const sf::OutBuffer &buffer, u64 address, u64 out_size),                                             (buffer, address, out_size))    \
    AMS_SF_METHOD_INFO(C, H, 65103, Result, WriteCheatProcessMemory,     (const sf::InBuffer &buffer, u64 address, u64 in_size),                                               (buffer, address, in_size))     \
    AMS_SF_METHOD_INFO(C, H, 65104, Result, QueryCheatProcessMemory,     (sf::Out<svc::MemoryInfo> mapping, u64 address),                                                      (mapping, address))             \
    AMS_SF_METHOD_INFO(C, H, 65200, Result, GetCheatCount,               (sf::Out<u64> out_count),                                                                             (out_count))                    \
    AMS_SF_METHOD_INFO(C, H, 65201, Result, GetCheats,                   (const sf::OutArray<dmnt::cheat::CheatEntry> &cheats, sf::Out<u64> out_count, u64 offset),            (cheats, out_count, offset))    \
    AMS_SF_METHOD_INFO(C, H, 65202, Result, GetCheatById,                (sf::Out<dmnt::cheat::CheatEntry> cheat, u32 cheat_id),                                               (cheat, cheat_id))              \
    AMS_SF_METHOD_INFO(C, H, 65203, Result, ToggleCheat,                 (u32 cheat_id),                                                                                       (cheat_id))                     \
    AMS_SF_METHOD_INFO(C, H, 65204, Result, AddCheat,                    (const dmnt::cheat::CheatDefinition &cheat, sf::Out<u32> out_cheat_id, bool enabled),                 (cheat, out_cheat_id, enabled)) \
    AMS_SF_METHOD_INFO(C, H, 65205, Result, RemoveCheat,                 (u32 cheat_id),                                                                                       (cheat_id))                     \
    AMS_SF_METHOD_INFO(C, H, 65206, Result, ReadStaticRegister,          (sf::Out<u64> out, u8 which),                                                                         (out, which))                   \
    AMS_SF_METHOD_INFO(C, H, 65207, Result, WriteStaticRegister,         (u8 which, u64 value),                                                                                (which, value))                 \
    AMS_SF_METHOD_INFO(C, H, 65208, Result, ResetStaticRegisters,        (),                                                                                                   ())                             \
    AMS_SF_METHOD_INFO(C, H, 65209, Result, SetMasterCheat,              (const dmnt::cheat::CheatDefinition &cheat),                                                          (cheat))                        \
    AMS_SF_METHOD_INFO(C, H, 65300, Result, GetFrozenAddressCount,       (sf::Out<u64> out_count),                                                                             (out_count))                    \
    AMS_SF_METHOD_INFO(C, H, 65301, Result, GetFrozenAddresses,          (const sf::OutArray<dmnt::cheat::FrozenAddressEntry> &addresses, sf::Out<u64> out_count, u64 offset), (addresses, out_count, offset)) \
    AMS_SF_METHOD_INFO(C, H, 65302, Result, GetFrozenAddress,            (sf::Out<dmnt::cheat::FrozenAddressEntry> entry, u64 address),                                        (entry, address))               \
    AMS_SF_METHOD_INFO(C, H, 65303, Result, EnableFrozenAddress,         (sf::Out<u64> out_value, u64 address, u64 width),                                                     (out_value, address, width))    \
    AMS_SF_METHOD_INFO(C, H, 65304, Result, DisableFrozenAddress,        (u64 address),                                                                                        (address))

AMS_SF_DEFINE_INTERFACE(ams::dmnt::cheat::impl, ICheatInterface, AMS_DMNT_I_CHEAT_INTERFACE_INTERFACE_INFO)

namespace ams::dmnt::cheat {

    class CheatService {
        public:
            void HasCheatProcess(sf::Out<bool> out);
            void GetCheatProcessEvent(sf::OutCopyHandle out_event);
            Result GetCheatProcessMetadata(sf::Out<CheatProcessMetadata> out_metadata);
            Result ForceOpenCheatProcess();
            Result PauseCheatProcess();
            Result ResumeCheatProcess();
            Result ForceCloseCheatProcess();

            Result GetCheatProcessMappingCount(sf::Out<u64> out_count);
            Result GetCheatProcessMappings(const sf::OutArray<svc::MemoryInfo> &mappings, sf::Out<u64> out_count, u64 offset);
            Result ReadCheatProcessMemory(const sf::OutBuffer &buffer, u64 address, u64 out_size);
            Result WriteCheatProcessMemory(const sf::InBuffer &buffer, u64 address, u64 in_size);
            Result QueryCheatProcessMemory(sf::Out<svc::MemoryInfo> mapping, u64 address);

            Result GetCheatCount(sf::Out<u64> out_count);
            Result GetCheats(const sf::OutArray<CheatEntry> &cheats, sf::Out<u64> out_count, u64 offset);
            Result GetCheatById(sf::Out<CheatEntry> cheat, u32 cheat_id);
            Result ToggleCheat(u32 cheat_id);
            Result AddCheat(const CheatDefinition &cheat, sf::Out<u32> out_cheat_id, bool enabled);
            Result RemoveCheat(u32 cheat_id);
            Result ReadStaticRegister(sf::Out<u64> out, u8 which);
            Result WriteStaticRegister(u8 which, u64 value);
            Result ResetStaticRegisters();
            Result SetMasterCheat(const CheatDefinition &cheat);

            Result GetFrozenAddressCount(sf::Out<u64> out_count);
            Result GetFrozenAddresses(const sf::OutArray<FrozenAddressEntry> &addresses, sf::Out<u64> out_count, u64 offset);
            Result GetFrozenAddress(sf::Out<FrozenAddressEntry> entry, u64 address);
            Result EnableFrozenAddress(sf::Out<u64> out_value, u64 address, u64 width);
            Result DisableFrozenAddress(u64 address);
    };
    static_assert(impl::IsICheatInterface<CheatService>);

}
