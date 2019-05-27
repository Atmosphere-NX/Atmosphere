/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <stratosphere.hpp>

#include "updater_types.hpp"

class Boot0Accessor;
enum class Boot0Partition;
enum class Package2Type;

class Updater {
    private:
        static bool HasEks(BootImageUpdateType boot_image_update_type);
        static bool HasAutoRcmPreserve(BootImageUpdateType boot_image_update_type);
        static u32 GetNcmTitleType(BootModeType mode);
        static Result ValidateWorkBuffer(const void *work_buffer, size_t work_buffer_size);
        static Result GetVerificationState(VerificationState *out, void *work_buffer, size_t work_buffer_size);
        static Result VerifyBootImagesAndRepairIfNeeded(bool *out_repaired, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        static Result GetBootImagePackageDataId(u64 *out_data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size);
        static Result VerifyBootImages(u64 data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        static Result VerifyBootImagesNormal(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        static Result VerifyBootImagesSafe(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        static Result UpdateBootImages(u64 data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        static Result UpdateBootImagesNormal(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        static Result UpdateBootImagesSafe(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        static Result SetVerificationNeeded(BootModeType mode, bool needed, void *work_buffer, size_t work_buffer_size);
        static Result RepairBootImages(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);

        /* Path functionality. */
        static const char *GetBootImagePackageMountPath();
        static const char *GetBctPath(BootImageUpdateType boot_image_update_type);
        static const char *GetPackage1Path(BootImageUpdateType boot_image_update_type);
        static const char *GetPackage2Path(BootImageUpdateType boot_image_update_type);

        /* File helpers. */
        static Result ReadFile(size_t *out_size, void *dst, size_t dst_size, const char *path);
        static Result GetFileHash(size_t *out_size, void *dst_hash, const char *path, void *work_buffer, size_t work_buffer_size);

        /* Package helpers. */
        static Result ValidateBctFileHash(Boot0Accessor &accessor, Boot0Partition which, const void *stored_hash, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        static Result GetPackage2Hash(void *dst_hash, size_t package2_size, void *work_buffer, size_t work_buffer_size, Package2Type which);
        static Result WritePackage2(void *work_buffer, size_t work_buffer_size, Package2Type which, BootImageUpdateType boot_image_update_type);
    public:
        static BootImageUpdateType GetBootImageUpdateType(HardwareType hw_type);
        static Result VerifyBootImagesAndRepairIfNeeded(bool *out_repaired_normal, bool *out_repaired_safe, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
};