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
#include <stratosphere/updater.hpp>

#include "updater_bis_save.hpp"
#include "updater_files.hpp"
#include "updater_paths.hpp"

namespace ams::updater {

    namespace {

        /* Validation Prototypes. */
        Result ValidateWorkBuffer(const void *work_buffer, size_t work_buffer_size);

        /* Configuration Prototypes. */
        bool HasEks(BootImageUpdateType boot_image_update_type);
        bool HasAutoRcmPreserve(BootImageUpdateType boot_image_update_type);
        NcmContentMetaType GetNcmContentMetaType(BootModeType mode);
        Result GetBootImagePackageDataId(u64 *out_data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size);

        /* Verification Prototypes. */
        Result GetVerificationState(VerificationState *out, void *work_buffer, size_t work_buffer_size);
        Result VerifyBootImages(u64 data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        Result VerifyBootImagesNormal(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        Result VerifyBootImagesSafe(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);

        /* Update Prototypes. */
        Result SetVerificationNeeded(BootModeType mode, bool needed, void *work_buffer, size_t work_buffer_size);
        Result UpdateBootImages(u64 data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        Result UpdateBootImagesNormal(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        Result UpdateBootImagesSafe(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);

        /* Package helpers. */
        Result ValidateBctFileHash(Boot0Accessor &accessor, Boot0Partition which, const void *stored_hash, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type);
        Result GetPackage2Hash(void *dst_hash, size_t package2_size, void *work_buffer, size_t work_buffer_size, Package2Type which);
        Result WritePackage2(void *work_buffer, size_t work_buffer_size, Package2Type which, BootImageUpdateType boot_image_update_type);

        /* Implementations. */
        Result ValidateWorkBuffer(const void *work_buffer, size_t work_buffer_size) {
            R_UNLESS(work_buffer_size >= BctSize + EksSize,            ResultTooSmallWorkBuffer());
            R_UNLESS(util::IsAligned(work_buffer, os::MemoryPageSize), ResultNotAlignedWorkBuffer());
            R_UNLESS(util::IsAligned(work_buffer_size, 0x200),         ResultNotAlignedWorkBuffer());
            return ResultSuccess();
        }

        bool HasEks(BootImageUpdateType boot_image_update_type) {
            switch (boot_image_update_type) {
                case BootImageUpdateType::Erista:
                    return true;
                case BootImageUpdateType::Mariko:
                    return false;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        bool HasAutoRcmPreserve(BootImageUpdateType boot_image_update_type) {
            switch (boot_image_update_type) {
                case BootImageUpdateType::Erista:
                    return true;
                case BootImageUpdateType::Mariko:
                    return false;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        NcmContentMetaType GetNcmContentMetaType(BootModeType mode) {
            switch (mode) {
                case BootModeType::Normal:
                    return NcmContentMetaType_BootImagePackage;
                case BootModeType::Safe:
                    return NcmContentMetaType_BootImagePackageSafe;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        Result GetVerificationState(VerificationState *out, void *work_buffer, size_t work_buffer_size) {
            /* Always set output to true before doing anything else. */
            out->needs_verify_normal = true;
            out->needs_verify_safe = true;

            /* Ensure work buffer is big enough for us to do what we want to do. */
            R_TRY(ValidateWorkBuffer(work_buffer, work_buffer_size));

            /* Initialize boot0 save accessor. */
            BisSave save;
            R_TRY(save.Initialize(work_buffer, work_buffer_size));
            ON_SCOPE_EXIT { save.Finalize(); };

            /* Load save from NAND. */
            R_TRY(save.Load());

            /* Read data from save. */
            out->needs_verify_normal = save.GetNeedsVerification(BootModeType::Normal);
            out->needs_verify_safe = save.GetNeedsVerification(BootModeType::Safe);
            return ResultSuccess();
        }

        Result VerifyBootImagesAndRepairIfNeeded(bool *out_repaired, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
            /* Get system data id for boot images (819/81A/81B/81C). */
            u64 bip_data_id = 0;
            R_TRY(GetBootImagePackageDataId(&bip_data_id, mode, work_buffer, work_buffer_size));

            /* Verify the boot images in NAND. */
            R_TRY_CATCH(VerifyBootImages(bip_data_id, mode, work_buffer, work_buffer_size, boot_image_update_type)) {
                R_CATCH(ResultNeedsRepairBootImages) {
                    /* Perform repair. */
                    *out_repaired = true;
                    R_TRY(UpdateBootImages(bip_data_id, mode, work_buffer, work_buffer_size, boot_image_update_type));
                }
            } R_END_TRY_CATCH;

            /* We've either just verified or just repaired. Either way, we don't need to verify any more. */
            return SetVerificationNeeded(mode, false, work_buffer, work_buffer_size);
        }

        Result GetBootImagePackageDataId(u64 *out_data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size) {
            /* Ensure we can read content metas. */
            constexpr size_t MaxContentMetas = 0x40;
            AMS_ASSERT(work_buffer_size >= sizeof(NcmContentMetaKey) * MaxContentMetas);

            /* Open NAND System meta database, list contents. */
            NcmContentMetaDatabase meta_db;
            R_TRY(ncmOpenContentMetaDatabase(&meta_db, NcmStorageId_BuiltInSystem));
            ON_SCOPE_EXIT { serviceClose(&meta_db.s); };

            NcmContentMetaKey *records = reinterpret_cast<NcmContentMetaKey *>(work_buffer);

            const auto content_meta_type = GetNcmContentMetaType(mode);
            s32 written_entries;
            s32 total_entries;
            R_TRY(ncmContentMetaDatabaseList(&meta_db, &total_entries, &written_entries, records, MaxContentMetas * sizeof(*records), content_meta_type, 0, 0, UINT64_MAX, NcmContentInstallType_Full));
            if (total_entries <= 0) {
                return ResultBootImagePackageNotFound();
            }

            AMS_ASSERT(total_entries == written_entries);

            /* Output is sorted, return the lowest valid exfat entry. */
            if (total_entries > 1) {
                for (size_t i = 0; i < size_t(total_entries); i++) {
                    u8 attr;
                    R_TRY(ncmContentMetaDatabaseGetAttributes(&meta_db, &records[i], &attr));

                    if (attr & NcmContentMetaAttribute_IncludesExFatDriver) {
                        *out_data_id = records[i].id;
                        return ResultSuccess();
                    }
                }
            }

            /* If there's only one entry or no exfat entries, return that entry. */
            *out_data_id = records[0].id;
            return ResultSuccess();
        }

        Result VerifyBootImages(u64 data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
            switch (mode) {
                case BootModeType::Normal:
                    return VerifyBootImagesNormal(data_id, work_buffer, work_buffer_size, boot_image_update_type);
                case BootModeType::Safe:
                    return VerifyBootImagesSafe(data_id, work_buffer, work_buffer_size, boot_image_update_type);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        Result VerifyBootImagesNormal(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
            /* Ensure work buffer is big enough for us to do what we want to do. */
            R_TRY(ValidateWorkBuffer(work_buffer, work_buffer_size));

            R_TRY_CATCH(romfsMountFromDataArchive(data_id, NcmStorageId_BuiltInSystem, GetBootImagePackageMountPath())) {
                R_CONVERT(fs::ResultTargetNotFound, ResultBootImagePackageNotFound())
            } R_END_TRY_CATCH;
            ON_SCOPE_EXIT { R_ASSERT(romfsUnmount(GetBootImagePackageMountPath())); };

            /* Read and validate hashes of boot images. */
            {
                size_t size;
                u8 nand_hash[SHA256_HASH_SIZE];
                u8 file_hash[SHA256_HASH_SIZE];

                Boot0Accessor boot0_accessor;
                R_TRY(boot0_accessor.Initialize());
                ON_SCOPE_EXIT { boot0_accessor.Finalize(); };

                /* Compare BCT hashes. */
                R_TRY(boot0_accessor.GetHash(nand_hash, BctSize, work_buffer, work_buffer_size, Boot0Partition::BctNormalMain));
                R_TRY(ValidateBctFileHash(boot0_accessor, Boot0Partition::BctNormalMain, nand_hash, work_buffer, work_buffer_size, boot_image_update_type));

                /* Compare BCT Sub hashes. */
                R_TRY(boot0_accessor.GetHash(nand_hash, BctSize, work_buffer, work_buffer_size, Boot0Partition::BctNormalSub));
                R_TRY(ValidateBctFileHash(boot0_accessor, Boot0Partition::BctNormalSub, nand_hash, work_buffer, work_buffer_size, boot_image_update_type));

                /* Compare Package1 Normal/Sub hashes. */
                R_TRY(GetFileHash(&size, file_hash, GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size));
                R_TRY(boot0_accessor.GetHash(nand_hash, size, work_buffer, work_buffer_size, Boot0Partition::Package1NormalMain));
                if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
                    return ResultNeedsRepairBootImages();
                }
                R_TRY(boot0_accessor.GetHash(nand_hash, size, work_buffer, work_buffer_size, Boot0Partition::Package1NormalSub));
                if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
                    return ResultNeedsRepairBootImages();
                }

                /* Compare Package2 Normal/Sub hashes. */
                R_TRY(GetFileHash(&size, file_hash, GetPackage2Path(boot_image_update_type), work_buffer, work_buffer_size));
                R_TRY(GetPackage2Hash(nand_hash, size, work_buffer, work_buffer_size, Package2Type::NormalMain));
                if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
                    return ResultNeedsRepairBootImages();
                }
                R_TRY(GetPackage2Hash(nand_hash, size, work_buffer, work_buffer_size, Package2Type::NormalSub));
                if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
                    return ResultNeedsRepairBootImages();
                }
            }

            return ResultSuccess();
        }

        Result VerifyBootImagesSafe(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
            /* Ensure work buffer is big enough for us to do what we want to do. */
            R_TRY(ValidateWorkBuffer(work_buffer, work_buffer_size));

            R_TRY_CATCH(romfsMountFromDataArchive(data_id, NcmStorageId_BuiltInSystem, GetBootImagePackageMountPath())) {
                R_CONVERT(fs::ResultTargetNotFound, ResultBootImagePackageNotFound())
            } R_END_TRY_CATCH;
            ON_SCOPE_EXIT { R_ASSERT(romfsUnmount(GetBootImagePackageMountPath())); };

            /* Read and validate hashes of boot images. */
            {
                size_t size;
                u8 nand_hash[SHA256_HASH_SIZE];
                u8 file_hash[SHA256_HASH_SIZE];

                Boot0Accessor boot0_accessor;
                R_TRY(boot0_accessor.Initialize());
                ON_SCOPE_EXIT { boot0_accessor.Finalize(); };

                Boot1Accessor boot1_accessor;
                R_TRY(boot1_accessor.Initialize());
                ON_SCOPE_EXIT { boot1_accessor.Finalize(); };


                /* Compare BCT hashes. */
                R_TRY(boot0_accessor.GetHash(nand_hash, BctSize, work_buffer, work_buffer_size, Boot0Partition::BctSafeMain));
                R_TRY(ValidateBctFileHash(boot0_accessor, Boot0Partition::BctSafeMain, nand_hash, work_buffer, work_buffer_size, boot_image_update_type));

                /* Compare BCT Sub hashes. */
                R_TRY(boot0_accessor.GetHash(nand_hash, BctSize, work_buffer, work_buffer_size, Boot0Partition::BctSafeSub));
                R_TRY(ValidateBctFileHash(boot0_accessor, Boot0Partition::BctSafeSub, nand_hash, work_buffer, work_buffer_size, boot_image_update_type));

                /* Compare Package1 Normal/Sub hashes. */
                R_TRY(GetFileHash(&size, file_hash, GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size));
                R_TRY(boot1_accessor.GetHash(nand_hash, size, work_buffer, work_buffer_size, Boot1Partition::Package1SafeMain));
                if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
                    return ResultNeedsRepairBootImages();
                }
                R_TRY(boot1_accessor.GetHash(nand_hash, size, work_buffer, work_buffer_size, Boot1Partition::Package1SafeSub));
                if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
                    return ResultNeedsRepairBootImages();
                }

                /* Compare Package2 Normal/Sub hashes. */
                R_TRY(GetFileHash(&size, file_hash, GetPackage2Path(boot_image_update_type), work_buffer, work_buffer_size));
                R_TRY(GetPackage2Hash(nand_hash, size, work_buffer, work_buffer_size, Package2Type::SafeMain));
                if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
                    return ResultNeedsRepairBootImages();
                }
                R_TRY(GetPackage2Hash(nand_hash, size, work_buffer, work_buffer_size, Package2Type::SafeSub));
                if (std::memcmp(file_hash, nand_hash, SHA256_HASH_SIZE) != 0) {
                    return ResultNeedsRepairBootImages();
                }
            }

            return ResultSuccess();
        }

        Result UpdateBootImages(u64 data_id, BootModeType mode, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
            switch (mode) {
                case BootModeType::Normal:
                    return UpdateBootImagesNormal(data_id, work_buffer, work_buffer_size, boot_image_update_type);
                case BootModeType::Safe:
                    return UpdateBootImagesSafe(data_id, work_buffer, work_buffer_size, boot_image_update_type);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        Result UpdateBootImagesNormal(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
            /* Ensure work buffer is big enough for us to do what we want to do. */
            R_TRY(ValidateWorkBuffer(work_buffer, work_buffer_size));

            R_TRY_CATCH(romfsMountFromDataArchive(data_id, NcmStorageId_BuiltInSystem, GetBootImagePackageMountPath())) {
                R_CONVERT(fs::ResultTargetNotFound, ResultBootImagePackageNotFound())
            } R_END_TRY_CATCH;
            ON_SCOPE_EXIT { R_ASSERT(romfsUnmount(GetBootImagePackageMountPath())); };

            {
                Boot0Accessor boot0_accessor;
                R_TRY(boot0_accessor.Initialize());
                ON_SCOPE_EXIT { boot0_accessor.Finalize(); };

                /* Write Package1 sub. */
                R_TRY(boot0_accessor.Clear(work_buffer, work_buffer_size, Boot0Partition::Package1NormalSub));
                R_TRY(boot0_accessor.Write(GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size, Boot0Partition::Package1NormalSub));

                /* Write Package2 sub. */
                R_TRY(WritePackage2(work_buffer, work_buffer_size, Package2Type::NormalSub, boot_image_update_type));

                /* Write BCT sub + BCT main, in that order. */
                {
                    void *bct = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + 0);
                    void *work = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + BctSize);

                    size_t size;
                    R_TRY(ReadFile(&size, bct, BctSize, GetBctPath(boot_image_update_type)));
                    if (HasEks(boot_image_update_type)) {
                        R_TRY(boot0_accessor.UpdateEks(bct, work));
                    }

                    /* Only preserve autorcm if on a unit with unpatched rcm bug. */
                    if (HasAutoRcmPreserve(boot_image_update_type) && !exosphere::IsRcmBugPatched()) {
                        R_TRY(boot0_accessor.PreserveAutoRcm(bct, work, Boot0Partition::BctNormalSub));
                        R_TRY(boot0_accessor.Write(bct, BctSize, Boot0Partition::BctNormalSub));
                        R_TRY(boot0_accessor.PreserveAutoRcm(bct, work, Boot0Partition::BctNormalMain));
                        R_TRY(boot0_accessor.Write(bct, BctSize, Boot0Partition::BctNormalMain));
                    } else {
                        R_TRY(boot0_accessor.Write(bct, BctSize, Boot0Partition::BctNormalSub));
                        R_TRY(boot0_accessor.Write(bct, BctSize, Boot0Partition::BctNormalMain));
                    }
                }

                /* Write Package2 main. */
                R_TRY(WritePackage2(work_buffer, work_buffer_size, Package2Type::NormalMain, boot_image_update_type));

                /* Write Package1 main. */
                R_TRY(boot0_accessor.Clear(work_buffer, work_buffer_size, Boot0Partition::Package1NormalMain));
                R_TRY(boot0_accessor.Write(GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size, Boot0Partition::Package1NormalMain));
            }

            return ResultSuccess();
        }

        Result UpdateBootImagesSafe(u64 data_id, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
            /* Ensure work buffer is big enough for us to do what we want to do. */
            R_TRY(ValidateWorkBuffer(work_buffer, work_buffer_size));

            R_TRY_CATCH(romfsMountFromDataArchive(data_id, NcmStorageId_BuiltInSystem, GetBootImagePackageMountPath())) {
                R_CONVERT(fs::ResultTargetNotFound, ResultBootImagePackageNotFound())
            } R_END_TRY_CATCH;
            ON_SCOPE_EXIT { R_ASSERT(romfsUnmount(GetBootImagePackageMountPath())); };

            {
                Boot0Accessor boot0_accessor;
                R_TRY(boot0_accessor.Initialize());
                ON_SCOPE_EXIT { boot0_accessor.Finalize(); };

                Boot1Accessor boot1_accessor;
                R_TRY(boot1_accessor.Initialize());
                ON_SCOPE_EXIT { boot1_accessor.Finalize(); };


                /* Write Package1 sub. */
                R_TRY(boot1_accessor.Clear(work_buffer, work_buffer_size, Boot1Partition::Package1SafeSub));
                R_TRY(boot1_accessor.Write(GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size, Boot1Partition::Package1SafeSub));

                /* Write Package2 sub. */
                R_TRY(WritePackage2(work_buffer, work_buffer_size, Package2Type::SafeSub, boot_image_update_type));

                /* Write BCT sub + BCT main, in that order. */
                {
                    void *bct = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + 0);
                    void *work = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + BctSize);

                    size_t size;
                    R_TRY(ReadFile(&size, bct, BctSize, GetBctPath(boot_image_update_type)));
                    if (HasEks(boot_image_update_type)) {
                        R_TRY(boot0_accessor.UpdateEks(bct, work));
                    }
                    /* Only preserve autorcm if on a unit with unpatched rcm bug. */
                    if (HasAutoRcmPreserve(boot_image_update_type) && !exosphere::IsRcmBugPatched()) {
                        R_TRY(boot0_accessor.PreserveAutoRcm(bct, work, Boot0Partition::BctSafeSub));
                        R_TRY(boot0_accessor.Write(bct, BctSize, Boot0Partition::BctSafeSub));
                        R_TRY(boot0_accessor.PreserveAutoRcm(bct, work, Boot0Partition::BctSafeMain));
                        R_TRY(boot0_accessor.Write(bct, BctSize, Boot0Partition::BctSafeMain));
                    } else {
                        R_TRY(boot0_accessor.Write(bct, BctSize, Boot0Partition::BctSafeSub));
                        R_TRY(boot0_accessor.Write(bct, BctSize, Boot0Partition::BctSafeMain));
                    }
                }

                /* Write Package2 main. */
                R_TRY(WritePackage2(work_buffer, work_buffer_size, Package2Type::SafeMain, boot_image_update_type));

                /* Write Package1 main. */
                R_TRY(boot1_accessor.Clear(work_buffer, work_buffer_size, Boot1Partition::Package1SafeMain));
                R_TRY(boot1_accessor.Write(GetPackage1Path(boot_image_update_type), work_buffer, work_buffer_size, Boot1Partition::Package1SafeMain));
            }

            return ResultSuccess();
        }

        Result SetVerificationNeeded(BootModeType mode, bool needed, void *work_buffer, size_t work_buffer_size) {
            /* Ensure work buffer is big enough for us to do what we want to do. */
            R_TRY(ValidateWorkBuffer(work_buffer, work_buffer_size));

            /* Initialize boot0 save accessor. */
            BisSave save;
            R_TRY(save.Initialize(work_buffer, work_buffer_size));
            ON_SCOPE_EXIT { save.Finalize(); };

            /* Load save from NAND. */
            R_TRY(save.Load());

            /* Set whether we need to verify, then save to nand. */
            save.SetNeedsVerification(mode, needed);
            R_TRY(save.Save());

            return ResultSuccess();
        }

        Result ValidateBctFileHash(Boot0Accessor &accessor, Boot0Partition which, const void *stored_hash, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
            /* Ensure work buffer is big enough for us to do what we want to do. */
            R_TRY(ValidateWorkBuffer(work_buffer, work_buffer_size));

            void *bct = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + 0);
            void *work = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(work_buffer) + BctSize);

            size_t size;
            R_TRY(ReadFile(&size, bct, BctSize, GetBctPath(boot_image_update_type)));
            if (HasEks(boot_image_update_type)) {
                R_TRY(accessor.UpdateEks(bct, work));
            }
            if (HasAutoRcmPreserve(boot_image_update_type)) {
                R_TRY(accessor.PreserveAutoRcm(bct, work, which));
            }

            u8 file_hash[SHA256_HASH_SIZE];
            sha256CalculateHash(file_hash, bct, BctSize);

            if (std::memcmp(file_hash, stored_hash, SHA256_HASH_SIZE) != 0) {
                return ResultNeedsRepairBootImages();
            }

            return ResultSuccess();
        }

        Result GetPackage2Hash(void *dst_hash, size_t package2_size, void *work_buffer, size_t work_buffer_size, Package2Type which) {
            Package2Accessor accessor(which);
            R_TRY(accessor.Initialize());
            ON_SCOPE_EXIT { accessor.Finalize(); };

            return accessor.GetHash(dst_hash, package2_size, work_buffer, work_buffer_size, Package2Partition::Package2);
        }

        Result WritePackage2(void *work_buffer, size_t work_buffer_size, Package2Type which, BootImageUpdateType boot_image_update_type) {
            Package2Accessor accessor(which);
            R_TRY(accessor.Initialize());
            ON_SCOPE_EXIT { accessor.Finalize(); };

            return accessor.Write(GetPackage2Path(boot_image_update_type), work_buffer, work_buffer_size, Package2Partition::Package2);
        }

    }

    BootImageUpdateType GetBootImageUpdateType(spl::HardwareType hw_type) {
        switch (hw_type) {
            case spl::HardwareType::Icosa:
            case spl::HardwareType::Copper:
                return BootImageUpdateType::Erista;
            case spl::HardwareType::Hoag:
            case spl::HardwareType::Iowa:
                return BootImageUpdateType::Mariko;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result VerifyBootImagesAndRepairIfNeeded(bool *out_repaired_normal, bool *out_repaired_safe, void *work_buffer, size_t work_buffer_size, BootImageUpdateType boot_image_update_type) {
        /* Always set output to false before doing anything else. */
        *out_repaired_normal = false;
        *out_repaired_safe = false;

        /* Ensure work buffer is big enough for us to do what we want to do. */
        R_TRY(ValidateWorkBuffer(work_buffer, work_buffer_size));

        /* Get verification state from NAND. */
        VerificationState verification_state;
        R_TRY(GetVerificationState(&verification_state, work_buffer, work_buffer_size));

        /* If we don't need to verify anything, we're done. */
        if (!verification_state.needs_verify_normal && !verification_state.needs_verify_safe) {
            return ResultSuccess();
        }

        /* Get a session to ncm. */
        sm::ScopedServiceHolder<ncmInitialize, ncmExit> ncm_holder;
        R_ASSERT(ncm_holder.GetResult());

        /* Verify normal, verify safe as needed. */
        if (verification_state.needs_verify_normal) {
            R_TRY_CATCH(VerifyBootImagesAndRepairIfNeeded(out_repaired_normal, BootModeType::Normal, work_buffer, work_buffer_size, boot_image_update_type)) {
                R_CATCH(ResultBootImagePackageNotFound) { /* Nintendo considers failure to locate bip a success. TODO: don't do that? */ }
            } R_END_TRY_CATCH;
        }

        if (verification_state.needs_verify_safe) {
            R_TRY_CATCH(VerifyBootImagesAndRepairIfNeeded(out_repaired_safe, BootModeType::Safe, work_buffer, work_buffer_size, boot_image_update_type)) {
                R_CATCH(ResultBootImagePackageNotFound) { /* Nintendo considers failure to locate bip a success. TODO: don't do that? */ }
            } R_END_TRY_CATCH;
        }

        return ResultSuccess();
    }

}
