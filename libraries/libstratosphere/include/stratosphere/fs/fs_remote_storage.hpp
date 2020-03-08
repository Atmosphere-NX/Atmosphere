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
#include "fs_common.hpp"
#include "fs_istorage.hpp"
#include "impl/fs_newable.hpp"

namespace ams::fs {

    class RemoteStorage : public IStorage, public impl::Newable {
        private:
            std::unique_ptr<::FsStorage, impl::Deleter> base_storage;
        public:
            RemoteStorage(::FsStorage &s) {
                this->base_storage = impl::MakeUnique<::FsStorage>();
                *this->base_storage = s;
            }

            virtual ~RemoteStorage() { fsStorageClose(this->base_storage.get()); }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                return fsStorageRead(this->base_storage.get(), offset, buffer, size);
            };

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                return fsStorageWrite(this->base_storage.get(), offset, buffer, size);
            };

            virtual Result Flush() override {
                return fsStorageFlush(this->base_storage.get());
            };

            virtual Result GetSize(s64 *out_size) override {
                return fsStorageGetSize(this->base_storage.get(), out_size);
            };

            virtual Result SetSize(s64 size) override {
                return fsStorageSetSize(this->base_storage.get(), size);
            };

            virtual Result OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                /* TODO: How to deal with this? */
                return fs::ResultUnsupportedOperation();
            };
    };

}
