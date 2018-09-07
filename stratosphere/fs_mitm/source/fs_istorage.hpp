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

enum class FsIStorageCmd {
    Read = 0,
    Write = 1,
    Flush = 2,
    SetSize = 3,
    GetSize = 4,
    OperateRange = 5,
};

class IStorage {
    public:
        virtual ~IStorage();
        
        virtual IStorage *Clone() = 0; 
        
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
        
        IStorageInterface *clone() override {
            return new IStorageInterface(this->base_storage->Clone());
        }
        
        ~IStorageInterface() {
            delete base_storage;
        };
        
        Result dispatch(IpcParsedCommand &r, IpcCommand &out_c, u64 cmd_id, u8 *pointer_buffer, size_t pointer_buffer_size) final {
            Result rc = 0xF601;
            switch ((FsIStorageCmd)cmd_id) {
                case FsIStorageCmd::Read:
                    rc = WrapIpcCommandImpl<&IStorageInterface::read>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                    break;
                case FsIStorageCmd::Write:
                    rc = WrapIpcCommandImpl<&IStorageInterface::write>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                    break;
                case FsIStorageCmd::Flush:
                    rc = WrapIpcCommandImpl<&IStorageInterface::flush>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                    break;
                case FsIStorageCmd::SetSize:
                    rc = WrapIpcCommandImpl<&IStorageInterface::set_size>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                    break;
                case FsIStorageCmd::GetSize:
                    rc = WrapIpcCommandImpl<&IStorageInterface::get_size>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                    break;
                case FsIStorageCmd::OperateRange:
                    if (kernelAbove400()) {
                        rc = WrapIpcCommandImpl<&IStorageInterface::operate_range>(this, r, out_c, pointer_buffer, pointer_buffer_size);
                    }
                    break;
                default:
                    break;
            }
            return rc;
        };
        
        Result handle_deferred() final {
            /* TODO: Panic, we can never defer. */
            return 0;
        };
    private:
        /* Actual command API. */
        virtual std::tuple<Result> read(OutBuffer<u8, BufferType_Type1> buffer, u64 offset, u64 size) final {
            return {this->base_storage->Read(buffer.buffer, std::min(buffer.num_elements, size), offset)};
        };
        virtual std::tuple<Result> write(InBuffer<u8, BufferType_Type1> buffer, u64 offset, u64 size) final {
           return {this->base_storage->Write(buffer.buffer, std::min(buffer.num_elements, size), offset)};

        };
        virtual std::tuple<Result> flush() final {
            return {this->base_storage->Flush()};
        };
        virtual std::tuple<Result> set_size(u64 size) final {
            return {this->base_storage->SetSize(size)};
        };
        virtual std::tuple<Result, u64> get_size() final {
            u64 out_size = 0;
            Result rc = this->base_storage->GetSize(&out_size);
            return {rc, out_size};
        };
        virtual std::tuple<Result, FsRangeInfo> operate_range(u32 operation_type, u64 offset, u64 size) final {
            FsRangeInfo out_range_info = {0};
            Result rc = this->base_storage->OperateRange(operation_type, offset, size, &out_range_info);
            return {rc, out_range_info};
        };
};

class IROStorage : public IStorage {
    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) = 0;
        Result Write(void *buffer, size_t size, u64 offset) final {
            (void)(buffer);
            (void)(offset);
            (void)(size);
            return 0x313802;
        };
        Result Flush() final {
            return 0x0;
        };
        Result SetSize(u64 size) final {
            (void)(size);
            return 0x313802;
        };
        virtual Result GetSize(u64 *out_size) = 0;
        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) = 0;
};
