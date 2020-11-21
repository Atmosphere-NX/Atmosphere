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
#include "sysupdater_apply_manager.hpp"

namespace ams::mitm::sysupdater {

    namespace {

        alignas(os::MemoryPageSize) u8 g_boot_image_update_buffer[64_KB];

        updater::BootImageUpdateType GetBootImageUpdateType() {
            /* NOTE: Here Nintendo uses the value of system setting systeminitializer!boot_image_update_type...but we prefer not to take the risk. */
            return updater::GetBootImageUpdateType(spl::GetHardwareType());
        }

        Result MarkPreCommitForBootImages() {
            /* Set verification required for both normal and safe mode. */
            R_TRY(updater::MarkVerifyingRequired(updater::BootModeType::Normal, g_boot_image_update_buffer, sizeof(g_boot_image_update_buffer)));
            R_TRY(updater::MarkVerifyingRequired(updater::BootModeType::Safe, g_boot_image_update_buffer, sizeof(g_boot_image_update_buffer)));

            /* Pre-commit is now marked. */
            return ResultSuccess();
        }

        Result UpdateBootImages() {
            /* Define a helper to update the images. */
            auto UpdateBootImageImpl = [](updater::BootModeType boot_mode, updater::BootImageUpdateType boot_image_update_type) -> Result {
                /* Get the boot image package id. */
                ncm::SystemDataId boot_image_package_id = {};
                R_TRY_CATCH(updater::GetBootImagePackageId(std::addressof(boot_image_package_id), boot_mode, g_boot_image_update_buffer, sizeof(g_boot_image_update_buffer))) {
                    R_CATCH(updater::ResultBootImagePackageNotFound) {
                        /* Nintendo simply falls through when the package is not found. */
                    }
                } R_END_TRY_CATCH;


                /* Update the boot images. */
                R_TRY_CATCH(updater::UpdateBootImagesFromPackage(boot_image_package_id, boot_mode, g_boot_image_update_buffer, sizeof(g_boot_image_update_buffer), boot_image_update_type)) {
                    R_CATCH(updater::ResultBootImagePackageNotFound) {
                        /* Nintendo simply falls through when the package is not found. */
                    }
                } R_END_TRY_CATCH;

                /* Mark the images verified. */
                R_TRY(updater::MarkVerified(boot_mode, g_boot_image_update_buffer, sizeof(g_boot_image_update_buffer)));

                /* The boot images are updated. */
                return ResultSuccess();
            };

            /* Get the boot image update type. */
            auto boot_image_update_type = GetBootImageUpdateType();

            /* Update boot images for safe mode. */
            R_TRY(UpdateBootImageImpl(updater::BootModeType::Safe, boot_image_update_type));

            /* Update boot images for normal mode. */
            R_TRY(UpdateBootImageImpl(updater::BootModeType::Normal, boot_image_update_type));

            /* Both sets of images are updated. */
            return ResultSuccess();
        }

    }

    Result SystemUpdateApplyManager::ApplyPackageTask(ncm::PackageSystemDowngradeTask *task) {
        /* Lock the apply mutex. */
        std::scoped_lock lk(this->apply_mutex);

        /* NOTE: Here, Nintendo creates a system report for the update. */

        /* Mark boot images to note that we're updating. */
        R_TRY(MarkPreCommitForBootImages());

        /* Commit the task. */
        R_TRY(task->Commit());

        /* Update the boot images. */
        R_TRY(UpdateBootImages());

        return ResultSuccess();
    }

}
