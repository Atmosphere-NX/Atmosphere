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
#include <stratosphere/fssystem/fssystem_pooled_buffer.hpp>
#include <stratosphere/fssystem/fssystem_bucket_tree.hpp>
#include <stratosphere/fssystem/fssystem_bucket_tree_utils.hpp>

namespace ams::fssystem {

    template<typename EntryType>
    Result BucketTree::ScanContinuousReading(ContinuousReadingInfo *out_info, const ContinuousReadingParam<EntryType> &param) const {
        static_assert(util::is_pod<ContinuousReadingParam<EntryType>>::value);

        /* Validate our preconditions. */
        AMS_ASSERT(this->IsInitialized());
        AMS_ASSERT(out_info != nullptr);
        AMS_ASSERT(this->entry_size == sizeof(EntryType));

        /* Reset the output. */
        out_info->Reset();

        /* If there's nothing to read, we're done. */
        R_SUCCEED_IF(param.size == 0);

        /* If we're reading a fragment, we're done. */
        R_SUCCEED_IF(param.entry.IsFragment());

        /* Validate the first entry. */
        auto entry = param.entry;
        auto cur_offset  = param.offset;
        R_UNLESS(entry.GetVirtualOffset() <= cur_offset, fs::ResultOutOfRange());

        /* Create a pooled buffer for our scan. */
        PooledBuffer pool(this->node_size, 1);
        char *buffer = nullptr;

        /* Read the node. */
        if (this->node_size <= pool.GetSize()) {
            buffer = pool.GetBuffer();
            const auto ofs = param.entry_set.index * static_cast<s64>(this->node_size);
            R_TRY(this->entry_storage.Read(ofs, buffer, this->node_size));
        }

        /* Calculate extents. */
        const auto end_offset = cur_offset + static_cast<s64>(param.size);
        s64 phys_offset       = entry.GetPhysicalOffset();

        /* Start merge tracking. */
        s64 merge_size = 0;
        s64 readable_size = 0;
        bool merged = false;

        /* Iterate. */
        auto entry_index = param.entry_index;
        for (const auto entry_count = param.entry_set.count; entry_index < entry_count; ++entry_index) {
            /* If we're past the end, we're done. */
            if (end_offset <= cur_offset) {
                break;
            }

            /* Validate the entry offset. */
            const auto entry_offset = entry.GetVirtualOffset();
            R_UNLESS(entry_offset <= cur_offset, fs::ResultInvalidIndirectEntryOffset());

            /* Get the next entry. */
            EntryType next_entry = {};
            s64 next_entry_offset;

            if (entry_index + 1 < entry_count) {
                if (buffer != nullptr) {
                    const auto ofs = impl::GetBucketTreeEntryOffset(0, this->entry_size, entry_index + 1);
                    std::memcpy(std::addressof(next_entry), buffer + ofs, this->entry_size);
                } else {
                    const auto ofs = impl::GetBucketTreeEntryOffset(param.entry_set.index, this->node_size, this->entry_size, entry_index + 1);
                    R_TRY(this->entry_storage.Read(ofs, std::addressof(next_entry), this->entry_size));
                }

                next_entry_offset = next_entry.GetVirtualOffset();
                R_UNLESS(this->Includes(next_entry_offset), fs::ResultInvalidIndirectEntryOffset());
            } else {
                next_entry_offset = param.entry_set.offset;
            }

            /* Validate the next entry offset. */
            R_UNLESS(cur_offset < next_entry_offset, fs::ResultInvalidIndirectEntryOffset());

            /* Determine the much data there is. */
            const auto data_size = next_entry_offset - cur_offset;
            AMS_ASSERT(data_size > 0);

            /* Determine how much data we should read. */
            const auto remaining_size = end_offset - cur_offset;
            const size_t read_size = static_cast<size_t>(std::min(data_size, remaining_size));
            AMS_ASSERT(read_size <= param.size);

            /* Update our merge tracking. */
            if (entry.IsFragment()) {
                /* If we can't merge, stop looping. */
                if (EntryType::FragmentSizeMax <= read_size || remaining_size <= data_size) {
                    break;
                }

                /* Otherwise, add the current size to the merge size. */
                merge_size += read_size;
            } else {
                /*  If we can't merge, stop looping. */
                if (phys_offset != entry.GetPhysicalOffset()) {
                    break;
                }

                /* Add the size to the readable amount. */
                readable_size += merge_size + read_size;
                AMS_ASSERT(readable_size <= static_cast<s64>(param.size));

                /* Update whether we've merged. */
                merged |= merge_size > 0;
                merge_size = 0;
            }

            /* Advance. */
            cur_offset += read_size;
            AMS_ASSERT(cur_offset <= end_offset);

            phys_offset += next_entry_offset - entry_offset;
            entry = next_entry;
        }

        /* If we merged, set our readable size. */
        if (merged) {
            out_info->SetReadSize(static_cast<size_t>(readable_size));
        }
        out_info->SetSkipCount(entry_index - param.entry_index);

        return ResultSuccess();
    }

    template<typename EntryType>
    Result BucketTree::Visitor::ScanContinuousReading(ContinuousReadingInfo *out_info, s64 offset, size_t size) const {
        static_assert(util::is_pod<EntryType>::value);
        AMS_ASSERT(this->IsValid());

        /* Create our parameters. */
        ContinuousReadingParam<EntryType> param = {
            offset, size, this->entry_set.header, this->entry_index
        };
        std::memcpy(std::addressof(param.entry), this->entry, sizeof(EntryType));

        /* Scan. */
        return this->tree->ScanContinuousReading<EntryType>(out_info, param);
    }

}
