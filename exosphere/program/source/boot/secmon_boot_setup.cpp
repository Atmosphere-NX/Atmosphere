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
#include <exosphere.hpp>
#include "secmon_boot.hpp"
#include "secmon_boot_cache.hpp"
#include "../secmon_setup.hpp"
#include "../secmon_key_storage.hpp"

namespace ams::secmon::boot {

    namespace {

        void ValidateSystemCounters() {
            const uintptr_t sysctr0 = MemoryRegionVirtualDeviceSysCtr0.GetAddress();

            /* Validate the system counter frequency is as expected. */
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_CNTFID0) == 19'200'000u);

            /* Validate the system counters are as expected. */
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 0)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 1)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 2)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 3)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 4)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 5)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 6)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 7)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 8)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID( 9)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID(10)) == 0);
            AMS_ABORT_UNLESS(reg::Read(sysctr0 + SYSCTR0_COUNTERID(11)) == 0);
        }

        void SetupPmcRegisters() {
            const auto pmc = MemoryRegionVirtualDevicePmc.GetAddress();

            /* Set the physical address of the warmboot binary to scratch 1. */
            if (GetSocType() == fuse::SocType_Mariko) {
                reg::Write(pmc + APBDEV_PMC_SECURE_SCRATCH119, static_cast<u32>(MemoryRegionPhysicalDramSecureDataStoreWarmbootFirmware.GetAddress()));
            } else /* if (GetSocType() == fuse::SocType_Erista) */ {
                reg::Write(pmc + APBDEV_PMC_SCRATCH1, static_cast<u32>(MemoryRegionPhysicalDramSecureDataStoreWarmbootFirmware.GetAddress()));
            }


            /* Configure logging by setting bits 18-19 of scratch 20. */
            reg::ReadWrite(pmc + APBDEV_PMC_SCRATCH20, REG_BITS_VALUE(18, 2, 0));

            /* Clear the wdt reset flag. */
            reg::ReadWrite(pmc + APBDEV_PMC_SCRATCH190, REG_BITS_VALUE(0, 1, 0));

            /* Configure warmboot to set  Set FUSE_PRIVATEKEYDISABLE to KEY_INVISIBLE. */
            reg::ReadWrite(pmc + APBDEV_PMC_SECURE_SCRATCH21, REG_BITS_VALUE(4, 1, 1));

            /* Write the warmboot key. */
            /* TODO: This is necessary for mariko. We should decide how to handle this. */
            /* In particular, mariko will need to support loading older-than-expected warmboot firmware. */
            /* We could hash the warmboot firmware and use a lookup table, or require bootloader to provide */
            /* The warmboot key as a parameter. The latter is a better solution, but it would be nice to take */
            /* care of it here. Perhaps we should read the number of anti-downgrade fuses burnt, and translate that */
            /* to the warmboot key? To be decided during the process of implementing ams-on-mariko support. */
            reg::Write(pmc + APBDEV_PMC_SECURE_SCRATCH32, 0x129);
        }

        constinit const u8 DeviceMasterKeySourceKekSource[se::AesBlockSize] = {
            0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28
        };

        /* This function derives the master kek and device keys using the tsec root key. */
        void DeriveMasterKekAndDeviceKeyErista(bool is_prod) {
            /* NOTE: Exosphere does not use this in practice, and expects the bootloader to set up keys already. */
            /* NOTE: This function is currently not implemented. If implemented, it will only be a reference implementation. */
            if constexpr (false) {
                /* TODO: Consider implementing this as a reference. */
            }

            AMS_UNUSED(is_prod);
        }

        /* NOTE: These are just latest-master-kek encrypted with BEK. */
        /* We can get away with only including latest because exosphere supports newer-than-expected master key in engine. */
        /* TODO: Update on next change of keys. */
        constinit const u8 MarikoMasterKekSourceProd[se::AesBlockSize] = {
            0x0E, 0x44, 0x0C, 0xED, 0xB4, 0x36, 0xC0, 0x3F, 0xAA, 0x1D, 0xAE, 0xBF, 0x62, 0xB1, 0x09, 0x82
        };

        constinit const u8 MarikoMasterKekSourceDev[se::AesBlockSize] = {
            0xF9, 0x37, 0xCF, 0x9A, 0xBD, 0x86, 0xBB, 0xA9, 0x9C, 0x9E, 0x03, 0xC4, 0xFC, 0xBC, 0x3B, 0xCE
        };

        constinit const u8 MasterKeySource[se::AesBlockSize] = {
            0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C
        };

        void DeriveMasterKekAndDeviceKeyMariko(bool is_prod) {
            /* Clear all keyslots other than KEK and SBK in SE1. */
            for (int i = 0; i < pkg1::AesKeySlot_Count; ++i) {
                if (i != pkg1::AesKeySlot_MarikoKek && i != pkg1::AesKeySlot_SecureBoot) {
                    se::ClearAesKeySlot(i);
                }
            }

            /* Clear all keyslots in SE2. */
            for (int i = 0; i < pkg1::AesKeySlot_Count; ++i) {
                se::ClearAesKeySlot2(i);
            }

            /* Derive the master kek. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_MasterKek, pkg1::AesKeySlot_MarikoKek, is_prod ? MarikoMasterKekSourceProd : MarikoMasterKekSourceDev, se::AesBlockSize);

            /* Derive the device master key source kek. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_DeviceMasterKeySourceKekMariko, pkg1::AesKeySlot_SecureBoot, DeviceMasterKeySourceKekSource, se::AesBlockSize);

            /* Clear the KEK, now that we're done using it. */
            se::ClearAesKeySlot(pkg1::AesKeySlot_MarikoKek);
        }

        void DeriveMasterKekAndDeviceKey(bool is_prod) {
            if (GetSocType() == fuse::SocType_Mariko) {
                DeriveMasterKekAndDeviceKeyMariko(is_prod);
            } else /* if (GetSocType() == fuse::SocType_Erista) */ {
                DeriveMasterKekAndDeviceKeyErista(is_prod);
            }
        }

        void DeriveMasterKey() {
            if (GetSocType() == fuse::SocType_Mariko) {
                se::SetEncryptedAesKey128(pkg1::AesKeySlot_Master, pkg1::AesKeySlot_MasterKek, MasterKeySource, se::AesBlockSize);
            } else /* if (GetSocType() == fuse::SocType_Erista) */ {
                /* Nothing to do here; erista bootloader will have derived master key already. */
            }
        }

        void SetupRandomKey(int slot, se::KeySlotLockFlags flags) {
            /* Create an aligned buffer to hold the key. */
            constexpr size_t KeySize = se::AesBlockSize;
            util::AlignedBuffer<hw::DataCacheLineSize, KeySize> key;

            /* Ensure data is consistent before triggering the SE. */
            hw::FlushDataCache(key, KeySize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Generate random bytes into the key. */
            se::GenerateRandomBytes(key, KeySize);

            /* Ensure that the CPU sees consistent data. */
            hw::DataSynchronizationBarrierInnerShareable();
            hw::FlushDataCache(key, KeySize);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Use the random bytes as a key source. */
            se::SetEncryptedAesKey128(slot, pkg1::AesKeySlot_DeviceMaster, key, KeySize);

            /* Lock the keyslot. */
            se::LockAesKeySlot(slot, flags);
        }

        constinit const u8 MasterKeyVectorsDev[pkg1::OldMasterKeyCount + 1][se::AesBlockSize] = {
            {0x46, 0x22, 0xB4, 0x51, 0x9A, 0x7E, 0xA7, 0x7F, 0x62, 0xA1, 0x1F, 0x8F, 0xC5, 0x3A, 0xDB, 0xFE}, /* Zeroes encrypted with Master Key 00. */
            {0x39, 0x33, 0xF9, 0x31, 0xBA, 0xE4, 0xA7, 0x21, 0x2C, 0xDD, 0xB7, 0xD8, 0xB4, 0x4E, 0x37, 0x23}, /* Master key 00 encrypted with Master key 01. */
            {0x97, 0x29, 0xB0, 0x32, 0x43, 0x14, 0x8C, 0xA6, 0x85, 0xE9, 0x5A, 0x94, 0x99, 0x39, 0xAC, 0x5D}, /* Master key 01 encrypted with Master key 02. */
            {0x2C, 0xCA, 0x9C, 0x31, 0x1E, 0x07, 0xB0, 0x02, 0x97, 0x0A, 0xD8, 0x03, 0xA2, 0x76, 0x3F, 0xA3}, /* Master key 02 encrypted with Master key 03. */
            {0x9B, 0x84, 0x76, 0x14, 0x72, 0x94, 0x52, 0xCB, 0x54, 0x92, 0x9B, 0xC4, 0x8C, 0x5B, 0x0F, 0xBA}, /* Master key 03 encrypted with Master key 04. */
            {0x78, 0xD5, 0xF1, 0x20, 0x3D, 0x16, 0xE9, 0x30, 0x32, 0x27, 0x34, 0x6F, 0xCF, 0xE0, 0x27, 0xDC}, /* Master key 04 encrypted with Master key 05. */
            {0x6F, 0xD2, 0x84, 0x1D, 0x05, 0xEC, 0x40, 0x94, 0x5F, 0x18, 0xB3, 0x81, 0x09, 0x98, 0x8D, 0x4E}, /* Master key 05 encrypted with Master key 06. */
            {0x37, 0xAF, 0xAB, 0x35, 0x79, 0x09, 0xD9, 0x48, 0x29, 0xD2, 0xDB, 0xA5, 0xA5, 0xF5, 0x30, 0x19}, /* Master key 06 encrypted with Master key 07. */
            {0xEC, 0xE1, 0x46, 0x89, 0x37, 0xFD, 0xD2, 0x15, 0x8C, 0x3F, 0x24, 0x82, 0xEF, 0x49, 0x68, 0x04}, /* Master key 07 encrypted with Master key 08. */
            {0x43, 0x3D, 0xC5, 0x3B, 0xEF, 0x91, 0x02, 0x21, 0x61, 0x54, 0x63, 0x8A, 0x35, 0xE7, 0xCA, 0xEE}, /* Master key 08 encrypted with Master key 09. */
            {0x6C, 0x2E, 0xCD, 0xB3, 0x34, 0x61, 0x77, 0xF5, 0xF9, 0xB1, 0xDD, 0x61, 0x98, 0x19, 0x3E, 0xD4}, /* Master key 09 encrypted with Master key 0A. */
        };

        constinit const u8 MasterKeyVectorsProd[pkg1::OldMasterKeyCount + 1][se::AesBlockSize] = {
            {0x0C, 0xF0, 0x59, 0xAC, 0x85, 0xF6, 0x26, 0x65, 0xE1, 0xE9, 0x19, 0x55, 0xE6, 0xF2, 0x67, 0x3D}, /* Zeroes encrypted with Master Key 00. */
            {0x29, 0x4C, 0x04, 0xC8, 0xEB, 0x10, 0xED, 0x9D, 0x51, 0x64, 0x97, 0xFB, 0xF3, 0x4D, 0x50, 0xDD}, /* Master key 00 encrypted with Master key 01. */
            {0xDE, 0xCF, 0xEB, 0xEB, 0x10, 0xAE, 0x74, 0xD8, 0xAD, 0x7C, 0xF4, 0x9E, 0x62, 0xE0, 0xE8, 0x72}, /* Master key 01 encrypted with Master key 02. */
            {0x0A, 0x0D, 0xDF, 0x34, 0x22, 0x06, 0x6C, 0xA4, 0xE6, 0xB1, 0xEC, 0x71, 0x85, 0xCA, 0x4E, 0x07}, /* Master key 02 encrypted with Master key 03. */
            {0x6E, 0x7D, 0x2D, 0xC3, 0x0F, 0x59, 0xC8, 0xFA, 0x87, 0xA8, 0x2E, 0xD5, 0x89, 0x5E, 0xF3, 0xE9}, /* Master key 03 encrypted with Master key 04. */
            {0xEB, 0xF5, 0x6F, 0x83, 0x61, 0x9E, 0xF8, 0xFA, 0xE0, 0x87, 0xD7, 0xA1, 0x4E, 0x25, 0x36, 0xEE}, /* Master key 04 encrypted with Master key 05. */
            {0x1E, 0x1E, 0x22, 0xC0, 0x5A, 0x33, 0x3C, 0xB9, 0x0B, 0xA9, 0x03, 0x04, 0xBA, 0xDB, 0x07, 0x57}, /* Master key 05 encrypted with Master key 06. */
            {0xA4, 0xD4, 0x52, 0x6F, 0xD1, 0xE4, 0x36, 0xAA, 0x9F, 0xCB, 0x61, 0x27, 0x1C, 0x67, 0x65, 0x1F}, /* Master key 06 encrypted with Master key 07. */
            {0xEA, 0x60, 0xB3, 0xEA, 0xCE, 0x8F, 0x24, 0x46, 0x7D, 0x33, 0x9C, 0xD1, 0xBC, 0x24, 0x98, 0x29}, /* Master key 07 encrypted with Master key 08. */
            {0x4D, 0xD9, 0x98, 0x42, 0x45, 0x0D, 0xB1, 0x3C, 0x52, 0x0C, 0x9A, 0x44, 0xBB, 0xAD, 0xAF, 0x80}, /* Master key 08 encrypted with Master key 09. */
            {0xB8, 0x96, 0x9E, 0x4A, 0x00, 0x0D, 0xD6, 0x28, 0xB3, 0xD1, 0xDB, 0x68, 0x5F, 0xFB, 0xE1, 0x2A}, /* Master key 09 encrypted with Master key 0A. */
        };

        bool TestKeyGeneration(int generation, bool is_prod) {
            /* Decrypt the vector chain from generation to start. */
            int slot = pkg1::AesKeySlot_Master;
            for (int i = generation; i > 0; --i) {
                se::SetEncryptedAesKey128(pkg1::AesKeySlot_Temporary, slot, is_prod ? MasterKeyVectorsProd[i] : MasterKeyVectorsDev[i], se::AesBlockSize);
                slot = pkg1::AesKeySlot_Temporary;
            }

            /* Decrypt the final vector. */
            u8 test_vector[se::AesBlockSize];
            se::DecryptAes128(test_vector, se::AesBlockSize, slot, is_prod ? MasterKeyVectorsProd[0] : MasterKeyVectorsDev[0], se::AesBlockSize);

            constexpr u8 ZeroBlock[se::AesBlockSize] = {};
            return crypto::IsSameBytes(ZeroBlock, test_vector, se::AesBlockSize);
        }

        int DetermineKeyGeneration(bool is_prod) {
            /* Test each generation in order. */
            for (int generation = 0; generation < pkg1::KeyGeneration_Count; ++generation) {
                if (TestKeyGeneration(generation, is_prod)) {
                    return generation;
                }
            }

            /* We must have found a correct key generation. */
            AMS_ABORT();
        }

        void DeriveAllMasterKeys(bool is_prod, u8 * const work_block) {

            /* Determine the generation. */
            const int generation = DetermineKeyGeneration(is_prod);

            /* Set the global generation. */
            ::ams::secmon::impl::SetKeyGeneration(generation);

            /* Derive all old keys. */
            int slot = pkg1::AesKeySlot_Master;
            for (int i = generation; i > 0; --i) {
                /* Decrypt the old master key. */
                se::DecryptAes128(work_block, se::AesBlockSize, slot, is_prod ? MasterKeyVectorsProd[i] : MasterKeyVectorsDev[i], se::AesBlockSize);

                /* Set the old master key. */
                SetMasterKey(i - 1, work_block, se::AesBlockSize);

                /* Set the old master key into a temporary keyslot. */
                se::SetAesKey(pkg1::AesKeySlot_Temporary, work_block, se::AesBlockSize);

                /* Perform the next decryption with the older master key. */
                slot = pkg1::AesKeySlot_Temporary;
            }
        }

        constinit const u8 DeviceMasterKeySourceSources[pkg1::OldDeviceMasterKeyCount][se::AesBlockSize] = {
            {0x8B, 0x4E, 0x1C, 0x22, 0x42, 0x07, 0xC8, 0x73, 0x56, 0x94, 0x08, 0x8B, 0xCC, 0x47, 0x0F, 0x5D}, /* 4.0.0 Device Master Key Source Source. */
            {0x6C, 0xEF, 0xC6, 0x27, 0x8B, 0xEC, 0x8A, 0x91, 0x99, 0xAB, 0x24, 0xAC, 0x4F, 0x1C, 0x8F, 0x1C}, /* 5.0.0 Device Master Key Source Source. */
            {0x70, 0x08, 0x1B, 0x97, 0x44, 0x64, 0xF8, 0x91, 0x54, 0x9D, 0xC6, 0x84, 0x8F, 0x1A, 0xB2, 0xE4}, /* 6.0.0 Device Master Key Source Source. */
            {0x8E, 0x09, 0x1F, 0x7A, 0xBB, 0xCA, 0x6A, 0xFB, 0xB8, 0x9B, 0xD5, 0xC1, 0x25, 0x9C, 0xA9, 0x17}, /* 6.2.0 Device Master Key Source Source. */
            {0x8F, 0x77, 0x5A, 0x96, 0xB0, 0x94, 0xFD, 0x8D, 0x28, 0xE4, 0x19, 0xC8, 0x16, 0x1C, 0xDB, 0x3D}, /* 7.0.0 Device Master Key Source Source. */
            {0x67, 0x62, 0xD4, 0x8E, 0x55, 0xCF, 0xFF, 0x41, 0x31, 0x15, 0x3B, 0x24, 0x0C, 0x7C, 0x07, 0xAE}, /* 8.1.0 Device Master Key Source Source. */
            {0x4A, 0xC3, 0x4E, 0x14, 0x8B, 0x96, 0x4A, 0xD5, 0xD4, 0x99, 0x73, 0xC4, 0x45, 0xAB, 0x8B, 0x49}, /* 9.0.0 Device Master Key Source Source. */
            {0x14, 0xB8, 0x74, 0x12, 0xCB, 0xBD, 0x0B, 0x8F, 0x20, 0xFB, 0x30, 0xDA, 0x27, 0xE4, 0x58, 0x94}, /* 9.1.0 Device Master Key Source Source. */
        };

        constinit const u8 DeviceMasterKekSourcesDev[pkg1::OldDeviceMasterKeyCount][se::AesBlockSize] = {
            {0xD6, 0xBD, 0x9F, 0xC6, 0x18, 0x09, 0xE1, 0x96, 0x20, 0x39, 0x60, 0xD2, 0x89, 0x83, 0x31, 0x34}, /* 4.0.0 Device Master Kek Source. */
            {0x59, 0x2D, 0x20, 0x69, 0x33, 0xB5, 0x17, 0xBA, 0xCF, 0xB1, 0x4E, 0xFD, 0xE4, 0xC2, 0x7B, 0xA8}, /* 5.0.0 Device Master Kek Source. */
            {0xF6, 0xD8, 0x59, 0x63, 0x8F, 0x47, 0xCB, 0x4A, 0xD8, 0x74, 0x05, 0x7F, 0x88, 0x92, 0x33, 0xA5}, /* 6.0.0 Device Master Kek Source. */
            {0x20, 0xAB, 0xF2, 0x0F, 0x05, 0xE3, 0xDE, 0x2E, 0xA1, 0xFB, 0x37, 0x5E, 0x8B, 0x22, 0x1A, 0x38}, /* 6.2.0 Device Master Kek Source. */
            {0x60, 0xAE, 0x56, 0x68, 0x11, 0xE2, 0x0C, 0x99, 0xDE, 0x05, 0xAE, 0x68, 0x78, 0x85, 0x04, 0xAE}, /* 7.0.0 Device Master Kek Source. */
            {0x94, 0xD6, 0xA8, 0xC0, 0x95, 0xAF, 0xD0, 0xA6, 0x27, 0x53, 0x5E, 0xE5, 0x8E, 0x70, 0x1F, 0x87}, /* 8.1.0 Device Master Kek Source. */
            {0x61, 0x6A, 0x88, 0x21, 0xA3, 0x52, 0xB0, 0x19, 0x16, 0x25, 0xA4, 0xE3, 0x4C, 0x54, 0x02, 0x0F}, /* 9.0.0 Device Master Kek Source. */
            {0x9D, 0xB1, 0xAE, 0xCB, 0xF6, 0xF6, 0xE3, 0xFE, 0xAB, 0x6F, 0xCB, 0xAF, 0x38, 0x03, 0xFC, 0x7B}, /* 9.1.0 Device Master Kek Source. */
        };

        constinit const u8 DeviceMasterKekSourcesProd[pkg1::OldDeviceMasterKeyCount][se::AesBlockSize] = {
            {0x88, 0x62, 0x34, 0x6E, 0xFA, 0xF7, 0xD8, 0x3F, 0xE1, 0x30, 0x39, 0x50, 0xF0, 0xB7, 0x5D, 0x5D}, /* 4.0.0 Device Master Kek Source. */
            {0x06, 0x1E, 0x7B, 0xE9, 0x6D, 0x47, 0x8C, 0x77, 0xC5, 0xC8, 0xE7, 0x94, 0x9A, 0xA8, 0x5F, 0x2E}, /* 5.0.0 Device Master Kek Source. */
            {0x99, 0xFA, 0x98, 0xBD, 0x15, 0x1C, 0x72, 0xFD, 0x7D, 0x9A, 0xD5, 0x41, 0x00, 0xFD, 0xB2, 0xEF}, /* 6.0.0 Device Master Kek Source. */
            {0x81, 0x3C, 0x6C, 0xBF, 0x5D, 0x21, 0xDE, 0x77, 0x20, 0xD9, 0x6C, 0xE3, 0x22, 0x06, 0xAE, 0xBB}, /* 6.2.0 Device Master Kek Source. */
            {0x86, 0x61, 0xB0, 0x16, 0xFA, 0x7A, 0x9A, 0xEA, 0xF6, 0xF5, 0xBE, 0x1A, 0x13, 0x5B, 0x6D, 0x9E}, /* 7.0.0 Device Master Kek Source. */
            {0xA6, 0x81, 0x71, 0xE7, 0xB5, 0x23, 0x74, 0xB0, 0x39, 0x8C, 0xB7, 0xFF, 0xA0, 0x62, 0x9F, 0x8D}, /* 8.1.0 Device Master Kek Source. */
            {0x03, 0xE7, 0xEB, 0x43, 0x1B, 0xCF, 0x5F, 0xB5, 0xED, 0xDC, 0x97, 0xAE, 0x21, 0x8D, 0x19, 0xED}, /* 9.0.0 Device Master Kek Source. */
            {0xCE, 0xFE, 0x41, 0x0F, 0x46, 0x9A, 0x30, 0xD6, 0xF2, 0xE9, 0x0C, 0x6B, 0xB7, 0x15, 0x91, 0x36}, /* 9.1.0 Device Master Kek Source. */
        };

        void DeriveAllDeviceMasterKeys(bool is_prod, u8 * const work_block) {
            /* Get the current key generation. */
            const int current_generation = secmon::GetKeyGeneration();

            /* Get the kek slot. */
            const int kek_slot = GetSocType() == fuse::SocType_Mariko ? pkg1::AesKeySlot_DeviceMasterKeySourceKekMariko : pkg1::AesKeySlot_DeviceMasterKeySourceKekErista;

            /* Iterate for all generations. */
            for (int i = 0; i < pkg1::OldDeviceMasterKeyCount; ++i) {
                const int generation = pkg1::KeyGeneration_4_0_0 + i;

                /* Load the first master key into the temporary keyslot keyslot. */
                LoadMasterKey(pkg1::AesKeySlot_Temporary, pkg1::KeyGeneration_1_0_0);

                /* Decrypt the device master kek for the generation. */
                se::SetEncryptedAesKey128(pkg1::AesKeySlot_Temporary, pkg1::AesKeySlot_Temporary, is_prod ? DeviceMasterKekSourcesProd[i] : DeviceMasterKekSourcesDev[i], se::AesBlockSize);

                /* Decrypt the device master key source into the work block. */
                se::DecryptAes128(work_block, se::AesBlockSize, kek_slot, DeviceMasterKeySourceSources[i], se::AesBlockSize);

                /* If we're decrypting the current device master key, decrypt into the keyslot. */
                if (generation == current_generation) {
                    se::SetEncryptedAesKey128(pkg1::AesKeySlot_DeviceMaster, pkg1::AesKeySlot_Temporary, work_block, se::AesBlockSize);
                } else {
                    /* Otherwise, decrypt the work block into itself and set the old device master key. */
                    se::DecryptAes128(work_block, se::AesBlockSize, pkg1::AesKeySlot_Temporary, work_block, se::AesBlockSize);

                    /* Set the device master key. */
                    SetDeviceMasterKey(generation, work_block, se::AesBlockSize);
                }
            }

            /* Clear and lock the Device Master Key Source Kek. */
            se::ClearAesKeySlot(pkg1::AesKeySlot_DeviceMasterKeySourceKekMariko);
            se::LockAesKeySlot(pkg1::AesKeySlot_DeviceMasterKeySourceKekMariko, se::KeySlotLockFlags_AllLockKek);
        }

        void DeriveAllKeys(bool is_prod) {
            /* Get the ephemeral work block. */
            u8 * const work_block = se::GetEphemeralWorkBlock();
            ON_SCOPE_EXIT { util::ClearMemory(work_block, se::AesBlockSize); };

            /* Lock the master key as a key. */
            se::LockAesKeySlot(pkg1::AesKeySlot_Master, se::KeySlotLockFlags_AllLockKey);

            /* Setup a random key to protect the old master and device master keys. */
            SetupRandomKey(pkg1::AesKeySlot_RandomForKeyStorageWrap, se::KeySlotLockFlags_AllLockKey);

            /* Derive the master keys. */
            DeriveAllMasterKeys(is_prod, work_block);

            /* Lock the master key as a kek. */
            se::LockAesKeySlot(pkg1::AesKeySlot_Master, se::KeySlotLockFlags_AllLockKek);

            /* Derive the device master keys. */
            DeriveAllDeviceMasterKeys(is_prod, work_block);

            /* Lock the device master key as a kek. */
            se::LockAesKeySlot(pkg1::AesKeySlot_DeviceMaster, se::KeySlotLockFlags_AllLockKek);

            /* Setup a random key to protect user keys. */
            SetupRandomKey(pkg1::AesKeySlot_RandomForUserWrap, se::KeySlotLockFlags_AllLockKek);
        }

        void InitializeKeys() {
            /* Read lock all aes keys. */
            for (int i = 0; i < se::AesKeySlotCount; ++i) {
                se::LockAesKeySlot(i, se::KeySlotLockFlags_AllReadLock);
            }

            /* Lock the secure monitor aes keys to be secmon only and non-readable. */
            for (int i = pkg1::AesKeySlot_SecmonStart; i < pkg1::AesKeySlot_SecmonEnd; ++i) {
                se::LockAesKeySlot(i, se::KeySlotLockFlags_KeyUse | se::KeySlotLockFlags_PerKey);
            }

            /* Lock the unused keyslots entirely. */
            static_assert(pkg1::AesKeySlot_UserEnd <= pkg1::AesKeySlot_SecmonStart);
            for (int i = pkg1::AesKeySlot_UserEnd; i < pkg1::AesKeySlot_SecmonStart; ++i) {
                se::LockAesKeySlot(i, se::KeySlotLockFlags_AllLockKek);
            }

            /* Read lock all rsa keys. */
            for (int i = 0;  i < se::RsaKeySlotCount; ++i) {
                se::LockRsaKeySlot(i, se::KeySlotLockFlags_KeyUse | se::KeySlotLockFlags_PerKey | se::KeySlotLockFlags_KeyRead);
            }

            /* Initialize the rng. */
            se::InitializeRandom();

            /* Determine whether we're production. */
            const bool is_prod = IsProduction();

            /* Derive the master kek and device key. */
            /* NOTE: This is a no-op on erista, because fusee will have set up keys. */
            DeriveMasterKekAndDeviceKey(is_prod);

            /* Lock the device key as only usable as a kek. */
            se::LockAesKeySlot(pkg1::AesKeySlot_Device, se::KeySlotLockFlags_AllLockKek);

            /* Derive the master key. */
            DeriveMasterKey();

            /* Derive all other keys. */
            DeriveAllKeys(is_prod);
        }

    }

    namespace {

        using namespace ams::mmu;

        constexpr void UnmapPhysicalIdentityMappingImpl(u64 *l1, u64 *l2, u64 *l3) {
            /* Invalidate the L3 entries for the tzram and iram boot code regions. */
            InvalidateL3Entries(l3, MemoryRegionPhysicalTzram.GetAddress(),        MemoryRegionPhysicalTzram.GetSize());
            InvalidateL3Entries(l3, MemoryRegionPhysicalIramBootCode.GetAddress(), MemoryRegionPhysicalIramBootCode.GetSize());

            /* Unmap the L2 entries corresponding to those L3 entries. */
            InvalidateL2Entries(l2, MemoryRegionPhysicalIramL2.GetAddress(),  MemoryRegionPhysicalIramL2.GetSize());
            InvalidateL2Entries(l2, MemoryRegionPhysicalTzramL2.GetAddress(), MemoryRegionPhysicalTzramL2.GetSize());

            /* Unmap the L1 entry corresponding to to those L2 entries. */
            InvalidateL1Entries(l1, MemoryRegionPhysical.GetAddress(), MemoryRegionPhysical.GetSize());
        }

        constexpr void UnmapDramImpl(u64 *l1, u64 *l2, u64 *l3) {
            /* Unmap the L1 entry corresponding to to the Dram entries. */
            AMS_UNUSED(l2, l3);
            InvalidateL1Entries(l1, MemoryRegionDram.GetAddress(), MemoryRegionDram.GetSize());
        }

    }

    void InitializeColdBoot() {
        /* Ensure that the system counters are valid. */
        ValidateSystemCounters();

        /* Set the security engine to Tzram Secure. */
        se::SetTzramSecure();

        /* Set the security engine to Per Key Secure. */
        se::SetPerKeySecure();

        /* Set the security engine to Context Save Secure. */
        se::SetContextSaveSecure();

        /* Setup the PMC registers. */
        SetupPmcRegisters();

        /* Lockout the scratch that we've just written. */
        /* pmc::LockSecureRegisters(1); */

        /* Generate a random srk. */
        se::GenerateSrk();

        /* Initialize the SE keyslots. */
        InitializeKeys();

        /* Save a test vector for the SE keyslots. */
        SaveSecurityEngineAesKeySlotTestVector();
    }

    void UnmapPhysicalIdentityMapping() {
        /* Get the tables. */
        u64 * const l1    = MemoryRegionVirtualTzramL1PageTable.GetPointer<u64>();
        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        /* Unmap. */
        UnmapPhysicalIdentityMappingImpl(l1, l2_l3, l2_l3);

        /* Ensure the mappings are consistent. */
        secmon::boot::EnsureMappingConsistency();
    }

    void UnmapDram() {
        /* Get the tables. */
        u64 * const l1    = MemoryRegionVirtualTzramL1PageTable.GetPointer<u64>();
        u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

        /* Unmap. */
        UnmapDramImpl(l1, l2_l3, l2_l3);

        /* Ensure the mappings are consistent. */
        secmon::boot::EnsureMappingConsistency();
    }

}
