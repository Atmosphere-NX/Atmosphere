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
#include "erpt_srv_context.hpp"
#include "erpt_srv_cipher.hpp"
#include "erpt_srv_context_record.hpp"
#include "erpt_srv_report.hpp"

namespace ams::erpt::srv {

    namespace {

        using ContextList = util::IntrusiveListBaseTraits<Context>::ListType;

        ContextList g_category_list;

    }

    Context::Context(CategoryId cat, u32 max_records) : category(cat), max_record_count(max_records), record_count(0) {
        g_category_list.push_front(*this);
    }

    Context::~Context() {
        g_category_list.erase(g_category_list.iterator_to(*this));
    }

    Result Context::AddCategoryToReport(Report *report) {
        R_SUCCEED_IF(this->record_list.empty());

        for (auto it = this->record_list.begin(); it != this->record_list.end(); it++) {
            for (u32 i = 0; i < it->ctx.field_count; i++) {
                auto *field = std::addressof(it->ctx.fields[i]);
                u8 *arr_buf = it->ctx.array_buffer;

                switch (field->type) {
                    case FieldType_Bool:       R_TRY(Cipher::AddField(report, field->id, field->value_bool));  break;
                    case FieldType_NumericU8:  R_TRY(Cipher::AddField(report, field->id, field->value_u8));    break;
                    case FieldType_NumericU16: R_TRY(Cipher::AddField(report, field->id, field->value_u16));   break;
                    case FieldType_NumericU32: R_TRY(Cipher::AddField(report, field->id, field->value_u32));   break;
                    case FieldType_NumericU64: R_TRY(Cipher::AddField(report, field->id, field->value_u64));   break;
                    case FieldType_NumericI8:  R_TRY(Cipher::AddField(report, field->id, field->value_i8));    break;
                    case FieldType_NumericI16: R_TRY(Cipher::AddField(report, field->id, field->value_i16));   break;
                    case FieldType_NumericI32: R_TRY(Cipher::AddField(report, field->id, field->value_i32));   break;
                    case FieldType_NumericI64: R_TRY(Cipher::AddField(report, field->id, field->value_i64));   break;
                    case FieldType_String:     R_TRY(Cipher::AddField(report, field->id, reinterpret_cast<char *>(arr_buf + field->value_array.start_idx), field->value_array.size / sizeof(char))); break;
                    case FieldType_U8Array:    R_TRY(Cipher::AddField(report, field->id, reinterpret_cast<  u8 *>(arr_buf + field->value_array.start_idx), field->value_array.size / sizeof(u8)));   break;
                    case FieldType_U32Array:   R_TRY(Cipher::AddField(report, field->id, reinterpret_cast< u32 *>(arr_buf + field->value_array.start_idx), field->value_array.size / sizeof(u32)));  break;
                    case FieldType_U64Array:   R_TRY(Cipher::AddField(report, field->id, reinterpret_cast< u64 *>(arr_buf + field->value_array.start_idx), field->value_array.size / sizeof(u64)));  break;
                    case FieldType_I8Array:    R_TRY(Cipher::AddField(report, field->id, reinterpret_cast<  s8 *>(arr_buf + field->value_array.start_idx), field->value_array.size / sizeof(s8)));   break;
                    case FieldType_I32Array:   R_TRY(Cipher::AddField(report, field->id, reinterpret_cast< s32 *>(arr_buf + field->value_array.start_idx), field->value_array.size / sizeof(s32)));  break;
                    case FieldType_I64Array:   R_TRY(Cipher::AddField(report, field->id, reinterpret_cast< s64 *>(arr_buf + field->value_array.start_idx), field->value_array.size / sizeof(s64)));  break;
                    default:                   return erpt::ResultInvalidArgument();
                }
            }
        }

        return ResultSuccess();
    }

    Result Context::AddContextToCategory(const ContextEntry *entry, const u8 *data, u32 data_size) {
        auto record = std::make_unique<ContextRecord>();
        R_UNLESS(record != nullptr, erpt::ResultOutOfMemory());

        R_TRY(record->Initialize(entry, data, data_size));

        this->AddContextRecordToCategory(std::move(record));
        return ResultSuccess();
    }

    Result Context::AddContextRecordToCategory(std::unique_ptr<ContextRecord> record) {
        if (this->record_count < this->max_record_count) {
            this->record_list.push_front(*record.release());
            this->record_count++;
        } else {
            ContextRecord *back = std::addressof(this->record_list.back());
            this->record_list.pop_back();
            this->record_list.push_front(*record.release());
            delete back;
        }

        return ResultSuccess();
    }

    Result Context::SubmitContext(const ContextEntry *entry, const u8 *data, u32 data_size) {
        auto it = util::range::find_if(g_category_list, [&](const Context &cur) {
            return cur.category == entry->category;
        });
        R_UNLESS(it != g_category_list.end(), erpt::ResultCategoryNotFound());

        return it->AddContextToCategory(entry, data, data_size);
    }

    Result Context::SubmitContextRecord(std::unique_ptr<ContextRecord> record) {
        auto it = util::range::find_if(g_category_list, [&](const Context &cur) {
            return cur.category == record->ctx.category;
        });
        R_UNLESS(it != g_category_list.end(), erpt::ResultCategoryNotFound());

        return it->AddContextRecordToCategory(std::move(record));
    }

    Result Context::WriteContextsToReport(Report *report) {
        R_TRY(report->Open(ReportOpenType_Create));
        R_TRY(Cipher::Begin(report, ContextRecord::GetRecordCount()));

        for (auto it = g_category_list.begin(); it != g_category_list.end(); it++) {
            R_TRY(it->AddCategoryToReport(report));
        }

        Cipher::End(report);
        report->Close();

        return ResultSuccess();
    }

}
