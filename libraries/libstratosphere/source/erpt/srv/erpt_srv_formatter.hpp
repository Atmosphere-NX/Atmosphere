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
#include "erpt_srv_report.hpp"

namespace ams::erpt::srv {

    class Formatter {
        private:
            enum ElementSize {
                ElementSize_16    = 16,
                ElementSize_32    = 32,
                ElementSize_256   = 256,
                ElementSize_16384 = 16384,
            };
        private:
            static ValueTypeTag GetTag(s8)  { return ValueTypeTag::I8; }
            static ValueTypeTag GetTag(s16) { return ValueTypeTag::I16; }
            static ValueTypeTag GetTag(s32) { return ValueTypeTag::I32; }
            static ValueTypeTag GetTag(s64) { return ValueTypeTag::I64; }
            static ValueTypeTag GetTag(u8)  { return ValueTypeTag::U8; }
            static ValueTypeTag GetTag(u16) { return ValueTypeTag::U16; }
            static ValueTypeTag GetTag(u32) { return ValueTypeTag::U32; }
            static ValueTypeTag GetTag(u64) { return ValueTypeTag::U64; }

            static Result AddStringValue(Report *report, const char *str, u32 len) {
                const u32 str_len = str != nullptr ? static_cast<u32>(strnlen(str, len)) : 0;

                if (str_len < ElementSize_32) {
                    R_TRY(report->Write(static_cast<u8>(static_cast<u8>(ValueTypeTag::FixStr) | str_len)));
                } else if (str_len < ElementSize_256) {
                    R_TRY(report->Write(static_cast<u8>(ValueTypeTag::Str8)));
                    R_TRY(report->Write(static_cast<u8>(str_len)));
                } else {
                    R_UNLESS(str_len < ElementSize_16384, erpt::ResultFormatterError());
                    R_TRY(report->Write(static_cast<u8>(ValueTypeTag::Str16)));

                    u16 be_str_len;
                    util::StoreBigEndian(std::addressof(be_str_len), static_cast<u16>(str_len));
                    R_TRY(report->Write(be_str_len));
                }

                R_TRY(report->Write(str, str_len));

                return ResultSuccess();
            }

            static Result AddId(Report *report, FieldId field_id) {
                static_assert(MaxFieldStringSize < ElementSize_256);

                R_TRY(AddStringValue(report, FieldString[field_id], strnlen(FieldString[field_id], MaxFieldStringSize)));

                return ResultSuccess();
            }

            template<typename T>
            static Result AddValue(Report *report, T value) {
                const u8 tag = static_cast<u8>(GetTag(value));

                T big_endian_value;
                util::StoreBigEndian(std::addressof(big_endian_value), value);

                R_TRY(report->Write(tag));
                R_TRY(report->Write(reinterpret_cast<u8 *>(std::addressof(big_endian_value)), sizeof(big_endian_value)));

                return ResultSuccess();
            }

            template<typename T>
            static Result AddValueArray(Report *report, T *arr, u32 arr_size) {
                if (arr_size < ElementSize_16) {
                    R_TRY(report->Write(static_cast<u8>(static_cast<u8>(ValueTypeTag::FixArray) | arr_size)));
                } else {
                    R_UNLESS(arr_size < ElementSize_16384, erpt::ResultFormatterError());

                    u16 be_arr_size;
                    util::StoreBigEndian(std::addressof(be_arr_size), static_cast<u16>(arr_size));

                    R_TRY(report->Write(static_cast<u8>(ValueTypeTag::Array16)));
                    R_TRY(report->Write(be_arr_size));
                }

                for (u32 i = 0; i < arr_size; i++) {
                    R_TRY(AddValue(report, arr[i]));
                }

                return ResultSuccess();
            }

            template<typename T>
            static Result AddIdValuePair(Report *report, FieldId field_id, T value) {
                R_TRY(AddId(report, field_id));
                R_TRY(AddValue(report, value));
                return ResultSuccess();
            }

            template<typename T>
            static Result AddIdValueArray(Report *report, FieldId field_id, T *arr, u32 arr_size) {
                R_TRY(AddId(report, field_id));
                R_TRY(AddValueArray(report, arr, arr_size));
                return ResultSuccess();
            }
        public:
            static Result Begin(Report *report, u32 record_count) {
                if (record_count < ElementSize_16) {
                    R_TRY(report->Write(static_cast<u8>(static_cast<u8>(ValueTypeTag::FixMap) | record_count)));
                } else {
                    R_UNLESS(record_count < ElementSize_16384, erpt::ResultFormatterError());

                    u16 be_count;
                    util::StoreBigEndian(std::addressof(be_count), static_cast<u16>(record_count));

                    R_TRY(report->Write(static_cast<u8>(ValueTypeTag::Map16)));
                    R_TRY(report->Write(be_count));
                }

                return ResultSuccess();
            }

            static Result End(Report *report) {
                return ResultSuccess();
            }

            template<typename T>
            static Result AddField(Report *report, FieldId field_id, T value) {
                return AddIdValuePair<T>(report, field_id, value);
            }

            template<typename T>
            static Result AddField(Report *report, FieldId field_id, T *arr, u32 arr_size) {
                return AddIdValueArray(report, field_id, arr, arr_size);
            }

            static Result AddField(Report *report, FieldId field_id, bool value) {
                R_TRY(AddId(report, field_id));
                R_TRY(report->Write(static_cast<u8>(value ? ValueTypeTag::True : ValueTypeTag::False)));
                return ResultSuccess();
            }

            static Result AddField(Report *report, FieldId field_id, char *str, u32 len) {
                R_TRY(AddId(report, field_id));

                R_TRY(AddStringValue(report, str, len));

                return ResultSuccess();
            }

            static Result AddField(Report *report, FieldId field_id, u8 *bin, u32 len) {
                R_TRY(AddId(report, field_id));

                if (len < ElementSize_256) {
                    R_TRY(report->Write(static_cast<u8>(ValueTypeTag::Bin8)));
                    R_TRY(report->Write(static_cast<u8>(len)));
                } else {
                    R_UNLESS(len < ElementSize_16384, erpt::ResultFormatterError());
                    R_TRY(report->Write(static_cast<u8>(ValueTypeTag::Bin16)));

                    u16 be_len;
                    util::StoreBigEndian(std::addressof(be_len), static_cast<u16>(len));
                    R_TRY(report->Write(be_len));
                }

                R_TRY(report->Write(bin, len));

                return ResultSuccess();
            }
    };

}
