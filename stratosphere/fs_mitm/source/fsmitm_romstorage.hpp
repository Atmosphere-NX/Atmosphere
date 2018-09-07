/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "fs_istorage.hpp"

/* Represents a RomFS stored in some file. */
class RomFileStorage : public IROStorage {
    private:
        FsFile *base_file;
    public:
        RomFileStorage(FsFile *f) : base_file(f) {
            /* ... */
        };
        RomFileStorage(FsFile f) {
            this->base_file = new FsFile(f);
        };
        ~RomFileStorage() {
            fsFileClose(base_file);
            delete base_file;
        };
        
        RomFileStorage *Clone() override {
            return new RomFileStorage(this->base_file);
        };
    public:
        Result Read(void *buffer, size_t size, u64 offset) override {
            size_t out_sz = 0;
            Result rc = fsFileRead(this->base_file, offset, buffer, size, &out_sz);
            if (R_SUCCEEDED(rc) && out_sz != size && out_sz) {
                return this->Read((void *)((uintptr_t)buffer + out_sz), size - out_sz, offset + out_sz);
            }
            return rc;
        };
        Result GetSize(u64 *out_size) override {
            return fsFileGetSize(this->base_file, out_size);
        };
        Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            /* TODO: Merge into libnx? */
            return fsFileOperateRange(this->base_file, operation_type, offset, size, out_range_info);
        };
};

/* Represents a RomFS accessed via some IStorage. */
class RomInterfaceStorage : public IROStorage {
    private:
        FsStorage *base_storage;
    public:
        RomInterfaceStorage(FsStorage *s) : base_storage(s) {
            /* ... */
        };
        RomInterfaceStorage(FsStorage s) {
            this->base_storage = new FsStorage(s);
        };
        ~RomInterfaceStorage() {
            fsStorageClose(base_storage);
            delete base_storage;
        };
        
        RomInterfaceStorage *Clone() override {
            return new RomInterfaceStorage(this->base_storage);
        };
    public:
        Result Read(void *buffer, size_t size, u64 offset) override {
            return fsStorageRead(this->base_storage, offset, buffer, size);
        };
        Result GetSize(u64 *out_size) override {
            /* TODO: Merge into libnx? */
            return fsStorageGetSize(this->base_storage, out_size);
        };
        Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            /* TODO: Merge into libnx? */
            return fsStorageOperateRange(this->base_storage, operation_type, offset, size, out_range_info);
        };
};
