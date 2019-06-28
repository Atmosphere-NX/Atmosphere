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

#include "dmnt_cheat_types.hpp"

class DmntCheatService final : public IServiceObject {
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
            MAKE_SERVICE_COMMAND_META(DmntCheatService, HasCheatProcess),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetCheatProcessEvent),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetCheatProcessMetadata),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, ForceOpenCheatProcess),

            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetCheatProcessMappingCount),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetCheatProcessMappings),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, ReadCheatProcessMemory),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, WriteCheatProcessMemory),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, QueryCheatProcessMemory),

            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetCheatCount),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetCheats),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetCheatById),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, ToggleCheat),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, AddCheat),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, RemoveCheat),

            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetFrozenAddressCount),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetFrozenAddresses),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, GetFrozenAddress),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, EnableFrozenAddress),
            MAKE_SERVICE_COMMAND_META(DmntCheatService, DisableFrozenAddress),
        };
};
