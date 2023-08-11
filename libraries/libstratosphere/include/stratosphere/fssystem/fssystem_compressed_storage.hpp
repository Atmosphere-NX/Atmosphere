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
#include <stratosphere/fssystem/fssystem_asynchronous_access.hpp>
#include <stratosphere/fssystem/fssystem_bucket_tree.hpp>
#include <stratosphere/fssystem/fssystem_compression_common.hpp>
#include <stratosphere/fs/fs_i_buffer_manager.hpp>
#include <stratosphere/fssystem/impl/fssystem_block_cache_manager.hpp>

namespace ams::fssystem {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class CompressedStorage : public ::ams::fs::IStorage, public ::ams::fssystem::IAsynchronousAccessSplitter, public ::ams::fs::impl::Newable {
        NON_COPYABLE(CompressedStorage);
        NON_MOVEABLE(CompressedStorage);
        public:
            static constexpr size_t NodeSize  = 16_KB;

            using IAllocator = BucketTree::IAllocator;

            struct Entry {
                s64 virt_offset;
                s64 phys_offset;
                CompressionType compression_type;
                s32 phys_size;

                s64 GetPhysicalSize() const {
                    return this->phys_size;
                }
            };
            static_assert(util::is_pod<Entry>::value);
            static_assert(sizeof(Entry) == 0x18);
        public:
            static constexpr s64 QueryNodeStorageSize(s32 entry_count) {
                return BucketTree::QueryNodeStorageSize(NodeSize, sizeof(Entry), entry_count);
            }

            static constexpr s64 QueryEntryStorageSize(s32 entry_count) {
                return BucketTree::QueryEntryStorageSize(NodeSize, sizeof(Entry), entry_count);
            }
        private:
            class CompressedStorageCore {
                NON_COPYABLE(CompressedStorageCore);
                NON_MOVEABLE(CompressedStorageCore);
                private:
                    size_t m_block_size_max;
                    size_t m_continuous_reading_size_max;
                    BucketTree m_table;
                    fs::SubStorage m_data_storage;
                    GetDecompressorFunction m_get_decompressor_function;
                public:
                    CompressedStorageCore() : m_table(), m_data_storage() { /* ... */ }

                    ~CompressedStorageCore() {
                        this->Finalize();
                    }
                public:
                    Result Initialize(MemoryResource *bktr_allocator, fs::SubStorage data_storage, fs::SubStorage node_storage, fs::SubStorage entry_storage, s32 bktr_entry_count, size_t block_size_max, size_t continuous_reading_size_max, GetDecompressorFunction get_decompressor) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(bktr_allocator != nullptr);
                        AMS_ASSERT(0 < block_size_max);
                        AMS_ASSERT(block_size_max <= continuous_reading_size_max);
                        AMS_ASSERT(get_decompressor != nullptr);

                        /* Initialize our entry table. */
                        R_TRY(m_table.Initialize(bktr_allocator, node_storage, entry_storage, NodeSize, sizeof(Entry), bktr_entry_count));

                        /* Set our other fields. */
                        m_block_size_max              = block_size_max;
                        m_continuous_reading_size_max = continuous_reading_size_max;
                        m_data_storage                = data_storage;
                        m_get_decompressor_function   = get_decompressor;

                        R_SUCCEED();
                    }

                    void Finalize() {
                        if (this->IsInitialized()) {
                            m_table.Finalize();
                            m_data_storage = fs::SubStorage();
                        }
                    }

                    fs::IStorage *GetDataStorage() { return std::addressof(m_data_storage); }

                    Result GetDataStorageSize(s64 *out) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(out != nullptr);

                        /* Get size. */
                        R_RETURN(m_data_storage.GetSize(out));
                    }

                    BucketTree &GetEntryTable() { return m_table; }

                    Result GetEntryList(Entry *out_entries, s32 *out_read_count, s32 max_entry_count, s64 offset, s64 size) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(offset >= 0);
                        AMS_ASSERT(size >= 0);
                        AMS_ASSERT(this->IsInitialized());

                        /* Check that we can output the count. */
                        R_UNLESS(out_read_count != nullptr, fs::ResultNullptrArgument());

                        /* Check that we have anything to read at all. */
                        R_SUCCEED_IF(size == 0);

                        /* Check that either we have a buffer, or this is to determine how many we need. */
                        if (max_entry_count != 0) {
                            R_UNLESS(out_entries != nullptr, fs::ResultNullptrArgument());
                        }

                        /* Get the table offsets. */
                        BucketTree::Offsets table_offsets;
                        R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

                        /* Validate arguments. */
                        R_UNLESS(table_offsets.IsInclude(offset, size), fs::ResultOutOfRange());

                        /* Find the offset in our tree. */
                        BucketTree::Visitor visitor;
                        R_TRY(m_table.Find(std::addressof(visitor), offset));
                        {
                            const auto entry_offset = visitor.Get<Entry>()->virt_offset;
                            R_UNLESS(0 <= entry_offset && table_offsets.IsInclude(entry_offset), fs::ResultUnexpectedInCompressedStorageA());
                        }

                        /* Get the entries. */
                        const auto end_offset = offset + size;
                        s32 read_count = 0;
                        while (visitor.Get<Entry>()->virt_offset < end_offset) {
                            /* If we should be setting the output, do so. */
                            if (max_entry_count != 0) {
                                /* Ensure we only read as many entries as we can. */
                                if (read_count >= max_entry_count) {
                                    break;
                                }

                                /* Set the current output entry. */
                                out_entries[read_count] = *visitor.Get<Entry>();
                            }

                            /* Increase the read count. */
                            ++read_count;

                            /* If we're at the end, we're done. */
                            if (!visitor.CanMoveNext()) {
                                break;
                            }

                            /* Move to the next entry. */
                            R_TRY(visitor.MoveNext());
                        }

                        /* Set the output read count. */
                        *out_read_count = read_count;
                        R_SUCCEED();
                    }

                    Result GetSize(s64 *out) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(out != nullptr);

                        /* Get our table offsets. */
                        BucketTree::Offsets offsets;
                        R_TRY(m_table.GetOffsets(std::addressof(offsets)));

                        /* Set the output. */
                        *out = offsets.end_offset;
                        R_SUCCEED();
                    }

                    Result Invalidate() {
                        /* Invalidate our entry table. */
                        R_TRY(m_table.InvalidateCache());

                        /* Invalidate our data storage. */
                        R_TRY(m_data_storage.OperateRange(fs::OperationId::Invalidate, 0, std::numeric_limits<s64>::max()));

                        R_SUCCEED();
                    }

                    Result OperatePerEntry(s64 offset, s64 size, auto f) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(offset >= 0);
                        AMS_ASSERT(size >= 0);
                        AMS_ASSERT(this->IsInitialized());

                        /* Succeed if there's nothing to operate on. */
                        R_SUCCEED_IF(size == 0);

                        /* Get the table offsets. */
                        BucketTree::Offsets table_offsets;
                        R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

                        /* Validate arguments. */
                        R_UNLESS(table_offsets.IsInclude(offset, size), fs::ResultOutOfRange());

                        /* Find the offset in our tree. */
                        BucketTree::Visitor visitor;
                        R_TRY(m_table.Find(std::addressof(visitor), offset));
                        {
                            const auto entry_offset = visitor.Get<Entry>()->virt_offset;
                            R_UNLESS(0 <= entry_offset && table_offsets.IsInclude(entry_offset), fs::ResultUnexpectedInCompressedStorageA());
                        }

                        /* Prepare to operate in chunks. */
                        auto cur_offset = offset;
                        const auto end_offset = offset + static_cast<s64>(size);

                        while (cur_offset < end_offset) {
                            /* Get the current entry. */
                            const auto cur_entry = *visitor.Get<Entry>();

                            /* Get and validate the entry's offset. */
                            const auto cur_entry_offset = cur_entry.virt_offset;
                            R_UNLESS(cur_entry_offset <= cur_offset, fs::ResultUnexpectedInCompressedStorageA());

                            /* Get and validate the next entry offset. */
                            s64 next_entry_offset;
                            if (visitor.CanMoveNext()) {
                                R_TRY(visitor.MoveNext());
                                next_entry_offset = visitor.Get<Entry>()->virt_offset;
                                R_UNLESS(table_offsets.IsInclude(next_entry_offset), fs::ResultUnexpectedInCompressedStorageA());
                            } else {
                                next_entry_offset = table_offsets.end_offset;
                            }
                            R_UNLESS(cur_offset < next_entry_offset, fs::ResultUnexpectedInCompressedStorageA());

                            /* Get the offset of the entry in the data we read. */
                            const auto data_offset = cur_offset - cur_entry_offset;
                            const auto data_size   = (next_entry_offset - cur_entry_offset);
                            AMS_ASSERT(data_size > 0);

                            /* Determine how much is left. */
                            const auto remaining_size = end_offset - cur_offset;
                            const auto cur_size       = std::min<s64>(remaining_size, data_size - data_offset);
                            AMS_ASSERT(cur_size <= size);

                            /* Get the data storage size. */
                            s64 storage_size = 0;
                            R_TRY(m_data_storage.GetSize(std::addressof(storage_size)));

                            /* Check that our read remains naively physically in bounds. */
                            R_UNLESS(0 <= cur_entry.phys_offset && cur_entry.phys_offset <= storage_size, fs::ResultUnexpectedInCompressedStorageC());

                            /* If we have any compression, verify that we remain physically in bounds. */
                            if (cur_entry.compression_type != CompressionType_None) {
                                R_UNLESS(cur_entry.phys_offset + cur_entry.GetPhysicalSize() <= storage_size, fs::ResultUnexpectedInCompressedStorageC());
                            }

                            /* Check that block alignment requirements are met. */
                            if (CompressionTypeUtility::IsBlockAlignmentRequired(cur_entry.compression_type)) {
                                R_UNLESS(util::IsAligned(cur_entry.phys_offset, CompressionBlockAlignment), fs::ResultUnexpectedInCompressedStorageA());
                            }

                            /* Invoke the operator. */
                            bool is_continuous = true;
                            R_TRY(f(std::addressof(is_continuous), cur_entry, data_size, data_offset, cur_size));

                            /* If not continuous, we're done. */
                            if (!is_continuous) {
                                break;
                            }

                            /* Advance. */
                            cur_offset += cur_size;
                        }

                        R_SUCCEED();
                    }

                    Result OperateRange(s64 offset, s64 size, auto f) {
                        /* Get the table offsets. */
                        BucketTree::Offsets table_offsets;
                        R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

                        /* Validate arguments. */
                        R_UNLESS(table_offsets.IsInclude(offset, size), fs::ResultOutOfRange());

                        /* If our table is empty, we have nothing to operate on. */
                        R_SUCCEED_IF(m_table.IsEmpty());

                        /* Operate on the range. */
                        s64 required_access_physical_offset = 0;
                        s64 required_access_physical_size   = 0;
                        R_TRY(this->OperatePerEntry(offset, size, [&] (bool *out_continuous, const Entry &entry, s64 virtual_data_size, s64 data_offset, s64 read_size) -> Result {
                            AMS_UNUSED(virtual_data_size);

                            /* Determine the physical extents. */
                            s64 physical_offset, physical_size;
                            if (CompressionTypeUtility::IsRandomAccessible(entry.compression_type)) {
                                physical_offset = entry.phys_offset + data_offset;
                                physical_size   = read_size;
                            } else {
                                physical_offset = entry.phys_offset;
                                physical_size   = entry.GetPhysicalSize();
                            }

                            /* If we have a pending data storage operation, perform it if we have to. */
                            const s64 required_access_physical_end = required_access_physical_offset + required_access_physical_size;
                            if (required_access_physical_size > 0) {
                                /* Check that we can can coalesce this operation with the previous one; if we can't, we need to perform it. */
                                if (!(required_access_physical_end <= physical_offset && physical_offset <= util::AlignUp(required_access_physical_end, CompressionBlockAlignment))) {
                                    R_TRY(f(required_access_physical_offset, required_access_physical_size));

                                    required_access_physical_size = 0;
                                }
                            }

                            /* If we need to access the data storage, update our storage access parameters. */
                            if (CompressionTypeUtility::IsDataStorageAccessRequired(entry.compression_type)) {
                                /* Update the required access parameters. */
                                if (required_access_physical_size > 0) {
                                    required_access_physical_size += physical_size + (physical_offset - required_access_physical_end);
                                } else {
                                    required_access_physical_offset = physical_offset;
                                    required_access_physical_size   = physical_size;
                                }
                            } else {
                                /* Verify that we're allowed to be operating on the non-data-storage-access type. */
                                R_UNLESS(entry.compression_type == CompressionType_Zeros, fs::ResultUnexpectedInCompressedStorageB());
                            }

                            /* We're always continuous. */
                            *out_continuous = true;
                            R_SUCCEED();
                        }));

                        /* If we have a pending operation, perform it. */
                        if (required_access_physical_size > 0) {
                            R_TRY(f(required_access_physical_offset, required_access_physical_size));
                        }

                        R_SUCCEED();
                    }

                    Result QueryAppropriateOffsetForAsynchronousAccess(s64 *out, s64 offset, s64 access_size, s64 alignment_size) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(offset >= 0);
                        AMS_ASSERT(this->IsInitialized());

                        /* Check that we can write to the output. */
                        R_UNLESS(out != nullptr, fs::ResultNullptrArgument());

                        /* Get the table offsets. */
                        BucketTree::Offsets table_offsets;
                        R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

                        /* Validate arguments. */
                        R_UNLESS(table_offsets.IsInclude(offset, 1), fs::ResultOutOfRange());

                        /* Operate on the range. */
                        s64 required_access_physical_offset = 0;
                        s64 required_access_physical_size   = 0;
                        s64 required_access_physical_end    = 0;

                        s64 appropriate_virtual_offset      = offset;
                        R_TRY(this->OperatePerEntry(offset, table_offsets.end_offset - offset, [&] (bool *out_continuous, const Entry &entry, s64 virtual_data_size, s64 data_offset, s64 read_size) -> Result {
                            AMS_UNUSED(virtual_data_size);

                            /* Determine the physical extents. */
                            s64 physical_offset, physical_size;
                            if (CompressionTypeUtility::IsRandomAccessible(entry.compression_type)) {
                                physical_offset = entry.phys_offset + data_offset;
                                physical_size   = read_size;
                            } else {
                                physical_offset = entry.phys_offset;
                                physical_size   = entry.GetPhysicalSize();
                            }

                            /* If we don't need to access the data storage, update our storage access parameters simply. */
                            if (!CompressionTypeUtility::IsDataStorageAccessRequired(entry.compression_type)) {
                                /* Verify that we're allowed to be operating on the non-data-storage-access type. */
                                R_UNLESS(entry.compression_type == CompressionType_Zeros, fs::ResultUnexpectedInCompressedStorageB());

                                /* No access is required, so we can advance the offset freely. */
                                appropriate_virtual_offset += read_size;

                                /* A read to zeros is always continuous. */
                                *out_continuous = true;
                                R_SUCCEED();
                            }

                            /* Update the required access parameters. */
                            if (required_access_physical_size > 0) {
                                /* Check that we can can coalesce this operation with the previous one; if we can't, we need to account for the gap. */
                                if ((required_access_physical_end <= physical_offset && physical_offset <= util::AlignUp(required_access_physical_end, CompressionBlockAlignment))) {
                                    const s64 gap_size = physical_offset - required_access_physical_end;

                                    if (required_access_physical_size + gap_size > access_size) {
                                        *out_continuous = false;
                                        R_SUCCEED();
                                    }

                                    required_access_physical_size += gap_size;
                                }
                            } else {
                                required_access_physical_offset = physical_offset;
                            }

                            /* If we're within the access bounds, we want to continue on. */
                            if (physical_size + required_access_physical_size <= access_size) {
                                required_access_physical_size += physical_size;
                                required_access_physical_end   = physical_offset + physical_size;

                                appropriate_virtual_offset += read_size;
                                *out_continuous = true;
                                R_SUCCEED();
                            }

                            /* We're no longer within the access bounds, so we won't be continuous. */
                            *out_continuous = false;

                            /* Ensure we account for block alignment. */
                            if (CompressionTypeUtility::IsBlockAlignmentRequired(entry.compression_type)) {
                                if (appropriate_virtual_offset == offset) {
                                    appropriate_virtual_offset += read_size;
                                    access_size = std::max<s64>(access_size, read_size);
                                }
                            } else {
                                /* Get the default splitter. */
                                auto * const default_splitter = fssystem::IAsynchronousAccessSplitter::GetDefaultAsynchronousAccessSplitter();

                                /* Query for an appropriate offset. */
                                s64 appropriate_physical_offset = 0;
                                R_TRY(default_splitter->QueryAppropriateOffset(std::addressof(appropriate_physical_offset), physical_offset, access_size - required_access_physical_size, alignment_size));

                                /* Use it, if we should. */
                                if (const auto gap_size = appropriate_physical_offset - physical_offset; gap_size > 0) {
                                    appropriate_virtual_offset    += gap_size;
                                    required_access_physical_size += gap_size;
                                }
                            }

                            R_SUCCEED();
                        }));

                        /* Check that the offset is actually appropriate. */
                        AMS_ASSERT(offset <= appropriate_virtual_offset && appropriate_virtual_offset <= table_offsets.end_offset);
                        AMS_ASSERT(0 <= required_access_physical_size && required_access_physical_size <= access_size);

                        /* Set the output. */
                        *out = appropriate_virtual_offset;
                        R_SUCCEED();
                    }

                    Result QueryRange(void *dst, size_t dst_size, s64 offset, s64 size) {
                        /* Check arguments. */
                        R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                        R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidArgument());

                        /* If we have nothing to query, succeed immediately. */
                        R_SUCCEED_IF(size <= 0);

                        /* Operate on the range. */
                        fs::QueryRangeInfo full_info;
                        full_info.Clear();

                        R_TRY(this->OperateRange(offset, size, [&](s64 offset, s64 size) -> Result {
                            /* Operate on our data storage. */
                            fs::QueryRangeInfo cur_info;
                            R_TRY(m_data_storage.OperateRange(std::addressof(cur_info), sizeof(cur_info), fs::OperationId::QueryRange, offset, size, nullptr, 0));

                            /* Merge the info. */
                            full_info.Merge(cur_info);
                            R_SUCCEED();
                        }));

                        R_SUCCEED();
                    }
                public:
                    using ReadImplFunction = util::IFunction<Result(void *, size_t)>;
                    using ReadFunction     = util::IFunction<Result(size_t, const ReadImplFunction &)>;
                public:
                    Result Read(s64 offset, s64 size, const ReadFunction &read_func) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(offset >= 0);
                        AMS_ASSERT(this->IsInitialized());

                        /* Succeed immediately, if we have nothing to read. */
                        R_SUCCEED_IF(size == 0);

                        /* Declare read lambda. */
                        constexpr int EntriesCountMax = 0x80;
                        struct Entries {
                            CompressionType compression_type;
                            u32 gap_from_prev;
                            u32 physical_size;
                            u32 virtual_size;
                        };
                        Entries entries[EntriesCountMax];
                        s32 entry_count                     = 0;
                        Entry prev_entry                    = { .virt_offset = -1, };
                        bool will_allocate_pooled_buffer    = false;
                        s64 required_access_physical_offset = 0;
                        s64 required_access_physical_size   = 0;

                        auto PerformRequiredRead = [&]() -> Result {
                            /* If there are no entries, we have nothing to do. */
                            R_SUCCEED_IF(entry_count == 0);

                            /* Get the remaining size in a convenient form. */
                            const size_t total_required_size = static_cast<size_t>(required_access_physical_size);

                            /* Perform the read based on whether we need to allocate a buffer. */
                            if (will_allocate_pooled_buffer) {
                                /* Allocate a pooled buffer. */
                                fssystem::PooledBuffer pooled_buffer;
                                if (pooled_buffer.GetAllocatableSizeMax() >= total_required_size) {
                                    pooled_buffer.Allocate(total_required_size, m_block_size_max);
                                } else {
                                    pooled_buffer.AllocateParticularlyLarge(std::min<size_t>(total_required_size, PooledBuffer::GetAllocatableParticularlyLargeSizeMax()), m_block_size_max);
                                }

                                /* Read each of the entries. */
                                for (s32 entry_idx = 0; entry_idx < entry_count; ++entry_idx) {
                                    /* Determine the current read size. */
                                    bool will_use_pooled_buffer = false;
                                    const size_t cur_read_size = [&] () ALWAYS_INLINE_LAMBDA -> size_t {
                                        if (const size_t target_entry_size = static_cast<size_t>(entries[entry_idx].physical_size) + static_cast<size_t>(entries[entry_idx].gap_from_prev); target_entry_size <= pooled_buffer.GetSize()) {
                                            /* We'll be using the pooled buffer. */
                                            will_use_pooled_buffer = true;

                                            /* Determine how much we can read. */
                                            const size_t max_size = std::min<size_t>(required_access_physical_size, pooled_buffer.GetSize());

                                            size_t read_size = 0;
                                            for (auto n = entry_idx; n < entry_count; ++n) {
                                                const size_t cur_entry_size = static_cast<size_t>(entries[n].physical_size) + static_cast<size_t>(entries[n].gap_from_prev);
                                                if (read_size + cur_entry_size > max_size) {
                                                    break;
                                                }

                                                read_size += cur_entry_size;
                                            }

                                            return read_size;
                                        } else {
                                            /* If we don't fit, we must be uncompressed. */
                                            AMS_ASSERT(entries[entry_idx].compression_type == CompressionType_None);

                                            /* We can perform the whole of an uncompressed read directly. */
                                            return entries[entry_idx].virtual_size;
                                        }
                                    }();

                                    /* Perform the read based on whether or not we'll use the pooled buffer. */
                                    if (will_use_pooled_buffer) {
                                        /* Read the compressed data into the pooled buffer. */
                                        auto * const buffer = pooled_buffer.GetBuffer();
                                        R_TRY(m_data_storage.Read(required_access_physical_offset, buffer, cur_read_size));

                                        /* Temporarily increase our thread priority, while we decompress the data. */
                                        ScopedThreadPriorityChanger cp(+1, ScopedThreadPriorityChanger::Mode::Relative);

                                        /* Decompress the data. */
                                        size_t buffer_offset;
                                        for (buffer_offset = 0; entry_idx < entry_count && ((static_cast<size_t>(entries[entry_idx].physical_size) + static_cast<size_t>(entries[entry_idx].gap_from_prev)) == 0 || buffer_offset < cur_read_size); buffer_offset += entries[entry_idx++].physical_size) {
                                            /* Advance by the relevant gap. */
                                            buffer_offset += entries[entry_idx].gap_from_prev;

                                            const auto compression_type = entries[entry_idx].compression_type;
                                            switch (compression_type) {
                                                case CompressionType_None:
                                                    {
                                                        /* Check that we can remain within bounds. */
                                                        AMS_ASSERT(buffer_offset + entries[entry_idx].virtual_size <= cur_read_size);

                                                        /* Perform no decompression. */
                                                        R_TRY(read_func(entries[entry_idx].virtual_size, util::MakeIFunction([&] (void *dst, size_t dst_size) -> Result {
                                                            /* Check that the size is valid. */
                                                            AMS_ASSERT(dst_size == entries[entry_idx].virtual_size);
                                                            AMS_UNUSED(dst_size);

                                                            /* We have no compression, so just copy the data out. */
                                                            std::memcpy(dst, buffer + buffer_offset, entries[entry_idx].virtual_size);
                                                            R_SUCCEED();
                                                        })));
                                                    }
                                                    break;
                                                case CompressionType_Zeros:
                                                    {
                                                        /* Check that we can remain within bounds. */
                                                        AMS_ASSERT(buffer_offset <= cur_read_size);

                                                        /* Zero the memory. */
                                                        R_TRY(read_func(entries[entry_idx].virtual_size, util::MakeIFunction([&] (void *dst, size_t dst_size) -> Result {
                                                            /* Check that the size is valid. */
                                                            AMS_ASSERT(dst_size == entries[entry_idx].virtual_size);
                                                            AMS_UNUSED(dst_size);

                                                            /* The data is zeroes, so zero the buffer. */
                                                            std::memset(dst, 0, entries[entry_idx].virtual_size);
                                                            R_SUCCEED();
                                                        })));
                                                    }
                                                    break;
                                                default:
                                                    {
                                                        /* Check that we can remain within bounds. */
                                                        AMS_ASSERT(buffer_offset + entries[entry_idx].physical_size <= cur_read_size);

                                                        /* Get the decompressor. */
                                                        const auto decompressor = this->GetDecompressor(compression_type);
                                                        R_UNLESS(decompressor != nullptr, fs::ResultUnexpectedInCompressedStorageB());

                                                        /* Decompress the data. */
                                                        R_TRY(read_func(entries[entry_idx].virtual_size, util::MakeIFunction([&] (void *dst, size_t dst_size) -> Result {
                                                            /* Check that the size is valid. */
                                                            AMS_ASSERT(dst_size == entries[entry_idx].virtual_size);
                                                            AMS_UNUSED(dst_size);

                                                            /* Perform the decompression. */
                                                            R_RETURN(decompressor(dst, entries[entry_idx].virtual_size, buffer + buffer_offset, entries[entry_idx].physical_size));
                                                        })));
                                                    }
                                                    break;
                                            }
                                        }

                                        /* Check that we processed the correct amount of data. */
                                        AMS_ASSERT(buffer_offset == cur_read_size);
                                    } else {
                                        /* Account for the gap from the previous entry. */
                                        required_access_physical_offset += entries[entry_idx].gap_from_prev;
                                        required_access_physical_size   -= entries[entry_idx].gap_from_prev;

                                        /* We don't need the buffer (as the data is uncompressed), so just execute the read. */
                                        R_TRY(read_func(cur_read_size, util::MakeIFunction([&] (void *dst, size_t dst_size) -> Result {
                                            /* Check that the size is valid. */
                                            AMS_ASSERT(dst_size == cur_read_size);
                                            AMS_UNUSED(dst_size);

                                            /* Perform the read. */
                                            R_RETURN(m_data_storage.Read(required_access_physical_offset, dst, cur_read_size));
                                        })));
                                    }

                                    /* Advance on. */
                                    required_access_physical_offset += cur_read_size;
                                    required_access_physical_size   -= cur_read_size;
                                }

                                /* Verify that we have nothing remaining to read. */
                                AMS_ASSERT(required_access_physical_size == 0);

                                R_SUCCEED();
                            } else {
                                /* We don't need a buffer, so just execute the read. */
                                R_TRY(read_func(total_required_size, util::MakeIFunction([&] (void *dst, size_t dst_size) -> Result {
                                    /* Check that the size is valid. */
                                    AMS_ASSERT(dst_size == total_required_size);
                                    AMS_UNUSED(dst_size);

                                    /* Perform the read. */
                                    R_RETURN(m_data_storage.Read(required_access_physical_offset, dst, total_required_size));
                                })));
                            }

                            R_SUCCEED();
                        };

                        R_TRY(this->OperatePerEntry(offset, size, [&] (bool *out_continuous, const Entry &entry, s64 virtual_data_size, s64 data_offset, s64 read_size) -> Result {
                            /* Determine the physical extents. */
                            s64 physical_offset, physical_size;
                            if (CompressionTypeUtility::IsRandomAccessible(entry.compression_type)) {
                                physical_offset = entry.phys_offset + data_offset;
                                physical_size   = read_size;
                            } else {
                                physical_offset = entry.phys_offset;
                                physical_size   = entry.GetPhysicalSize();
                            }

                            /* If we have a pending data storage operation, perform it if we have to. */
                            const s64 required_access_physical_end = required_access_physical_offset + required_access_physical_size;
                            if (required_access_physical_size > 0) {
                                const bool required_by_gap             = !(required_access_physical_end <= physical_offset && physical_offset <= util::AlignUp(required_access_physical_end, CompressionBlockAlignment));
                                const bool required_by_continuous_size = ((physical_size + physical_offset) - required_access_physical_end) + required_access_physical_size > static_cast<s64>(m_continuous_reading_size_max);
                                const bool required_by_entry_count     = entry_count == EntriesCountMax;
                                if (required_by_gap || required_by_continuous_size || required_by_entry_count) {
                                    /* Check that our planned access is sane. */
                                    AMS_ASSERT(!will_allocate_pooled_buffer || required_access_physical_size <= static_cast<s64>(m_continuous_reading_size_max));

                                    /* Perform the required read. */
                                    R_TRY(PerformRequiredRead());

                                    /* Reset our requirements. */
                                    prev_entry.virt_offset        = -1;
                                    required_access_physical_size = 0;
                                    entry_count                   = 0;
                                    will_allocate_pooled_buffer   = false;
                                }
                            }

                            /* Sanity check that we're within bounds on entries. */
                            AMS_ASSERT(entry_count < EntriesCountMax);

                            /* Determine if a buffer allocation is needed. */
                            if (entry.compression_type != CompressionType_None || (prev_entry.virt_offset >= 0 && entry.virt_offset - prev_entry.virt_offset != entry.phys_offset - prev_entry.phys_offset)) {
                                will_allocate_pooled_buffer = true;
                            }

                            /* If we need to access the data storage, update our required access parameters. */
                            if (CompressionTypeUtility::IsDataStorageAccessRequired(entry.compression_type)) {
                                /* If the data is compressed, ensure the access is sane. */
                                if (entry.compression_type != CompressionType_None) {
                                    R_UNLESS(data_offset == 0,                                              fs::ResultInvalidOffset());
                                    R_UNLESS(virtual_data_size == read_size,                                fs::ResultInvalidSize());
                                    R_UNLESS(entry.GetPhysicalSize() <= static_cast<s64>(m_block_size_max), fs::ResultUnexpectedInCompressedStorageD());
                                }

                                /* Update the required access parameters. */
                                s64 gap_from_prev;
                                if (required_access_physical_size > 0) {
                                    gap_from_prev = physical_offset - required_access_physical_end;
                                } else {
                                    gap_from_prev = 0;
                                    required_access_physical_offset = physical_offset;
                                }
                                required_access_physical_size += physical_size + gap_from_prev;

                                /* Create an entry to access the data storage. */
                                entries[entry_count++] = {
                                    .compression_type = entry.compression_type,
                                    .gap_from_prev    = static_cast<u32>(gap_from_prev),
                                    .physical_size    = static_cast<u32>(physical_size),
                                    .virtual_size     = static_cast<u32>(read_size),
                                };
                            } else {
                                /* Verify that we're allowed to be operating on the non-data-storage-access type. */
                                R_UNLESS(entry.compression_type == CompressionType_Zeros, fs::ResultUnexpectedInCompressedStorageB());

                                /* If we have entries, create a fake entry for the zero region. */
                                if (entry_count != 0) {
                                    /* We need to have a physical size. */
                                    R_UNLESS(entry.GetPhysicalSize() != 0, fs::ResultUnexpectedInCompressedStorageD());

                                    /* Create a fake entry. */
                                    entries[entry_count++] = {
                                        .compression_type = CompressionType_Zeros,
                                        .gap_from_prev    = 0,
                                        .physical_size    = 0,
                                        .virtual_size     = static_cast<u32>(read_size),
                                    };
                                } else {
                                    /* We have no entries, so we can just perform the read. */
                                    R_TRY(read_func(static_cast<size_t>(read_size), util::MakeIFunction([&] (void *dst, size_t dst_size) -> Result {
                                        /* Check the space we should zero is correct. */
                                        AMS_ASSERT(dst_size == static_cast<size_t>(read_size));
                                        AMS_UNUSED(dst_size);

                                        /* Zero the memory. */
                                        std::memset(dst, 0, read_size);
                                        R_SUCCEED();
                                    })));
                                }
                            }


                            /* Set the previous entry. */
                            prev_entry = entry;

                            /* We're continuous. */
                            *out_continuous = true;
                            R_SUCCEED();
                        }));

                        /* If we still have a pending access, perform it. */
                        if (required_access_physical_size != 0) {
                            R_TRY(PerformRequiredRead());
                        }

                        R_SUCCEED();
                    }
                private:
                    DecompressorFunction GetDecompressor(CompressionType type) const {
                        /* Check that we can get a decompressor for the type. */
                        if (CompressionTypeUtility::IsUnknownType(type)) {
                            return nullptr;
                        }

                        /* Get the decompressor. */
                        return m_get_decompressor_function(type);
                    }

                    bool IsInitialized() const {
                        return m_table.IsInitialized();
                    }
            };

            class CacheManager {
                NON_COPYABLE(CacheManager);
                NON_MOVEABLE(CacheManager);
                private:
                    struct Range {
                        s64 offset;
                        size_t size;

                        s64 GetEndOffset() const {
                            return this->offset + this->size;
                        }

                        bool IsIncluded(s64 ofs) const {
                            return this->offset <= ofs && ofs < this->GetEndOffset();
                        }
                    };
                    static_assert(util::is_pod<Range>::value);

                    struct CacheEntry {
                        Range range;
                        fs::IBufferManager::CacheHandle handle;
                        uintptr_t memory_address;
                        u32 memory_size;
                        bool is_valid;
                        bool is_cached;
                        u16 lru_counter;

                        void Invalidate() {
                            /* ... */
                        }

                        bool IsAllocated() const {
                            return this->is_valid && this->handle != 0;
                        }

                        bool IsIncluded(s64 offset) const {
                            return this->is_valid && this->range.IsIncluded(offset);
                        }

                        bool IsWriteBack() const {
                            return false;
                        }
                    };
                    static_assert(util::is_pod<CacheEntry>::value);

                    struct AccessRange {
                        s64 virtual_offset;
                        s64 virtual_size;
                        u32 physical_size;
                        bool is_block_alignment_required;

                        s64 GetEndVirtualOffset() const {
                            return this->virtual_offset + this->virtual_size;
                        }
                    };
                    static_assert(util::is_pod<AccessRange>::value);

                    using BlockCacheManager = ::ams::fssystem::impl::BlockCacheManager<CacheEntry, fs::IBufferManager>;
                    using CacheIndex        = BlockCacheManager::CacheIndex;
                private:
                    size_t m_cache_size_unk_0;
                    size_t m_cache_size_unk_1;
                    os::SdkMutex m_mutex;
                    BlockCacheManager m_block_cache_manager;
                    s64 m_storage_size = 0;
                public:
                    CacheManager() = default;

                    ~CacheManager() { this->Finalize(); }
                public:
                    Result Initialize(fs::IBufferManager *cache_allocator, s64 storage_size, size_t cache_size_0, size_t cache_size_1, size_t max_cache_entries) {
                        /* Initialize our block cache manager. */
                        R_TRY(m_block_cache_manager.Initialize(cache_allocator, max_cache_entries));

                        /* Set our fields. */
                        m_cache_size_unk_0 = cache_size_0;
                        m_cache_size_unk_1 = cache_size_1;
                        m_storage_size     = storage_size;

                        R_SUCCEED();
                    }

                    void Finalize() {
                        /* If necessary, invalidate anything we have cached. */
                        if (m_block_cache_manager.IsInitialized()) {
                            this->Invalidate();
                        }

                        /* Finalize our block cache manager. */
                        m_block_cache_manager.Finalize();
                    }

                    void Invalidate() {
                        /* Acquire exclusive access to our manager. */
                        std::scoped_lock lk(m_mutex);

                        /* Invalidate all entries. */
                        return m_block_cache_manager.Invalidate();
                    }

                    Result Read(CompressedStorageCore &core, s64 offset, void *buffer, size_t size) {
                        /* If we have nothing to read, succeed. */
                        R_SUCCEED_IF(size == 0);

                        /* Check that we have a buffer to read into. */
                        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

                        /* Check that the read is in bounds. */
                        R_UNLESS(offset <= m_storage_size, fs::ResultInvalidOffset());

                        /* Determine how much we can read. */
                        const size_t read_size = std::min<size_t>(size, m_storage_size - offset);

                        /* Create head/tail ranges. */
                        AccessRange head_range = {};
                        AccessRange tail_range = {};
                        bool is_tail_set = false;

                        /* Operate to determine the head range. */
                        R_TRY(core.OperatePerEntry(offset, 1, [&] (bool *out_continuous, const Entry &entry, s64 virtual_data_size, s64 data_offset, s64 data_read_size) -> Result {
                            AMS_UNUSED(data_offset, data_read_size);

                            /* Set the head range. */
                            head_range = {
                                .virtual_offset              = entry.virt_offset,
                                .virtual_size                = virtual_data_size,
                                .physical_size               = static_cast<u32>(entry.phys_size),
                                .is_block_alignment_required = CompressionTypeUtility::IsBlockAlignmentRequired(entry.compression_type),
                            };

                            /* If required, set the tail range. */
                            if (static_cast<s64>(offset + read_size) <= entry.virt_offset + virtual_data_size) {
                                tail_range = {
                                    .virtual_offset              = entry.virt_offset,
                                    .virtual_size                = virtual_data_size,
                                    .physical_size               = static_cast<u32>(entry.phys_size),
                                    .is_block_alignment_required = CompressionTypeUtility::IsBlockAlignmentRequired(entry.compression_type),
                                };
                                is_tail_set = true;
                            }

                            /* We only want to determine the head range, so we're not continuous. */
                            *out_continuous = false;
                            R_SUCCEED();
                        }));

                        /* If necessary, determine the tail range. */
                        if (!is_tail_set) {
                            R_TRY(core.OperatePerEntry(offset + read_size - 1, 1, [&] (bool *out_continuous, const Entry &entry, s64 virtual_data_size, s64 data_offset, s64 data_read_size) -> Result {
                                AMS_UNUSED(data_offset, data_read_size);

                                /* Set the tail range. */
                                tail_range = {
                                    .virtual_offset              = entry.virt_offset,
                                    .virtual_size                = virtual_data_size,
                                    .physical_size               = static_cast<u32>(entry.phys_size),
                                    .is_block_alignment_required = CompressionTypeUtility::IsBlockAlignmentRequired(entry.compression_type),
                                };

                                /* We only want to determine the tail range, so we're not continuous. */
                                *out_continuous = false;
                                R_SUCCEED();
                            }));
                        }

                        /* Begin performing the accesses. */
                        s64 cur_offset  = offset;
                        size_t cur_size = read_size;
                        char *cur_dst   = static_cast<char *>(buffer);

                        /* If we can use the head/tail cache, do so. */
                        if (m_block_cache_manager.GetCount() > 0) {
                            /* Read the head cache. */
                            R_TRY(this->ReadHeadCache(core, cur_offset, cur_dst, cur_size, head_range, tail_range));

                            /* If we're now done, succeed. */
                            R_SUCCEED_IF(cur_size == 0);

                            /* Read the tail cache. */
                            R_TRY(this->ReadTailCache(core, cur_offset, cur_dst, cur_size, head_range, tail_range));

                            /* If we're now done, succeed. */
                            R_SUCCEED_IF(cur_size == 0);
                        }

                        /* Determine our alignment. */
                        const bool head_unaligned = head_range.is_block_alignment_required && (cur_offset != head_range.virtual_offset || static_cast<s64>(cur_size) < head_range.virtual_size);
                        const bool tail_unaligned = [&] () ALWAYS_INLINE_LAMBDA -> bool {
                            if (tail_range.is_block_alignment_required) {
                                if (static_cast<s64>(cur_size + cur_offset) == tail_range.GetEndVirtualOffset()) {
                                    return false;
                                } else if (!head_unaligned) {
                                    return true;
                                } else {
                                    return head_range.GetEndVirtualOffset() < static_cast<s64>(cur_size + cur_offset);
                                }
                            } else {
                                return false;
                            }
                        }();

                        /* Determine start/end offsets. */
                        const s64 start_offset = head_range.is_block_alignment_required ? head_range.virtual_offset : cur_offset;
                        const s64 end_offset   = tail_range.is_block_alignment_required ? tail_range.GetEndVirtualOffset() : cur_offset + cur_size;

                        /* Perform the read. */
                        bool is_burst_reading = false;
                        R_TRY(core.Read(start_offset, end_offset - start_offset, util::MakeIFunction([&] (size_t size_buffer_required, const CompressedStorageCore::ReadImplFunction &read_impl) -> Result {
                            /* Determine whether we're burst reading. */
                            const AccessRange *unaligned_range = nullptr;
                            if (!is_burst_reading) {
                                /* Check whether we're using head, tail, or none as unaligned. */
                                if (head_unaligned && head_range.virtual_offset <= cur_offset && cur_offset < head_range.GetEndVirtualOffset()) {
                                    unaligned_range = std::addressof(head_range);
                                } else if (tail_unaligned && tail_range.virtual_offset <= cur_offset && cur_offset < tail_range.GetEndVirtualOffset()) {
                                    unaligned_range = std::addressof(tail_range);
                                } else {
                                    is_burst_reading = true;
                                }
                            }
                            AMS_ASSERT((is_burst_reading ^ (unaligned_range != nullptr)));

                            /* Perform reading by burst, or not. */
                            if (is_burst_reading) {
                                /* Check that the access is valid for burst reading. */
                                AMS_ASSERT(size_buffer_required <= cur_size);

                                /* Perform the read. */
                                R_TRY(read_impl(cur_dst, size_buffer_required));

                                /* Advance. */
                                cur_dst    += size_buffer_required;
                                cur_offset += size_buffer_required;
                                cur_size   -= size_buffer_required;

                                /* Determine whether we're going to continue burst reading. */
                                const s64 offset_aligned = tail_unaligned ? tail_range.virtual_offset : end_offset;
                                AMS_ASSERT(cur_offset <= offset_aligned);

                                if (offset_aligned <= cur_offset) {
                                    is_burst_reading = false;
                                }
                            } else {
                                /* We're not burst reading, so we have some unaligned range. */
                                AMS_ASSERT(unaligned_range != nullptr);

                                /* Check that the size is correct. */
                                AMS_ASSERT(size_buffer_required == static_cast<size_t>(unaligned_range->virtual_size));

                                /* Get a pooled buffer for our read. */
                                fssystem::PooledBuffer pooled_buffer;
                                pooled_buffer.Allocate(size_buffer_required, size_buffer_required);

                                /* Perform read. */
                                R_TRY(read_impl(pooled_buffer.GetBuffer(), size_buffer_required));

                                /* Copy the data we read to the destination. */
                                const size_t skip_size = cur_offset - unaligned_range->virtual_offset;
                                const size_t copy_size = std::min<size_t>(cur_size, unaligned_range->GetEndVirtualOffset() - cur_offset);

                                std::memcpy(cur_dst, pooled_buffer.GetBuffer() + skip_size, copy_size);

                                /* Advance. */
                                cur_dst    += copy_size;
                                cur_offset += copy_size;
                                cur_size   -= copy_size;

                                /* If we should, cache what we read. */
                                if (m_block_cache_manager.GetCount() > 0 && unaligned_range->physical_size > m_cache_size_unk_1) {
                                    CacheEntry entry;
                                    for (s64 ofs = unaligned_range->virtual_offset; ofs < unaligned_range->GetEndVirtualOffset(); ofs += entry.range.size) {
                                        /* Find or allocate buffer. */
                                        fs::IBufferManager::MemoryRange memory_range;
                                        R_TRY(this->FindOrAllocateBuffer(std::addressof(memory_range), std::addressof(entry), ofs, unaligned_range->GetEndVirtualOffset() - ofs));

                                        /* If not cached, cache the data. */
                                        if (!entry.is_cached) {
                                            std::memcpy(reinterpret_cast<void *>(memory_range.first), pooled_buffer.GetBuffer() + (ofs - unaligned_range->virtual_offset), entry.range.size);
                                            entry.is_cached = true;
                                        }

                                        /* Store the associated buffer. */
                                        this->StoreAssociateBuffer(memory_range, entry);
                                    }
                                }
                            }

                            R_SUCCEED();
                        })));

                        R_SUCCEED();
                    }
                private:
                    Result FindBuffer(fs::IBufferManager::MemoryRange *out, CacheEntry *out_entry, s64 offset) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(m_block_cache_manager.IsInitialized());
                        AMS_ASSERT(out != nullptr);
                        AMS_ASSERT(out_entry != nullptr);

                        /* Acquire exclusive access to our entries. */
                        std::scoped_lock lk(m_mutex);

                        /* Find the buffer. */
                        R_RETURN(this->FindBufferImpl(out, out_entry, offset));
                    }

                    Result FindBufferImpl(fs::IBufferManager::MemoryRange *out, CacheEntry *out_entry, s64 offset) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(m_mutex.IsLockedByCurrentThread());

                        /* Get our block cache count */
                        const auto count = m_block_cache_manager.GetCount();

                        /* Try to find the buffer. */
                        CacheIndex index;
                        for (index = 0; index < count; ++index) {
                            if (const auto &buffer = m_block_cache_manager[index]; buffer.IsAllocated() && buffer.IsIncluded(offset)) {
                                break;
                            }
                        }

                        /* Set the output. */
                        if (index != count) {
                            /* Acquire the entry. */
                            m_block_cache_manager.AcquireCacheEntry(out_entry, out, index);
                            if (out->first == 0) {
                                *out       = {};
                                *out_entry = {};
                            }
                        } else {
                            *out       = {};
                            *out_entry = {};
                        }

                        R_SUCCEED();
                    }

                    Result FindOrAllocateBuffer(fs::IBufferManager::MemoryRange *out, CacheEntry *out_entry, s64 offset, size_t max_range_size) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(m_block_cache_manager.IsInitialized());
                        AMS_ASSERT(out != nullptr);
                        AMS_ASSERT(out_entry != nullptr);

                        /* Acquire exclusive access to our block cache manager. */
                        std::scoped_lock lk(m_mutex);

                        /* Try to find the buffer. */
                        R_TRY(this->FindBufferImpl(out, out_entry, offset));

                        /* Determine the range size. */
                        const size_t range_size = std::min<size_t>(max_range_size, m_cache_size_unk_0);

                        /* If necessary, allocate. */
                        if (out->first == 0) {
                            R_TRY(fssystem::buffers::AllocateBufferUsingBufferManagerContext(out, m_block_cache_manager.GetAllocator(), range_size, fs::IBufferManager::BufferAttribute(0x20), [] (const fs::IBufferManager::MemoryRange &buffer) -> bool {
                                return buffer.first != 0;
                            }, AMS_CURRENT_FUNCTION_NAME));

                            /* Set the entry for the allocated buffer. */
                            out_entry->is_valid       = out->first != 0;
                            out_entry->is_cached      = false;
                            out_entry->handle         = 0;
                            out_entry->memory_address = 0;
                            out_entry->memory_size    = 0;
                            out_entry->range.offset   = offset;
                            out_entry->range.size     = range_size;
                            out_entry->lru_counter    = 0;
                        }

                        /* Check that the result is valid. */
                        AMS_ASSERT(out_entry->range.size <= out->second);

                        R_SUCCEED();
                    }

                    Result ReadHeadCache(CompressedStorageCore &core, s64 &offset, char *&buffer, size_t &size, AccessRange &head_range, const AccessRange &tail_range) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(buffer != nullptr);

                        /* Read until we're done with the head cache */
                        while (head_range.virtual_size > 0 && head_range.virtual_offset < tail_range.GetEndVirtualOffset()) {
                            /* Cache the access extents. */
                            s64 access_offset  = offset;
                            char *access_buf   = buffer;
                            size_t access_size = size;

                            /* Determine the current access extents. */
                            s64 cur_offset = head_range.virtual_offset + util::AlignDown<s64>(access_offset - head_range.virtual_offset, m_cache_size_unk_0);
                            while (cur_offset < head_range.GetEndVirtualOffset() && cur_offset < static_cast<s64>(offset + size)) {
                                /* Find the relevant entry. */
                                fs::IBufferManager::MemoryRange memory_range = {};
                                CacheEntry entry = {};
                                R_TRY(this->FindBuffer(std::addressof(memory_range), std::addressof(entry), cur_offset));

                                /* If the entry isn't cached, we're done. */
                                R_SUCCEED_IF(!entry.is_cached);

                                /* Otherwise, copy the cacheed data. */
                                const size_t copy_size = std::min<size_t>(access_size, entry.range.GetEndOffset() - access_offset);

                                std::memcpy(access_buf, reinterpret_cast<const void *>(memory_range.first + access_offset - entry.range.offset), copy_size);

                                /* Advance. */
                                access_buf    += copy_size;
                                access_offset += copy_size;
                                access_size   -= copy_size;

                                cur_offset += entry.range.size;

                                /* Store the associated buffer. */
                                this->StoreAssociateBuffer(memory_range, entry);
                            }

                            /* Update the output extents. */
                            buffer = access_buf;
                            offset = access_offset;
                            size   = access_size;

                            /* Determine the new head range. */
                            AccessRange new_head_range = {
                                .virtual_offset              = head_range.GetEndVirtualOffset(),
                                .virtual_size                = 0,
                                .physical_size               = 0,
                                .is_block_alignment_required = true,
                            };
                            if (head_range.GetEndVirtualOffset() == tail_range.virtual_offset) {
                                /* We can use the tail range directly. */
                                new_head_range.virtual_size                = tail_range.virtual_size;
                                new_head_range.physical_size               = tail_range.physical_size;
                                new_head_range.is_block_alignment_required = tail_range.is_block_alignment_required;
                            } else if (head_range.GetEndVirtualOffset() < tail_range.GetEndVirtualOffset()) {
                                /* We need to find the new head range. */
                                R_TRY(core.OperatePerEntry(new_head_range.virtual_offset, 1, [&] (bool *out_continuous, const Entry &entry, s64 virtual_data_size, s64 data_offset, s64 data_read_size) -> Result {
                                    AMS_UNUSED(data_offset, data_read_size);

                                    /* If we can, use the current entry. */
                                    if (entry.virt_offset < tail_range.GetEndVirtualOffset()) {
                                        new_head_range = {
                                            .virtual_offset              = entry.virt_offset,
                                            .virtual_size                = virtual_data_size,
                                            .physical_size               = static_cast<u32>(entry.phys_size),
                                            .is_block_alignment_required = CompressionTypeUtility::IsBlockAlignmentRequired(entry.compression_type),
                                        };
                                    }

                                    /* We only want to determine the new head range, so we're not continuous. */
                                    *out_continuous = false;
                                    R_SUCCEED();
                                }));
                            }

                            /* Update the head range. */
                            head_range = new_head_range;
                        }

                        R_SUCCEED();
                    }

                    Result ReadTailCache(CompressedStorageCore &core, s64 offset, char *buffer, size_t &size, const AccessRange &head_range, AccessRange &tail_range) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(buffer != nullptr);

                        /* Read until we're done with the tail cache */
                        while (tail_range.virtual_offset >= offset) {
                            /* Loop reading, while we can. */
                            const s64 dst_end_offset = offset + size;
                            s64 cur_offset = tail_range.virtual_offset;
                            while (cur_offset < dst_end_offset) {
                                /* Find the relevant entry. */
                                fs::IBufferManager::MemoryRange memory_range = {};
                                CacheEntry entry = {};
                                R_TRY(this->FindBuffer(std::addressof(memory_range), std::addressof(entry), cur_offset));

                                /* If the entry isn't cached, we're done. */
                                R_SUCCEED_IF(!entry.is_cached);

                                /* Sanity check our current access. */
                                AMS_ASSERT(offset <= entry.range.offset);

                                /* Copy the cacheed data. */
                                const s64 cur_end_offset = std::min<s64>(dst_end_offset, entry.range.GetEndOffset());

                                std::memcpy(buffer + entry.range.offset - offset, reinterpret_cast<const void *>(memory_range.first), cur_end_offset - entry.range.offset);

                                /* Advance. */
                                cur_offset += entry.range.size;

                                /* Store the associated buffer. */
                                this->StoreAssociateBuffer(memory_range, entry);
                            }

                            /* Update the output extents. */
                            size -= std::min<s64>(dst_end_offset, tail_range.GetEndVirtualOffset()) - tail_range.virtual_offset;

                            /* Update the tail range. */
                            bool new_tail_found = false;
                            if (tail_range.virtual_offset - 1 >= 0) {
                                /* We need to find the new tail range. */
                                R_TRY(core.OperatePerEntry(tail_range.virtual_offset - 1, 1, [&] (bool *out_continuous, const Entry &entry, s64 virtual_data_size, s64 data_offset, s64 data_read_size) -> Result {
                                    AMS_UNUSED(data_offset, data_read_size);

                                    /* If we can, use the current entry. */
                                    if (head_range.virtual_offset != entry.virt_offset) {
                                        tail_range = {
                                            .virtual_offset              = entry.virt_offset,
                                            .virtual_size                = virtual_data_size,
                                            .physical_size               = static_cast<u32>(entry.phys_size),
                                            .is_block_alignment_required = CompressionTypeUtility::IsBlockAlignmentRequired(entry.compression_type),
                                        };

                                        new_tail_found = true;
                                    }

                                    /* We only want to determine the new head range, so we're not continuous. */
                                    *out_continuous = false;
                                    R_SUCCEED();
                                }));
                            }

                            /* If we didn't find a new tail, write a default (and we're done). */
                            if (!new_tail_found) {
                                tail_range = {
                                    .virtual_offset              = tail_range.virtual_offset,
                                    .virtual_size                = 0,
                                    .physical_size               = 0,
                                    .is_block_alignment_required = true,
                                };
                                break;
                            }
                        }

                        R_SUCCEED();
                    }

                    void StoreAssociateBuffer(const fs::IBufferManager::MemoryRange &memory_range, const CacheEntry &entry) {
                        /* Check pre-conditions. */
                        AMS_ASSERT(m_block_cache_manager.GetCount() > 0);

                        /* Acquire exclusive access to our manager. */
                        std::scoped_lock lk(m_mutex);

                        /* Get empty cache index. */
                        CacheIndex empty_index, lru_index;
                        m_block_cache_manager.GetEmptyCacheEntryIndex(std::addressof(empty_index), std::addressof(lru_index));

                        /* If nothing is empty, invalidate the least recently used entry. */
                        if (empty_index == BlockCacheManager::InvalidCacheIndex) {
                            m_block_cache_manager.InvalidateCacheEntry(lru_index);
                            empty_index = lru_index;
                        }

                        /* Set the entry. */
                        m_block_cache_manager.SetCacheEntry(empty_index, entry, memory_range);
                    }
            };
        private:
            CompressedStorageCore m_core;
            CacheManager m_cache_manager;
        public:
            CompressedStorage() = default;
            virtual ~CompressedStorage() { this->Finalize(); }

            Result Initialize(MemoryResource *bktr_allocator, fs::IBufferManager *cache_allocator, fs::SubStorage data_storage, fs::SubStorage node_storage, fs::SubStorage entry_storage, s32 bktr_entry_count, size_t block_size_max, size_t continuous_reading_size_max, GetDecompressorFunction get_decompressor, size_t cache_size_0, size_t cache_size_1, s32 max_cache_entries) {
                /* Initialize our core. */
                R_TRY(m_core.Initialize(bktr_allocator, data_storage, node_storage, entry_storage, bktr_entry_count, block_size_max, continuous_reading_size_max, get_decompressor));

                /* Get our core size. */
                s64 core_size = 0;
                R_TRY(m_core.GetSize(std::addressof(core_size)));

                /* Initialize our cache manager. */
                R_TRY(m_cache_manager.Initialize(cache_allocator, core_size, cache_size_0, cache_size_1, max_cache_entries));

                R_SUCCEED();
            }

            void Finalize() {
                m_cache_manager.Finalize();
                m_core.Finalize();
            }

            fs::IStorage *GetDataStorage() {
                return m_core.GetDataStorage();
            }

            Result GetDataStorageSize(s64 *out) {
                R_RETURN(m_core.GetDataStorageSize(out));
            }

            Result GetEntryList(Entry *out_entries, s32 *out_read_count, s32 max_entry_count, s64 offset, s64 size) {
                R_RETURN(m_core.GetEntryList(out_entries, out_read_count, max_entry_count, offset, size));
            }

            fssystem::BucketTree &GetEntryTable() {
                return m_core.GetEntryTable();
            }
        public:
            virtual Result QueryAppropriateOffset(s64 *out, s64 offset, s64 access_size, s64 alignment_size) override {
                R_RETURN(m_core.QueryAppropriateOffsetForAsynchronousAccess(out, offset, access_size, alignment_size));
            }
        public:
            virtual Result Read(s64 offset, void *buffer, size_t size) override {
                R_RETURN(m_cache_manager.Read(m_core, offset, buffer, size));
            }

            virtual Result OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                AMS_UNUSED(src, src_size);

                /* Check pre-conditions. */
                AMS_ASSERT(offset >= 0);
                AMS_ASSERT(size >= 0);

                /* Perform the operation. */
                switch (op_id) {
                    case fs::OperationId::Invalidate:
                        m_cache_manager.Invalidate();
                        R_TRY(m_core.Invalidate());
                        break;
                    case fs::OperationId::QueryRange:
                        R_TRY(m_core.QueryRange(dst, dst_size, offset, size));
                        break;
                    default:
                        R_THROW(fs::ResultUnsupportedOperateRangeForCompressedStorage());
                }

                R_SUCCEED();
            }

            virtual Result GetSize(s64 *out) override {
                R_RETURN(m_core.GetSize(out));
            }

            virtual Result Flush() override {
                R_SUCCEED();
            }

            virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                AMS_UNUSED(offset, buffer, size);
                R_THROW(fs::ResultUnsupportedWriteForCompressedStorage());
            }

            virtual Result SetSize(s64 size) override {
                AMS_UNUSED(size);
                /* NOTE: Is Nintendo returning the wrong result here? */
                R_THROW(fs::ResultUnsupportedSetSizeForIndirectStorage());
            }
    };

}
