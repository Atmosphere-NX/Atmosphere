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

namespace ams::prfile2::pdm::disk {

    pdm::Error OpenDisk(InitDisk *init_disk_table, HandleType *out) {
        /* Check the arguments. */
        if (out == nullptr || init_disk_table == nullptr || init_disk_table->function == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Find a free disk holder. */
        DiskHolder *holder = nullptr;
        for (auto &h : impl::g_disk_set.disk_holders) {
            if (h.disk == nullptr) {
                holder = std::addressof(h);
                break;
            }
        }
        if (holder == nullptr) {
            return pdm::Error_NotExistFreeDiskStruct;
        }

        /* Find a free disk. */
        Disk *disk = nullptr;
        for (auto &d : impl::g_disk_set.disks) {
            if (!d.IsOpen()) {
                disk = std::addressof(d);
                break;
            }
        }
        if (disk == nullptr) {
            return pdm::Error_NotExistFreeDiskStruct;
        }
        const auto disk_id = disk - impl::g_disk_set.disks;

        /* Call the disk initialze function. */
        init_disk_table->function(std::addressof(disk->disk_table), init_disk_table->ui_ext);

        /* Set the disk as open .*/
        disk->SetOpen(true);

        /* Set the init disk table. */
        disk->init_disk_table = init_disk_table;

        /* Note the opened disk. */
        ++impl::g_disk_set.num_allocated_disks;

        /* Increment the disk's signature. */
        disk->signature = static_cast<u16>(disk->signature + 1);

        /* Increment the disk's open count. */
        ++disk->open_count;

        /* Set the disk in the holder. */
        holder->signature = disk->signature;
        holder->disk      = disk;

        /* Set the output handle. */
        *out = ConstructDiskHandle(disk_id, disk->signature);

        /* If this was the first opening for the disk, initialize it. */
        if (disk->open_count == 1) {
            if (auto err = disk->disk_table.function_table->initialize(*out); err != pdm::Error_Ok) {
                /* Close the disk. */
                --disk->open_count;
                disk->SetOpen(false);
                --impl::g_disk_set.num_allocated_disks;

                /* Reset the holder. */
                holder->disk = nullptr;

                return pdm::Error_DriverError;
            }
        }

        return pdm::Error_Ok;
    }

    pdm::Error CloseDisk(HandleType handle) {
        /* Get the disk. */
        Disk *disk = GetDisk(handle);
        if (disk == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Check that the disk is open and unlocked. */
        if (!disk->IsOpen()) {
            return pdm::Error_StateClosed;
        }
        if (disk->IsLocked()) {
            return pdm::Error_StateLocked;
        }

        /* Get the disk holder. */
        DiskHolder *holder = GetDiskHolder(handle);
        if (holder == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Close the disk. */
        if (disk->open_count == 1) {
            /* Finalize the disk. */
            if (auto err = disk->disk_table.function_table->finalize(handle); err != pdm::Error_Ok) {
                if (auto part = disk->current_partition_handle; part != InvalidHandle) {
                    part::SetDriverErrorCode(part, err);
                }
                return pdm::Error_DriverError;
            }

            /* Set the disk as closed. */
            disk->SetOpen(false);
            --impl::g_disk_set.num_allocated_disks;

            /* Clear the disk holder. */
            holder->disk = nullptr;
        }

        /* Decrement the disk's open count. */
        --disk->open_count;

        return pdm::Error_Ok;
    }

}
