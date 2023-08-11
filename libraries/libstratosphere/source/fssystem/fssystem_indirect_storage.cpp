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
#include <stratosphere.hpp>

namespace ams::fssystem {

    Result IndirectStorage::Initialize(IAllocator *allocator, fs::SubStorage table_storage) {
        /* Read and verify the bucket tree header. */
        BucketTree::Header header;
        R_TRY(table_storage.Read(0, std::addressof(header), sizeof(header)));
        R_TRY(header.Verify());

        /* Determine extents. */
        const auto node_storage_size    = QueryNodeStorageSize(header.entry_count);
        const auto entry_storage_size   = QueryEntryStorageSize(header.entry_count);
        const auto node_storage_offset  = QueryHeaderStorageSize();
        const auto entry_storage_offset = node_storage_offset + node_storage_size;

        /* Initialize. */
        R_RETURN(this->Initialize(allocator, fs::SubStorage(std::addressof(table_storage), node_storage_offset, node_storage_size), fs::SubStorage(std::addressof(table_storage), entry_storage_offset, entry_storage_size), header.entry_count));
    }

    void IndirectStorage::Finalize() {
        if (this->IsInitialized()) {
            m_table.Finalize();
            for (auto i = 0; i < StorageCount; i++) {
                m_data_storage[i] = fs::SubStorage();
            }
        }
    }

    Result IndirectStorage::GetEntryList(Entry *out_entries, s32 *out_entry_count, s32 entry_count, s64 offset, s64 size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(size >= 0);
        AMS_ASSERT(this->IsInitialized());

        /* Clear the out count. */
        R_UNLESS(out_entry_count != nullptr, fs::ResultNullptrArgument());
        *out_entry_count = 0;

        /* Succeed if there's no range. */
        R_SUCCEED_IF(size == 0);

        /* If we have an output array, we need it to be non-null. */
        R_UNLESS(out_entries != nullptr || entry_count == 0, fs::ResultNullptrArgument());

        /* Check that our range is valid. */
        BucketTree::Offsets table_offsets;
        R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

        R_UNLESS(table_offsets.IsInclude(offset, size), fs::ResultOutOfRange());

        /* Find the offset in our tree. */
        BucketTree::Visitor visitor;
        R_TRY(m_table.Find(std::addressof(visitor), offset));
        {
            const auto entry_offset = visitor.Get<Entry>()->GetVirtualOffset();
            R_UNLESS(0 <= entry_offset && table_offsets.IsInclude(entry_offset), fs::ResultInvalidIndirectEntryOffset());
        }

        /* Prepare to loop over entries. */
        const auto end_offset = offset + static_cast<s64>(size);
        s32 count = 0;

        auto cur_entry = *visitor.Get<Entry>();
        while (cur_entry.GetVirtualOffset() < end_offset) {
            /* Try to write the entry to the out list. */
            if (entry_count != 0) {
                if (count >= entry_count) {
                    break;
                }
                std::memcpy(out_entries + count, std::addressof(cur_entry), sizeof(Entry));
            }

            count++;

            /* Advance. */
            if (visitor.CanMoveNext()) {
                R_TRY(visitor.MoveNext());
                cur_entry = *visitor.Get<Entry>();
            } else {
                break;
            }
        }

        /* Write the output count. */
        *out_entry_count = count;
        R_SUCCEED();
    }

    Result IndirectStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(this->IsInitialized());

        /* Succeed if there's nothing to read. */
        R_SUCCEED_IF(size == 0);

        /* Ensure that we have a buffer to read to. */
        R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

        R_TRY((this->OperatePerEntry<true, true>(offset, size, [=](fs::IStorage *storage, s64 data_offset, s64 cur_offset, s64 cur_size) -> Result {
            R_TRY(storage->Read(data_offset, reinterpret_cast<u8 *>(buffer) + (cur_offset - offset), static_cast<size_t>(cur_size)));
            R_SUCCEED();
        })));

        R_SUCCEED();
    }

    Result IndirectStorage::OperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        /* Validate pre-conditions. */
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(size >= 0);
        AMS_ASSERT(this->IsInitialized());

        switch (op_id) {
            case fs::OperationId::Invalidate:
                {
                    if (!m_table.IsEmpty()) {
                        /* Invalidate our table's cache. */
                        R_TRY(m_table.InvalidateCache());

                        /* Invalidate our storages. */
                        for (auto &storage : m_data_storage) {
                            R_TRY(storage.OperateRange(fs::OperationId::Invalidate, 0, std::numeric_limits<s64>::max()));
                        }
                    }
                    R_SUCCEED();
                }
            case fs::OperationId::QueryRange:
                {
                    /* Validate that we have an output range info. */
                    R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                    R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());

                    if (size > 0) {
                        /* Validate arguments. */
                        BucketTree::Offsets table_offsets;
                        R_TRY(m_table.GetOffsets(std::addressof(table_offsets)));

                        R_UNLESS(table_offsets.IsInclude(offset, size), fs::ResultOutOfRange());

                        if (!m_table.IsEmpty()) {
                            /* Create a new info. */
                            fs::QueryRangeInfo merged_info;
                            merged_info.Clear();

                            /* Operate on our entries. */
                            R_TRY((this->OperatePerEntry<false, true>(offset, size,  [=, &merged_info](fs::IStorage *storage, s64 data_offset, s64 cur_offset, s64 cur_size) -> Result {
                                AMS_UNUSED(cur_offset);

                                fs::QueryRangeInfo cur_info;
                                R_TRY(storage->OperateRange(std::addressof(cur_info), sizeof(cur_info), op_id, data_offset, cur_size, src, src_size));
                                merged_info.Merge(cur_info);
                                R_SUCCEED();
                            })));

                            /* Write the merged info. */
                            *reinterpret_cast<fs::QueryRangeInfo *>(dst) = merged_info;
                        }
                    }
                    R_SUCCEED();
                }
            default:
                R_THROW(fs::ResultUnsupportedOperateRangeForIndirectStorage());
        }

        R_SUCCEED();
    }

}
