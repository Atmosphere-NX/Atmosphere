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
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/erpt/erpt_ids.autogen.hpp>

namespace ams::erpt::srv {

    constexpr inline const char ReportOnSdStoragePath[] = "ersd";

    constexpr inline const char ReportStoragePath[]             = "save";
    constexpr inline const char JournalFileName[]               = "save:/journal";
    constexpr inline const char ForcedShutdownContextFileName[] = "save:/forced-shutdown";

    constexpr size_t ReportFileNameLength = 64;
    constexpr size_t AttachmentFileNameLength = 64;
    constexpr size_t MaxFieldStringSize   = 64;

    struct ReportFileName {
        char name[ReportFileNameLength];
    };

    struct AttachmentFileName {
        char name[AttachmentFileNameLength];
    };

    enum FieldFlag : u8 {
        FieldFlag_None    = 0,
        FieldFlag_Encrypt = 1,
    };

    #define STRINGIZE_HANDLER(NAME, ...) #NAME,
    constexpr inline const char * const FieldString[] = {
        AMS_ERPT_FOREACH_FIELD(STRINGIZE_HANDLER)
    };

    constexpr inline const char * const CategoryString[] = {
        AMS_ERPT_FOREACH_CATEGORY(STRINGIZE_HANDLER)
    };

    constexpr inline const char * const TypeString[] = {
        AMS_ERPT_FOREACH_FIELD_TYPE(STRINGIZE_HANDLER)
    };
    #undef STRINGIZE_HANDLER

    #define GET_FIELD_CATEGORY(FIELD, ID, CATEGORY, TYPE, FLAG) [FieldId_##FIELD] = CategoryId_##CATEGORY,
    constexpr inline const CategoryId FieldToCategoryMap[] = {
        AMS_ERPT_FOREACH_FIELD(GET_FIELD_CATEGORY)
    };
    #undef GET_FIELD_CATEGORY

    #define GET_FIELD_TYPE(FIELD, ID, CATEGORY, TYPE, FLAG) [FieldId_##FIELD] = TYPE,
    constexpr inline const FieldType FieldToTypeMap[] = {
        AMS_ERPT_FOREACH_FIELD(GET_FIELD_TYPE)
    };
    #undef GET_FIELD_TYPE

    #define GET_FIELD_FLAG(FIELD, ID, CATEGORY, TYPE, FLAG) [FieldId_##FIELD] = FLAG,
    constexpr inline const FieldFlag FieldToFlagMap[] = {
        AMS_ERPT_FOREACH_FIELD(GET_FIELD_FLAG)
    };
    #undef GET_FIELD_FLAG

    constexpr inline ReportFlagSet MakeNoReportFlags() {
        return util::MakeBitFlagSet<32, ReportFlag>();
    }

    constexpr inline AttachmentFlagSet MakeNoAttachmentFlags() {
        return util::MakeBitFlagSet<32, AttachmentFlag>();
    }


}