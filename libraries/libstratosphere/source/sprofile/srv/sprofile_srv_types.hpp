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

namespace ams::sprofile::srv {

    struct ProfileMetadataEntry {
        Identifier identifier_0;
        Identifier identifier_1;
        u8 unk_0E[0x32];
    };
    static_assert(util::is_pod<ProfileMetadataEntry>::value);
    static_assert(sizeof(ProfileMetadataEntry) == 0x40);

    constexpr inline const u32 ProfileMetadataVersion = 0;

    struct ProfileMetadata {
        u32 version;
        u32 num_entries;
        Identifier revision_key;
        u8 unk_10[0x30];
        ProfileMetadataEntry entries[50];
    };
    static_assert(util::is_pod<ProfileMetadata>::value);
    static_assert(sizeof(ProfileMetadata) == 0xCC0);

    struct ProfileMetadataForImportMetadata : public sf::LargeData, public sf::PrefersMapAliasTransferMode {
        ProfileMetadata metadata;
        u8 unk[0x8000 - sizeof(metadata)];
    };
    static_assert(util::is_pod<ProfileMetadataForImportMetadata>::value);
    static_assert(sizeof(ProfileMetadataForImportMetadata) == 0x8000);

    constexpr inline const u32 ProfileDataVersion = 0;

    enum ValueType : u8 {
        ValueType_Byte = 0,
        ValueType_U32  = 1,
        ValueType_S32  = 2,
        ValueType_U64  = 3,
        ValueType_S64  = 4,
    };

    struct ProfileDataEntry {
        Identifier key;
        ValueType type;
        union {
            s64 value_s64;
            u64 value_u64;
            s32 value_s32;
            u32 value_u32;
            u8 value_u8;
        };
    };
    static_assert(util::is_pod<ProfileDataEntry>::value);
    static_assert(sizeof(ProfileDataEntry) == 0x10);

    struct ProfileData {
        u32 version;
        u32 num_entries;
        u8 unk_08[0x28];
        ProfileDataEntry entries[(0x4000 - 0x30) / sizeof(ProfileDataEntry)];
    };
    static_assert(util::is_pod<ProfileData>::value);
    static_assert(sizeof(ProfileData) == 0x4000);

    struct ServiceProfile {
        Identifier name;
        ProfileData data;
    };
    static_assert(util::is_pod<ServiceProfile>::value);
    static_assert(sizeof(ServiceProfile) == 0x4008);

    struct ProfileDataForImportData : public sf::LargeData, public sf::PrefersMapAliasTransferMode {
        Identifier identifier_0;
        Identifier identifier_1;
        u8 unk_0E[2];
        ProfileData data;
        u8 unk_4010[0x4400 - 0x4010];
    };
    static_assert(util::is_pod<ProfileDataForImportData>::value);
    static_assert(sizeof(ProfileDataForImportData) == 0x4400);

    struct ReadMetadataEntry {
        u8 unk[0x100];
    };
    static_assert(util::is_pod<ReadMetadataEntry>::value);
    static_assert(sizeof(ReadMetadataEntry) == 0x100);

    struct ReadMetadataArgument : public sf::LargeData, public sf::PrefersMapAliasTransferMode {
        ProfileMetadata metadata;
        ReadMetadataEntry entries[(0x8000 - sizeof(metadata)) / sizeof(ReadMetadataEntry)];
        u8 unk_7FC0[0x40];
    };
    static_assert(util::is_pod<ReadMetadataArgument>::value);
    static_assert(sizeof(ReadMetadataArgument) == 0x8000);


}
