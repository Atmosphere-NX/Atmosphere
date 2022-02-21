/*
 * Copyright (c) Atmosphère-NX
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

    template<typename BaseStorageType>
    class HierarchicalSha256Storage : public ::ams::fs::IStorage, public ::ams::fs::impl::Newable {
        NON_COPYABLE(HierarchicalSha256Storage);
        NON_MOVEABLE(HierarchicalSha256Storage);
        public:
            static constexpr s32 LayerCount  = 3;
            static constexpr size_t HashSize = crypto::Sha256Generator::HashSize;
        private:
            BaseStorageType m_base_storage;
            s64 m_base_storage_size;
            char *m_hash_buffer;
            size_t m_hash_buffer_size;
            s32 m_hash_target_block_size;
            s32 m_log_size_ratio;
            fssystem::IHash256GeneratorFactory *m_hash_generator_factory;
            os::SdkMutex m_mutex;
        public:
            HierarchicalSha256Storage() : m_mutex() { /* ... */ }

            Result Initialize(BaseStorageType *base_storages, s32 layer_count, size_t htbs, void *hash_buf, size_t hash_buf_size, fssystem::IHash256GeneratorFactory *hgf);

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;

            virtual Result GetSize(s64 *out) override {
                return m_base_storage->GetSize(out);
            }

            virtual Result Flush() override {
                return m_base_storage->Flush();
            }

            virtual Result SetSize(s64 size) override {
                AMS_UNUSED(size);
                return fs::ResultUnsupportedOperationInHierarchicalSha256StorageA();
            }
    };

}
