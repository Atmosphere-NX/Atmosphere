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

class IStorage {
    public:
        virtual ~IStorage();

        virtual Result Read(void *buffer, size_t size, u64 offset) = 0;
        virtual Result Write(void *buffer, size_t size, u64 offset) = 0;
        virtual Result Flush() = 0;
        virtual Result SetSize(u64 size) = 0;
        virtual Result GetSize(u64 *out_size) = 0;
        virtual Result OperateRange(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) = 0;

        static inline bool IsRangeValid(uint64_t offset, uint64_t size, uint64_t total_size) {
            return size <= total_size && offset <= total_size - size;
        }
};

class IStorageInterface : public IServiceObject {
    private:
        enum class CommandId {
            Read         = 0,
            Write        = 1,
            Flush        = 2,
            SetSize      = 3,
            GetSize      = 4,
            OperateRange = 5,
        };
    private:
        IStorage *base_storage;
    public:
        IStorageInterface(IStorage *s) : base_storage(s) {
            /* ... */
        };

        ~IStorageInterface() {
            delete base_storage;
        };

    private:
        /* Actual command API. */
        virtual Result Read(OutBuffer<u8, BufferType_Type1> buffer, u64 offset, u64 size) final {
            return this->base_storage->Read(buffer.buffer, std::min(buffer.num_elements, size), offset);
        };
        virtual Result Write(InBuffer<u8, BufferType_Type1> buffer, u64 offset, u64 size) final {
           return this->base_storage->Write(buffer.buffer, std::min(buffer.num_elements, size), offset);
        };
        virtual Result Flush() final {
            return this->base_storage->Flush();
        };
        virtual Result SetSize(u64 size) final {
            return this->base_storage->SetSize(size);
        };
        virtual Result GetSize(Out<u64> size) final {
            return this->base_storage->GetSize(size.GetPointer());
        };
        virtual Result OperateRange(Out<FsRangeInfo> range_info, u32 operation_type, u64 offset, u64 size) final {
            return this->base_storage->OperateRange(static_cast<FsOperationId>(operation_type), offset, size, range_info.GetPointer());
        };
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0- */
            MAKE_SERVICE_COMMAND_META(IStorageInterface, Read),
            MAKE_SERVICE_COMMAND_META(IStorageInterface, Write),
            MAKE_SERVICE_COMMAND_META(IStorageInterface, Flush),
            MAKE_SERVICE_COMMAND_META(IStorageInterface, SetSize),
            MAKE_SERVICE_COMMAND_META(IStorageInterface, GetSize),

            /* 4.0.0- */
            MAKE_SERVICE_COMMAND_META(IStorageInterface, OperateRange, FirmwareVersion_400),
        };
};

class IROStorage : public IStorage {
    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) = 0;
        virtual Result Write(void *buffer, size_t size, u64 offset) final {
            (void)(buffer);
            (void)(offset);
            (void)(size);
            return ResultFsUnsupportedOperation;
        };
        virtual Result Flush() final {
            return ResultSuccess();
        };
        virtual Result SetSize(u64 size) final {
            (void)(size);
            return ResultFsUnsupportedOperation;
        };
        virtual Result GetSize(u64 *out_size) = 0;
        virtual Result OperateRange(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) = 0;
};


class ProxyStorage : public IStorage {
    private:
        FsStorage *base_storage;
    public:
        ProxyStorage(FsStorage *s) : base_storage(s) {
            /* ... */
        };
        ProxyStorage(FsStorage s) {
            this->base_storage = new FsStorage(s);
        };
        virtual ~ProxyStorage() {
            fsStorageClose(base_storage);
            delete base_storage;
        };
    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) override {
            return fsStorageRead(this->base_storage, offset, buffer, size);
        };
        virtual Result Write(void *buffer, size_t size, u64 offset) override {
            return fsStorageWrite(this->base_storage, offset, buffer, size);
        };
        virtual Result Flush() override {
            return fsStorageFlush(this->base_storage);
        };
        virtual Result GetSize(u64 *out_size) override {
            return fsStorageGetSize(this->base_storage, out_size);
        };
        virtual Result SetSize(u64 size) override {
            return fsStorageSetSize(this->base_storage, size);
        };
        virtual Result OperateRange(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            return fsStorageOperateRange(this->base_storage, operation_type, offset, size, out_range_info);
        };
};

class ReadOnlyStorageAdapter : public IROStorage {
    private:
        std::shared_ptr<IStorage> base_storage;
        IStorage *storage;
    public:
        ReadOnlyStorageAdapter(IStorage *s) : base_storage(s) {
            this->storage = this->base_storage.get();
        }
        ReadOnlyStorageAdapter(std::shared_ptr<IStorage> s) : base_storage(s) {
            this->storage = this->base_storage.get();
        }
        virtual ~ReadOnlyStorageAdapter() {
            /* ... */
        }
    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) override {
            return this->base_storage->Read(buffer, size, offset);
        };
        virtual Result GetSize(u64 *out_size) override {
            return this->base_storage->GetSize(out_size);
        };
        virtual Result OperateRange(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            return this->base_storage->OperateRange(operation_type, offset, size, out_range_info);
        };
};
