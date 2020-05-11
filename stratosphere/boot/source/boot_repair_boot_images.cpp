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
#include "boot_power_utils.hpp"
#include "boot_repair_boot_images.hpp"

namespace ams::boot {

    namespace {

        /* Globals. */
        alignas(os::MemoryPageSize) u8 g_boot_image_work_buffer[0x10000];

    }

    void CheckAndRepairBootImages() {
        const auto boot_image_update_type = updater::GetBootImageUpdateType(spl::GetHardwareType());

        bool repaired_normal, repaired_safe;
        if (R_SUCCEEDED(updater::VerifyBootImagesAndRepairIfNeeded(&repaired_normal, &repaired_safe, g_boot_image_work_buffer, sizeof(g_boot_image_work_buffer), boot_image_update_type)) && repaired_normal) {
            /* Nintendo only performs a reboot on successful normal repair. */
            RebootSystem();
        }
    }

}
