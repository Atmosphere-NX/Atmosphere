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

            /* TODO: Fix to ensure correct scratch contents on mariko, as otherwise wb is broken. */
            AMS_ABORT_UNLESS(fuse::GetSocType() != fuse::SocType_Mariko);
        }

        /* This function derives the master kek and device keys using the tsec root key. */
        void DeriveMasterKekAndDeviceKeyErista(bool is_prod) {
            /* NOTE: Exosphere does not use this in practice, and expects the bootloader to set up keys already. */
            /* NOTE: This function is currently not implemented. If implemented, it will only be a reference implementation. */
            if constexpr (false) {
                /* TODO: Consider implementing this as a reference. */
            }

            AMS_UNUSED(is_prod);
        }

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
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_MasterKek, pkg1::AesKeySlot_MarikoKek, GetMarikoMasterKekSource(is_prod), se::AesBlockSize);

            /* Derive the device master key source kek. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_DeviceMasterKeySourceKekMariko, pkg1::AesKeySlot_SecureBoot, GetDeviceMasterKeySourceKekSource(), se::AesBlockSize);

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
                se::SetEncryptedAesKey128(pkg1::AesKeySlot_Master, pkg1::AesKeySlot_MasterKek, GetMasterKeySource(), se::AesBlockSize);
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

        bool TestKeyGeneration(int generation, bool is_prod) {
            /* Decrypt the vector chain from generation to start. */
            int slot = pkg1::AesKeySlot_Master;
            for (int i = generation; i > 0; --i) {
                se::SetEncryptedAesKey128(pkg1::AesKeySlot_Temporary, slot, GetMasterKeyVector(is_prod, i), se::AesBlockSize);
                slot = pkg1::AesKeySlot_Temporary;
            }

            /* Decrypt the final vector. */
            u8 test_vector[se::AesBlockSize];
            se::DecryptAes128(test_vector, se::AesBlockSize, slot, GetMasterKeyVector(is_prod, 0), se::AesBlockSize);

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
            AMS_SECMON_LOG("KeyGen: %02X\n", static_cast<unsigned int>(generation));

            /* Set the global generation. */
            ::ams::secmon::impl::SetKeyGeneration(generation);

            /* Derive all old keys. */
            int slot = pkg1::AesKeySlot_Master;
            for (int i = generation; i > 0; --i) {
                /* Decrypt the old master key. */
                se::DecryptAes128(work_block, se::AesBlockSize, slot, GetMasterKeyVector(is_prod, i), se::AesBlockSize);

                /* Set the old master key. */
                SetMasterKey(i - 1, work_block, se::AesBlockSize);

                /* Set the old master key into a temporary keyslot. */
                se::SetAesKey(pkg1::AesKeySlot_Temporary, work_block, se::AesBlockSize);

                /* Perform the next decryption with the older master key. */
                slot = pkg1::AesKeySlot_Temporary;
            }
        }

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
                se::SetEncryptedAesKey128(pkg1::AesKeySlot_Temporary, pkg1::AesKeySlot_Temporary, GetDeviceMasterKekSource(is_prod, i), se::AesBlockSize);

                /* Decrypt the device master key source into the work block. */
                se::DecryptAes128(work_block, se::AesBlockSize, kek_slot, GetDeviceMasterKeySourceSource(i), se::AesBlockSize);

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

        constexpr void UnmapMarikoProgramImpl(u64 *l1, u64 *l2, u64 *l3) {
            /* Unmap the L1 entry corresponding to to the Dram entries. */
            AMS_UNUSED(l1, l2);
            InvalidateL3Entries(l3, MemoryRegionVirtualTzramMarikoProgram.GetAddress(), MemoryRegionVirtualTzramMarikoProgram.GetSize());
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

    void LoadMarikoProgram() {
        void * const mariko_program_dst  = MemoryRegionVirtualTzramMarikoProgram.GetPointer<void>();
        void * const mariko_program_src  = MemoryRegionPhysicalMarikoProgramImage.GetPointer<void>();
        const size_t mariko_program_size = MemoryRegionVirtualTzramMarikoProgram.GetSize();

        if (fuse::GetSocType() == fuse::SocType_Mariko) {
            /* On Mariko, we want to load the mariko program image into mariko tzram. */
            std::memcpy(mariko_program_dst, mariko_program_src, mariko_program_size);
            hw::FlushDataCache(mariko_program_dst, mariko_program_size);
        } else {
            /* On Erista, we don't have mariko-only-tzram, so unmap it. */
            u64 * const l1    = MemoryRegionVirtualTzramL1PageTable.GetPointer<u64>();
            u64 * const l2_l3 = MemoryRegionVirtualTzramL2L3PageTable.GetPointer<u64>();

            UnmapMarikoProgramImpl(l1, l2_l3, l2_l3);
        }

        /* Clear the Mariko program image from DRAM. */
        util::ClearMemory(mariko_program_src, mariko_program_size);
        hw::FlushDataCache(mariko_program_src, mariko_program_size);
        hw::DataSynchronizationBarrierInnerShareable();

        /* Ensure the mappings are consistent. */
        secmon::boot::EnsureMappingConsistency();
    }

}
