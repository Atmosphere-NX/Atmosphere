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
#include <stratosphere/fssystem/fssystem_indirect_storage.hpp>

namespace ams::fssystem {

    template<bool ContinuousCheck, typename F>
    Result IndirectStorage::OperatePerEntry(s64 offset, s64 size, F func) {
        /* Validate preconditions. */
        AMS_ASSERT(offset >= 0);
        AMS_ASSERT(size >= 0);
        AMS_ASSERT(this->IsInitialized());

        /* Succeed if there's nothing to operate on. */
        R_SUCCEED_IF(size == 0);

        /* Validate arguments. */
        R_UNLESS(this->table.Includes(offset, size), fs::ResultOutOfRange());

        /* Find the offset in our tree. */
        BucketTree::Visitor visitor;
        R_TRY(this->table.Find(std::addressof(visitor), offset));
        {
            const auto entry_offset = visitor.Get<Entry>()->GetVirtualOffset();
            R_UNLESS(0 <= entry_offset && this->table.Includes(entry_offset), fs::ResultInvalidIndirectEntryOffset());
        }

        /* Prepare to operate in chunks. */
        auto cur_offset = offset;
        const auto end_offset = offset + static_cast<s64>(size);
        BucketTree::ContinuousReadingInfo cr_info;

        while (cur_offset < end_offset) {
            /* Get the current entry. */
            const auto cur_entry = *visitor.Get<Entry>();

            /* Get and validate the entry's offset. */
            const auto cur_entry_offset = cur_entry.GetVirtualOffset();
            R_UNLESS(cur_entry_offset <= cur_offset, fs::ResultInvalidIndirectEntryOffset());

            /* Validate the storage index. */
            R_UNLESS(0 <= cur_entry.storage_index && cur_entry.storage_index < StorageCount, fs::ResultInvalidIndirectEntryStorageIndex());

            /* If we need to check the continuous info, do so. */
            if constexpr (ContinuousCheck) {
                /* Scan, if we need to. */
                if (cr_info.CheckNeedScan()) {
                    R_TRY(visitor.ScanContinuousReading<ContinuousReadingEntry>(std::addressof(cr_info), cur_offset, static_cast<size_t>(end_offset - cur_offset)));
                }

                /* Process a base storage entry. */
                if (cr_info.CanDo()) {
                    /* Ensure that we can process. */
                    R_UNLESS(cur_entry.storage_index == 0, fs::ResultInvalidIndirectEntryStorageIndex());

                    /* Get the current data storage's size. */
                    s64 cur_data_storage_size;
                    R_TRY(this->data_storage[0].GetSize(std::addressof(cur_data_storage_size)));

                    /* Ensure that we remain within range. */
                    const auto data_offset           = cur_offset - cur_entry_offset;
                    const auto cur_entry_phys_offset = cur_entry.GetPhysicalOffset();
                    const auto cur_size              = static_cast<s64>(cr_info.GetReadSize());
                    R_UNLESS(0 <= cur_entry_phys_offset && cur_entry_phys_offset <= cur_data_storage_size, fs::ResultInvalidIndirectEntryOffset());
                    R_UNLESS(cur_entry_phys_offset + data_offset + cur_size <= cur_data_storage_size,      fs::ResultInvalidIndirectStorageSize());

                    /* Operate. */
                    R_TRY(func(std::addressof(this->data_storage[0]), cur_entry_phys_offset + data_offset, cur_offset, cur_size));

                    /* Mark as done. */
                    cr_info.Done();
                }
            }

            /* Get and validate the next entry offset. */
            s64 next_entry_offset;
            if (visitor.CanMoveNext()) {
                R_TRY(visitor.MoveNext());
                next_entry_offset = visitor.Get<Entry>()->GetVirtualOffset();
                R_UNLESS(this->table.Includes(next_entry_offset), fs::ResultInvalidIndirectEntryOffset());
            } else {
                next_entry_offset = this->table.GetEnd();
            }
            R_UNLESS(cur_offset < next_entry_offset, fs::ResultInvalidIndirectEntryOffset());

            /* Get the offset of the entry in the data we read. */
            const auto data_offset = cur_offset - cur_entry_offset;
            const auto data_size   = (next_entry_offset - cur_entry_offset) - data_offset;
            AMS_ASSERT(data_size > 0);

            /* Determine how much is left. */
            const auto remaining_size = end_offset - cur_offset;
            const auto cur_size       = std::min(remaining_size, data_size);
            AMS_ASSERT(cur_size <= size);

            /* Operate, if we need to. */
            bool needs_operate;
            if constexpr (!ContinuousCheck) {
                needs_operate = true;
            } else {
                needs_operate = !cr_info.IsDone() || cur_entry.storage_index != 0;
            }

            if (needs_operate) {
                /* Get the current data storage's size. */
                s64 cur_data_storage_size;
                R_TRY(this->data_storage[cur_entry.storage_index].GetSize(std::addressof(cur_data_storage_size)));

                /* Ensure that we remain within range. */
                const auto cur_entry_phys_offset = cur_entry.GetPhysicalOffset();
                R_UNLESS(0 <= cur_entry_phys_offset && cur_entry_phys_offset <= cur_data_storage_size, fs::ResultIndirectStorageCorrupted());
                R_UNLESS(cur_entry_phys_offset + data_offset + cur_size <= cur_data_storage_size,      fs::ResultIndirectStorageCorrupted());

                R_TRY(func(std::addressof(this->data_storage[cur_entry.storage_index]), cur_entry_phys_offset + data_offset, cur_offset, cur_size));
            }

            cur_offset += cur_size;
        }

        return ResultSuccess();
    }

}
