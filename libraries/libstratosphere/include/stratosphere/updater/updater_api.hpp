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
#include <stratosphere/updater/updater_types.hpp>
#include <stratosphere/spl/spl_types.hpp>

namespace ams::updater {

    /* Public API. */
    BootImageUpdateType GetBootImageUpdateType(int boot_image_update_type);
    BootImageUpdateType GetBootImageUpdateType(spl::HardwareType hw_type);

    Result GetBootImagePackageId(ncm::SystemDataId *out_data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size);

    Result MarkVerifyingRequired(BootModeType mode, void *work_buffer, size_t work_buffer_size);
    Result MarkVerified(BootModeType mode, void *work_buffer, size_t work_buffer_size);

    Result UpdateBootImagesFromPackage(ncm::SystemDataId boot_image_package_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);

    Result VerifyBootImagesAndRepairIfNeeded(bool *out_repaired_normal, bool *out_repaired_safe, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);

}
