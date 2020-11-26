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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "prfile2_pdm_disk_set.hpp"

namespace ams::prfile2::pdm::part {

    pdm::Error OpenPartition(HandleType disk_handle, u16 partition_id, HandleType *out) {
        /* Check the arguments. */
        if (out == nullptr || disk_handle == InvalidHandle) {
            return pdm::Error_InvalidParameter;
        }

        /* Find a free partition holder. */
        PartitionHolder *holder = nullptr;
        for (auto &h : impl::g_disk_set.partition_holders) {
            if (h.partition == nullptr) {
                holder = std::addressof(h);
                break;
            }
        }
        if (holder == nullptr) {
            return pdm::Error_NotExistFreePartitionStruct;
        }

        /* Locate a free (or matching) partition. */
        Partition *part = nullptr;
        bool found_matching = false;
        for (auto &p : impl::g_disk_set.partitions) {
            if (p.IsOpen()) {
                if (p.disk_handle == disk_handle && p.partition_id == partition_id) {
                    part = std::addressof(p);
                    found_matching = true;
                    break;
                }
            } else {
                if (part == nullptr) {
                    part = std::addressof(p);
                }
            }
        }
        if (part == nullptr) {
            return pdm::Error_NotExistFreePartitionStruct;
        }
        const auto part_id = part - impl::g_disk_set.partitions;

        /* If we're not working with a match, open the new partition. */
        if (!found_matching) {
            /* Set the partition as open. */
            part->SetOpen(true);

            /* Increment the number of open partitions. */
            ++impl::g_disk_set.num_partitions;

            /* Set the partition's disk/id. */
            part->disk_handle  = disk_handle;
            part->partition_id = partition_id;
        }

        /* Increment the partition's signature. */
        part->signature = static_cast<u16>(part->signature + 1);

        /* Increment the partition's open count. */
        ++part->open_count;

        /* Set the holder. */
        holder->signature = part->signature;
        holder->partition = part;

        /* Set the output handle. */
        *out = ConstructPartitionHandle(part_id, part->signature);

        return pdm::Error_Ok;
    }

    pdm::Error ClosePartition(HandleType part_handle) {
        /* Get the partition. */
        Partition *part = GetPartition(part_handle);
        if (part == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Check that the partition is open and unlocked. */
        if (!part->IsOpen()) {
            return pdm::Error_StateClosed;
        }
        if (part->IsLocked()) {
            return pdm::Error_StateLocked;
        }

        /* Get the partition holder. */
        PartitionHolder *holder = GetPartitionHolder(part_handle);
        if (holder == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Close the partition. */
        if (part->open_count == 1) {
            /* Set the partition as closed. */
            part->SetOpen(false);
            --impl::g_disk_set.num_partitions;

            /* Clear the partition holder. */
            holder->partition = nullptr;
        }

        /* Decrement the partition's open count. */
        --part->open_count;

        return pdm::Error_Ok;
    }

    void SetDriverErrorCode(HandleType part_handle, pdm::Error err) {
        GetPartitionUnsafe(part_handle)->last_driver_error = err;
    }

    void CheckPartitionOpen(HandleType disk_handle, bool *out) {
        /* TODO */
        AMS_UNUSED(disk_handle, out);
        AMS_ABORT("CheckPartitionOpen");
    }

    void NotifyMediaEvent(HandleType disk_handle, u32 event) {
        /* TODO */
        AMS_UNUSED(disk_handle, event);
        AMS_ABORT("NotifyMediaEvent");
    }

}
