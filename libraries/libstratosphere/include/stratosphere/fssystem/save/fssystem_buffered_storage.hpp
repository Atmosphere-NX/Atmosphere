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
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fs_substorage.hpp>
#include <stratosphere/fssystem/buffers/fssystem_i_buffer_manager.hpp>

namespace ams::fssystem::save {

    class BufferedStorage : public ::ams::fs::IStorage {
        NON_COPYABLE(BufferedStorage);
        NON_MOVEABLE(BufferedStorage);
        private:
            class Cache;
            class UniqueCache;
            class SharedCache;
        private:
            fs::SubStorage m_base_storage;
            IBufferManager *m_buffer_manager;
            size_t m_block_size;
            s64 m_base_storage_size;
            std::unique_ptr<Cache[]> m_caches;
            s32 m_cache_count;
            Cache *m_next_acquire_cache;
            Cache *m_next_fetch_cache;
            os::SdkMutex m_mutex;
            bool m_bulk_read_enabled;
        public:
            BufferedStorage();
            virtual ~BufferedStorage();

            Result Initialize(fs::SubStorage base_storage, IBufferManager *buffer_manager, size_t block_size, s32 buffer_count);
            void Finalize();

            bool IsInitialized() const { return m_caches != nullptr; }

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;

            virtual Result GetSize(s64 *out) override;
            virtual Result SetSize(s64 size) override;

            virtual Result Flush() override;
            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
            using IStorage::OperateRange;

            void InvalidateCaches();

            IBufferManager *GetBufferManager() const { return m_buffer_manager; }

            void EnableBulkRead() { m_bulk_read_enabled = true; }
        private:
            Result PrepareAllocation();
            Result ControlDirtiness();
            Result ReadCore(s64 offset, void *buffer, size_t size);

            bool ReadHeadCache(s64 *offset, void *buffer, size_t *size, s64 *buffer_offset);
            bool ReadTailCache(s64 offset, void *buffer, size_t *size, s64 buffer_offset);

            Result BulkRead(s64 offset, void *buffer, size_t size, bool head_cache_needed, bool tail_cache_needed);

            Result WriteCore(s64 offset, const void *buffer, size_t size);
    };

}
