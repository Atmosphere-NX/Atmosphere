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
#include "fs_ifile.hpp"

class FileStorage : public IStorage {
    public:
        static constexpr u64 InvalidSize = UINT64_MAX;
    private:
        std::shared_ptr<IFile> base_file;
        IFile *file;
        u64 size;
    public:
        FileStorage(IFile *f) : base_file(f) {
            this->file = this->base_file.get();
            this->size = InvalidSize;
        };
        FileStorage(std::shared_ptr<IFile> f) : base_file(f) {
            this->file = this->base_file.get();
            this->size = InvalidSize;
        };
        virtual ~FileStorage() {
            /* ... */
        };
    protected:
        Result UpdateSize();
    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) override;
        virtual Result Write(void *buffer, size_t size, u64 offset) override;
        virtual Result Flush() override;
        virtual Result GetSize(u64 *out_size) override;
        virtual Result SetSize(u64 size) override;
        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override;
};
