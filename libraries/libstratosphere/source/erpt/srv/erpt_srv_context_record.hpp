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
#include "erpt_srv_allocator.hpp"

namespace ams::erpt::srv {

    class Context;

    class ContextRecord : public Allocator, public util::IntrusiveListBaseNode<ContextRecord> {
        friend class Context;
        private:
            static u32 s_record_count;
        public:
            static u32 GetRecordCount() {
                return s_record_count;
            }
        private:
            ContextEntry ctx;
        private:
            Result Add(FieldId field_id, const void *arr, u32 size, FieldType type);
        public:
            ContextRecord();
            explicit ContextRecord(CategoryId category, u32 array_buf_size = ArrayBufferSizeDefault);
            ~ContextRecord();

            Result Initialize(const ContextEntry *ctx_ptr, const u8 *data, u32 data_size);

            Result Add(FieldId field_id, bool value_bool);
            Result Add(FieldId field_id, u32 value_u32);
            Result Add(FieldId field_id, u64 value_u64);
            Result Add(FieldId field_id, s32 value_i32);
            Result Add(FieldId field_id, s64 value_i64);
            Result Add(FieldId field_id, const char *str, u32 str_size);
            Result Add(FieldId field_id, const u8 *data, u32 size);
    };

}
