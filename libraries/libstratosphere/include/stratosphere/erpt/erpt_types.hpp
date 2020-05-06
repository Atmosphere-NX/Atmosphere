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
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/time/time_posix_time.hpp>
#include <stratosphere/erpt/erpt_ids.autogen.hpp>

namespace ams::erpt {

    #define GENERATE_ENUM(NAME, ID, ...) NAME = ID,

    enum FieldType {
        AMS_ERPT_FOREACH_FIELD_TYPE(GENERATE_ENUM)
        FieldType_Count,
    };

    #undef GENERATE_ENUM

    #define GENERATE_ENUM(NAME, ID, ...) CategoryId_##NAME = ID,

    enum CategoryId {
        AMS_ERPT_FOREACH_CATEGORY(GENERATE_ENUM)
        CategoryId_Count,
    };

    #undef GENERATE_ENUM

    #define GENERATE_ENUM(NAME, ID, ...) FieldId_##NAME = ID,

    enum FieldId {
        AMS_ERPT_FOREACH_FIELD(GENERATE_ENUM)
        FieldId_Count,
    };

    #undef GENERATE_ENUM

    constexpr inline u32 ArrayBufferSizeDefault = 0x100;
    constexpr inline u32 ArrayBufferSizeMax     = 96_KB;
    constexpr inline u32 ArrayFieldSizeMax      = 16_KB - 1;

    enum ReportType {
        ReportType_Start     = 0,

        ReportType_Visible   = ReportType_Start,
        ReportType_Invisible = 1,

        ReportType_End       = 2,

        ReportType_Count     = ReportType_End,

        ReportType_Any       = ReportType_Count,
    };

    constexpr inline u32 ReportCountMax          = 50;
    constexpr inline u32 AttachmentsPerReportMax = 5;
    constexpr inline u32 AttachmentCountMax      = ReportCountMax * AttachmentsPerReportMax;

    constexpr inline u32 ReportMetaDataSize = 0x20;
    struct ReportMetaData {
        u8 user_data[ReportMetaDataSize];
    };


    constexpr inline u32 ReportIdSize = 20;
    struct ReportId {
        union {
            u8 id[ReportIdSize];
            util::Uuid uuid;
            #pragma pack(push, 1)
            struct {
                u32 time_low;
                u16 time_mid;
                u16 time_high_and_version;
                u8  clock_high;
                u8  clock_low;
                u64 node;
            } uuid_data;
            #pragma pack(pop)
        };
    };
    static_assert(sizeof(ReportId) == ReportIdSize);

    inline bool operator==(const ReportId &lhs, const ReportId &rhs) {
        return std::memcmp(lhs.id, rhs.id, sizeof(lhs.uuid)) == 0;
    }

    inline bool operator!=(const ReportId &lhs, const ReportId &rhs) {
        return !(lhs == rhs);
    }

    struct ReportFlag {
        using Transmitted   = util::BitFlagSet<BITSIZEOF(u32), ReportFlag>::Flag<0>;
        using HasAttachment = util::BitFlagSet<BITSIZEOF(u32), ReportFlag>::Flag<1>;
    };

    using ReportFlagSet = util::BitFlagSet<BITSIZEOF(u32), ReportFlag>;
    static_assert(util::is_pod<ReportFlagSet>::value);
    static_assert(sizeof(ReportFlagSet) == sizeof(u32));

    struct ReportInfo {
        ReportType      type;
        ReportId        id;
        ReportMetaData  meta_data;
        ReportFlagSet   flags;
        time::PosixTime timestamp_user;
        time::PosixTime timestamp_network;
        s64             report_size;
        u64             reserved[3];
    };

    struct ReportList {
        u32 report_count;
        ReportInfo reports[ReportCountMax];
    };

    constexpr inline u32 AttachmentIdSize = 20;
    struct AttachmentId {
        union {
            u8 id[AttachmentIdSize];
            util::Uuid uuid;
        };
    };
    static_assert(sizeof(AttachmentId) == AttachmentIdSize);

    inline bool operator==(const AttachmentId &lhs, const AttachmentId &rhs) {
        return std::memcmp(lhs.id, rhs.id, sizeof(lhs.uuid)) == 0;
    }

    inline bool operator!=(const AttachmentId &lhs, const AttachmentId &rhs) {
        return !(lhs == rhs);
    }

    struct AttachmentFlag {
        using HasOwner = util::BitFlagSet<BITSIZEOF(u32), AttachmentFlag>::Flag<1>;
    };

    using AttachmentFlagSet = util::BitFlagSet<BITSIZEOF(u32), AttachmentFlag>;
    static_assert(util::is_pod<AttachmentFlagSet>::value);
    static_assert(sizeof(AttachmentFlagSet) == sizeof(u32));

    constexpr inline u32 AttachmentNameSizeMax = 0x20;
    struct AttachmentInfo {
        ReportId owner_report_id;
        AttachmentId attachment_id;
        AttachmentFlagSet flags;
        s64 attachment_size;
        char attachment_name[AttachmentNameSizeMax];
    };

    struct AttachmentList {
        u32 attachment_count;
        AttachmentInfo attachments[AttachmentsPerReportMax];
    };

    constexpr inline u32 AttachmentSizeMax = 512_KB;

    struct FieldEntry {
        FieldId id;
        FieldType type;
        union {
            u64  value_u64;
            u32  value_u32;
            u16  value_u16;
            u8   value_u8;
            s64  value_i64;
            s32  value_i32;
            s16  value_i16;
            s8   value_i8;
            bool value_bool;
            struct {
                u32 start_idx;
                u32 size;
            } value_array;
        };
    };

    constexpr inline u32 FieldsPerContext = 20;
    struct ContextEntry {
        u32 version;
        u32 field_count;
        CategoryId category;
        FieldEntry fields[FieldsPerContext];
        u8 *array_buffer;
        u32 array_free_count;
        u32 array_buffer_size;
    };

    struct StorageUsageStatistics {
        util::Uuid journal_uuid;
        u32 used_storage_size;
        s64 max_report_size;
        u32 report_count[ReportType_Count];
        u32 transmitted_count[ReportType_Count];
        u32 untransmitted_count[ReportType_Count];
    };

    /* https://github.com/msgpack/msgpack/blob/master/spec.md#overview  */
    enum class ValueTypeTag {
        FixMap   = 0x80,
        FixArray = 0x90,
        FixStr   = 0xA0,
        False    = 0xC2,
        True     = 0xC3,
        Bin8     = 0xC4,
        Bin16    = 0xC5,
        U8       = 0xCC,
        U16      = 0xCD,
        U32      = 0xCE,
        U64      = 0xCF,
        I8       = 0xD0,
        I16      = 0xD1,
        I32      = 0xD2,
        I64      = 0xD3,
        Str8     = 0xD9,
        Str16    = 0xDA,
        Array16  = 0xDC,
        Map16    = 0xDE,
    };

}
