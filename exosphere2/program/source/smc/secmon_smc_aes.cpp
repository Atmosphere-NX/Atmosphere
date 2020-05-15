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
#include "../secmon_error.hpp"
#include "../secmon_key_storage.hpp"
#include "../secmon_misc.hpp"
#include "secmon_smc_aes.hpp"
#include "secmon_smc_se_lock.hpp"

namespace ams::secmon::smc {

    namespace {

        constexpr inline auto AesKeySize = se::AesBlockSize;

        enum SealKey {
            SealKey_LoadAesKey                  = 0,
            SealKey_DecryptDeviceUniqueData     = 1,
            SealKey_LoadLotusKey                = 2,
            SealKey_LoadEsDeviceKey             = 3,
            SealKey_ReencryptDeviceUniqueData   = 4,
            SealKey_LoadSslKey                  = 5,
            SealKey_LoadEsClientCertKey         = 6,

            SealKey_Count,
        };

        enum KeyType {
            KeyType_Default           = 0,
            KeyType_NormalOnly        = 1,
            KeyType_RecoveryOnly      = 2,
            KeyType_NormalAndRecovery = 3,

            KeyType_Count,
        };

        struct GenerateAesKekOption {
            using IsDeviceUnique = util::BitPack32::Field<0,  1, bool>;
            using KeyTypeIndex   = util::BitPack32::Field<1,  4, KeyType>;
            using SealKeyIndex   = util::BitPack32::Field<5,  3, SealKey>;
            using Reserved       = util::BitPack32::Field<8, 24, u32>;
        };

        constexpr const u8 SealKeySources[SealKey_Count][AesKeySize] = {
            [SealKey_LoadAesKey]                  = { 0xF4, 0x0C, 0x16, 0x26, 0x0D, 0x46, 0x3B, 0xE0, 0x8C, 0x6A, 0x56, 0xE5, 0x82, 0xD4, 0x1B, 0xF6 },
            [SealKey_DecryptDeviceUniqueData]     = { 0x7F, 0x54, 0x2C, 0x98, 0x1E, 0x54, 0x18, 0x3B, 0xBA, 0x63, 0xBD, 0x4C, 0x13, 0x5B, 0xF1, 0x06 },
            [SealKey_LoadLotusKey]                = { 0xC7, 0x3F, 0x73, 0x60, 0xB7, 0xB9, 0x9D, 0x74, 0x0A, 0xF8, 0x35, 0x60, 0x1A, 0x18, 0x74, 0x63 },
            [SealKey_LoadEsDeviceKey]             = { 0x0E, 0xE0, 0xC4, 0x33, 0x82, 0x66, 0xE8, 0x08, 0x39, 0x13, 0x41, 0x7D, 0x04, 0x64, 0x2B, 0x6D },
            [SealKey_ReencryptDeviceUniqueData]   = { 0xE1, 0xA8, 0xAA, 0x6A, 0x2D, 0x9C, 0xDE, 0x43, 0x0C, 0xDE, 0xC6, 0x17, 0xF6, 0xC7, 0xF1, 0xDE },
            [SealKey_LoadSslKey]                  = { 0x74, 0x20, 0xF6, 0x46, 0x77, 0xB0, 0x59, 0x2C, 0xE8, 0x1B, 0x58, 0x64, 0x47, 0x41, 0x37, 0xD9 },
            [SealKey_LoadEsClientCertKey]         = { 0xAA, 0x19, 0x0F, 0xFA, 0x4C, 0x30, 0x3B, 0x2E, 0xE6, 0xD8, 0x9A, 0xCF, 0xE5, 0x3F, 0xB3, 0x4B },
        };

        constexpr const u8 KeyTypeSources[KeyType_Count][AesKeySize] = {
            [KeyType_Default]           = { 0x4D, 0x87, 0x09, 0x86, 0xC4, 0x5D, 0x20, 0x72, 0x2F, 0xBA, 0x10, 0x53, 0xDA, 0x92, 0xE8, 0xA9 },
            [KeyType_NormalOnly]        = { 0x25, 0x03, 0x31, 0xFB, 0x25, 0x26, 0x0B, 0x79, 0x8C, 0x80, 0xD2, 0x69, 0x98, 0xE2, 0x22, 0x77 },
            [KeyType_RecoveryOnly]      = { 0x76, 0x14, 0x1D, 0x34, 0x93, 0x2D, 0xE1, 0x84, 0x24, 0x7B, 0x66, 0x65, 0x55, 0x04, 0x65, 0x81 },
            [KeyType_NormalAndRecovery] = { 0xAF, 0x3D, 0xB7, 0xF3, 0x08, 0xA2, 0xD8, 0xA2, 0x08, 0xCA, 0x18, 0xA8, 0x69, 0x46, 0xC9, 0x0B },
        };

        constexpr const u8 SealKeyMasks[SealKey_Count][AesKeySize] = {
            [SealKey_LoadAesKey]                  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
            [SealKey_DecryptDeviceUniqueData]     = { 0xA2, 0xAB, 0xBF, 0x9C, 0x92, 0x2F, 0xBB, 0xE3, 0x78, 0x79, 0x9B, 0xC0, 0xCC, 0xEA, 0xA5, 0x74 },
            [SealKey_LoadLotusKey]                = { 0x57, 0xE2, 0xD9, 0x45, 0xE4, 0x92, 0xF4, 0xFD, 0xC3, 0xF9, 0x86, 0x38, 0x89, 0x78, 0x9F, 0x3C },
            [SealKey_LoadEsDeviceKey]             = { 0xE5, 0x4D, 0x9A, 0x02, 0xF0, 0x4F, 0x5F, 0xA8, 0xAD, 0x76, 0x0A, 0xF6, 0x32, 0x95, 0x59, 0xBB },
            [SealKey_ReencryptDeviceUniqueData]   = { 0x59, 0xD9, 0x31, 0xF4, 0xA7, 0x97, 0xB8, 0x14, 0x40, 0xD6, 0xA2, 0x60, 0x2B, 0xED, 0x15, 0x31 },
            [SealKey_LoadSslKey]                  = { 0xFD, 0x6A, 0x25, 0xE5, 0xD8, 0x38, 0x7F, 0x91, 0x49, 0xDA, 0xF8, 0x59, 0xA8, 0x28, 0xE6, 0x75 },
            [SealKey_LoadEsClientCertKey]         = { 0x89, 0x96, 0x43, 0x9A, 0x7C, 0xD5, 0x59, 0x55, 0x24, 0xD5, 0x24, 0x18, 0xAB, 0x6C, 0x04, 0x61 },
        };

        int PrepareMasterKey(int generation) {
            if (generation == GetKeyGeneration()) {
                return pkg1::AesKeySlot_Master;
            }

            constexpr int Slot = pkg1::AesKeySlot_Smc;
            LoadMasterKey(Slot, generation);

            return Slot;
        }

        int PrepareDeviceMasterKey(int generation) {
            if (generation == pkg1::KeyGeneration_1_0_0) {
                return pkg1::AesKeySlot_Device;
            }
            if (generation == GetKeyGeneration()) {
                return pkg1::AesKeySlot_DeviceMaster;
            }

            constexpr int Slot = pkg1::AesKeySlot_Smc;
            LoadDeviceMasterKey(Slot, generation);

            return Slot;
        }

        SmcResult GenerateAesKekImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 kek_source[AesKeySize];
            std::memcpy(kek_source, std::addressof(args.r[1]), AesKeySize);

            const int generation = std::min<int>(args.r[3] - 1, pkg1::KeyGeneration_1_0_0);

            const util::BitPack32 option = { static_cast<u32>(args.r[4]) };
            const bool is_device_unique  = option.Get<GenerateAesKekOption::IsDeviceUnique>();
            const auto key_type          = option.Get<GenerateAesKekOption::KeyTypeIndex>();
            const auto seal_key          = option.Get<GenerateAesKekOption::SealKeyIndex>();
            const u32  reserved          = option.Get<GenerateAesKekOption::Reserved>();

            /* Validate arguments. */
            SMC_R_UNLESS(reserved == 0, InvalidArgument);

            if (is_device_unique) {
                SMC_R_UNLESS(pkg1::IsValidDeviceUniqueKeyGeneration(generation), InvalidArgument);
            } else {
                SMC_R_UNLESS(pkg1::IsValidKeyGeneration(generation), InvalidArgument);
                SMC_R_UNLESS(generation <= GetKeyGeneration(),       InvalidArgument);
            }

            SMC_R_UNLESS(0 <= key_type && key_type < KeyType_Count, InvalidArgument);
            SMC_R_UNLESS(0 <= seal_key && seal_key < SealKey_Count, InvalidArgument);

            switch (key_type) {
                case KeyType_NormalOnly:   SMC_R_UNLESS(!IsRecoveryBoot(), InvalidArgument); break;
                case KeyType_RecoveryOnly: SMC_R_UNLESS( IsRecoveryBoot(), InvalidArgument); break;
                default:                                                                     break;
            }

            /* Declare temporary data storage. */
            u8 static_source[AesKeySize];
            u8 generated_key[AesKeySize];
            u8 access_key[AesKeySize];

            /* Derive the static source. */
            for (size_t i = 0; i < sizeof(static_source); ++i) {
                static_source[i] = KeyTypeSources[key_type][i] ^ SealKeyMasks[seal_key][i];
            }

            /* Get the seal key source. */
            const u8 * const seal_key_source = SealKeySources[seal_key];

            /* Get the key slot. */
            const int slot = is_device_unique ? PrepareDeviceMasterKey(generation) : PrepareMasterKey(generation);

            /* Derive a static generation kek. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, slot, static_source, sizeof(static_source));

            /* Decrypt the input with the static generation kek. */
            se::DecryptAes128(generated_key, sizeof(generated_key), pkg1::AesKeySlot_Smc, kek_source, sizeof(kek_source));

            /* Generate the seal key. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, pkg1::AesKeySlot_RandomForUserWrap, seal_key_source, AesKeySize);

            /* Seal the generated key. */
            se::EncryptAes128(access_key, sizeof(access_key), pkg1::AesKeySlot_Smc, generated_key, sizeof(generated_key));

            /* Copy the access key out. */
            std::memcpy(std::addressof(args.r[1]), access_key, sizeof(access_key));
            return SmcResult::Success;
        }

        SmcResult LoadAesKeyImpl(SmcArguments &args) {
            /* Decode arguments. */
            const int slot = args.r[1];

            u8 access_key[AesKeySize];
            std::memcpy(access_key, std::addressof(args.r[2]), sizeof(access_key));

            u8 key_source[AesKeySize];
            std::memcpy(key_source, std::addressof(args.r[4]), sizeof(key_source));

            /* Validate arguments. */
            SMC_R_UNLESS(pkg1::IsUserAesKeySlot(slot), InvalidArgument);

            /* Get the seal key source. */
            constexpr const u8 * const SealKeySource = SealKeySources[SealKey_LoadAesKey];

            /* Derive the seal key. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, pkg1::AesKeySlot_RandomForUserWrap, SealKeySource, AesKeySize);

            /* Unseal the access key. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, pkg1::AesKeySlot_Smc, access_key, sizeof(access_key));

            /* Derive the key. */
            se::SetEncryptedAesKey128(slot, pkg1::AesKeySlot_Smc, key_source, sizeof(key_source));

            return SmcResult::Success;
        }

    }

    SmcResult SmcGenerateAesKek(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, GenerateAesKekImpl);
    }

    SmcResult SmcLoadAesKey(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, LoadAesKeyImpl);
    }

    SmcResult SmcComputeAes(SmcArguments &args) {
        /* TODO */
        return SmcResult::NotImplemented;
    }

    SmcResult SmcGenerateSpecificAesKey(SmcArguments &args) {
        /* TODO */
        return SmcResult::NotImplemented;
    }

    SmcResult SmcComputeCmac(SmcArguments &args) {
        /* TODO */
        return SmcResult::NotImplemented;
    }

    SmcResult SmcLoadPreparedAesKey(SmcArguments &args) {
        /* TODO */
        return SmcResult::NotImplemented;
    }

}
