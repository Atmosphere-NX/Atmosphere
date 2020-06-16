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
#include "fs_common.hpp"

namespace ams::fs {

    using SaveDataId           = u64;
    using SystemSaveDataId     = u64;
    using SystemBcatSaveDataId = SystemSaveDataId;

    enum class SaveDataSpaceId : u8 {
        System       = 0,
        User         = 1,
        SdSystem     = 2,
        Temporary    = 3,
        SdUser       = 4,

        ProperSystem = 100,
        SafeMode     = 101,
    };

    enum class SaveDataType : u8 {
        System     = 0,
        Account    = 1,
        Bcat       = 2,
        Device     = 3,
        Temporary  = 4,
        Cache      = 5,
        SystemBcat = 6,
    };

    enum class SaveDataRank : u8 {
        Primary   = 0,
        Secondary = 1,
    };

    struct UserId {
        u64 data[2];
    };
    static_assert(util::is_pod<UserId>::value);

    constexpr inline bool operator<(const UserId &lhs, const UserId &rhs) {
        if (lhs.data[0] < rhs.data[0]) {
            return true;
        } else if (lhs.data[0] == rhs.data[0] && lhs.data[1] < rhs.data[1]) {
            return true;
        } else {
            return false;
        }
    }

    constexpr inline bool operator==(const UserId &lhs, const UserId &rhs) {
        return lhs.data[0] == rhs.data[0] && lhs.data[1] == rhs.data[1];
    }

    constexpr inline bool operator!=(const UserId &lhs, const UserId &rhs) {
        return !(lhs == rhs);
    }

    constexpr inline SystemSaveDataId InvalidSystemSaveDataId = 0;
    constexpr inline UserId InvalidUserId = {};

    enum SaveDataFlags : u32 {
        SaveDataFlags_None                                                = (0 << 0),
        SaveDataFlags_KeepAfterResettingSystemSaveData                    = (1 << 0),
        SaveDataFlags_KeepAfterRefurbishment                              = (1 << 1),
        SaveDataFlags_KeepAfterResettingSystemSaveDataWithoutUserSaveData = (1 << 2),
        SaveDataFlags_NeedsSecureDelete                                   = (1 << 3),
    };

    struct SaveDataCreationInfo {
        s64 size;
        s64 journal_size;
        s64 block_size;
        u64 owner_id;
        u32 flags;
        SaveDataSpaceId space_id;
        bool pseudo;
        u8 reserved[0x1A];
    };
    static_assert(util::is_pod<SaveDataCreationInfo>::value);
    static_assert(sizeof(SaveDataCreationInfo) == 0x40);

    struct SaveDataAttribute {
        ncm::ProgramId program_id;
        UserId user_id;
        SystemSaveDataId system_save_data_id;
        SaveDataType type;
        SaveDataRank rank;
        u16 index;
        u8 reserved[0x1C];

        static constexpr SaveDataAttribute Make(ncm::ProgramId program_id, SaveDataType type, UserId user_id, SystemSaveDataId system_save_data_id, u16 index, SaveDataRank rank) {
            return {
                .program_id          = program_id,
                .user_id             = user_id,
                .system_save_data_id = system_save_data_id,
                .type                = type,
                .rank                = rank,
                .index               = index,
            };
        }

        static constexpr SaveDataAttribute Make(ncm::ProgramId program_id, SaveDataType type, UserId user_id, SystemSaveDataId system_save_data_id, u16 index) {
            return Make(program_id, type, user_id, system_save_data_id, index, SaveDataRank::Primary);
        }

        static constexpr SaveDataAttribute Make(ncm::ProgramId program_id, SaveDataType type, UserId user_id, SystemSaveDataId system_save_data_id) {
            return Make(program_id, type, user_id, system_save_data_id, 0, SaveDataRank::Primary);
        }
    };
    static_assert(sizeof(SaveDataAttribute) == 0x40);
    static_assert(std::is_trivially_destructible<SaveDataAttribute>::value);

    constexpr inline bool operator<(const SaveDataAttribute &lhs, const SaveDataAttribute &rhs) {
        return std::tie(lhs.program_id, lhs.user_id, lhs.system_save_data_id, lhs.index, lhs.rank) <
               std::tie(rhs.program_id, rhs.user_id, rhs.system_save_data_id, rhs.index, rhs.rank);
    }

    constexpr inline bool operator==(const SaveDataAttribute &lhs, const SaveDataAttribute &rhs) {
        return std::tie(lhs.program_id, lhs.user_id, lhs.system_save_data_id, lhs.type, lhs.rank, lhs.index) ==
               std::tie(rhs.program_id, rhs.user_id, rhs.system_save_data_id, rhs.type, rhs.rank, rhs.index);
    }

    constexpr inline bool operator!=(const SaveDataAttribute &lhs, const SaveDataAttribute &rhs) {
        return !(lhs == rhs);
    }

    constexpr inline size_t DefaultSaveDataBlockSize = 16_KB;

    struct SaveDataExtraData {
        SaveDataAttribute attr;
        u64 owner_id;
        s64 timestamp;
        u32 flags;
        u8 pad[4];
        s64 available_size;
        s64 journal_size;
        s64 commit_id;
        u8 unused[0x190];
    };
    static_assert(sizeof(SaveDataExtraData) == 0x200);
    static_assert(util::is_pod<SaveDataExtraData>::value);

    struct HashSalt {
        static constexpr size_t Size = 32;

        u8 value[Size];
    };
    static_assert(util::is_pod<HashSalt>::value);
    static_assert(sizeof(HashSalt) == HashSalt::Size);

    using SaveDataHashSalt = std::optional<HashSalt>;

}
