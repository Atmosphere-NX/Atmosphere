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
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/fs/fs_storage_type.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fs_memory_management.hpp>
#include <stratosphere/fssystem/save/fssystem_i_save_file_system_driver.hpp>
#include <stratosphere/fssystem/buffers/fssystem_file_system_buffer_manager.hpp>

namespace ams::fssystem::save {

    constexpr inline size_t IntegrityMinLayerCount          = 2;
    constexpr inline size_t IntegrityMaxLayerCount          = 7;
    constexpr inline size_t IntegrityLayerCountSave         = 5;
    constexpr inline size_t IntegrityLayerCountSaveDataMeta = 4;

    struct FileSystemBufferManagerSet {
        IBufferManager *buffers[IntegrityMaxLayerCount];
    };
    static_assert(util::is_pod<FileSystemBufferManagerSet>::value);

    class BlockCacheBufferedStorage : public ::ams::fs::IStorage {
        NON_COPYABLE(BlockCacheBufferedStorage);
        NON_MOVEABLE(BlockCacheBufferedStorage);
        public:
            static constexpr size_t DefaultMaxCacheEntryCount = 24;
        private:
            using MemoryRange = std::pair<uintptr_t, size_t>;
            using CacheIndex = s32;

            struct CacheEntry {
                size_t size;
                bool is_valid;
                bool is_write_back;
                bool is_cached;
                bool is_flushing;
                s64 offset;
                IBufferManager::CacheHandle handle;
                uintptr_t memory_address;
                size_t memory_size;
            };
            static_assert(util::is_pod<CacheEntry>::value);

            enum Flag : s32 {
                Flag_KeepBurstMode = (1 << 8),
                Flag_RealData      = (1 << 10),
            };
        private:
            IBufferManager *buffer_manager;
            os::Mutex *mutex;
            std::unique_ptr<CacheEntry[], ::ams::fs::impl::Deleter> entries;
            IStorage *data_storage;
            Result last_result;
            s64 data_size;
            size_t verification_block_size;
            size_t verification_block_shift;
            CacheIndex invalidate_index;
            s32 max_cache_entry_count;
            s32 flags;
            s32 buffer_level;
            fs::StorageType storage_type;
        public:
            BlockCacheBufferedStorage();
            virtual ~BlockCacheBufferedStorage() override;

            Result Initialize(IBufferManager *bm, os::Mutex *mtx, IStorage *data, s64 data_size, size_t verif_block_size, s32 max_cache_entries, bool is_real_data, s8 buffer_level, bool is_keep_burst_mode, fs::StorageType storage_type);
            void Finalize();

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;

            virtual Result SetSize(s64 size) override { return fs::ResultUnsupportedOperationInBlockCacheBufferedStorageA(); }
            virtual Result GetSize(s64 *out) override;

            virtual Result Flush() override;

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
            using IStorage::OperateRange;

            Result Commit();
            Result OnRollback();

            bool IsEnabledKeepBurstMode() const {
                return (this->flags & Flag_KeepBurstMode) != 0;
            }

            bool IsRealDataCache() const {
                return (this->flags & Flag_RealData) != 0;
            }

            void SetKeepBurstMode(bool en) {
                if (en) {
                    this->flags |= Flag_KeepBurstMode;
                } else {
                    this->flags &= ~Flag_KeepBurstMode;
                }
            }

            void SetRealDataCache(bool en) {
                if (en) {
                    this->flags |= Flag_RealData;
                } else {
                    this->flags &= ~Flag_RealData;
                }
            }
        private:
            s32 GetMaxCacheEntryCount() const {
                return this->max_cache_entry_count;
            }

            Result ClearImpl(s64 offset, s64 size);
            Result ClearSignatureImpl(s64 offset, s64 size);
            Result InvalidateCacheImpl(s64 offset, s64 size);
            Result QueryRangeImpl(void *dst, size_t dst_size, s64 offset, s64 size);

            bool ExistsRedundantCacheEntry(const CacheEntry &entry) const;

            Result GetAssociateBuffer(MemoryRange *out_range, CacheEntry *out_entry, s64 offset, size_t ideal_size, bool is_allocate_for_write);

            void DestroyBuffer(CacheEntry *entry, const MemoryRange &range);

            Result StoreAssociateBuffer(CacheIndex *out, const MemoryRange &range, const CacheEntry &entry);
            Result StoreAssociateBuffer(const MemoryRange &range, const CacheEntry &entry) {
                CacheIndex dummy;
                return this->StoreAssociateBuffer(std::addressof(dummy), range, entry);
            }

            Result StoreOrDestroyBuffer(const MemoryRange &range, CacheEntry *entry) {
                AMS_ASSERT(entry != nullptr);

                auto buf_guard = SCOPE_GUARD { this->DestroyBuffer(entry, range); };

                R_TRY(this->StoreAssociateBuffer(range, *entry));

                buf_guard.Cancel();
                return ResultSuccess();
            }

            Result FlushCacheEntry(CacheIndex index, bool invalidate);
            Result FlushRangeCacheEntries(s64 offset, s64 size, bool invalidate);
            void InvalidateRangeCacheEntries(s64 offset, s64 size);

            Result FlushAllCacheEntries();
            Result InvalidateAllCacheEntries();
            Result ControlDirtiness();

            Result UpdateLastResult(Result result);

            Result ReadHeadCache(MemoryRange *out_range, CacheEntry *out_entry, bool *out_cache_needed, s64 *offset, s64 *aligned_offset, s64 aligned_offset_end, char **buffer, size_t *size);
            Result ReadTailCache(MemoryRange *out_range, CacheEntry *out_entry, bool *out_cache_needed, s64 offset, s64 aligned_offset, s64 *aligned_offset_end, char *buffer, size_t *size);

            Result BulkRead(s64 offset, void *buffer, size_t size, MemoryRange *range_head, MemoryRange *range_tail, CacheEntry *entry_head, CacheEntry *entry_tail, bool head_cache_needed, bool tail_cache_needed);
    };

}
