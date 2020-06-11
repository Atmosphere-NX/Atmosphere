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
#include "warmboot_secure_monitor.hpp"

namespace ams::warmboot {

    namespace {

        constexpr inline const uintptr_t PMC      = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();

        constexpr inline const pkg1::AesKeySlot SavedAesKeySlots[] = {
            pkg1::AesKeySlot_TzramSaveKek,
            pkg1::AesKeySlot_RandomForUserWrap,
            pkg1::AesKeySlot_RandomForKeyStorageWrap,
            pkg1::AesKeySlot_DeviceMaster,
            pkg1::AesKeySlot_Master,
            pkg1::AesKeySlot_Device,
        };

        constexpr ALWAYS_INLINE bool IsSavedAesKeySlot(int slot) {
            for (const auto SavedSlot : SavedAesKeySlots) {
                if (slot == SavedSlot) {
                    return true;
                }
            }

            return false;
        }

        void ClearUnsavedSecurityEngineKeySlots() {
            /* Clear unsaved aes keys and all ivs. */
            for (int slot = 0; slot < se::AesKeySlotCount; ++slot) {
                if (!IsSavedAesKeySlot(slot)) {
                    se::ClearAesKeySlot(slot);
                }
                se::ClearAesKeyIv(slot);
            }

            /* Clear all rsa keys. */
            for (int slot = 0; slot < se::RsaKeySlotCount; ++slot) {
                se::ClearRsaKeySlot(slot);
            }
        }

        void RestoreEncryptedTzram(void * const tzram_dst, const void * const tzram_src, size_t tzram_size) {
            /* Derive the save key from the save kek. */
            const u32 key_source[se::AesBlockSize / sizeof(u32)] = { reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH24), reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH25), reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH26), reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH27)};
            se::ClearAesKeySlot(pkg1::AesKeySlot_TzramSaveKey);
            se::SetEncryptedAesKey256(pkg1::AesKeySlot_TzramSaveKey, pkg1::AesKeySlot_TzramSaveKek, key_source, sizeof(key_source));

            /* Decrypt tzram. */
            const u8 tzram_iv[se::AesBlockSize] = {};
            se::DecryptAes256Cbc(tzram_dst, tzram_size, pkg1::AesKeySlot_TzramSaveKey, tzram_src, tzram_size, tzram_iv, sizeof(tzram_iv));

            /* Calculate the cmac of decrypted tzram. */
            u8 tzram_mac[se::AesBlockSize] = {};
            se::ComputeAes256Cmac(tzram_mac, sizeof(tzram_mac), pkg1::AesKeySlot_TzramSaveKey, tzram_dst, tzram_size);

            /* Get the expected mac from pmc scratch. */
            const u32 expected_mac[sizeof(tzram_mac) / sizeof(u32)] = { reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH112), reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH113), reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH114), reg::Read(PMC + APBDEV_PMC_SECURE_SCRATCH115)};

            /* Validate that the calculated mac is correct. */
            AMS_ABORT_UNLESS(crypto::IsSameBytes(tzram_mac, expected_mac, sizeof(tzram_mac)));
        }

        void RestoreSecureMonitorToTzramErista(const TargetFirmware target_fw) {
            /* Clear all unsaved security engine keyslots. */
            ClearUnsavedSecurityEngineKeySlots();

            /* Restore encrypted tzram contents. */
            void * const tzram_src  = secmon::MemoryRegionPhysicalDramSecureDataStoreTzram.GetPointer<void>();
            void * const tzram_dst  = secmon::MemoryRegionPhysicalTzramNonVolatile.GetPointer<void>();
            const size_t tzram_size = secmon::MemoryRegionPhysicalTzramNonVolatile.GetSize();
            RestoreEncryptedTzram(tzram_dst, tzram_src, tzram_size);

            /* Clear the tzram kek registers. */
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH24, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH25, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH26, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH27, 0);

            /* Clear the tzram cmac registers. */
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH112, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH113, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH114, 0);
            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH115, 0);

            /* Clear the keydata used to protect tzram. */
            se::ClearAesKeySlot(pkg1::AesKeySlot_TzramSaveKek);
            se::ClearAesKeySlot(pkg1::AesKeySlot_TzramSaveKey);

            /* Clear the encrypted copy of tzram in dram. */
            /* NOTE: This does not actually clear the encrypted copy, because BPMP access to main memory has been restricted. */
            /*       Nintendo seems to not realize this, though, so we'll do the same. */
            std::memset(tzram_src, 0, tzram_size);

            /* Set Tzram to secure-world only. */
            se::SetTzramSecure();
        }

    }

    void RestoreSecureMonitorToTzram(const TargetFirmware target_fw) {
        /* If erista, perform restoration procedure. */
        if (fuse::GetSocType() == fuse::SocType_Erista) {
            RestoreSecureMonitorToTzramErista(target_fw);
        }

        /* Lock secure scratch. */
        pmc::LockSecureRegister(static_cast<pmc::SecureRegister>(pmc::SecureRegister_DramParameters | pmc::SecureRegister_Other));

        /* Lockout fuses. */
        fuse::Lockout();
    }

}
