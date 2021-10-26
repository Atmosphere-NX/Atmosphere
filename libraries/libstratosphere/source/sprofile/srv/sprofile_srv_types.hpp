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

namespace ams::sprofile::srv {

    constexpr inline const u32 ProfileFormatVersion = 1;

    constexpr inline bool IsValidProfileFormatVersion(u32 version) {
        return version == ProfileFormatVersion;
    }

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
        u32 num_entries;
        u8 unk_04[0x0C];
        u8 unk_10[0x20];
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
        struct {
            Identifier identifier_0;
            Identifier identifier_1;
            u8 unk_0E[2];
            u32 version;
            u8 unk_14[0x1C];
        } header;
        u8 hash[crypto::Md5Generator::HashSize];
        ProfileData data;
        u8 unk_4040[0x4400 - 0x4040];
    };
    static_assert(util::is_pod<ProfileDataForImportData>::value);
    static_assert(sizeof(ProfileDataForImportData) == 0x4400);

    struct ProfileMetadataEntry {
        Identifier identifier_0;
        Identifier identifier_1;
        u8 unk_0E[0x32];
    };
    static_assert(util::is_pod<ProfileMetadataEntry>::value);
    static_assert(sizeof(ProfileMetadataEntry) == 0x40);

    struct ProfileMetadataEntryData : public sf::PrefersMapAliasTransferMode {
        u8 unk[0x100];
    };
    static_assert(util::is_pod<ProfileMetadataEntryData>::value);
    static_assert(sizeof(ProfileMetadataEntryData) == 0x100);

    struct ProfileMetadata {
        u32 num_entries;
        u32 unk_04;
        Identifier revision_key;
        u8 unk_10[0x30];
        ProfileMetadataEntry entries[50];
    };
    static_assert(util::is_pod<ProfileMetadata>::value);
    static_assert(sizeof(ProfileMetadata) == 0xCC0);

    struct ProfileMetadataForImportMetadata : public sf::LargeData, public sf::PrefersMapAliasTransferMode {
        struct {
            u32 version;
            u8 unk_04[0x1C];
        } header;
        u8 hash[crypto::Md5Generator::HashSize];
        ProfileMetadata metadata;
        ProfileMetadataEntryData entries[50];
        u8 unk[0x8000 - 0x3EF0];
    };
    static_assert(util::is_pod<ProfileMetadataForImportMetadata>::value);
    static_assert(sizeof(ProfileMetadataForImportMetadata) == 0x8000);

}
