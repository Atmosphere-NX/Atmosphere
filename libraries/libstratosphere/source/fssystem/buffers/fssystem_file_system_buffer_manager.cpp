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
#include <stratosphere.hpp>

namespace ams::fssystem {

    Result FileSystemBufferManager::CacheHandleTable::Initialize(s32 max_cache_count) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->entries == nullptr);
        AMS_ASSERT(this->internal_entry_buffer == nullptr);

        /* If we don't have an external buffer, try to allocate an internal one. */
        if (this->external_entry_buffer == nullptr) {
            this->entry_buffer_size     = sizeof(Entry) * max_cache_count;
            this->internal_entry_buffer = fs::impl::MakeUnique<char[]>(this->entry_buffer_size);
        }

        /* We need to have at least one entry buffer. */
        R_UNLESS(this->internal_entry_buffer != nullptr || this->external_entry_buffer != nullptr, fs::ResultAllocationFailureInFileSystemBufferManagerA());

        /* Set entries. */
        this->entries         = reinterpret_cast<Entry *>(this->external_entry_buffer != nullptr ? this->external_entry_buffer : this->internal_entry_buffer.get());
        this->entry_count     = 0;
        this->entry_count_max = max_cache_count;
        AMS_ASSERT(this->entries != nullptr);

        this->cache_count_min = max_cache_count / 16;
        this->cache_size_min  = this->cache_count_min * 0x100;

        return ResultSuccess();
    }

    void FileSystemBufferManager::CacheHandleTable::Finalize() {
        if (this->entries != nullptr) {
            AMS_ASSERT(this->entry_count == 0);

            if (this->external_attr_info_buffer == nullptr) {
                auto it = this->attr_list.begin();
                while (it != this->attr_list.end()) {
                    const auto attr_info = std::addressof(*it);
                    it = this->attr_list.erase(it);
                    delete attr_info;
                }
            }

            this->internal_entry_buffer.reset();
            this->external_entry_buffer = nullptr;
            this->entry_buffer_size     = 0;
            this->entries               = nullptr;
            this->total_cache_size      = 0;
        }
    }

    bool FileSystemBufferManager::CacheHandleTable::Register(CacheHandle *out, uintptr_t address, size_t size, const BufferAttribute &attr) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->entries != nullptr);
        AMS_ASSERT(out != nullptr);

        /* Get the entry. */
        auto entry = this->AcquireEntry(address, size, attr);

        /* If we don't have an entry, we can't register. */
        if (entry == nullptr) {
            return false;
        }

        /* Get the attr info. If we have one, increment. */
        if (const auto attr_info = this->FindAttrInfo(attr); attr_info != nullptr) {
            attr_info->IncrementCacheCount();
            attr_info->AddCacheSize(size);
        } else {
            /* Make a new attr info and add it to the list. */
            AttrInfo *new_info = nullptr;

            if (this->external_attr_info_buffer == nullptr) {
                new_info = new AttrInfo(attr.GetLevel(), 1, size);
            } else if (0 <= attr.GetLevel() && attr.GetLevel() < this->external_attr_info_count) {
                const auto buffer = this->external_attr_info_buffer + attr.GetLevel() * sizeof(AttrInfo);
                new_info = new (buffer) AttrInfo(attr.GetLevel(), 1, size);
            }

            /* If we failed to make a new attr info, we can't register. */
            if (new_info == nullptr) {
                this->ReleaseEntry(entry);
                return false;
            }

            this->attr_list.push_back(*new_info);
        }

        this->total_cache_size += size;
        *out = entry->GetHandle();
        return true;
    }

    bool FileSystemBufferManager::CacheHandleTable::Unregister(uintptr_t *out_address, size_t *out_size, CacheHandle handle) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->entries != nullptr);
        AMS_ASSERT(out_address != nullptr);
        AMS_ASSERT(out_size != nullptr);

        /* Find the lower bound for the entry. */
        const auto entry = std::lower_bound(this->entries, this->entries + this->entry_count, handle, [](const Entry &entry, CacheHandle handle) {
            return entry.GetHandle() < handle;
        });

        /* If the entry is a match, unregister it. */
        if (entry != this->entries + this->entry_count && entry->GetHandle() == handle) {
            this->UnregisterCore(out_address, out_size, entry);
            return true;
        } else {
            return false;
        }
    }

    bool FileSystemBufferManager::CacheHandleTable::UnregisterOldest(uintptr_t *out_address, size_t *out_size, const BufferAttribute &attr, size_t required_size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->entries != nullptr);
        AMS_ASSERT(out_address != nullptr);
        AMS_ASSERT(out_size != nullptr);

        /* If we have no entries, we can't unregister any. */
        if (this->entry_count == 0) {
            return false;
        }

        const auto CanUnregister = [this](const Entry &entry) {
            const auto attr_info = this->FindAttrInfo(entry.GetBufferAttribute());
            AMS_ASSERT(attr_info != nullptr);

            const auto ccm = this->GetCacheCountMin(entry.GetBufferAttribute());
            const auto csm = this->GetCacheSizeMin(entry.GetBufferAttribute());

            return ccm < attr_info->GetCacheCount() && csm + entry.GetSize() <= attr_info->GetCacheSize();
        };

        /* Find an entry, falling back to the first entry. */
        auto entry = std::find_if(this->entries, this->entries + this->entry_count, CanUnregister);
        if (entry == this->entries + this->entry_count) {
            entry = this->entries;
        }

        AMS_ASSERT(entry != this->entries + this->entry_count);
        this->UnregisterCore(out_address, out_size, entry);
        return true;
    }

    void FileSystemBufferManager::CacheHandleTable::UnregisterCore(uintptr_t *out_address, size_t *out_size, Entry *entry) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->entries != nullptr);
        AMS_ASSERT(out_address != nullptr);
        AMS_ASSERT(out_size != nullptr);
        AMS_ASSERT(entry != nullptr);

        /* Get the attribute info. */
        const auto attr_info = this->FindAttrInfo(entry->GetBufferAttribute());
        AMS_ASSERT(attr_info != nullptr);
        AMS_ASSERT(attr_info->GetCacheCount() > 0);
        AMS_ASSERT(attr_info->GetCacheSize() >= entry->GetSize());

        /* Release from the attr info. */
        attr_info->DecrementCacheCount();
        attr_info->SubtractCacheSize(entry->GetSize());

        /* Release from cached size. */
        AMS_ASSERT(this->total_cache_size >= entry->GetSize());
        this->total_cache_size -= entry->GetSize();

        /* Release the entry. */
        *out_address = entry->GetAddress();
        *out_size    = entry->GetSize();
        this->ReleaseEntry(entry);
    }

    FileSystemBufferManager::CacheHandle FileSystemBufferManager::CacheHandleTable::PublishCacheHandle() {
        AMS_ASSERT(this->entries != nullptr);
        return (++this->current_handle);
    }

    size_t FileSystemBufferManager::CacheHandleTable::GetTotalCacheSize() const {
        return this->total_cache_size;
    }

    FileSystemBufferManager::CacheHandleTable::Entry *FileSystemBufferManager::CacheHandleTable::AcquireEntry(uintptr_t address, size_t size, const BufferAttribute &attr) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->entries != nullptr);

        Entry *entry = nullptr;
        if (this->entry_count < this->entry_count_max) {
            entry = this->entries + this->entry_count;
            entry->Initialize(this->PublishCacheHandle(), address, size, attr);
            ++this->entry_count;
            AMS_ASSERT(this->entry_count == 1 || (entry-1)->GetHandle() < entry->GetHandle());
        }

        return entry;
    }

    void FileSystemBufferManager::CacheHandleTable::ReleaseEntry(Entry *entry) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->entries != nullptr);
        AMS_ASSERT(entry != nullptr);

        /* Ensure the entry is valid. */
        const auto entry_buffer = this->external_entry_buffer != nullptr ? this->external_entry_buffer : this->internal_entry_buffer.get();
        AMS_ASSERT(static_cast<void *>(entry_buffer) <= static_cast<void *>(entry));
        AMS_ASSERT(static_cast<void *>(entry) < static_cast<void *>(entry_buffer + this->entry_buffer_size));

        /* Copy the entries back by one. */
        std::memmove(entry, entry + 1, sizeof(Entry) * (this->entry_count - ((entry + 1) - this->entries)));

        /* Decrement our entry count. */
        --this->entry_count;
    }

    FileSystemBufferManager::CacheHandleTable::AttrInfo *FileSystemBufferManager::CacheHandleTable::FindAttrInfo(const BufferAttribute &attr) {
        const auto it = std::find_if(this->attr_list.begin(), this->attr_list.end(), [&attr](const AttrInfo &info) {
            return attr.GetLevel() == info.GetLevel();
        });

        return it != this->attr_list.end() ? std::addressof(*it) : nullptr;
    }

    const std::pair<uintptr_t, size_t> FileSystemBufferManager::AllocateBufferImpl(size_t size, const BufferAttribute &attr) {
        std::scoped_lock lk(this->mutex);

        std::pair<uintptr_t, size_t> range = {};
        const auto order = this->buddy_heap.GetOrderFromBytes(size);
        AMS_ASSERT(order >= 0);

        while (true) {
            if (auto address = this->buddy_heap.AllocateByOrder(order); address != 0) {
                const auto allocated_size = this->buddy_heap.GetBytesFromOrder(order);
                AMS_ASSERT(size <= allocated_size);

                range.first  = reinterpret_cast<uintptr_t>(address);
                range.second = allocated_size;

                const size_t free_size = this->buddy_heap.GetTotalFreeSize();
                this->peak_free_size = std::min(this->peak_free_size, free_size);

                const size_t total_allocatable_size = free_size + this->cache_handle_table.GetTotalCacheSize();
                this->peak_total_allocatable_size = std::min(this->peak_total_allocatable_size, total_allocatable_size);
                break;
            }

            /* Deallocate a buffer. */
            uintptr_t deallocate_address = 0;
            size_t    deallocate_size    = 0;

            ++this->retried_count;
            if (this->cache_handle_table.UnregisterOldest(std::addressof(deallocate_address), std::addressof(deallocate_size), attr, size)) {
                this->DeallocateBuffer(deallocate_address, deallocate_size);
            } else {
                break;
            }
        }

        return range;
    }

    void FileSystemBufferManager::DeallocateBufferImpl(uintptr_t address, size_t size) {
        AMS_ASSERT(util::IsPowerOfTwo(size));

        std::scoped_lock lk(this->mutex);

        this->buddy_heap.Free(reinterpret_cast<void *>(address), this->buddy_heap.GetOrderFromBytes(size));
    }

    FileSystemBufferManager::CacheHandle FileSystemBufferManager::RegisterCacheImpl(uintptr_t address, size_t size, const BufferAttribute &attr) {
        std::scoped_lock lk(this->mutex);

        CacheHandle handle = 0;
        while (true) {
            /* Try to register the handle. */
            if (this->cache_handle_table.Register(std::addressof(handle), address, size, attr)) {
                break;
            }

            /* Deallocate a buffer. */
            uintptr_t deallocate_address = 0;
            size_t    deallocate_size    = 0;

            ++this->retried_count;
            if (this->cache_handle_table.UnregisterOldest(std::addressof(deallocate_address), std::addressof(deallocate_size), attr)) {
                this->DeallocateBuffer(deallocate_address, deallocate_size);
            } else {
                this->DeallocateBuffer(address, size);
                handle = this->cache_handle_table.PublishCacheHandle();
                break;
            }
        }

        return handle;
    }

    const std::pair<uintptr_t, size_t> FileSystemBufferManager::AcquireCacheImpl(CacheHandle handle) {
        std::scoped_lock lk(this->mutex);

        std::pair<uintptr_t, size_t> range = {};
        if (this->cache_handle_table.Unregister(std::addressof(range.first), std::addressof(range.second), handle)) {
            const size_t total_allocatable_size = this->buddy_heap.GetTotalFreeSize() + this->cache_handle_table.GetTotalCacheSize();
            this->peak_total_allocatable_size = std::min(this->peak_total_allocatable_size, total_allocatable_size);
        } else {
            range.first  = 0;
            range.second = 0;
        }

        return range;
    }

    size_t FileSystemBufferManager::GetTotalSizeImpl() const {
        return this->total_size;
    }

    size_t FileSystemBufferManager::GetFreeSizeImpl() const {
        std::scoped_lock lk(this->mutex);

        return this->buddy_heap.GetTotalFreeSize();
    }

    size_t FileSystemBufferManager::GetTotalAllocatableSizeImpl() const {
        return this->GetFreeSize() + this->cache_handle_table.GetTotalCacheSize();
    }

    size_t FileSystemBufferManager::GetPeakFreeSizeImpl() const {
        return this->peak_free_size;
    }

    size_t FileSystemBufferManager::GetPeakTotalAllocatableSizeImpl() const {
        return this->peak_total_allocatable_size;
    }

    size_t FileSystemBufferManager::GetRetriedCountImpl() const {
        return this->retried_count;
    }

    void FileSystemBufferManager::ClearPeakImpl() {
        this->peak_free_size = this->GetFreeSize();
        this->retried_count  = 0;
    }

}
