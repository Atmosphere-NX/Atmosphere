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
#pragma once
#include <stratosphere.hpp>

namespace ams::dmnt::cheat {

    class CheatService final : public sf::IServiceObject {
        private:
            enum class CommandId {
                /* Meta */
                HasCheatProcess         = 65000,
                GetCheatProcessEvent    = 65001,
                GetCheatProcessMetadata = 65002,
                ForceOpenCheatProcess   = 65003,

                /* Interact with Memory */
                GetCheatProcessMappingCount = 65100,
                GetCheatProcessMappings     = 65101,
                ReadCheatProcessMemory      = 65102,
                WriteCheatProcessMemory     = 65103,
                QueryCheatProcessMemory     = 65104,

                /* Interact with Cheats */
                GetCheatCount = 65200,
                GetCheats     = 65201,
                GetCheatById  = 65202,
                ToggleCheat   = 65203,
                AddCheat      = 65204,
                RemoveCheat   = 65205,

                /* Interact with Frozen Addresses */
                GetFrozenAddressCount = 65300,
                GetFrozenAddresses    = 65301,
                GetFrozenAddress      = 65302,
                EnableFrozenAddress   = 65303,
                DisableFrozenAddress  = 65304,
            };
        private:
            void HasCheatProcess(sf::Out<bool> out);
            void GetCheatProcessEvent(sf::OutCopyHandle out_event);
            Result GetCheatProcessMetadata(sf::Out<CheatProcessMetadata> out_metadata);
            Result ForceOpenCheatProcess();

            Result GetCheatProcessMappingCount(sf::Out<u64> out_count);
            Result GetCheatProcessMappings(const sf::OutArray<MemoryInfo> &mappings, sf::Out<u64> out_count, u64 offset);
            Result ReadCheatProcessMemory(const sf::OutBuffer &buffer, u64 address, u64 out_size);
            Result WriteCheatProcessMemory(const sf::InBuffer &buffer, u64 address, u64 in_size);
            Result QueryCheatProcessMemory(sf::Out<MemoryInfo> mapping, u64 address);

            Result GetCheatCount(sf::Out<u64> out_count);
            Result GetCheats(const sf::OutArray<CheatEntry> &cheats, sf::Out<u64> out_count, u64 offset);
            Result GetCheatById(sf::Out<CheatEntry> cheat, u32 cheat_id);
            Result ToggleCheat(u32 cheat_id);
            Result AddCheat(const CheatDefinition &cheat, sf::Out<u32> out_cheat_id, bool enabled);
            Result RemoveCheat(u32 cheat_id);

            Result GetFrozenAddressCount(sf::Out<u64> out_count);
            Result GetFrozenAddresses(const sf::OutArray<FrozenAddressEntry> &addresses, sf::Out<u64> out_count, u64 offset);
            Result GetFrozenAddress(sf::Out<FrozenAddressEntry> entry, u64 address);
            Result EnableFrozenAddress(sf::Out<u64> out_value, u64 address, u64 width);
            Result DisableFrozenAddress(u64 address);

        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(HasCheatProcess),
                MAKE_SERVICE_COMMAND_META(GetCheatProcessEvent),
                MAKE_SERVICE_COMMAND_META(GetCheatProcessMetadata),
                MAKE_SERVICE_COMMAND_META(ForceOpenCheatProcess),

                MAKE_SERVICE_COMMAND_META(GetCheatProcessMappingCount),
                MAKE_SERVICE_COMMAND_META(GetCheatProcessMappings),
                MAKE_SERVICE_COMMAND_META(ReadCheatProcessMemory),
                MAKE_SERVICE_COMMAND_META(WriteCheatProcessMemory),
                MAKE_SERVICE_COMMAND_META(QueryCheatProcessMemory),

                MAKE_SERVICE_COMMAND_META(GetCheatCount),
                MAKE_SERVICE_COMMAND_META(GetCheats),
                MAKE_SERVICE_COMMAND_META(GetCheatById),
                MAKE_SERVICE_COMMAND_META(ToggleCheat),
                MAKE_SERVICE_COMMAND_META(AddCheat),
                MAKE_SERVICE_COMMAND_META(RemoveCheat),

                MAKE_SERVICE_COMMAND_META(GetFrozenAddressCount),
                MAKE_SERVICE_COMMAND_META(GetFrozenAddresses),
                MAKE_SERVICE_COMMAND_META(GetFrozenAddress),
                MAKE_SERVICE_COMMAND_META(EnableFrozenAddress),
                MAKE_SERVICE_COMMAND_META(DisableFrozenAddress),
            };
    };

}
