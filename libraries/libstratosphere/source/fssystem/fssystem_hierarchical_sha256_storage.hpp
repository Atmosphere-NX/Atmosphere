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
#include <stratosphere.hpp>

namespace ams::fssystem {

    class HierarchicalSha256Storage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(HierarchicalSha256Storage);
        NON_MOVEABLE(HierarchicalSha256Storage);
        public:
            static constexpr s32 LayerCount  = 3;
            static constexpr size_t HashSize = crypto::Sha256Generator::HashSize;
        private:
            os::Mutex mutex;
            IStorage *base_storage;
            s64 base_storage_size;
            char *hash_buffer;
            size_t hash_buffer_size;
            s32 hash_target_block_size;
            s32 log_size_ratio;
        public:
            HierarchicalSha256Storage() : mutex(false) { /* ... */ }

            Result Initialize(IStorage **base_storages, s32 layer_count, size_t htbs, void *hash_buf, size_t hash_buf_size);

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;

            virtual Result GetSize(s64 *out) override {
                return this->base_storage->GetSize(out);
            }

            virtual Result Flush() override {
                return ResultSuccess();
            }

            virtual Result SetSize(s64 size) override {
                return fs::ResultUnsupportedOperationInHierarchicalSha256StorageA();
            }
    };

}
