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

#include "fs_istorage.hpp"

class MitmProxyStorage : public IStorage {
    private:
        Service srv_holder;
        std::unique_ptr<IStorage> fwd_storage;
    public:
        MitmProxyStorage(std::unique_ptr<IStorage> st, Service sr = {}) : srv_holder(sr), fwd_storage(std::move(st)) {
            /* ... */
        }
        virtual ~MitmProxyStorage() {
            if (serviceIsActive(&srv_holder)) {
                serviceClose(&srv_holder);
            }
        }
    public:
        virtual Result Read(void *buffer, size_t size, u64 offset) override {
            return this->fwd_storage->Read(buffer, size, offset);
        }

        virtual Result Write(void *buffer, size_t size, u64 offset) override {
            return this->fwd_storage->Write(buffer, size, offset);
        }

        virtual Result Flush() override {
            return this->fwd_storage->Flush();
        }

        virtual Result GetSize(u64 *out_size) override {
            return this->fwd_storage->GetSize(out_size);
        }

        virtual Result SetSize(u64 size) override {
            return this->fwd_storage->SetSize(size);
        }

        virtual Result OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) override {
            return this->fwd_storage->OperateRange(operation_type, offset, size, out_range_info);
        }
};
