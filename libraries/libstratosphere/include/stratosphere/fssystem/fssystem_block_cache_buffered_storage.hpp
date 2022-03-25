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
#include <stratosphere/fs/fs_storage_type.hpp>
#include <stratosphere/fs/fs_istorage.hpp>
#include <stratosphere/fs/fs_memory_management.hpp>
#include <stratosphere/fssystem/buffers/fssystem_file_system_buffer_manager.hpp>
#include <stratosphere/fssystem/impl/fssystem_block_cache_manager.hpp>

namespace ams::fssystem {

    constexpr inline size_t IntegrityMinLayerCount          = 2;
    constexpr inline size_t IntegrityMaxLayerCount          = 7;
    constexpr inline size_t IntegrityLayerCountSave         = 5;
    constexpr inline size_t IntegrityLayerCountSaveDataMeta = 4;

    struct FileSystemBufferManagerSet {
        fs::IBufferManager *buffers[IntegrityMaxLayerCount];
    };
    static_assert(util::is_pod<FileSystemBufferManagerSet>::value);

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class BlockCacheBufferedStorage : public ::ams::fs::IStorage {
        NON_COPYABLE(BlockCacheBufferedStorage);
        NON_MOVEABLE(BlockCacheBufferedStorage);
        public:
            static constexpr size_t DefaultMaxCacheEntryCount = 24;
        private:
            using MemoryRange = fs::IBufferManager::MemoryRange;

            struct AccessRange {
                s64 offset;
                size_t size;

                s64 GetEndOffset() const {
                    return this->offset + this->size;
                }

                bool IsIncluded(s64 ofs) const {
                    return this->offset <= ofs && ofs < this->GetEndOffset();
                }
            };
            static_assert(util::is_pod<AccessRange>::value);

            struct CacheEntry {
                AccessRange range;
                bool is_valid;
                bool is_write_back;
                bool is_cached;
                bool is_flushing;
                u16 lru_counter;
                fs::IBufferManager::CacheHandle handle;
                uintptr_t memory_address;
                size_t memory_size;

                void Invalidate() {
                    this->is_write_back = false;
                    this->is_flushing   = false;
                }

                bool IsAllocated() const {
                    return this->is_valid && (this->is_write_back ? this->memory_address != 0 : this->handle != 0);
                }

                bool IsWriteBack() const {
                    return this->is_write_back;
                }
            };
            static_assert(util::is_pod<CacheEntry>::value);

            using BlockCacheManager = ::ams::fssystem::impl::BlockCacheManager<CacheEntry, fs::IBufferManager>;
            using CacheIndex        = BlockCacheManager::CacheIndex;

            enum Flag : s32 {
                Flag_KeepBurstMode = (1 << 8),
                Flag_RealData      = (1 << 10),
            };
        private:
            os::SdkRecursiveMutex *m_mutex;
            IStorage *m_data_storage;
            Result m_last_result;
            s64 m_data_size;
            size_t m_verification_block_size;
            size_t m_verification_block_shift;
            s32 m_flags;
            s32 m_buffer_level;
            BlockCacheManager m_block_cache_manager;
            bool m_is_writable;
        public:
            BlockCacheBufferedStorage();
            virtual ~BlockCacheBufferedStorage() override;

            Result Initialize(fs::IBufferManager *bm, os::SdkRecursiveMutex *mtx, IStorage *data, s64 data_size, size_t verif_block_size, s32 max_cache_entries, bool is_real_data, s8 buffer_level, bool is_keep_burst_mode, bool is_writable);
            void Finalize();

            virtual Result Read(s64 offset, void *buffer, size_t size) override;
            virtual Result Write(s64 offset, const void *buffer, size_t size) override;

            virtual Result SetSize(s64) override { R_THROW(fs::ResultUnsupportedSetSizeForBlockCacheBufferedStorage()); }
            virtual Result GetSize(s64 *out) override;

            virtual Result Flush() override;

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override;
            using IStorage::OperateRange;

            Result Commit();
            Result OnRollback();

            bool IsEnabledKeepBurstMode() const {
                return (m_flags & Flag_KeepBurstMode) != 0;
            }

            bool IsRealDataCache() const {
                return (m_flags & Flag_RealData) != 0;
            }

            void SetKeepBurstMode(bool en) {
                if (en) {
                    m_flags |= Flag_KeepBurstMode;
                } else {
                    m_flags &= ~Flag_KeepBurstMode;
                }
            }

            void SetRealDataCache(bool en) {
                if (en) {
                    m_flags |= Flag_RealData;
                } else {
                    m_flags &= ~Flag_RealData;
                }
            }
        private:
            Result FillZeroImpl(s64 offset, s64 size);
            Result DestroySignatureImpl(s64 offset, s64 size);
            Result InvalidateImpl();
            Result QueryRangeImpl(void *dst, size_t dst_size, s64 offset, s64 size);

            Result GetAssociateBuffer(MemoryRange *out_range, CacheEntry *out_entry, s64 offset, size_t ideal_size, bool is_allocate_for_write);

            Result StoreOrDestroyBuffer(CacheIndex *out, const MemoryRange &range, CacheEntry *entry);

            Result StoreOrDestroyBuffer(const MemoryRange &range, CacheEntry *entry) {
                AMS_ASSERT(entry != nullptr);

                CacheIndex dummy;
                R_RETURN(this->StoreOrDestroyBuffer(std::addressof(dummy), range, entry));
            }

            Result FlushCacheEntry(CacheIndex index, bool invalidate);
            Result FlushRangeCacheEntries(s64 offset, s64 size, bool invalidate);

            Result FlushAllCacheEntries();
            Result InvalidateAllCacheEntries();
            Result ControlDirtiness();

            Result UpdateLastResult(Result result);

            Result ReadHeadCache(MemoryRange *out_range, CacheEntry *out_entry, bool *out_cache_needed, s64 *offset, s64 *aligned_offset, s64 aligned_offset_end, char **buffer, size_t *size);
            Result ReadTailCache(MemoryRange *out_range, CacheEntry *out_entry, bool *out_cache_needed, s64 offset, s64 aligned_offset, s64 *aligned_offset_end, char *buffer, size_t *size);

            Result BulkRead(s64 offset, void *buffer, size_t size, MemoryRange *range_head, MemoryRange *range_tail, CacheEntry *entry_head, CacheEntry *entry_tail, bool head_cache_needed, bool tail_cache_needed);
    };

}
