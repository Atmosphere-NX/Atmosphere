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

    pdm::Error CheckDataEraseRequest(HandleType part_handle, bool *out) {
        /* Check parameters. */
        Partition *part = GetPartition(part_handle);
        if (out == nullptr || part == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Check the disk. */
        return pdm::disk::CheckDataEraseRequest(part->disk_handle, out);
    }

    pdm::Error CheckMediaDetect(HandleType part_handle, bool *out) {
        /* Check parameters. */
        Partition *part = GetPartition(part_handle);
        if (out == nullptr || part == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Get the disk (unsafe/not checked). */
        Disk *disk = GetDiskUnsafe(part->disk_handle);

        /* Default to no status change detected. */
        *out = false;

        /* Detect status change via partition nbc detect. */
        volatile NonBlockingProtocolType nbc;
        do {
            do {
                nbc = disk->nbc;
            } while ((nbc & 1) != 0);
            if (nbc != part->nbc_detect) {
                *out = true;
            }
        } while (nbc != disk->nbc);

        return pdm::Error_Ok;
    }

    pdm::Error CheckMediaInsert(HandleType part_handle, bool *out) {
        /* Check parameters. */
        Partition *part = GetPartition(part_handle);
        if (out == nullptr || part == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Check if the disk is inserted. */
        return pdm::disk::CheckMediaInsert(part->disk_handle, out);
    }

}
