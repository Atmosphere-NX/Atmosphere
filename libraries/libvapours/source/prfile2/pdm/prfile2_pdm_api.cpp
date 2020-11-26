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

namespace ams::prfile2::pdm {

    namespace impl {

        constinit DiskSet g_disk_set;

    }

    pdm::Error Initialize(u32 config, void *param) {
        AMS_UNUSED(config, param);

        /* Clear the disk set. */
        std::memset(std::addressof(impl::g_disk_set), 0, sizeof(impl::g_disk_set));

        return pdm::Error_Ok;
    }

    pdm::Error OpenDisk(InitDisk *init_disk_table, HandleType *out) {
        /* Check the arguments. */
        if (out == nullptr) {
            return pdm::Error_InvalidParameter;
        }

        /* Set the output as invalid. */
        *out = InvalidHandle;

        /* Open the disk. */
        return disk::OpenDisk(init_disk_table, out);
    }

    pdm::Error CloseDisk(HandleType handle) {
        /* Check the input. */
        if (IsInvalidHandle(handle)) {
            return pdm::Error_InvalidParameter;
        }

        /* Close the disk. */
        return disk::CloseDisk(handle);
    }

    pdm::Error OpenPartition(HandleType disk_handle, u16 part_id, HandleType *out) {
        /* Check the arguments. */
        if (out == nullptr || IsInvalidHandle(disk_handle)) {
            return pdm::Error_InvalidParameter;
        }

        /* Set the output as invalid. */
        *out = InvalidHandle;

        /* Open the partition. */
        return part::OpenPartition(disk_handle, part_id, out);
    }

    pdm::Error ClosePartition(HandleType handle) {
        /* Check the input. */
        if (IsInvalidHandle(handle)) {
            return pdm::Error_InvalidParameter;
        }

        /* Close the partition. */
        return part::ClosePartition(handle);
    }

}
