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
#include "fs_shim.h"

#include "debug.hpp"

enum FsIStorageCmd : u32 {
    FsIStorageCmd_Read = 0,
    FsIStorageCmd_Write = 1,
    FsIStorageCmd_Flush = 2,
    FsIStorageCmd_SetSize = 3,
    FsIStorageCmd_GetSize = 4,
    FsIStorageCmd_OperateRange = 5,
};

class IStorage {
    public:
        virtual ~IStorage();
                
        virtual Result Read(void *buffer, size_t size, u64 offset) = 0;
        virtual Result Write(void *buffer, size_t size, u64 offset) = 0;
        virtual Result Flush() = 0;
        virtual Result SetSize(u64 size) = 0;
        virtual Result GetSize(u64 *out_size) = 0;
        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) = 0;
};

class IStorageInterface : public IServiceObject {
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
            return this->base_storage->OperateRange(operation_type, offset, size, range_info.GetPointer());
        };
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            /* 1.0.0- */
            MakeServiceCommandMeta<FsIStorageCmd_Read, &IStorageInterface::Read>(),
            MakeServiceCommandMeta<FsIStorageCmd_Write, &IStorageInterface::Write>(),
            MakeServiceCommandMeta<FsIStorageCmd_Flush, &IStorageInterface::Flush>(),
            MakeServiceCommandMeta<FsIStorageCmd_SetSize, &IStorageInterface::SetSize>(),
            MakeServiceCommandMeta<FsIStorageCmd_GetSize, &IStorageInterface::GetSize>(),
            
            /* 4.0.0- */
            MakeServiceCommandMeta<FsIStorageCmd_OperateRange, &IStorageInterface::OperateRange, FirmwareVersion_400>(),
        };
};

class IROStorage : public IStorage {
    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) = 0;
        virtual Result Write(void *buffer, size_t size, u64 offset) final {
            (void)(buffer);
            (void)(offset);
            (void)(size);
            return 0x313802;
        };
        virtual Result Flush() final {
            return 0x0;
        };
        virtual Result SetSize(u64 size) final {
            (void)(size);
            return 0x313802;
        };
        virtual Result GetSize(u64 *out_size) = 0;
        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) = 0;
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
        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            return fsStorageOperateRange(this->base_storage, operation_type, offset, size, out_range_info);
        };
};

class ROProxyStorage : public IROStorage {
    private:
        FsStorage *base_storage;
    public:
        ROProxyStorage(FsStorage *s) : base_storage(s) {
            /* ... */
        };
        ROProxyStorage(FsStorage s) {
            this->base_storage = new FsStorage(s);
        };
        virtual ~ROProxyStorage() {
            fsStorageClose(base_storage);
            delete base_storage;
        };
    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) override {
            return fsStorageRead(this->base_storage, offset, buffer, size);
        };
        virtual Result GetSize(u64 *out_size) override {
            return fsStorageGetSize(this->base_storage, out_size);
        };
        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            return fsStorageOperateRange(this->base_storage, operation_type, offset, size, out_range_info);
        };
};