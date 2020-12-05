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
#include "impl/fs_newable.hpp"
#include "fs_istorage.hpp"
#include "fs_query_range.hpp"

namespace ams::fs {

    class MemoryStorage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        private:
            u8 * const buf;
            const s64 size;
        public:
            MemoryStorage(void *b, s64 sz) : buf(static_cast<u8 *>(b)), size(sz) { /* .. */ }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                /* Succeed immediately on zero-sized read. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments. */
                R_UNLESS(buffer != nullptr,                                    fs::ResultNullptrArgument());
                R_UNLESS(IStorage::CheckAccessRange(offset, size, this->size), fs::ResultOutOfRange());

                /* Copy from memory. */
                std::memcpy(buffer, this->buf + offset, size);
                return ResultSuccess();
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                /* Succeed immediately on zero-sized write. */
                R_SUCCEED_IF(size == 0);

                /* Validate arguments. */
                R_UNLESS(buffer != nullptr,                                    fs::ResultNullptrArgument());
                R_UNLESS(IStorage::CheckAccessRange(offset, size, this->size), fs::ResultOutOfRange());

                /* Copy to memory. */
                std::memcpy(this->buf + offset, buffer, size);
                return ResultSuccess();
            }

            virtual Result Flush() override {
                return ResultSuccess();
            }

            virtual Result GetSize(s64 *out) override {
                *out = this->size;
                return ResultSuccess();
            }

            virtual Result SetSize(s64 size) override {
                return fs::ResultUnsupportedOperationInMemoryStorageA();
            }

            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                switch (op_id) {
                    case OperationId::InvalidateCache:
                        return ResultSuccess();
                    case OperationId::QueryRange:
                        R_UNLESS(dst != nullptr,                     fs::ResultNullptrArgument());
                        R_UNLESS(dst_size == sizeof(QueryRangeInfo), fs::ResultInvalidSize());
                        reinterpret_cast<QueryRangeInfo *>(dst)->Clear();
                        return ResultSuccess();
                    default:
                        return fs::ResultUnsupportedOperationInMemoryStorageB();
                }
            }
    };

}
