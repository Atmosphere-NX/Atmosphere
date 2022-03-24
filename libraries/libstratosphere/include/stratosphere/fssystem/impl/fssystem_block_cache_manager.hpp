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
#include <stratosphere/fs/impl/fs_newable.hpp>

namespace ams::fssystem::impl {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    template<typename CacheEntryType, typename AllocatorType>
    class BlockCacheManager {
        NON_COPYABLE(BlockCacheManager);
        NON_MOVEABLE(BlockCacheManager);
        public:
            using MemoryRange = typename AllocatorType::MemoryRange;
            using CacheIndex  = s32;

            using BufferAttribute = typename AllocatorType::BufferAttribute;

            static constexpr CacheIndex InvalidCacheIndex = -1;

            using CacheEntry = CacheEntryType;
            static_assert(util::is_pod<CacheEntry>::value);
        private:
            AllocatorType *m_allocator = nullptr;
            std::unique_ptr<CacheEntry[], ::ams::fs::impl::Deleter> m_entries{};
            s32 m_max_cache_entry_count = 0;
        public:
            constexpr BlockCacheManager() = default;
        public:
            Result Initialize(AllocatorType *allocator, s32 max_entries) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_allocator == nullptr);
                AMS_ASSERT(m_entries == nullptr);
                AMS_ASSERT(allocator != nullptr);

                /* Setup our entries buffer, if necessary. */
                if (max_entries > 0) {
                    /* Create the entries. */
                    m_entries = fs::impl::MakeUnique<CacheEntry[]>(static_cast<size_t>(max_entries));
                    R_UNLESS(m_entries != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

                    /* Clear the entries. */
                    std::memset(m_entries.get(), 0, sizeof(CacheEntry) * max_entries);
                }

                /* Set fields. */
                m_allocator             = allocator;
                m_max_cache_entry_count = max_entries;

                R_SUCCEED();
            }

            void Finalize() {
                /* Reset all fields. */
                m_entries.reset(nullptr);
                m_allocator             = nullptr;
                m_max_cache_entry_count = 0;
            }

            bool IsInitialized() const {
                return m_allocator != nullptr;
            }

            AllocatorType *GetAllocator() { return m_allocator; }
            s32 GetCount() const { return m_max_cache_entry_count; }

            void AcquireCacheEntry(CacheEntry *out_entry, MemoryRange *out_range, CacheIndex index) {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(index < this->GetCount());

                /* Get the entry. */
                auto &entry = m_entries[index];

                /* Set the out range. */
                if (entry.IsWriteBack()) {
                    *out_range = AllocatorType::MakeMemoryRange(entry.memory_address, entry.memory_size);
                } else {
                    *out_range = m_allocator->AcquireCache(entry.handle);
                }

                /* Set the out entry. */
                *out_entry = entry;

                /* Sanity check. */
                AMS_ASSERT(out_entry->is_valid);
                AMS_ASSERT(out_entry->is_cached);

                /* Clear our local entry. */
                entry.is_valid       = false;
                entry.handle         = 0;
                entry.memory_address = 0;
                entry.memory_size    = 0;
                entry.lru_counter    = 0;

                /* Update the out entry. */
                out_entry->is_valid       = true;
                out_entry->handle         = 0;
                out_entry->memory_address = 0;
                out_entry->memory_size    = 0;
                out_entry->lru_counter    = 0;
            }

            bool ExistsRedundantCacheEntry(const CacheEntry &entry) const {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsInitialized());

                /* Iterate over all entries, checking if any contain our extents. */
                for (auto i = 0; i < this->GetCount(); ++i) {
                    if (const auto &cur_entry = m_entries[i]; cur_entry.IsAllocated()) {
                        if (cur_entry.range.offset < entry.range.GetEndOffset() && entry.range.offset < cur_entry.range.GetEndOffset()) {
                            return true;
                        }
                    }
                }

                return false;
            }

            void GetEmptyCacheEntryIndex(CacheIndex *out_empty, CacheIndex *out_lru) {
                /* Find empty and lru indices. */
                CacheIndex empty = InvalidCacheIndex, lru = InvalidCacheIndex;
                for (auto i = 0; i < this->GetCount(); ++i) {
                    if (auto &entry = m_entries[i]; entry.is_valid) {
                        /* Get/Update the lru counter. */
                        if (entry.lru_counter != std::numeric_limits<decltype(entry.lru_counter)>::max()) {
                            ++entry.lru_counter;
                        }

                        /* Update the lru index. */
                        if (lru == InvalidCacheIndex || m_entries[lru].lru_counter < entry.lru_counter) {
                            lru = i;
                        }
                    } else {
                        /* The entry is invalid, so we can update the empty index. */
                        if (empty == InvalidCacheIndex) {
                            empty = i;
                        }
                    }
                }

                /* Set the output. */
                *out_empty = empty;
                *out_lru   = lru;
            }

            void Invalidate() {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsInitialized());

                /* Invalidate all entries. */
                for (auto i = 0; i < this->GetCount(); ++i) {
                    if (m_entries[i].is_valid) {
                        this->InvalidateCacheEntry(i);
                    }
                }
            }

            void InvalidateCacheEntry(CacheIndex index) {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(index < this->GetCount());

                /* Get the entry. */
                auto &entry = m_entries[index];
                AMS_ASSERT(entry.is_valid);

                /* If necessary, perform write-back. */
                if (entry.IsWriteBack()) {
                    AMS_ASSERT(entry.memory_address != 0 && entry.handle == 0);
                    m_allocator->DeallocateBuffer(AllocatorType::MakeMemoryRange(entry.memory_address, entry.memory_size));
                } else {
                    AMS_ASSERT(entry.memory_address == 0 && entry.handle != 0);

                    if (const auto memory_range = m_allocator->AcquireCache(entry.handle); memory_range.first) {
                        m_allocator->DeallocateBuffer(memory_range);
                    }
                }

                /* Set entry as invalid. */
                entry.is_valid = false;
                entry.Invalidate();
            }

            void RegisterCacheEntry(CacheIndex index, const MemoryRange &memory_range, const BufferAttribute &attribute) {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsInitialized());

                /* Register the entry. */
                if (auto &entry = m_entries[index]; entry.IsWriteBack()) {
                    entry.handle         = 0;
                    entry.memory_address = memory_range.first;
                    entry.memory_size    = memory_range.second;
                } else {
                    entry.handle         = m_allocator->RegisterCache(memory_range, attribute);
                    entry.memory_address = 0;
                    entry.memory_size    = 0;
                }
            }

            void ReleaseCacheEntry(CacheEntry *entry, const MemoryRange &memory_range) {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsInitialized());

                /* Release the entry. */
                m_allocator->DeallocateBuffer(memory_range);
                entry->is_valid  = false;
                entry->is_cached = false;
            }

            void ReleaseCacheEntry(CacheIndex index, const MemoryRange &memory_range) {
                return this->ReleaseCacheEntry(std::addressof(m_entries[index]), memory_range);
            }

            bool SetCacheEntry(CacheIndex index, const CacheEntry &entry, const MemoryRange &memory_range, const BufferAttribute &attr) {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(0 <= index && index < this->GetCount());

                /* Write the entry. */
                m_entries[index] = entry;

                /* Sanity check. */
                AMS_ASSERT(entry.is_valid);
                AMS_ASSERT(entry.is_cached);
                AMS_ASSERT(entry.handle == 0);
                AMS_ASSERT(entry.memory_address == 0);

                /* Register or release. */
                if (this->ExistsRedundantCacheEntry(entry)) {
                    this->ReleaseCacheEntry(index, memory_range);
                    return false;
                } else {
                    this->RegisterCacheEntry(index, memory_range, attr);
                    return true;
                }
            }

            bool SetCacheEntry(CacheIndex index, const CacheEntry &entry, const MemoryRange &memory_range) {
                const BufferAttribute attr{};
                return this->SetCacheEntry(index, entry, memory_range, attr);
            }

            void SetFlushing(CacheIndex index, bool en) {
                if constexpr (requires { m_entries[index].is_flushing; }) {
                    m_entries[index].is_flushing = en;
                }
            }

            void SetWriteBack(CacheIndex index, bool en) {
                if constexpr (requires { m_entries[index].is_write_back; }) {
                    m_entries[index].is_write_back = en;
                }
            }

            const CacheEntry &operator[](CacheIndex index) const {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsInitialized());
                AMS_ASSERT(0 <= index && index < this->GetCount());

                return m_entries[index];
            }
    };

}
