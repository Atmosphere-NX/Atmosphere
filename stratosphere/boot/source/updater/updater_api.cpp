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

#include <switch.h>
#include <stratosphere.hpp>

#include "updater_api.hpp"
#include "updater_bis_save.hpp"

Result Updater::ValidateWorkBuffer(const void *work_buffer, size_t work_buffer_size) {
    if (work_buffer_size < BctSize + EksSize) {
        return ResultUpdaterTooSmallWorkBuffer;
    }
    if (reinterpret_cast<uintptr_t>(work_buffer) & 0xFFF) {
        return ResultUpdaterMisalignedWorkBuffer;
    }
    if (reinterpret_cast<uintptr_t>(work_buffer_size) & 0x1FF) {
        return ResultUpdaterMisalignedWorkBuffer;
    }
    return ResultSuccess;
}

BootImageUpdateType Updater::GetBootImageUpdateType(HardwareType hw_type) {
    switch (hw_type) {
        case HardwareType_Icosa:
        case HardwareType_Copper:
            return BootImageUpdateType_Erista;
        case HardwareType_Hoag:
        case HardwareType_Iowa:
            return BootImageUpdateType_Mariko;
        default:
            std::abort();
    }
}

bool Updater::HasEks(BootImageUpdateType boot_image_update_type) {
    switch (boot_image_update_type) {
        case BootImageUpdateType_Erista:
            return true;
        case BootImageUpdateType_Mariko:
            return false;
        default:
            std::abort();
    }
}

bool Updater::HasAutoRcmPreserve(BootImageUpdateType boot_image_update_type) {
    switch (boot_image_update_type) {
        case BootImageUpdateType_Erista:
            return true;
        case BootImageUpdateType_Mariko:
            return false;
        default:
            std::abort();
    }
}

u32 Updater::GetNcmTitleType(BootModeType mode) {
    switch (mode) {
        case BootModeType_Normal:
            return NcmContentMetaType_BootImagePackage;
        case BootModeType_Safe:
            return NcmContentMetaType_BootImagePackageSafe;
        default:
            std::abort();
    }
}

Result Updater::GetVerificationState(VerificationState *out, void *work_buffer, size_t work_buffer_size) {
    Result rc;

    /* Always set output to true before doing anything else. */
    out->needs_verify_normal = true;
    out->needs_verify_safe = true;

    /* Ensure work buffer is big enough for us to do what we want to do. */
    if (R_FAILED((rc = ValidateWorkBuffer(work_buffer, work_buffer_size)))) {
        return rc;
    }

    /* Initialize boot0 save accessor. */
    BisSave save;
    if (R_FAILED((rc = save.Initialize(work_buffer, work_buffer_size)))) {
        return rc;
    }
    ON_SCOPE_EXIT { save.Finalize(); };

    /* Load save from NAND. */
    if (R_FAILED((rc = save.Load()))) {
        return rc;
    }

    /* Read data from save. */
    out->needs_verify_normal = save.GetNeedsVerification(BootModeType_Normal);
    out->needs_verify_safe = save.GetNeedsVerification(BootModeType_Safe);
    return ResultSuccess;
}

Result Updater::VerifyBootImagesAndRepairIfNeeded(bool *out_repaired_normal, bool *out_repaired_safe, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    Result rc;

    /* Always set output to false before doing anything else. */
    *out_repaired_normal = false;
    *out_repaired_safe = false;

    /* Ensure work buffer is big enough for us to do what we want to do. */
    if (R_FAILED((rc = ValidateWorkBuffer(work_buffer, work_buffer_size)))) {
        return rc;
    }

    /* Get verification state from NAND. */
    VerificationState verification_state;
    if (R_FAILED((rc = GetVerificationState(&verification_state, work_buffer, work_buffer_size)))) {
        return rc;
    }

    /* If we don't need to verify anything, we're done. */
    if (!verification_state.needs_verify_normal && !verification_state.needs_verify_safe) {
        return ResultSuccess;
    }

    /* Get a session to ncm. */
    DoWithSmSession([&]() {
        if (R_FAILED(ncmInitialize())) {
            std::abort();
        }
    });
    ON_SCOPE_EXIT { ncmExit(); };

    /* Verify normal, verify safe as needed. */
    if (verification_state.needs_verify_normal) {
        rc = VerifyBootImagesAndRepairIfNeeded(out_repaired_normal, BootModeType_Normal, work_buffer, work_buffer_size, boot_image_update_type);
        if (rc == ResultUpdaterBootImagePackageNotFound) {
            /* Nintendo considers failure to locate bip a success. TODO: don't do that? */
            rc = ResultSuccess;
        }
        if (R_FAILED(rc)) {
            return rc;
        }
    }
    if (verification_state.needs_verify_safe) {
        rc = VerifyBootImagesAndRepairIfNeeded(out_repaired_safe, BootModeType_Safe, work_buffer, work_buffer_size, boot_image_update_type);
        if (rc == ResultUpdaterBootImagePackageNotFound) {
            /* Nintendo considers failure to locate bip a success. TODO: don't do that? */
            rc = ResultSuccess;
        }
        if (R_FAILED(rc)) {
            return rc;
        }
    }

    return ResultSuccess;
}

Result Updater::VerifyBootImagesAndRepairIfNeeded(bool *out_repaired, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    Result rc;

    /* Get system data id for boot images (819/81A/81B/81C). */
    u64 bip_data_id;
    if (R_FAILED((rc = GetBootImagePackageDataId(&bip_data_id, mode, work_buffer, work_buffer_size)))) {
        return rc;
    }

    /* Verify the boot images in NAND. */
    if (R_FAILED((rc = VerifyBootImages(bip_data_id, mode, work_buffer, work_buffer_size, boot_image_update_type)))) {
        /* If we failed for a reason other than repair needed, bail out. */
        if (rc != ResultUpdaterNeedsRepairBootImages) {
            return rc;
        }
        /* Perform repair. */
        *out_repaired = true;
        if (R_FAILED((rc = UpdateBootImages(bip_data_id, mode, work_buffer, work_buffer_size, boot_image_update_type)))) {
            return rc;
        }
    }

    /* We've either just verified or just repaired. Either way, we don't need to verify any more. */
    return SetVerificationNeeded(mode, false, work_buffer, work_buffer_size);
}

Result Updater::GetBootImagePackageDataId(u64 *out_data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size) {
    Result rc;

    /* Ensure we can read content metas. */
    constexpr size_t MaxContentMetas = 0x40;
    if (work_buffer_size < sizeof(NcmMetaRecord) * MaxContentMetas) {
        std::abort();
    }

    /* Open NAND System meta database, list contents. */
    NcmContentMetaDatabase meta_db;
    if (R_FAILED((rc = ncmOpenContentMetaDatabase(FsStorageId_NandSystem, &meta_db)))) {
        return rc;
    }
    ON_SCOPE_EXIT { serviceClose(&meta_db.s); };

    NcmMetaRecord *records = reinterpret_cast<NcmMetaRecord *>(work_buffer);

    const u32 title_type = GetNcmTitleType(mode);
    u32 written_entries;
    u32 total_entries;
    if (R_FAILED((rc = ncmContentMetaDatabaseList(&meta_db, title_type, 0, 0, UINT64_MAX, records, MaxContentMetas * sizeof(*records), &written_entries, &total_entries)))) {
        return rc;
    }
    if (total_entries == 0) {
        return ResultUpdaterBootImagePackageNotFound;
    }

    if (total_entries != written_entries) {
        std::abort();
    }

    /* Output is sorted, return the lowest valid exfat entry. */
    if (total_entries > 1) {
        for (size_t i = 0; i < total_entries; i++) {
            u8 attr;
            if (R_FAILED((rc = ncmContentMetaDatabaseGetAttributes(&meta_db, &records[i], &attr)))) {
                return rc;
            }

            if (attr & NcmContentMetaAttribute_Exfat) {
                *out_data_id = records[i].titleId;
                return ResultSuccess;
            }
        }
    }

    /* If there's only one entry or no exfat entries, return that entry. */
    *out_data_id = records[0].titleId;
    return ResultSuccess;
}

Result Updater::VerifyBootImages(u64 data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    switch (mode) {
        case BootModeType_Normal:
            return VerifyBootImagesNormal(data_id, work_buffer, work_buffer_size, boot_image_update_type);
        case BootModeType_Safe:
            return VerifyBootImagesSafe(data_id, work_buffer, work_buffer_size, boot_image_update_type);
        default:
            std::abort();
    }
}

Result Updater::ValidateBctFileHash(Boot0Accessor &accessor, Boot0Partition which, const void *stored_hash, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    Result rc;

    /* Ensure work buffer is big enough for us to do what we want to do. */
    if (R_FAILED((rc = ValidateWorkBuffer(work_buffer, work_buffer_size)))) {
        return rc;
    }

    void *bct = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + 0);
    void *work = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + BctSize);

    size_t size;
    if (R_FAILED((rc = ReadFile(&size, bct, BctSize, GetBctPath(boot_image_update_type))))) {
        return rc;
    }
    if (HasEks(boot_image_update_type)) {
        if (R_FAILED((rc = accessor.UpdateEks(bct, work)))) {
            return rc;
        }
    }
    if (HasAutoRcmPreserve(boot_image_update_type)) {
        if (R_FAILED((rc = accessor.PreserveAutoRcm(bct, work, which)))) {
            return rc;
        }
    }

    u8 file_hash[SHA256_HASH_SIZE];
    sha256CalculateHash(file_hash, bct, BctSize);

    if (std::memcmp(file_hash, stored_hash, SHA256_HASH_SIZE) == 0) {
        return ResultSuccess;
    }

    return ResultUpdaterNeedsRepairBootImages;
}

Result Updater::VerifyBootImagesNormal(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    Result rc;

    /* Ensure work buffer is big enough for us to do what we want to do. */
    if (R_FAILED((rc = ValidateWorkBuffer(work_buffer, work_buffer_size)))) {
        return rc;
    }

    if (R_FAILED((rc = romfsMountFromDataArchive(data_id, FsStorageId_NandSystem, GetBootImagePackageMountPath())))) {
        if (rc == ResultFsTargetNotFound) {
            return ResultUpdaterBootImagePackageNotFound;
        }
        return rc;
    }
    ON_SCOPE_EXIT { if (R_FAILED(romfsUnmount(GetBootImagePackageMountPath()))) { std::abort(); } };

    /* Read and validate hashes of boot images. */
    {
        size_t size;
        u8 nand_hash[SHA256_HASH_SIZE];
        u8 file_hash[SHA256_HASH_SIZE];

        Boot0Accessor boot0_accessor;
        if (R_FAILED((rc = boot0_accessor.Initialize()))) {
            return rc;
        }
        ON_SCOPE_EXIT { boot0_accessor.Finalize(); };

        /* Compare BCT hashes. */
        if (R_FAILED((rc = boot0_accessor.GetHash(nand_hash, BctSize, work_buffer, work_buffer_size, Boot0Partition::BctNormalMain)))) {
            return rc;
        }
        if (R_FAILED((rc = ValidateBctFileHash(boot0_accessor, Boot0Partition::BctNormalMain, nand_hash, work_buffer, work_buffer_size, boot_image_update_type)))) {
            return rc;
        }

        /* Compare BCT Sub hashes. */
        if (R_FAILED((rc = boot0_accessor.GetHash(nand_hash, BctSize, work_buffer, work_buffer_size, Boot0Partition::BctNormalSub)))) {
            return rc;
        }
        if (R_FAILED((rc = ValidateBctFileHash(boot0_accessor, Boot0Partition::BctNormalSub, nand_hash, work_buffer, work_buffer_size, boot_image_update_type)))) {
            return rc;
        }

        /* Compare Package1 Normal/Sub hashes. */
        if (R_FAILED((rc = GetFileHash(&size, file_hash, GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size)))) {
            return rc;
        }
        if (R_FAILED((rc = boot0_accessor.GetHash(nand_hash, size, work_buffer, work_buffer_size, Boot0Partition::Package1NormalMain)))) {
            return rc;
        }
        if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
            return ResultUpdaterNeedsRepairBootImages;
        }
        if (R_FAILED((rc = boot0_accessor.GetHash(nand_hash, size, work_buffer, work_buffer_size, Boot0Partition::Package1NormalSub)))) {
            return rc;
        }
        if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
            return ResultUpdaterNeedsRepairBootImages;
        }

        /* Compare Package2 Normal/Sub hashes. */
        if (R_FAILED((rc = GetFileHash(&size, file_hash, GetPackage2Path(boot_image_update_type), work_buffer, work_buffer_size)))) {
            return rc;
        }
        if (R_FAILED((rc = GetPackage2Hash(nand_hash, size, work_buffer, work_buffer_size, Package2Type::NormalMain)))) {
            return rc;
        }
        if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
            return ResultUpdaterNeedsRepairBootImages;
        }
        if (R_FAILED((rc = GetPackage2Hash(nand_hash, size, work_buffer, work_buffer_size, Package2Type::NormalSub)))) {
            return rc;
        }
        if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
            return ResultUpdaterNeedsRepairBootImages;
        }
    }

    return ResultSuccess;
}

Result Updater::VerifyBootImagesSafe(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    Result rc;

    /* Ensure work buffer is big enough for us to do what we want to do. */
    if (R_FAILED((rc = ValidateWorkBuffer(work_buffer, work_buffer_size)))) {
        return rc;
    }

    if (R_FAILED((rc = romfsMountFromDataArchive(data_id, FsStorageId_NandSystem, GetBootImagePackageMountPath())))) {
        if (rc == ResultFsTargetNotFound) {
            return ResultUpdaterBootImagePackageNotFound;
        }
        return rc;
    }
    ON_SCOPE_EXIT { if (R_FAILED(romfsUnmount(GetBootImagePackageMountPath()))) { std::abort(); } };

    /* Read and validate hashes of boot images. */
    {
        size_t size;
        u8 nand_hash[SHA256_HASH_SIZE];
        u8 file_hash[SHA256_HASH_SIZE];

        Boot0Accessor boot0_accessor;
        if (R_FAILED((rc = boot0_accessor.Initialize()))) {
            return rc;
        }
        ON_SCOPE_EXIT { boot0_accessor.Finalize(); };

        Boot1Accessor boot1_accessor;
        if (R_FAILED((rc = boot1_accessor.Initialize()))) {
            return rc;
        }
        ON_SCOPE_EXIT { boot1_accessor.Finalize(); };


        /* Compare BCT hashes. */
        if (R_FAILED((rc = boot0_accessor.GetHash(nand_hash, BctSize, work_buffer, work_buffer_size, Boot0Partition::BctSafeMain)))) {
            return rc;
        }
        if (R_FAILED((rc = ValidateBctFileHash(boot0_accessor, Boot0Partition::BctSafeMain, nand_hash, work_buffer, work_buffer_size, boot_image_update_type)))) {
            return rc;
        }

        /* Compare BCT Sub hashes. */
        if (R_FAILED((rc = boot0_accessor.GetHash(nand_hash, BctSize, work_buffer, work_buffer_size, Boot0Partition::BctSafeSub)))) {
            return rc;
        }
        if (R_FAILED((rc = ValidateBctFileHash(boot0_accessor, Boot0Partition::BctSafeSub, nand_hash, work_buffer, work_buffer_size, boot_image_update_type)))) {
            return rc;
        }

        /* Compare Package1 Normal/Sub hashes. */
        if (R_FAILED((rc = GetFileHash(&size, file_hash, GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size)))) {
            return rc;
        }
        if (R_FAILED((rc = boot1_accessor.GetHash(nand_hash, size, work_buffer, work_buffer_size, Boot1Partition::Package1SafeMain)))) {
            return rc;
        }
        if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
            return ResultUpdaterNeedsRepairBootImages;
        }
        if (R_FAILED((rc = boot1_accessor.GetHash(nand_hash, size, work_buffer, work_buffer_size, Boot1Partition::Package1SafeSub)))) {
            return rc;
        }
        if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
            return ResultUpdaterNeedsRepairBootImages;
        }

        /* Compare Package2 Normal/Sub hashes. */
        if (R_FAILED((rc = GetFileHash(&size, file_hash, GetPackage2Path(boot_image_update_type), work_buffer, work_buffer_size)))) {
            return rc;
        }
        if (R_FAILED((rc = GetPackage2Hash(nand_hash, size, work_buffer, work_buffer_size, Package2Type::SafeMain)))) {
            return rc;
        }
        if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
            return ResultUpdaterNeedsRepairBootImages;
        }
        if (R_FAILED((rc = GetPackage2Hash(nand_hash, size, work_buffer, work_buffer_size, Package2Type::SafeSub)))) {
            return rc;
        }
        if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
            return ResultUpdaterNeedsRepairBootImages;
        }
    }

    return ResultSuccess;
}

Result Updater::UpdateBootImages(u64 data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    switch (mode) {
        case BootModeType_Normal:
            return UpdateBootImagesNormal(data_id, work_buffer, work_buffer_size, boot_image_update_type);
        case BootModeType_Safe:
            return UpdateBootImagesSafe(data_id, work_buffer, work_buffer_size, boot_image_update_type);
        default:
            std::abort();
    }
}

Result Updater::UpdateBootImagesNormal(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    Result rc;

    /* Ensure work buffer is big enough for us to do what we want to do. */
    if (R_FAILED((rc = ValidateWorkBuffer(work_buffer, work_buffer_size)))) {
        return rc;
    }

    if (R_FAILED((rc = romfsMountFromDataArchive(data_id, FsStorageId_NandSystem, GetBootImagePackageMountPath())))) {
        if (rc == ResultFsTargetNotFound) {
            return ResultUpdaterBootImagePackageNotFound;
        }
        return rc;
    }
    ON_SCOPE_EXIT { if (R_FAILED(romfsUnmount(GetBootImagePackageMountPath()))) { std::abort(); } };

    {
        Boot0Accessor boot0_accessor;
        if (R_FAILED((rc = boot0_accessor.Initialize()))) {
            return rc;
        }
        ON_SCOPE_EXIT { boot0_accessor.Finalize(); };

        /* Write Package1 sub. */
        if (R_FAILED((rc = boot0_accessor.Clear(work_buffer, work_buffer_size, Boot0Partition::Package1NormalSub)))) {
            return rc;
        }
        if (R_FAILED((rc = boot0_accessor.Write(GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size, Boot0Partition::Package1NormalSub)))) {
            return rc;
        }

        /* Write Package2 sub. */
        if (R_FAILED((rc = WritePackage2(work_buffer, work_buffer_size, Package2Type::NormalSub, boot_image_update_type)))) {
            return rc;
        }

        /* Write BCT sub + BCT main, in that order. */
        {
            void *bct = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + 0);
            void *work = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + BctSize);

            size_t size;
            if (R_FAILED((rc = ReadFile(&size, bct, BctSize, GetBctPath(boot_image_update_type))))) {
                return rc;
            }
            if (HasEks(boot_image_update_type)) {
                if (R_FAILED((rc = boot0_accessor.UpdateEks(bct, work)))) {
                    return rc;
                }
            }

            /* Only preserve autorcm if on a unit with unpatched rcm bug. */
            if (HasAutoRcmPreserve(boot_image_update_type) && !IsRcmBugPatched()) {
                if (R_FAILED((rc = boot0_accessor.PreserveAutoRcm(bct, work, Boot0Partition::BctNormalSub)))) {
                    return rc;
                }
                if (R_FAILED((rc = boot0_accessor.Write(bct, BctSize, Boot0Partition::BctNormalSub)))) {
                    return rc;
                }
                if (R_FAILED((rc = boot0_accessor.PreserveAutoRcm(bct, work, Boot0Partition::BctNormalMain)))) {
                    return rc;
                }
                if (R_FAILED((rc = boot0_accessor.Write(bct, BctSize, Boot0Partition::BctNormalMain)))) {
                    return rc;
                }
            } else {
                if (R_FAILED((rc = boot0_accessor.Write(bct, BctSize, Boot0Partition::BctNormalSub)))) {
                    return rc;
                }
                if (R_FAILED((rc = boot0_accessor.Write(bct, BctSize, Boot0Partition::BctNormalMain)))) {
                    return rc;
                }
            }
        }

        /* Write Package2 main. */
        if (R_FAILED((rc = WritePackage2(work_buffer, work_buffer_size, Package2Type::NormalMain, boot_image_update_type)))) {
            return rc;
        }

        /* Write Package1 main. */
        if (R_FAILED((rc = boot0_accessor.Clear(work_buffer, work_buffer_size, Boot0Partition::Package1NormalMain)))) {
            return rc;
        }
        if (R_FAILED((rc = boot0_accessor.Write(GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size, Boot0Partition::Package1NormalMain)))) {
            return rc;
        }
    }

    return ResultSuccess;
}

Result Updater::UpdateBootImagesSafe(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
    Result rc;

    /* Ensure work buffer is big enough for us to do what we want to do. */
    if (R_FAILED((rc = ValidateWorkBuffer(work_buffer, work_buffer_size)))) {
        return rc;
    }

    if (R_FAILED((rc = romfsMountFromDataArchive(data_id, FsStorageId_NandSystem, GetBootImagePackageMountPath())))) {
        if (rc == ResultFsTargetNotFound) {
            return ResultUpdaterBootImagePackageNotFound;
        }
        return rc;
    }
    ON_SCOPE_EXIT { if (R_FAILED(romfsUnmount(GetBootImagePackageMountPath()))) { std::abort(); } };

    {
        Boot0Accessor boot0_accessor;
        if (R_FAILED((rc = boot0_accessor.Initialize()))) {
            return rc;
        }
        ON_SCOPE_EXIT { boot0_accessor.Finalize(); };

        Boot1Accessor boot1_accessor;
        if (R_FAILED((rc = boot1_accessor.Initialize()))) {
            return rc;
        }
        ON_SCOPE_EXIT { boot1_accessor.Finalize(); };


        /* Write Package1 sub. */
        if (R_FAILED((rc = boot1_accessor.Clear(work_buffer, work_buffer_size, Boot1Partition::Package1SafeSub)))) {
            return rc;
        }
        if (R_FAILED((rc = boot1_accessor.Write(GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size, Boot1Partition::Package1SafeSub)))) {
            return rc;
        }

        /* Write Package2 sub. */
        if (R_FAILED((rc = WritePackage2(work_buffer, work_buffer_size, Package2Type::SafeSub, boot_image_update_type)))) {
            return rc;
        }

        /* Write BCT sub + BCT main, in that order. */
        {
            void *bct = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + 0);
            void *work = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + BctSize);

            size_t size;
            if (R_FAILED((rc = ReadFile(&size, bct, BctSize, GetBctPath(boot_image_update_type))))) {
                return rc;
            }
            if (HasEks(boot_image_update_type)) {
                if (R_FAILED((rc = boot0_accessor.UpdateEks(bct, work)))) {
                    return rc;
                }
            }
            /* Only preserve autorcm if on a unit with unpatched rcm bug. */
            if (HasAutoRcmPreserve(boot_image_update_type) && !IsRcmBugPatched()) {
                if (R_FAILED((rc = boot0_accessor.PreserveAutoRcm(bct, work, Boot0Partition::BctSafeSub)))) {
                    return rc;
                }
                if (R_FAILED((rc = boot0_accessor.Write(bct, BctSize, Boot0Partition::BctSafeSub)))) {
                    return rc;
                }
                if (R_FAILED((rc = boot0_accessor.PreserveAutoRcm(bct, work, Boot0Partition::BctSafeMain)))) {
                    return rc;
                }
                if (R_FAILED((rc = boot0_accessor.Write(bct, BctSize, Boot0Partition::BctSafeMain)))) {
                    return rc;
                }
            } else {
                if (R_FAILED((rc = boot0_accessor.Write(bct, BctSize, Boot0Partition::BctSafeSub)))) {
                    return rc;
                }
                if (R_FAILED((rc = boot0_accessor.Write(bct, BctSize, Boot0Partition::BctSafeMain)))) {
                    return rc;
                }
            }
        }

        /* Write Package2 main. */
        if (R_FAILED((rc = WritePackage2(work_buffer, work_buffer_size, Package2Type::SafeMain, boot_image_update_type)))) {
            return rc;
        }

        /* Write Package1 main. */
        if (R_FAILED((rc = boot1_accessor.Clear(work_buffer, work_buffer_size, Boot1Partition::Package1SafeMain)))) {
            return rc;
        }
        if (R_FAILED((rc = boot1_accessor.Write(GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size, Boot1Partition::Package1SafeMain)))) {
            return rc;
        }
    }

    return ResultSuccess;
}

Result Updater::SetVerificationNeeded(BootModeType mode, bool needed, void *work_buffer, size_t work_buffer_size) {
    Result rc;

    /* Ensure work buffer is big enough for us to do what we want to do. */
    if (R_FAILED((rc = ValidateWorkBuffer(work_buffer, work_buffer_size)))) {
        return rc;
    }

    /* Initialize boot0 save accessor. */
    BisSave save;
    if (R_FAILED((rc = save.Initialize(work_buffer, work_buffer_size)))) {
        return rc;
    }
    ON_SCOPE_EXIT { save.Finalize(); };

    /* Load save from NAND. */
    if (R_FAILED((rc = save.Load()))) {
        return rc;
    }

    /* Set whether we need to verify, then save to nand. */
    save.SetNeedsVerification(mode, needed);
    if (R_FAILED((rc = save.Save()))) {
        return rc;
    }

    return ResultSuccess;
}

Result Updater::GetPackage2Hash(void *dst_hash, size_t package2_size, void *work_buffer, size_t work_buffer_size, Package2Type which) {
    Result rc;

    Package2Accessor accessor(which);
    if (R_FAILED((rc = accessor.Initialize()))) {
        return rc;
    }
    ON_SCOPE_EXIT { accessor.Finalize(); };

    return accessor.GetHash(dst_hash, package2_size, work_buffer, work_buffer_size, Package2Partition::Package2);
}

Result Updater::WritePackage2(void *work_buffer, size_t work_buffer_size, Package2Type which, BootImageUpdateType boot_image_update_type) {
    Result rc;

    Package2Accessor accessor(which);
    if (R_FAILED((rc = accessor.Initialize()))) {
        return rc;
    }
    ON_SCOPE_EXIT { accessor.Finalize(); };

    return accessor.Write(GetPackage2Path(boot_image_update_type), work_buffer, work_buffer_size, Package2Partition::Package2);
}
