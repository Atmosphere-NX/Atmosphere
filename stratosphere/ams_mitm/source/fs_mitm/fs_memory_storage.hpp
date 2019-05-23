/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>
#include "fs_shim.h"

#include "../debug.hpp"
#include "fs_istorage.hpp"

class MemoryStorage : public IStorage {
    private:
        u8 * const buffer;
        const u64 size;
    public:
        MemoryStorage(void *b, u64 sz) : buffer(static_cast<u8 *>(b)), size(sz) {
            /* ... */
        }

        virtual ~MemoryStorage() {
            /* ... */
        }

    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) override {
            if (size == 0) {
                return ResultSuccess;
            }
            if (buffer == nullptr) {
                return ResultFsNullptrArgument;
            }
            if (!IStorage::IsRangeValid(offset, size, this->size)) {
                return ResultFsOutOfRange;
            }

            std::memcpy(buffer, this->buffer + offset, size);
            return ResultSuccess;
        }

        virtual Result Write(void *buffer, size_t size, u64 offset) override {
            if (size == 0) {
                return ResultSuccess;
            }
            if (buffer == nullptr) {
                return ResultFsNullptrArgument;
            }
            if (!IStorage::IsRangeValid(offset, size, this->size)) {
                return ResultFsOutOfRange;
            }

            std::memcpy(this->buffer + offset, buffer, size);
            return ResultSuccess;
        }

        virtual Result Flush() override {
            return ResultSuccess;
        }

        virtual Result GetSize(u64 *out_size) override {
            *out_size = this->size;
            return ResultSuccess;
        }

        virtual Result SetSize(u64 size) override {
            return ResultFsUnsupportedOperation;
        }

        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            switch (operation_type) {
                case 2: /* TODO: OperationType_Invalidate */
                    return ResultSuccess;
                case 3: /* TODO: OperationType_Query */
                    if (out_range_info == nullptr) {
                        return ResultFsNullptrArgument;
                    }
                    /* N checks for size == sizeof(*out_range_info) here, but that's because their wrapper api is bad. */
                    std::memset(out_range_info, 0, sizeof(*out_range_info));
                    return ResultSuccess;
                default:
                    return ResultFsUnsupportedOperation;
            }
        }
};

class ReadOnlyMemoryStorage : public MemoryStorage {
    public:
        ReadOnlyMemoryStorage(const void *b, u64 sz) : MemoryStorage(const_cast<void *>(b), sz) {
            /* ... */
        }

        virtual ~ReadOnlyMemoryStorage() {
            /* ... */
        }

    public:
        virtual Result Write(void *buffer, size_t size, u64 offset) override {
            return ResultFsUnsupportedOperation;
        }
};
