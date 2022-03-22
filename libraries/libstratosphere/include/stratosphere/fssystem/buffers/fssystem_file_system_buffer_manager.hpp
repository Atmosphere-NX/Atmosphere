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
#include <stratosphere/lmem.hpp>
#include <stratosphere/fs/fs_memory_management.hpp>
#include <stratosphere/fssystem/buffers/fssystem_i_buffer_manager.hpp>
#include <stratosphere/fssystem/buffers/fssystem_file_system_buddy_heap.hpp>

namespace ams::fssystem {

    class FileSystemBufferManager : public IBufferManager {
        NON_COPYABLE(FileSystemBufferManager);
        NON_MOVEABLE(FileSystemBufferManager);
        public:
            using BuddyHeap = FileSystemBuddyHeap;
        private:
            class CacheHandleTable {
                NON_COPYABLE(CacheHandleTable);
                NON_MOVEABLE(CacheHandleTable);
                private:
                    class Entry {
                        private:
                            CacheHandle m_handle;
                            uintptr_t m_address;
                            size_t m_size;
                            BufferAttribute m_attr;
                        public:
                            constexpr void Initialize(CacheHandle h, uintptr_t a, size_t sz, BufferAttribute t) {
                                m_handle  = h;
                                m_address = a;
                                m_size    = sz;
                                m_attr    = t;
                            }

                            constexpr CacheHandle GetHandle() const {
                                return m_handle;
                            }

                            constexpr uintptr_t GetAddress() const {
                                return m_address;
                            }

                            constexpr size_t GetSize() const {
                                return m_size;
                            }

                            constexpr BufferAttribute GetBufferAttribute() const {
                                return m_attr;
                            }
                    };

                    class AttrInfo : public util::IntrusiveListBaseNode<AttrInfo>, public ::ams::fs::impl::Newable {
                        NON_COPYABLE(AttrInfo);
                        NON_MOVEABLE(AttrInfo);
                        private:
                            s32 m_level;
                            s32 m_cache_count;
                            size_t m_cache_size;
                        public:
                            constexpr AttrInfo(s32 l, s32 cc, size_t cs) : m_level(l), m_cache_count(cc), m_cache_size(cs) {
                                /* ... */
                            }

                            constexpr s32 GetLevel() const {
                                return m_level;
                            }

                            constexpr s32 GetCacheCount() const {
                                return m_cache_count;
                            }

                            constexpr void IncrementCacheCount() {
                                ++m_cache_count;
                            }

                            constexpr void DecrementCacheCount() {
                                --m_cache_count;
                            }

                            constexpr size_t GetCacheSize() const {
                                return m_cache_size;
                            }

                            constexpr void AddCacheSize(size_t diff) {
                                m_cache_size += diff;
                            }

                            constexpr void SubtractCacheSize(size_t diff) {
                                AMS_ASSERT(m_cache_size >= diff);
                                m_cache_size -= diff;
                            }

                            using Newable::operator new;
                            using Newable::operator delete;

                            static ALWAYS_INLINE void *operator new(size_t, void *p) noexcept { return p; }
                            static ALWAYS_INLINE void operator delete(void *, size_t, void*) noexcept { /* ... */ }
                    };

                    using AttrListTraits = util::IntrusiveListBaseTraits<AttrInfo>;
                    using AttrList       = typename AttrListTraits::ListType;
                private:
                    std::unique_ptr<char[], ::ams::fs::impl::Deleter> m_internal_entry_buffer;
                    char *m_external_entry_buffer;
                    size_t m_entry_buffer_size;
                    Entry *m_entries;
                    s32 m_entry_count;
                    s32 m_entry_count_max;
                    AttrList m_attr_list;
                    char *m_external_attr_info_buffer;
                    s32 m_external_attr_info_count;
                    s32 m_cache_count_min;
                    size_t m_cache_size_min;
                    size_t m_total_cache_size;
                    CacheHandle m_current_handle;
                public:
                    static constexpr size_t QueryWorkBufferSize(s32 max_cache_count) {
                        AMS_ASSERT(max_cache_count > 0);
                        const auto entry_size     = sizeof(Entry) * max_cache_count;
                        const auto attr_list_size = sizeof(AttrInfo) * 0x100;
                        return util::AlignUp(entry_size + attr_list_size + alignof(Entry) + alignof(AttrInfo), 8);
                    }
                public:
                    CacheHandleTable() : m_internal_entry_buffer(), m_external_entry_buffer(), m_entry_buffer_size(), m_entries(), m_entry_count(), m_entry_count_max(), m_attr_list(), m_external_attr_info_buffer(), m_external_attr_info_count(), m_cache_count_min(), m_cache_size_min(), m_total_cache_size(), m_current_handle() {
                        /* ... */
                    }

                    ~CacheHandleTable() {
                        this->Finalize();
                    }

                    Result Initialize(s32 max_cache_count);
                    Result Initialize(s32 max_cache_count, void *work, size_t work_size) {
                        const auto aligned_entry_buf = util::AlignUp(reinterpret_cast<uintptr_t>(work), alignof(Entry));
                        m_external_entry_buffer  = reinterpret_cast<char *>(aligned_entry_buf);
                        m_entry_buffer_size      = sizeof(Entry) * max_cache_count;

                        const auto aligned_attr_info_buf = util::AlignUp(reinterpret_cast<uintptr_t>(m_external_entry_buffer + m_entry_buffer_size), alignof(AttrInfo));
                        const auto work_end              = reinterpret_cast<uintptr_t>(work) + work_size;
                        m_external_attr_info_buffer  = reinterpret_cast<char *>(aligned_attr_info_buf);
                        m_external_attr_info_count   = static_cast<s32>((work_end - aligned_attr_info_buf) / sizeof(AttrInfo));

                        return ResultSuccess();
                    }

                    void Finalize();

                    bool Register(CacheHandle *out, uintptr_t address, size_t size, const BufferAttribute &attr);
                    bool Unregister(uintptr_t *out_address, size_t *out_size, CacheHandle handle);


                    bool UnregisterOldest(uintptr_t *out_address, size_t *out_size, const BufferAttribute &attr, size_t required_size = 0);

                    CacheHandle PublishCacheHandle();

                    size_t GetTotalCacheSize() const;
                private:
                    void UnregisterCore(uintptr_t *out_address, size_t *out_size, Entry *entry);

                    Entry *AcquireEntry(uintptr_t address, size_t size, const BufferAttribute &attr);

                    void ReleaseEntry(Entry *entry);

                    AttrInfo *FindAttrInfo(const BufferAttribute &attr);

                    s32 GetCacheCountMin(const BufferAttribute &attr) {
                        AMS_UNUSED(attr);
                        return m_cache_count_min;
                    }

                    size_t GetCacheSizeMin(const BufferAttribute &attr) {
                        AMS_UNUSED(attr);
                        return m_cache_size_min;
                    }
            };
        private:
            BuddyHeap m_buddy_heap;
            CacheHandleTable m_cache_handle_table;
            size_t m_total_size;
            size_t m_peak_free_size;
            size_t m_peak_total_allocatable_size;
            size_t m_retried_count;
            mutable os::SdkRecursiveMutex m_mutex;
        public:
            static constexpr size_t QueryWorkBufferSize(s32 max_cache_count, s32 max_order) {
                const auto buddy_size = FileSystemBuddyHeap::QueryWorkBufferSize(max_order);
                const auto table_size = CacheHandleTable::QueryWorkBufferSize(max_cache_count);
                return buddy_size + table_size;
            }
        public:
            FileSystemBufferManager() : m_total_size(), m_peak_free_size(), m_peak_total_allocatable_size(), m_retried_count(), m_mutex() { /* ... */ }

            virtual ~FileSystemBufferManager() { /* ... */ }

            Result Initialize(s32 max_cache_count, uintptr_t address, size_t buffer_size, size_t block_size) {
                AMS_ASSERT(buffer_size > 0);
                R_TRY(m_cache_handle_table.Initialize(max_cache_count));
                R_TRY(m_buddy_heap.Initialize(address, buffer_size, block_size));

                m_total_size                  = m_buddy_heap.GetTotalFreeSize();
                m_peak_free_size              = m_total_size;
                m_peak_total_allocatable_size = m_total_size;

                return ResultSuccess();
            }

            Result Initialize(s32 max_cache_count, uintptr_t address, size_t buffer_size, size_t block_size, s32 max_order) {
                AMS_ASSERT(buffer_size > 0);
                R_TRY(m_cache_handle_table.Initialize(max_cache_count));
                R_TRY(m_buddy_heap.Initialize(address, buffer_size, block_size, max_order));

                m_total_size                  = m_buddy_heap.GetTotalFreeSize();
                m_peak_free_size              = m_total_size;
                m_peak_total_allocatable_size = m_total_size;

                return ResultSuccess();
            }

            Result Initialize(s32 max_cache_count, uintptr_t address, size_t buffer_size, size_t block_size, void *work, size_t work_size) {
                const auto table_size = CacheHandleTable::QueryWorkBufferSize(max_cache_count);
                const auto buddy_size = work_size - table_size;
                AMS_ASSERT(work_size > table_size);
                const auto table_buffer = static_cast<char *>(work);
                const auto buddy_buffer = table_buffer + table_size;

                R_TRY(m_cache_handle_table.Initialize(max_cache_count, table_buffer, table_size));
                R_TRY(m_buddy_heap.Initialize(address, buffer_size, block_size, buddy_buffer, buddy_size));

                m_total_size                  = m_buddy_heap.GetTotalFreeSize();
                m_peak_free_size              = m_total_size;
                m_peak_total_allocatable_size = m_total_size;

                return ResultSuccess();
            }

            Result Initialize(s32 max_cache_count, uintptr_t address, size_t buffer_size, size_t block_size, s32 max_order, void *work, size_t work_size) {
                const auto table_size = CacheHandleTable::QueryWorkBufferSize(max_cache_count);
                const auto buddy_size = work_size - table_size;
                AMS_ASSERT(work_size > table_size);
                const auto table_buffer = static_cast<char *>(work);
                const auto buddy_buffer = table_buffer + table_size;

                R_TRY(m_cache_handle_table.Initialize(max_cache_count, table_buffer, table_size));
                R_TRY(m_buddy_heap.Initialize(address, buffer_size, block_size, max_order, buddy_buffer, buddy_size));

                m_total_size                  = m_buddy_heap.GetTotalFreeSize();
                m_peak_free_size              = m_total_size;
                m_peak_total_allocatable_size = m_total_size;

                return ResultSuccess();
            }

            void Finalize() {
                m_buddy_heap.Finalize();
                m_cache_handle_table.Finalize();
            }
        private:
            virtual const std::pair<uintptr_t, size_t> AllocateBufferImpl(size_t size, const BufferAttribute &attr) override;

            virtual void DeallocateBufferImpl(uintptr_t address, size_t size) override;

            virtual CacheHandle RegisterCacheImpl(uintptr_t address, size_t size, const BufferAttribute &attr) override;

            virtual const std::pair<uintptr_t, size_t> AcquireCacheImpl(CacheHandle handle) override;

            virtual size_t GetTotalSizeImpl() const override;

            virtual size_t GetFreeSizeImpl() const override;

            virtual size_t GetTotalAllocatableSizeImpl() const override;

            virtual size_t GetPeakFreeSizeImpl() const override;

            virtual size_t GetPeakTotalAllocatableSizeImpl() const override;

            virtual size_t GetRetriedCountImpl() const override;

            virtual void ClearPeakImpl() override;
    };

}
