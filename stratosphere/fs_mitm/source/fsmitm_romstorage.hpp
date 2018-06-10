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
    protected:
        Result Read(void *buffer, size_t size, u64 offset, u64 *out_read_size) override {
            size_t out_sz = 0;
            Result rc = fsFileRead(this->base_file, offset, buffer, size, &out_sz);
            if (R_SUCCEEDED(rc)) {
                *out_read_size = out_sz;
            }
            return rc;
        };
        Result GetSize(u64 *out_size) override {
            return fsFileGetSize(this->base_file, out_size);
        };
        Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            /* TODO: Merge into libnx. */
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
    protected:
        Result Read(void *buffer, size_t size, u64 offset, u64 *out_read_size) override {
            Result rc = fsStorageRead(this->base_storage, offset, buffer, size);
            if (R_SUCCEEDED(rc)) {
                *out_read_size = size;
            }
            return rc;
        };
        Result GetSize(u64 *out_size) override {
            /* TODO: Merge into libnx. */
            return fsStorageGetSize(this->base_storage, out_size);
        };
        Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            /* TODO: Merge into libnx. */
            return fsStorageOperateRange(this->base_storage, operation_type, offset, size, out_range_info);
        };
};
