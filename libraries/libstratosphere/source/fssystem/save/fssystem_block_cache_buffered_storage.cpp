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

namespace ams::fssystem::save {

    BlockCacheBufferedStorage::BlockCacheBufferedStorage()
        : buffer_manager(), mutex(), entries(), data_storage(), last_result(ResultSuccess()), data_size(), verification_block_size(), verification_block_shift(), invalidate_index(), max_cache_entry_count(), flags(), buffer_level(-1)
    {
        /* ... */
    }

    BlockCacheBufferedStorage::~BlockCacheBufferedStorage() {
        this->Finalize();
    }

    Result BlockCacheBufferedStorage::Initialize(IBufferManager *bm, os::Mutex *mtx, IStorage *data, s64 data_size, size_t verif_block_size, s32 max_cache_entries, bool is_real_data, s8 buffer_level, bool is_keep_burst_mode, fs::StorageType storage_type) {
        /* Validate preconditions. */
        AMS_ASSERT(data != nullptr);
        AMS_ASSERT(bm   != nullptr);
        AMS_ASSERT(mtx  != nullptr);
        AMS_ASSERT(this->buffer_manager == nullptr);
        AMS_ASSERT(this->mutex          == nullptr);
        AMS_ASSERT(this->data_storage   == nullptr);
        AMS_ASSERT(this->entries        == nullptr);
        AMS_ASSERT(max_cache_entries > 0);

        /* Create the entry. */
        this->entries = fs::impl::MakeUnique<CacheEntry[]>(static_cast<size_t>(max_cache_entries));
        R_UNLESS(this->entries != nullptr, fs::ResultAllocationFailureInBlockCacheBufferedStorageA());

        /* Set members. */
        this->buffer_manager           = bm;
        this->mutex                    = mtx;
        this->data_storage             = data;
        this->data_size                = data_size;
        this->verification_block_size  = verif_block_size;
        this->last_result              = ResultSuccess();
        this->invalidate_index         = 0;
        this->max_cache_entry_count    = max_cache_entries;
        this->flags                    = 0;
        this->buffer_level             = buffer_level;
        this->storage_type             = storage_type;

        /* Calculate block shift. */
        this->verification_block_shift = ILog2(static_cast<u32>(verif_block_size));
        AMS_ASSERT(static_cast<size_t>(1ull << this->verification_block_size) == this->verification_block_size);

        /* Clear the entry. */
        std::memset(this->entries.get(), 0, sizeof(CacheEntry) * this->max_cache_entry_count);

        /* Set burst mode. */
        this->SetKeepBurstMode(is_keep_burst_mode);

        /* Set real data cache. */
        this->SetRealDataCache(is_real_data);

        return ResultSuccess();
    }

    void BlockCacheBufferedStorage::Finalize() {
        if (this->entries != nullptr) {
            /* Invalidate all cache entries. */
            this->InvalidateAllCacheEntries();

            /* Clear members. */
            this->buffer_manager           = nullptr;
            this->mutex                    = nullptr;
            this->data_storage             = nullptr;
            this->data_size                = 0;
            this->verification_block_size  = 0;
            this->verification_block_shift = 0;
            this->invalidate_index         = 0;
            this->max_cache_entry_count    = 0;

            this->entries.reset();
        }
    }

    Result BlockCacheBufferedStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->data_storage != nullptr);
        AMS_ASSERT(this->buffer_manager != nullptr);

        /* Ensure we aren't already in a failed state. */
        R_TRY(this->last_result);

        /* Succeed if zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Determine the extents to read. */
        s64 read_offset  = offset;
        size_t read_size = size;

        R_UNLESS(read_offset < this->data_size, fs::ResultInvalidOffset());

        if (static_cast<s64>(read_offset + read_size) > this->data_size) {
            read_size = static_cast<size_t>(this->data_size - read_offset);
        }

        /* Determine the aligned range to read. */
        const size_t block_alignment = this->verification_block_size;
        s64 aligned_offset           = util::AlignDown(read_offset, block_alignment);
        s64 aligned_offset_end       = util::AlignUp(read_offset + read_size, block_alignment);

        AMS_ASSERT(0 <= aligned_offset && aligned_offset_end <= static_cast<s64>(util::AlignUp(this->data_size, block_alignment)));

        /* Try to read using cache. */
        char *dst = static_cast<char *>(buffer);
        {
            /* Determine if we can do bulk reads. */
            constexpr s64 BulkReadSizeMax = 2_MB;
            const bool bulk_read_enabled = (read_offset != aligned_offset || static_cast<s64>(read_offset + read_size) != aligned_offset_end) && aligned_offset_end - aligned_offset <= BulkReadSizeMax;

            /* Read the head cache. */
            CacheEntry  head_entry = {};
            MemoryRange head_range = {};
            bool head_cache_needed = true;
            R_TRY(this->ReadHeadCache(std::addressof(head_range), std::addressof(head_entry), std::addressof(head_cache_needed), std::addressof(read_offset), std::addressof(aligned_offset), aligned_offset_end, std::addressof(dst), std::addressof(read_size)));

            /* We may be done after reading the head cache, so check if we are. */
            R_SUCCEED_IF(aligned_offset >= aligned_offset_end);

            /* Ensure we destroy the head buffer. */
            auto head_guard = SCOPE_GUARD { this->DestroyBuffer(std::addressof(head_entry), head_range); };

            /* Read the tail cache. */
            CacheEntry  tail_entry = {};
            MemoryRange tail_range = {};
            bool tail_cache_needed = true;
            R_TRY(this->ReadTailCache(std::addressof(tail_range), std::addressof(tail_entry), std::addressof(tail_cache_needed), read_offset, aligned_offset, std::addressof(aligned_offset_end), dst, std::addressof(read_size)));

            /* We may be done after reading the tail cache, so check if we are. */
            R_SUCCEED_IF(aligned_offset >= aligned_offset_end);

            /* Ensure that we destroy the tail buffer. */
            auto tail_guard = SCOPE_GUARD { this->DestroyBuffer(std::addressof(tail_entry), tail_range); };

            /* Try to do a bulk read. */
            if (bulk_read_enabled) {
                /* The bulk read will destroy our head/tail buffers. */
                head_guard.Cancel();
                tail_guard.Cancel();

                do {
                    /* Do the bulk read. If we fail due to pooled buffer allocation failing, fall back to the normal read codepath. */
                    R_TRY_CATCH(this->BulkRead(read_offset, dst, read_size, std::addressof(head_range), std::addressof(tail_range), std::addressof(head_entry), std::addressof(tail_entry), head_cache_needed, tail_cache_needed)) {
                        R_CATCH(fs::ResultAllocationFailurePooledBufferNotEnoughSize) { break; }
                    } R_END_TRY_CATCH;

                    /* Se successfully did a bulk read, so we're done. */
                    return ResultSuccess();
                } while (0);
            }
        }

        /* Read the data using non-bulk reads. */
        while (aligned_offset < aligned_offset_end) {
            /* Ensure that there is data for us to read. */
            AMS_ASSERT(read_size > 0);

            /* If conditions allow us to, read in burst mode. This doesn't use the cache. */
            if (this->IsEnabledKeepBurstMode() && read_offset == aligned_offset && (block_alignment * 2 <= read_size)) {
                const size_t aligned_size = util::AlignDown(read_size, block_alignment);

                /* Flush the entries. */
                R_TRY(this->UpdateLastResult(this->FlushRangeCacheEntries(read_offset, aligned_size, false)));

                /* Read the data. */
                R_TRY(this->UpdateLastResult(this->data_storage->Read(read_offset, dst, aligned_size)));

                /* Advance. */
                dst            += aligned_size;
                read_offset    += aligned_size;
                read_size      -= aligned_size;
                aligned_offset += aligned_size;
            } else {
                /* Get the buffer associated with what we're reading. */
                CacheEntry  entry;
                MemoryRange range;
                R_TRY(this->UpdateLastResult(this->GetAssociateBuffer(std::addressof(range), std::addressof(entry), aligned_offset, static_cast<size_t>(aligned_offset_end - aligned_offset), true)));

                /* Determine where to read data into, and ensure that our entry is aligned. */
                char *src = reinterpret_cast<char *>(range.first);
                AMS_ASSERT(util::IsAligned(entry.size, block_alignment));

                /* If the entry isn't cached, read the data. */
                if (!entry.is_cached) {
                    if (Result result = this->data_storage->Read(entry.offset, src, entry.size); R_FAILED(result)) {
                        this->DestroyBuffer(std::addressof(entry), range);
                        return this->UpdateLastResult(result);
                    }
                    entry.is_cached = true;
                }

                /* Validate the entry extents. */
                AMS_ASSERT(static_cast<s64>(entry.offset) <= aligned_offset);
                AMS_ASSERT(aligned_offset < static_cast<s64>(entry.offset + entry.size));
                AMS_ASSERT(aligned_offset <= read_offset);

                /* Copy the data. */
                {
                    /* Determine where and how much to copy. */
                    const s64 buffer_offset = read_offset - entry.offset;
                    const size_t copy_size  = std::min(read_size, static_cast<size_t>(entry.offset + entry.size - read_offset));

                    /* Actually copy the data. */
                    std::memcpy(dst, src + buffer_offset, copy_size);

                    /* Advance. */
                    dst         += copy_size;
                    read_offset += copy_size;
                    read_size   -= copy_size;
                }

                /* Release the cache entry. */
                R_TRY(this->UpdateLastResult(this->StoreOrDestroyBuffer(range, std::addressof(entry))));
                aligned_offset = entry.offset + entry.size;
            }
        }

        /* Ensure that we read all the data. */
        AMS_ASSERT(read_size == 0);

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::Write(s64 offset, const void *buffer, size_t size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->data_storage != nullptr);
        AMS_ASSERT(this->buffer_manager != nullptr);

        /* Ensure we aren't already in a failed state. */
        R_TRY(this->last_result);

        /* Succeed if zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        /* Determine the extents to read. */
        R_UNLESS(offset < this->data_size, fs::ResultInvalidOffset());

        if (static_cast<s64>(offset + size) > this->data_size) {
            size = static_cast<size_t>(this->data_size - offset);
        }

        /* The actual extents may be zero-size, so succeed if that's the case. */
        R_SUCCEED_IF(size == 0);

        /* Determine the aligned range to read. */
        const size_t block_alignment = this->verification_block_size;
        s64 aligned_offset           = util::AlignDown(offset, block_alignment);
        const s64 aligned_offset_end = util::AlignUp(offset + size, block_alignment);

        AMS_ASSERT(0 <= aligned_offset && aligned_offset_end <= static_cast<s64>(util::AlignUp(this->data_size, block_alignment)));

        /* Write the data. */
        const u8 *src = static_cast<const u8 *>(buffer);
        while (aligned_offset < aligned_offset_end) {
            /* If conditions allow us to, write in burst mode. This doesn't use the cache. */
            if (this->IsEnabledKeepBurstMode() && offset == aligned_offset && (block_alignment * 2 <= size)) {
                const size_t aligned_size = util::AlignDown(size, block_alignment);

                /* Flush the entries. */
                R_TRY(this->UpdateLastResult(this->FlushRangeCacheEntries(offset, aligned_size, true)));

                /* Read the data. */
                R_TRY(this->UpdateLastResult(this->data_storage->Write(offset, src, aligned_size)));

                /* Set blocking buffer manager allocations. */
                buffers::EnableBlockingBufferManagerAllocation();

                /* Advance. */
                src            += aligned_size;
                offset         += aligned_size;
                size           -= aligned_size;
                aligned_offset += aligned_size;
            } else {
                /* Get the buffer associated with what we're writing. */
                CacheEntry  entry;
                MemoryRange range;
                R_TRY(this->UpdateLastResult(this->GetAssociateBuffer(std::addressof(range), std::addressof(entry), aligned_offset, static_cast<size_t>(aligned_offset_end - aligned_offset), true)));

                /* Determine where to write data into. */
                char *dst = reinterpret_cast<char *>(range.first);

                /* If the entry isn't cached and we're writing a partial entry, read in the entry. */
                if (!entry.is_cached && ((offset != entry.offset) || (offset + size < entry.offset + entry.size))) {
                    if (Result result = this->data_storage->Read(entry.offset, dst, entry.size); R_FAILED(result)) {
                        this->DestroyBuffer(std::addressof(entry), range);
                        return this->UpdateLastResult(result);
                    }
                }
                entry.is_cached = true;

                /* Validate the entry extents. */
                AMS_ASSERT(static_cast<s64>(entry.offset) <= aligned_offset);
                AMS_ASSERT(aligned_offset < static_cast<s64>(entry.offset + entry.size));
                AMS_ASSERT(aligned_offset <= offset);

                /* Copy the data. */
                {
                    /* Determine where and how much to copy. */
                    const s64 buffer_offset = offset - entry.offset;
                    const size_t copy_size  = std::min(size, static_cast<size_t>(entry.offset + entry.size - offset));

                    /* Actually copy the data. */
                    std::memcpy(dst + buffer_offset, src, copy_size);

                    /* Advance. */
                    src    += copy_size;
                    offset += copy_size;
                    size   -= copy_size;
                }

                /* Set the entry as write-back. */
                entry.is_write_back = true;

                /* Set blocking buffer manager allocations. */
                buffers::EnableBlockingBufferManagerAllocation();

                /* Store the associated buffer. */
                CacheIndex index;
                R_TRY(this->UpdateLastResult(this->StoreAssociateBuffer(std::addressof(index), range, entry)));

                /* If we need to, flush the cache entry. */
                if (index >= 0 && IsEnabledKeepBurstMode() && offset == aligned_offset && (block_alignment * 2 <= size)) {
                    R_TRY(this->UpdateLastResult(this->FlushCacheEntry(index, false)));
                }
            }
        }

        /* Ensure that didn't end up in a failure state. */
        R_TRY(this->last_result);

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::GetSize(s64 *out) {
        /* Validate pre-conditions. */
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(this->data_storage != nullptr);

        /* Set the size. */
        *out = this->data_size;
        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::Flush() {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->data_storage != nullptr);
        AMS_ASSERT(this->buffer_manager != nullptr);

        /* Ensure we aren't already in a failed state. */
        R_TRY(this->last_result);

        /* Flush all cache entries. */
        R_TRY(this->UpdateLastResult(this->FlushAllCacheEntries()));

        /* Flush the data storage. */
        R_TRY(this->UpdateLastResult(this->data_storage->Flush()));

        /* Set blocking buffer manager allocations. */
        buffers::EnableBlockingBufferManagerAllocation();

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->data_storage != nullptr);

        switch (op_id) {
            case fs::OperationId::Clear:
                {
                    R_TRY(this->ClearImpl(offset, size));
                    return ResultSuccess();
                }
            case fs::OperationId::ClearSignature:
                {
                    R_TRY(this->ClearSignatureImpl(offset, size));
                    return ResultSuccess();
                }
            case fs::OperationId::InvalidateCache:
                {
                    R_UNLESS(this->storage_type != fs::StorageType_SaveData, fs::ResultUnsupportedOperationInBlockCacheBufferedStorageB());
                    R_TRY(this->InvalidateCacheImpl(offset, size));
                    return ResultSuccess();
                }
            case fs::OperationId::QueryRange:
                {
                    R_TRY(this->QueryRangeImpl(dst, dst_size, offset, size));
                    return ResultSuccess();
                }
            default:
                return fs::ResultUnsupportedOperationInBlockCacheBufferedStorageC();
        }
    }

    Result BlockCacheBufferedStorage::Commit() {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->data_storage != nullptr);
        AMS_ASSERT(this->buffer_manager != nullptr);

        /* Ensure we aren't already in a failed state. */
        R_TRY(this->last_result);

        /* Flush all cache entries. */
        R_TRY(this->UpdateLastResult(this->FlushAllCacheEntries()));

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::OnRollback() {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->buffer_manager != nullptr);

        /* Ensure we aren't already in a failed state. */
        R_TRY(this->last_result);

        /* Release all valid entries back to the buffer manager. */
        const auto max_cache_entry_count = this->GetMaxCacheEntryCount();
        for (s32 i = 0; i < max_cache_entry_count; i++) {
            const auto &entry = this->entries[i];
            if (entry.is_valid) {
                if (entry.is_write_back) {
                    AMS_ASSERT(entry.memory_address != 0 && entry.handle == 0);
                    this->buffer_manager->DeallocateBuffer(entry.memory_address, entry.memory_size);
                } else {
                    AMS_ASSERT(entry.memory_address == 0 && entry.handle != 0);
                    const auto memory_range = this->buffer_manager->AcquireCache(entry.handle);
                    if (memory_range.first != 0) {
                        this->buffer_manager->DeallocateBuffer(memory_range.first, memory_range.second);
                    }
                }
            }
        }

        /* Clear all entries. */
        std::memset(this->entries.get(), 0, sizeof(CacheEntry) * max_cache_entry_count);

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::ClearImpl(s64 offset, s64 size) {
        /* Ensure we aren't already in a failed state. */
        R_TRY(this->last_result);

        /* Get our storage size. */
        s64 storage_size = 0;
        R_TRY(this->GetSize(std::addressof(storage_size)));

        /* Check the access range. */
        R_UNLESS(0 <= offset && offset < storage_size, fs::ResultInvalidOffset());

        /* Determine the extents to data signature for. */
        auto start_offset = util::AlignDown(offset, this->verification_block_size);
        auto end_offset   = util::AlignUp(std::min(offset + size, storage_size), this->verification_block_size);

        /* Flush the entries. */
        R_TRY(this->UpdateLastResult(this->FlushRangeCacheEntries(offset, size, true)));

        /* Handle any data before or after the aligned range. */
        if (start_offset < offset || offset + size < end_offset) {
            /* Allocate a work buffer. */
            std::unique_ptr<char[], fs::impl::Deleter> work = fs::impl::MakeUnique<char[]>(this->verification_block_size);
            R_UNLESS(work != nullptr, fs::ResultAllocationFailureInBlockCacheBufferedStorageB());

            /* Handle data before the aligned range. */
            if (start_offset < offset) {
                /* Read the block. */
                R_TRY(this->UpdateLastResult(this->data_storage->Read(start_offset, work.get(), this->verification_block_size)));

                /* Determine the partial extents to clear. */
                const auto clear_offset = static_cast<size_t>(offset - start_offset);
                const auto clear_size   = static_cast<size_t>(std::min(static_cast<s64>(this->verification_block_size - clear_offset), size));

                /* Clear the partial block. */
                std::memset(work.get() + clear_offset, 0, clear_size);

                /* Write the partially cleared block. */
                R_TRY(this->UpdateLastResult(this->data_storage->Write(start_offset, work.get(), this->verification_block_size)));

                /* Update the start offset. */
                start_offset += this->verification_block_size;

                /* Set blocking buffer manager allocations. */
                buffers::EnableBlockingBufferManagerAllocation();
            }

            /* Handle data after the aligned range. */
            if (start_offset < offset + size && offset + size < end_offset) {
                /* Read the block. */
                const auto last_offset = end_offset - this->verification_block_size;
                R_TRY(this->UpdateLastResult(this->data_storage->Read(last_offset, work.get(), this->verification_block_size)));

                /* Clear the partial block. */
                const auto clear_size = static_cast<size_t>((offset + size) - last_offset);
                std::memset(work.get(), 0, clear_size);

                /* Write the partially cleared block. */
                R_TRY(this->UpdateLastResult(this->data_storage->Write(last_offset, work.get(), this->verification_block_size)));

                /* Update the end offset. */
                end_offset -= this->verification_block_size;

                /* Set blocking buffer manager allocations. */
                buffers::EnableBlockingBufferManagerAllocation();
            }
        }

        /* We're done if there's no data to clear. */
        R_SUCCEED_IF(start_offset == end_offset);

        /* Clear the signature for the aligned range. */
        R_TRY(this->UpdateLastResult(this->data_storage->OperateRange(fs::OperationId::Clear, start_offset, end_offset - start_offset)));

        /* Set blocking buffer manager allocations. */
        buffers::EnableBlockingBufferManagerAllocation();

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::ClearSignatureImpl(s64 offset, s64 size) {
        /* Ensure we aren't already in a failed state. */
        R_TRY(this->last_result);

        /* Get our storage size. */
        s64 storage_size = 0;
        R_TRY(this->GetSize(std::addressof(storage_size)));

        /* Check the access range. */
        R_UNLESS(0 <= offset && offset < storage_size, fs::ResultInvalidOffset());

        /* Determine the extents to clear signature for. */
        const auto start_offset = util::AlignUp(offset, this->verification_block_size);
        const auto end_offset   = util::AlignDown(std::min(offset + size, storage_size), this->verification_block_size);

        /* Flush the entries. */
        R_TRY(this->UpdateLastResult(this->FlushRangeCacheEntries(offset, size, true)));

        /* Clear the signature for the aligned range. */
        R_TRY(this->UpdateLastResult(this->data_storage->OperateRange(fs::OperationId::ClearSignature, start_offset, end_offset - start_offset)));

        /* Set blocking buffer manager allocations. */
        buffers::EnableBlockingBufferManagerAllocation();

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::InvalidateCacheImpl(s64 offset, s64 size) {
        /* Invalidate the entries corresponding to the range. */
        /* NOTE: Nintendo does not check the result of this invalidation. */
        this->InvalidateRangeCacheEntries(offset, size);

        /* Get our storage size. */
        s64 storage_size = 0;
        R_TRY(this->GetSize(std::addressof(storage_size)));

        /* Determine the extents we can actually query. */
        const auto actual_size        = std::min(size, storage_size - offset);
        const auto aligned_offset     = util::AlignDown(offset, this->verification_block_size);
        const auto aligned_offset_end = util::AlignUp(offset + actual_size, this->verification_block_size);
        const auto aligned_size       = aligned_offset_end - aligned_offset;

        /* Invalidate the aligned range. */
        {
            Result result = this->data_storage->OperateRange(fs::OperationId::InvalidateCache, aligned_offset, aligned_size);
            AMS_ASSERT(!fs::ResultBufferAllocationFailed::Includes(result));
            R_TRY(result);
        }

        /* Clear our last result if we should. */
        if (fs::ResultIntegrityVerificationStorageCorrupted::Includes(this->last_result)) {
            this->last_result = ResultSuccess();
        }

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::QueryRangeImpl(void *dst, size_t dst_size, s64 offset, s64 size) {
        /* Get our storage size. */
        s64 storage_size = 0;
        R_TRY(this->GetSize(std::addressof(storage_size)));

        /* Determine the extents we can actually query. */
        const auto actual_size        = std::min(size, storage_size - offset);
        const auto aligned_offset     = util::AlignDown(offset, this->verification_block_size);
        const auto aligned_offset_end = util::AlignUp(offset + actual_size, this->verification_block_size);
        const auto aligned_size       = aligned_offset_end - aligned_offset;

        /* Query the aligned range. */
        R_TRY(this->UpdateLastResult(this->data_storage->OperateRange(dst, dst_size, fs::OperationId::QueryRange, aligned_offset, aligned_size, nullptr, 0)));

        return ResultSuccess();
    }

    bool BlockCacheBufferedStorage::ExistsRedundantCacheEntry(const CacheEntry &entry) const {
        /* Get the entry's extents. */
        const s64 offset  = entry.offset;
        const size_t size = entry.size;

        /* Lock our mutex. */
        std::scoped_lock lk(*this->mutex);

        /* Iterate over all entries, checking if any overlap our extents. */
        const auto max_cache_entry_count = this->GetMaxCacheEntryCount();
        for (auto i = 0; i < max_cache_entry_count; ++i) {
            const auto &entry = this->entries[i];
            if (entry.is_valid && (entry.is_write_back ? entry.memory_address != 0 : entry.handle != 0)) {
                if (entry.offset < static_cast<s64>(offset + size) && offset < static_cast<s64>(entry.offset + entry.size)) {
                    return true;
                }
            }
        }

        return false;
    }

    Result BlockCacheBufferedStorage::GetAssociateBuffer(MemoryRange *out_range, CacheEntry *out_entry, s64 offset, size_t ideal_size, bool is_allocate_for_write) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->data_storage != nullptr);
        AMS_ASSERT(this->buffer_manager != nullptr);
        AMS_ASSERT(out_range != nullptr);
        AMS_ASSERT(out_entry != nullptr);

        /* Lock our mutex. */
        std::scoped_lock lk(*this->mutex);

        /* Get the maximum cache entry count. */
        const CacheIndex max_cache_entry_count = static_cast<CacheIndex>(this->GetMaxCacheEntryCount());

        /* Locate the index of the cache entry, if present. */
        CacheIndex index;
        size_t actual_size = ideal_size;
        for (index = 0; index < max_cache_entry_count; ++index) {
            const auto &entry = this->entries[index];
            if (entry.is_valid && (entry.is_write_back ? entry.memory_address != 0 : entry.handle != 0)) {
                const s64 entry_offset = entry.offset;
                if (entry_offset <= offset && offset < static_cast<s64>(entry_offset + entry.size)) {
                    break;
                }

                if (offset <= entry_offset && entry_offset < static_cast<s64>(offset + actual_size)) {
                    actual_size = static_cast<s64>(entry_offset - offset);
                }
            }
        }

        /* Clear the out range. */
        out_range->first  = 0;
        out_range->second = 0;

        /* If we located an entry, use it. */
        if (index != max_cache_entry_count) {
            auto &entry = this->entries[index];

            /* Get the range of the found entry. */
            if (entry.is_write_back) {
                *out_range = std::make_pair(entry.memory_address, entry.memory_size);
            } else {
                *out_range = this->buffer_manager->AcquireCache(entry.handle);
            }

            /* Get the found entry. */
            *out_entry = entry;
            AMS_ASSERT(out_entry->is_valid);
            AMS_ASSERT(out_entry->is_cached);

            /* Clear the entry in the cache. */
            entry.is_valid       = false;
            entry.handle         = 0;
            entry.memory_address = 0;
            entry.memory_size    = 0;

            /* Set the output entry. */
            out_entry->is_valid       = true;
            out_entry->handle         = 0;
            out_entry->memory_address = 0;
            out_entry->memory_size    = 0;
        }

        /* If we don't have an out entry, allocate one. */
        if (out_range->first == 0) {
            /* Ensure that the allocatable size is above a threshold. */
            const auto size_threshold = this->buffer_manager->GetTotalSize() / 8;
            if (this->buffer_manager->GetTotalAllocatableSize() < size_threshold) {
                R_TRY(this->FlushAllCacheEntries());
            }

            /* Decide in advance on a block alignment. */
            const size_t block_alignment = this->verification_block_size;

            /* Ensure that the size we request is valid. */
            {
                AMS_ASSERT(actual_size >= 1);
                actual_size = std::min(actual_size, block_alignment * 2);
            }
            AMS_ASSERT(actual_size >= block_alignment);

            /* Allocate a buffer. */
            R_TRY(buffers::AllocateBufferUsingBufferManagerContext(out_range, this->buffer_manager, actual_size, IBufferManager::BufferAttribute(this->buffer_level), [=](const MemoryRange &buffer) {
                return buffer.first != 0 && block_alignment <= buffer.second;
            }, AMS_CURRENT_FUNCTION_NAME));

            /* Ensure our size is accurate. */
            actual_size = std::min(actual_size, out_range->second);

            /* Set the output entry. */
            out_entry->is_valid       = true;
            out_entry->is_write_back  = false;
            out_entry->is_cached      = false;
            out_entry->is_flushing    = false;
            out_entry->handle         = false;
            out_entry->memory_address = 0;
            out_entry->memory_size    = 0;
            out_entry->offset         = offset;
            out_entry->size           = actual_size;
        }

        /* Ensure that we ended up with a coherent out range. */
        AMS_ASSERT(out_range->second >= out_entry->size);

        return ResultSuccess();
    }

    void BlockCacheBufferedStorage::DestroyBuffer(CacheEntry *entry, const MemoryRange &range) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->buffer_manager != nullptr);
        AMS_ASSERT(entry != nullptr);

        /* Set the entry as invalid and not cached. */
        entry->is_cached = false;
        entry->is_valid  = false;

        /* Release the entry. */
        this->buffer_manager->DeallocateBuffer(range.first, range.second);
    }

    Result BlockCacheBufferedStorage::StoreAssociateBuffer(CacheIndex *out, const MemoryRange &range, const CacheEntry &entry) {
        /* Validate pre-conditions. */
        AMS_ASSERT(out != nullptr);

        /* Lock our mutex. */
        std::scoped_lock lk(*this->mutex);

        /* If the entry is write-back, ensure we don't exceed certain dirtiness thresholds. */
        if (entry.is_write_back) {
            R_TRY(this->ControlDirtiness());
        }

        /* Get the maximum cache entry count. */
        const CacheIndex max_cache_entry_count = static_cast<CacheIndex>(this->GetMaxCacheEntryCount());
        AMS_ASSERT(max_cache_entry_count > 0);

        /* Locate the index of an unused cache entry. */
        CacheIndex index;
        for (index = 0; index < max_cache_entry_count; ++index) {
            if (!this->entries[index].is_valid) {
                break;
            }
        }

        /* If all entries are valid, we need to invalidate one. */
        if (index == max_cache_entry_count) {
            /* Increment the index to invalidate. */
            this->invalidate_index = (this->invalidate_index + 1) % max_cache_entry_count;

            /* Get the entry to invalidate. */
            const CacheEntry *entry_to_invalidate = std::addressof(this->entries[this->invalidate_index]);

            /* Ensure that the entry can be invalidated. */
            AMS_ASSERT(entry_to_invalidate->is_valid);
            AMS_ASSERT(!entry_to_invalidate->is_flushing);

            /* Invalidate the entry. */
            R_TRY(this->FlushCacheEntry(this->invalidate_index, true));

            /* Check that the entry was invalidated successfully. */
            AMS_ASSERT(!entry_to_invalidate->is_valid);
            AMS_ASSERT(!entry_to_invalidate->is_flushing);

            index = this->invalidate_index;
        }

        /* Store the entry. */
        CacheEntry *entry_ptr = std::addressof(this->entries[index]);
        *entry_ptr            = entry;

        /* Assert that the entry is valid to store. */
        AMS_ASSERT(entry_ptr->is_valid);
        AMS_ASSERT(entry_ptr->is_cached);
        AMS_ASSERT(entry_ptr->handle == 0);
        AMS_ASSERT(entry_ptr->memory_address == 0);

        /* Ensure that the new entry isn't redundant. */
        if (!ExistsRedundantCacheEntry(*entry_ptr)) {
            /* Store the cache's buffer. */
            if (entry_ptr->is_write_back) {
                entry_ptr->handle = 0;
                entry_ptr->memory_address = range.first;
                entry_ptr->memory_size    = range.second;
            } else {
                entry_ptr->handle = this->buffer_manager->RegisterCache(range.first, range.second, IBufferManager::BufferAttribute(this->buffer_level));
                entry_ptr->memory_address = 0;
                entry_ptr->memory_size    = 0;
            }

            /* Set the out index. */
            AMS_ASSERT(entry_ptr->is_valid);
            *out                   = index;
            this->invalidate_index = index;
        } else {
            /* If a redundant entry exists, we don't need the newly stored entry. */
            this->buffer_manager->DeallocateBuffer(range.first, range.second);
            entry_ptr->is_valid = false;
            *out = -1;
        }

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::FlushCacheEntry(CacheIndex index, bool invalidate) {
        /* Lock our mutex. */
        std::scoped_lock lk(*this->mutex);

        /* Get the entry. */
        CacheEntry *entry = std::addressof(this->entries[index]);
        MemoryRange memory_range;

        /* Check that the entry's state allows for flush. */
        AMS_ASSERT(entry->is_valid);
        AMS_ASSERT(!entry->is_flushing);

        /* If we're not write back (i.e. an invalidate is happening), just release the buffer. */
        if (!entry->is_write_back) {
            AMS_ASSERT(invalidate);

            /* Get and release the buffer. */
            memory_range = this->buffer_manager->AcquireCache(entry->handle);
            if (memory_range.first != 0) {
                this->buffer_manager->DeallocateBuffer(memory_range.first, memory_range.second);
            }

            /* The entry is no longer valid. */
            entry->is_valid = false;

            return ResultSuccess();
        }

        /* Note that we've started flushing. */
        entry->is_flushing = true;

        /* Create and check our memory range. */
        memory_range = std::make_pair(entry->memory_address, entry->memory_size);
        AMS_ASSERT(memory_range.first != 0);
        AMS_ASSERT(memory_range.second >= entry->size);

        /* Validate the entry's offset. */
        AMS_ASSERT(entry->offset >= 0);
        AMS_ASSERT(entry->offset < this->data_size);
        AMS_ASSERT(util::IsAligned(entry->offset, this->verification_block_size));

        /* Write back the data. */
        Result result = ResultSuccess();
        size_t write_size = entry->size;
        if (R_SUCCEEDED(this->last_result)) {
            /* Set blocking buffer manager allocations. */
            result = this->data_storage->Write(entry->offset, reinterpret_cast<const void *>(memory_range.first), write_size);

            /* Check the result. */
            AMS_ASSERT(!fs::ResultBufferAllocationFailed::Includes(result));
        } else {
            result = this->last_result;
        }

        /* Set that we're not write-back. */
        entry->is_write_back = false;

        /* If we're invalidating, release the buffer. Otherwise, register the flushed data. */
        if (invalidate) {
            this->buffer_manager->DeallocateBuffer(memory_range.first, memory_range.second);
            entry->is_valid    = false;
            entry->is_flushing = false;
        } else {
            AMS_ASSERT(entry->is_valid);

            entry->handle = this->buffer_manager->RegisterCache(memory_range.first, memory_range.second, IBufferManager::BufferAttribute(this->buffer_level));

            entry->memory_address = 0;
            entry->memory_size    = 0;
            entry->is_flushing    = false;
        }

        /* Try to succeed. */
        R_TRY(result);

        /* We succeeded. */
        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::FlushRangeCacheEntries(s64 offset, s64 size, bool invalidate) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->data_storage != nullptr);
        AMS_ASSERT(this->buffer_manager != nullptr);

        /* Iterate over all entries that fall within the range. */
        Result result = ResultSuccess();
        const auto max_cache_entry_count = this->GetMaxCacheEntryCount();
        for (auto i = 0; i < max_cache_entry_count; ++i) {
            auto &entry = this->entries[i];
            if (entry.is_valid && (entry.is_write_back || invalidate) && (entry.offset < (offset + size)) && (offset < static_cast<s64>(entry.offset + entry.size))) {
                const auto cur_result = this->FlushCacheEntry(i, invalidate);
                if (R_FAILED(cur_result) && R_SUCCEEDED(result)) {
                    result = cur_result;
                }
            }
        }

        /* Try to succeed. */
        R_TRY(result);

        /* We succeeded. */
        return ResultSuccess();
    }

    void BlockCacheBufferedStorage::InvalidateRangeCacheEntries(s64 offset, s64 size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(this->data_storage != nullptr);
        AMS_ASSERT(this->buffer_manager != nullptr);

        /* Iterate over all entries that fall within the range. */
        const auto max_cache_entry_count = this->GetMaxCacheEntryCount();
        for (auto i = 0; i < max_cache_entry_count; ++i) {
            auto &entry = this->entries[i];
            if (entry.is_valid && (entry.offset < (offset + size)) && (offset < static_cast<s64>(entry.offset + entry.size))) {
                if (entry.is_write_back) {
                    AMS_ASSERT(entry.memory_address != 0 && entry.handle == 0);
                    this->buffer_manager->DeallocateBuffer(entry.memory_address, entry.memory_size);
                } else {
                    AMS_ASSERT(entry.memory_address == 0 && entry.handle != 0);
                    const auto memory_range = this->buffer_manager->AcquireCache(entry.handle);
                    if (memory_range.first != 0) {
                        this->buffer_manager->DeallocateBuffer(memory_range.first, memory_range.second);
                    }
                }

                /* Mark the entry as invalidated. */
                entry.is_valid      = false;
                entry.is_write_back = false;
                entry.is_flushing   = true;
            }
        }
    }

    Result BlockCacheBufferedStorage::FlushAllCacheEntries() {
        R_TRY(this->FlushRangeCacheEntries(0, std::numeric_limits<s64>::max(), false));
        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::InvalidateAllCacheEntries() {
        R_TRY(this->FlushRangeCacheEntries(0, std::numeric_limits<s64>::max(), true));
        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::ControlDirtiness() {
        /* Get and validate the max cache entry count. */
        const auto max_cache_entry_count = this->GetMaxCacheEntryCount();
        AMS_ASSERT(max_cache_entry_count > 0);

        /* Get size metrics from the buffer manager. */
        const auto total_size       = this->buffer_manager->GetTotalSize();
        const auto allocatable_size = this->buffer_manager->GetTotalAllocatableSize();

        /* If we have enough allocatable space, we don't need to do anything. */
        R_SUCCEED_IF(allocatable_size >= total_size / 4);

        /* Setup for flushing dirty entries. */
        auto threshold     = 2;
        auto dirty_count   = 0;
        auto flushed_index = this->invalidate_index;

        /* Iterate over all entries (starting with the invalidate index), and flush dirty entries once threshold is met. */
        for (auto i = 0; i < max_cache_entry_count; ++i) {
            auto index = (this->invalidate_index + 1 + i) % max_cache_entry_count;
            if (this->entries[index].is_valid && this->entries[index].is_write_back) {
                ++dirty_count;
                if (threshold <= dirty_count) {
                    R_TRY(this->FlushCacheEntry(index, false));
                    flushed_index = index;
                }
            }
        }

        /* Update the invalidate index. */
        this->invalidate_index = flushed_index;

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::UpdateLastResult(Result result) {
        /* Update the last result. */
        if (R_FAILED(result) && !fs::ResultBufferAllocationFailed::Includes(result) && R_SUCCEEDED(this->last_result)) {
            this->last_result = result;
        }

        /* Try to succeed with the result. */
        R_TRY(result);

        /* We succeeded. */
        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::ReadHeadCache(MemoryRange *out_range, CacheEntry *out_entry, bool *out_cache_needed, s64 *offset, s64 *aligned_offset, s64 aligned_offset_end, char **buffer, size_t *size) {
        /* Valdiate pre-conditions. */
        AMS_ASSERT(out_range != nullptr);
        AMS_ASSERT(out_entry != nullptr);
        AMS_ASSERT(out_cache_needed != nullptr);
        AMS_ASSERT(offset != nullptr);
        AMS_ASSERT(aligned_offset != nullptr);
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(*buffer != nullptr);
        AMS_ASSERT(size != nullptr);

        AMS_ASSERT(*aligned_offset < aligned_offset_end);

        /* Iterate over the region. */
        CacheEntry entry         = {};
        MemoryRange memory_range = {};
        *out_cache_needed        = true;

        while (*aligned_offset < aligned_offset_end) {
            /* Get the associated buffer for the offset. */
            R_TRY(this->UpdateLastResult(this->GetAssociateBuffer(std::addressof(memory_range), std::addressof(entry), *aligned_offset, this->verification_block_size, true)));

            /* If the entry isn't cached, we're done. */
            if (!entry.is_cached) {
                break;
            }

            /* Set cache not needed. */
            *out_cache_needed = false;

            /* Determine the size to copy. */
            const s64 buffer_offset = *offset - entry.offset;
            const size_t copy_size = std::min(*size, static_cast<size_t>(entry.offset + entry.size - *offset));

            /* Copy data from the entry. */
            std::memcpy(*buffer, reinterpret_cast<const void *>(memory_range.first + buffer_offset), copy_size);

            /* Advance. */
            *buffer         += copy_size;
            *offset         += copy_size;
            *size           -= copy_size;
            *aligned_offset  = entry.offset + entry.size;

            /* Handle the buffer. */
            R_TRY(this->UpdateLastResult(this->StoreOrDestroyBuffer(memory_range, std::addressof(entry))));
        }

        /* Set the output entry. */
        *out_entry = entry;
        *out_range = memory_range;

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::ReadTailCache(MemoryRange *out_range, CacheEntry *out_entry, bool *out_cache_needed, s64 offset, s64 aligned_offset, s64 *aligned_offset_end, char *buffer, size_t *size) {
        /* Valdiate pre-conditions. */
        AMS_ASSERT(out_range != nullptr);
        AMS_ASSERT(out_entry != nullptr);
        AMS_ASSERT(out_cache_needed != nullptr);
        AMS_ASSERT(aligned_offset_end != nullptr);
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(size != nullptr);

        AMS_ASSERT(aligned_offset < *aligned_offset_end);

        /* Iterate over the region. */
        CacheEntry entry         = {};
        MemoryRange memory_range = {};
        *out_cache_needed        = true;

        while (aligned_offset < *aligned_offset_end) {
            /* Get the associated buffer for the offset. */
            R_TRY(this->UpdateLastResult(this->GetAssociateBuffer(std::addressof(memory_range), std::addressof(entry), *aligned_offset_end - this->verification_block_size, this->verification_block_size, true)));

            /* If the entry isn't cached, we're done. */
            if (!entry.is_cached) {
                break;
            }

            /* Set cache not needed. */
            *out_cache_needed = false;

            /* Determine the size to copy. */
            const s64 buffer_offset = std::max(static_cast<s64>(0), offset - entry.offset);
            const size_t copy_size  = std::min(*size, static_cast<size_t>(offset + *size - entry.offset));

            /* Copy data from the entry. */
            std::memcpy(buffer + *size - copy_size, reinterpret_cast<const void *>(memory_range.first + buffer_offset), copy_size);

            /* Advance. */
            *size               -= copy_size;
            *aligned_offset_end  = entry.offset;

            /* Handle the buffer. */
            R_TRY(this->UpdateLastResult(this->StoreOrDestroyBuffer(memory_range, std::addressof(entry))));
        }

        /* Set the output entry. */
        *out_entry = entry;
        *out_range = memory_range;

        return ResultSuccess();
    }

    Result BlockCacheBufferedStorage::BulkRead(s64 offset, void *buffer, size_t size, MemoryRange *range_head, MemoryRange *range_tail, CacheEntry *entry_head, CacheEntry *entry_tail, bool head_cache_needed, bool tail_cache_needed) {
        /* Validate pre-conditions. */
        AMS_ASSERT(buffer != nullptr);
        AMS_ASSERT(range_head != nullptr);
        AMS_ASSERT(range_tail != nullptr);
        AMS_ASSERT(entry_head != nullptr);
        AMS_ASSERT(entry_tail != nullptr);

        /* Determine bulk read offsets. */
        const s64 read_offset        = offset;
        const size_t read_size       = size;
        const s64 aligned_offset     = util::AlignDown(read_offset, this->verification_block_size);
        const s64 aligned_offset_end = util::AlignUp(read_offset + read_size, this->verification_block_size);
        char *dst                    = static_cast<char *>(buffer);

        /* Prepare to do our reads. */
        auto head_guard = SCOPE_GUARD { this->DestroyBuffer(entry_head, *range_head); };
        auto tail_guard = SCOPE_GUARD { this->DestroyBuffer(entry_tail, *range_tail); };

        /* Flush the entries. */
        R_TRY(this->UpdateLastResult(this->FlushRangeCacheEntries(aligned_offset, aligned_offset_end - aligned_offset, false)));

        /* Determine the buffer to read into. */
        PooledBuffer pooled_buffer;
        const size_t buffer_size = static_cast<size_t>(aligned_offset_end - aligned_offset);
        char *read_buffer        = nullptr;
        if (read_offset == aligned_offset && read_size == buffer_size) {
            read_buffer = dst;
        } else if (tail_cache_needed && entry_tail->offset == aligned_offset && entry_tail->size == buffer_size) {
            read_buffer = reinterpret_cast<char *>(range_tail->first);
        } else if (head_cache_needed && entry_head->offset == aligned_offset && entry_head->size == buffer_size) {
            read_buffer = reinterpret_cast<char *>(range_head->first);
        } else {
            pooled_buffer.AllocateParticularlyLarge(buffer_size, 1);
            R_UNLESS(pooled_buffer.GetSize() >= buffer_size, fs::ResultAllocationFailurePooledBufferNotEnoughSize());
            read_buffer = pooled_buffer.GetBuffer();
        }

        /* Read the data. */
        R_TRY(this->data_storage->Read(aligned_offset, read_buffer, buffer_size));

        /* Copy the data out. */
        if (dst != read_buffer) {
            std::memcpy(dst, read_buffer + read_offset - aligned_offset, read_size);
        }

        /* Create a helper to populate our caches. */
        const auto PopulateCacheFromPooledBuffer = [&](CacheEntry *entry, MemoryRange *range) {
            AMS_ASSERT(entry != nullptr);
            AMS_ASSERT(range != nullptr);

            if (aligned_offset <= entry->offset && entry->offset + entry->size <= aligned_offset + buffer_size) {
                AMS_ASSERT(!entry->is_cached);
                if (reinterpret_cast<void *>(range->first) != read_buffer) {
                    std::memcpy(reinterpret_cast<void *>(range->first), read_buffer + entry->offset - aligned_offset, entry->size);
                }
                entry->is_cached = true;
            }
        };

        /* Populate tail cache if needed. */
        if (tail_cache_needed) {
            PopulateCacheFromPooledBuffer(entry_tail, range_tail);
        }

        /* Populate head cache if needed. */
        if (head_cache_needed) {
            PopulateCacheFromPooledBuffer(entry_head, range_head);
        }

        /* If both entries are cached, one may contain the other; in that case, we need only the larger entry. */
        if (entry_head->is_cached && entry_tail->is_cached) {
            if (entry_tail->offset <= entry_head->offset && entry_head->offset + entry_head->size <= entry_tail->offset + entry_tail->size) {
                entry_head->is_cached = false;
            } else if (entry_head->offset <= entry_tail->offset && entry_tail->offset + entry_tail->size <= entry_head->offset + entry_head->size) {
                entry_tail->is_cached = false;
            }
        }

        /* Destroy the tail cache. */
        tail_guard.Cancel();
        if (entry_tail->is_cached) {
            R_TRY(this->UpdateLastResult(this->StoreOrDestroyBuffer(*range_tail, entry_tail)));
        } else {
            this->DestroyBuffer(entry_tail, *range_tail);
        }

        /* Destroy the head cache. */
        head_guard.Cancel();
        if (entry_head->is_cached) {
            R_TRY(this->UpdateLastResult(this->StoreOrDestroyBuffer(*range_head, entry_head)));
        } else {
            this->DestroyBuffer(entry_head, *range_head);
        }

        return ResultSuccess();
    }

}
