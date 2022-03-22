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
#include <stratosphere.hpp>
#include "erpt_srv_context_record.hpp"

namespace ams::erpt::srv {

    constinit u32 ContextRecord::s_record_count  = 0;

    namespace {

        bool IsArrayFieldType(FieldType type) {
            return type == FieldType_String   ||
                   type == FieldType_U8Array  ||
                   type == FieldType_U32Array ||
                   type == FieldType_U64Array ||
                   type == FieldType_I32Array ||
                   type == FieldType_I64Array;
        }

    }

    ContextRecord::ContextRecord() {
        m_ctx = {};
    }

    ContextRecord::ContextRecord(CategoryId category, u32 array_buf_size) {
        m_ctx = {
            .category     = category,
            .array_buffer = static_cast<u8 *>(Allocate(array_buf_size)),
        };
        if (m_ctx.array_buffer != nullptr) {
            m_ctx.array_buffer_size = array_buf_size;
            m_ctx.array_free_count  = array_buf_size;
        }
    }

    ContextRecord::~ContextRecord() {
        if (m_ctx.array_buffer != nullptr) {
            Deallocate(m_ctx.array_buffer);
        }

        AMS_ABORT_UNLESS(s_record_count >= m_ctx.field_count);
        s_record_count -= m_ctx.field_count;
    }

    Result ContextRecord::Initialize(const ContextEntry *ctx_ptr, const u8 *data, u32 data_size) {
        R_UNLESS(data_size <= ArrayBufferSizeMax, erpt::ResultInvalidArgument());

        m_ctx.version           = ctx_ptr->version;
        m_ctx.field_count       = ctx_ptr->field_count;
        m_ctx.category          = ctx_ptr->category;
        m_ctx.array_buffer      = nullptr;
        m_ctx.array_buffer_size = data_size;
        m_ctx.array_free_count  = 0;

        auto guard = SCOPE_GUARD { m_ctx.field_count = 0; };

        R_UNLESS(m_ctx.field_count <= FieldsPerContext,                    erpt::ResultInvalidArgument());
        R_UNLESS(0 <= m_ctx.category && m_ctx.category < CategoryId_Count, erpt::ResultInvalidArgument());

        for (u32 i = 0; i < m_ctx.field_count; i++) {
            m_ctx.fields[i] = ctx_ptr->fields[i];

            R_UNLESS(0 <= m_ctx.fields[i].id   && m_ctx.fields[i].id   < FieldId_Count,   erpt::ResultInvalidArgument());
            R_UNLESS(0 <= m_ctx.fields[i].type && m_ctx.fields[i].type < FieldType_Count, erpt::ResultInvalidArgument());

            R_UNLESS(m_ctx.fields[i].type == FieldToTypeMap[m_ctx.fields[i].id],     erpt::ResultFieldTypeMismatch());
            R_UNLESS(m_ctx.category       == FieldToCategoryMap[m_ctx.fields[i].id], erpt::ResultFieldCategoryMismatch());

            if (IsArrayFieldType(m_ctx.fields[i].type)) {
                const u32 start_idx = m_ctx.fields[i].value_array.start_idx;
                const u32 size      = m_ctx.fields[i].value_array.size;
                const u32 end_idx   = start_idx + size;

                R_UNLESS(start_idx <= data_size,    erpt::ResultInvalidArgument());
                R_UNLESS(size      <= data_size,    erpt::ResultInvalidArgument());
                R_UNLESS(end_idx   <= data_size,    erpt::ResultInvalidArgument());
                R_UNLESS(size <= ArrayFieldSizeMax, erpt::ResultInvalidArgument());
            }
        }

        if (data_size > 0) {
            /* If array buffer isn't nullptr, we'll leak memory here, so verify that it is. */
            AMS_ABORT_UNLESS(m_ctx.array_buffer == nullptr);

            m_ctx.array_buffer = static_cast<u8 *>(AllocateWithAlign(data_size, alignof(u64)));
            R_UNLESS(m_ctx.array_buffer != nullptr, erpt::ResultOutOfMemory());

            std::memcpy(m_ctx.array_buffer, data, data_size);
        }

        guard.Cancel();
        s_record_count += m_ctx.field_count;

        return ResultSuccess();
    }

    Result ContextRecord::Add(FieldId field_id, bool value_bool) {
        R_UNLESS(m_ctx.field_count < FieldsPerContext, erpt::ResultOutOfFieldSpace());

        s_record_count++;
        auto &field = m_ctx.fields[m_ctx.field_count++];

        field.id   = field_id;
        field.type = FieldType_Bool;

        field.value_bool = value_bool;

        return ResultSuccess();
    }

    Result ContextRecord::Add(FieldId field_id, u32 value_u32) {
        R_UNLESS(m_ctx.field_count < FieldsPerContext, erpt::ResultOutOfFieldSpace());

        s_record_count++;
        auto &field = m_ctx.fields[m_ctx.field_count++];

        field.id   = field_id;
        field.type = FieldType_NumericU32;

        field.value_u32 = value_u32;

        return ResultSuccess();
    }

    Result ContextRecord::Add(FieldId field_id, u64 value_u64) {
        R_UNLESS(m_ctx.field_count < FieldsPerContext, erpt::ResultOutOfFieldSpace());

        s_record_count++;
        auto &field = m_ctx.fields[m_ctx.field_count++];

        field.id   = field_id;
        field.type = FieldType_NumericU64;

        field.value_u64 = value_u64;

        return ResultSuccess();
    }

    Result ContextRecord::Add(FieldId field_id, s32 value_i32) {
        R_UNLESS(m_ctx.field_count < FieldsPerContext, erpt::ResultOutOfFieldSpace());

        s_record_count++;
        auto &field = m_ctx.fields[m_ctx.field_count++];

        field.id   = field_id;
        field.type = FieldType_NumericI32;

        field.value_i32 = value_i32;

        return ResultSuccess();
    }

    Result ContextRecord::Add(FieldId field_id, s64 value_i64) {
        R_UNLESS(m_ctx.field_count < FieldsPerContext, erpt::ResultOutOfFieldSpace());

        s_record_count++;
        auto &field = m_ctx.fields[m_ctx.field_count++];

        field.id   = field_id;
        field.type = FieldType_NumericI64;

        field.value_i64 = value_i64;

        return ResultSuccess();
    }

    Result ContextRecord::Add(FieldId field_id, const void *arr, u32 size, FieldType type) {
        R_UNLESS(m_ctx.field_count < FieldsPerContext, erpt::ResultOutOfFieldSpace());
        R_UNLESS(size <= m_ctx.array_free_count,   erpt::ResultOutOfArraySpace());

        const u32 start_idx = m_ctx.array_buffer_size - m_ctx.array_free_count;
        m_ctx.array_free_count -= size;

        s_record_count++;
        auto &field = m_ctx.fields[m_ctx.field_count++];

        field.id   = field_id;
        field.type = type;

        field.value_array = {
            .start_idx = start_idx,
            .size      = size,
        };

        std::memcpy(m_ctx.array_buffer + start_idx, arr, size);
        return ResultSuccess();
    }

    Result ContextRecord::Add(FieldId field_id, const char *str, u32 str_size) {
        return this->Add(field_id, str, str_size, FieldType_String);
    }

    Result ContextRecord::Add(FieldId field_id, const u8 *data, u32 size) {
        return this->Add(field_id, data, size, FieldType_U8Array);
    }

}
