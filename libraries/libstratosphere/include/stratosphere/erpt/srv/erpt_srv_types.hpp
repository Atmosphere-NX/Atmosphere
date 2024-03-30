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
    constexpr inline const char ReportOnSdStorageRootDirectoryPath[] = "ersd:/";

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

    #define GET_FIELD_CATEGORY(FIELD, ID, CATEGORY, TYPE, FLAG) CategoryId_##CATEGORY,
    constexpr inline const CategoryId FieldIndexToCategoryMap[] = {
        AMS_ERPT_FOREACH_FIELD(GET_FIELD_CATEGORY)
    };
    #undef GET_FIELD_CATEGORY

    #define GET_FIELD_TYPE(FIELD, ID, CATEGORY, TYPE, FLAG) TYPE,
    constexpr inline const FieldType FieldIndexToTypeMap[] = {
        AMS_ERPT_FOREACH_FIELD(GET_FIELD_TYPE)
    };
    #undef GET_FIELD_TYPE

    #define GET_FIELD_FLAG(FIELD, ID, CATEGORY, TYPE, FLAG) FLAG,
    constexpr inline const FieldFlag FieldIndexToFlagMap[] = {
        AMS_ERPT_FOREACH_FIELD(GET_FIELD_FLAG)
    };
    #undef GET_FIELD_FLAG

    #define GET_FIELD_ID(FIELD, ...) FieldId_##FIELD,
    constexpr inline const FieldId FieldIndexToFieldIdMap[] = {
        AMS_ERPT_FOREACH_FIELD(GET_FIELD_ID)
    };
    #undef GET_FIELD_ID

    #define GET_CATEGORY_ID(CATEGORY, ...) CategoryId_##CATEGORY,
    constexpr inline const CategoryId CategoryIndexToCategoryIdMap[] = {
        AMS_ERPT_FOREACH_CATEGORY(GET_CATEGORY_ID)
    };
    #undef GET_CATEGORY_ID

    constexpr util::optional<size_t> FindFieldIndex(FieldId id) {
        if (std::is_constant_evaluated()) {
            for (size_t i = 0; i < util::size(FieldIndexToFieldIdMap); ++i) {
                if (FieldIndexToFieldIdMap[i] == id) {
                    return i;
                }
            }

            return util::nullopt;
        } else {
            if (const auto it = std::lower_bound(std::begin(FieldIndexToFieldIdMap), std::end(FieldIndexToFieldIdMap), id); it != std::end(FieldIndexToFieldIdMap) && *it == id) {
                return std::distance(FieldIndexToFieldIdMap, it);
            } else {
                return util::nullopt;
            }
        }
    }

    constexpr util::optional<size_t> FindCategoryIndex(CategoryId id) {
        if (std::is_constant_evaluated()) {
            for (size_t i = 0; i < util::size(CategoryIndexToCategoryIdMap); ++i) {
                if (CategoryIndexToCategoryIdMap[i] == id) {
                    return i;
                }
            }

            return util::nullopt;
        } else {
            if (const auto it = std::lower_bound(std::begin(CategoryIndexToCategoryIdMap), std::end(CategoryIndexToCategoryIdMap), id); it != std::end(CategoryIndexToCategoryIdMap) && *it == id) {
                return std::distance(CategoryIndexToCategoryIdMap, it);
            } else {
                return util::nullopt;
            }
        }
    }

    constexpr inline CategoryId ConvertFieldToCategory(FieldId id) {
        const auto index = FindFieldIndex(id);
        AMS_ASSERT(index.has_value());
        return FieldIndexToCategoryMap[index.value()];
    }

    constexpr inline FieldType ConvertFieldToType(FieldId id) {
        const auto index = FindFieldIndex(id);
        AMS_ASSERT(index.has_value());
        return FieldIndexToTypeMap[index.value()];
    }

    constexpr inline FieldFlag ConvertFieldToFlag(FieldId id) {
        const auto index = FindFieldIndex(id);
        AMS_ASSERT(index.has_value());
        return FieldIndexToFlagMap[index.value()];
    }

    constexpr inline ReportFlagSet MakeNoReportFlags() {
        return util::MakeBitFlagSet<32, ReportFlag>();
    }

    constexpr inline CreateReportOptionFlagSet MakeNoCreateReportOptionFlags() {
        return util::MakeBitFlagSet<32, CreateReportOptionFlag>();
    }

    constexpr inline AttachmentFlagSet MakeNoAttachmentFlags() {
        return util::MakeBitFlagSet<32, AttachmentFlag>();
    }


}