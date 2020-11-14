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
#include "../secmon_page_mapper.hpp"
#include "secmon_smc_aes.hpp"
#include "secmon_smc_device_unique_data.hpp"
#include "secmon_smc_se_lock.hpp"

namespace ams::secmon::smc {

    namespace {

        constexpr inline auto   AesKeySize              = se::AesBlockSize;
        constexpr inline size_t CmacSizeMax             = 1_KB;
        constexpr inline size_t DeviceUniqueDataSizeMin = 0x130;
        constexpr inline size_t DeviceUniqueDataSizeMax = 0x240;

        enum SealKey {
            SealKey_LoadAesKey                  = 0,
            SealKey_DecryptDeviceUniqueData     = 1,
            SealKey_ImportLotusKey              = 2,
            SealKey_ImportEsDeviceKey           = 3,
            SealKey_ReencryptDeviceUniqueData   = 4,
            SealKey_ImportSslKey                = 5,
            SealKey_ImportEsClientCertKey       = 6,

            SealKey_Count,
        };

        enum KeyType {
            KeyType_Default           = 0,
            KeyType_NormalOnly        = 1,
            KeyType_RecoveryOnly      = 2,
            KeyType_NormalAndRecovery = 3,

            KeyType_Count,
        };

        enum CipherMode {
            CipherMode_CbcEncryption = 0,
            CipherMode_CbcDecryption = 1,
            CipherMode_Ctr           = 2,
            CipherMode_Cmac          = 3,
        };

        enum SpecificAesKey {
            SpecificAesKey_CalibrationEncryption0 = 0,
            SpecificAesKey_CalibrationEncryption1 = 1,

            SpecificAesKey_Count,
        };

        enum DeviceUniqueData {
            DeviceUniqueData_DecryptDeviceUniqueData = 0,
            DeviceUniqueData_ImportLotusKey          = 1,
            DeviceUniqueData_ImportEsDeviceKey       = 2,
            DeviceUniqueData_ImportSslKey            = 3,
            DeviceUniqueData_ImportEsClientCertKey   = 4,

            DeviceUniqueData_Count,
        };

        /* Ensure that our "subtract one" simplification is valid for the cases we care about. */
        static_assert(DeviceUniqueData_ImportLotusKey        - 1 == ImportRsaKey_Lotus);
        static_assert(DeviceUniqueData_ImportEsDeviceKey     - 1 == ImportRsaKey_EsDrmCert);
        static_assert(DeviceUniqueData_ImportSslKey          - 1 == ImportRsaKey_Ssl);
        static_assert(DeviceUniqueData_ImportEsClientCertKey - 1 == ImportRsaKey_EsClientCert);

        constexpr ImportRsaKey ConvertToImportRsaKey(DeviceUniqueData data) {
            /* Not necessary, but if this is invoked at compile-time this will force a compile-time error. */
            AMS_ASSUME(data != DeviceUniqueData_DecryptDeviceUniqueData);
            AMS_ASSUME(data <  DeviceUniqueData_Count);

            return static_cast<ImportRsaKey>(static_cast<int>(data) - 1);
        }

        enum SecureData {
            SecureData_Calibration                = 0,
            SecureData_SafeMode                   = 1,
            SecureData_UserSystemProperEncryption = 2,
            SecureData_UserSystem                 = 3,

            SecureData_Count,
        };

        struct GenerateAesKekOption {
            using IsDeviceUnique = util::BitPack32::Field<0,  1, bool>;
            using KeyTypeIndex   = util::BitPack32::Field<1,  4, KeyType>;
            using SealKeyIndex   = util::BitPack32::Field<5,  3, SealKey>;
            using Reserved       = util::BitPack32::Field<8, 24, u32>;
        };

        struct ComputeAesOption {
            using KeySlot         = util::BitPack32::Field<0, 3, int>;
            using CipherModeIndex = util::BitPack32::Field<4, 2, CipherMode>;
        };

        struct DecryptDeviceUniqueDataOption {
            using DeviceUniqueDataIndex = util::BitPack32::Field<0,  3, DeviceUniqueData>;
            using Reserved              = util::BitPack32::Field<3, 29, u32>;

            /* Legacy. */
            using EnforceDeviceUnique   = util::BitPack32::Field<0,  1, bool>;
        };

        constexpr const u8 SealKeySources[SealKey_Count][AesKeySize] = {
            [SealKey_LoadAesKey]                  = { 0xF4, 0x0C, 0x16, 0x26, 0x0D, 0x46, 0x3B, 0xE0, 0x8C, 0x6A, 0x56, 0xE5, 0x82, 0xD4, 0x1B, 0xF6 },
            [SealKey_DecryptDeviceUniqueData]     = { 0x7F, 0x54, 0x2C, 0x98, 0x1E, 0x54, 0x18, 0x3B, 0xBA, 0x63, 0xBD, 0x4C, 0x13, 0x5B, 0xF1, 0x06 },
            [SealKey_ImportLotusKey]              = { 0xC7, 0x3F, 0x73, 0x60, 0xB7, 0xB9, 0x9D, 0x74, 0x0A, 0xF8, 0x35, 0x60, 0x1A, 0x18, 0x74, 0x63 },
            [SealKey_ImportEsDeviceKey]           = { 0x0E, 0xE0, 0xC4, 0x33, 0x82, 0x66, 0xE8, 0x08, 0x39, 0x13, 0x41, 0x7D, 0x04, 0x64, 0x2B, 0x6D },
            [SealKey_ReencryptDeviceUniqueData]   = { 0xE1, 0xA8, 0xAA, 0x6A, 0x2D, 0x9C, 0xDE, 0x43, 0x0C, 0xDE, 0xC6, 0x17, 0xF6, 0xC7, 0xF1, 0xDE },
            [SealKey_ImportSslKey]                = { 0x74, 0x20, 0xF6, 0x46, 0x77, 0xB0, 0x59, 0x2C, 0xE8, 0x1B, 0x58, 0x64, 0x47, 0x41, 0x37, 0xD9 },
            [SealKey_ImportEsClientCertKey]       = { 0xAA, 0x19, 0x0F, 0xFA, 0x4C, 0x30, 0x3B, 0x2E, 0xE6, 0xD8, 0x9A, 0xCF, 0xE5, 0x3F, 0xB3, 0x4B },
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
            [SealKey_ImportLotusKey]              = { 0x57, 0xE2, 0xD9, 0x45, 0xE4, 0x92, 0xF4, 0xFD, 0xC3, 0xF9, 0x86, 0x38, 0x89, 0x78, 0x9F, 0x3C },
            [SealKey_ImportEsDeviceKey]           = { 0xE5, 0x4D, 0x9A, 0x02, 0xF0, 0x4F, 0x5F, 0xA8, 0xAD, 0x76, 0x0A, 0xF6, 0x32, 0x95, 0x59, 0xBB },
            [SealKey_ReencryptDeviceUniqueData]   = { 0x59, 0xD9, 0x31, 0xF4, 0xA7, 0x97, 0xB8, 0x14, 0x40, 0xD6, 0xA2, 0x60, 0x2B, 0xED, 0x15, 0x31 },
            [SealKey_ImportSslKey]                = { 0xFD, 0x6A, 0x25, 0xE5, 0xD8, 0x38, 0x7F, 0x91, 0x49, 0xDA, 0xF8, 0x59, 0xA8, 0x28, 0xE6, 0x75 },
            [SealKey_ImportEsClientCertKey]       = { 0x89, 0x96, 0x43, 0x9A, 0x7C, 0xD5, 0x59, 0x55, 0x24, 0xD5, 0x24, 0x18, 0xAB, 0x6C, 0x04, 0x61 },
        };

        constexpr const SealKey DeviceUniqueDataToSealKey[DeviceUniqueData_Count] = {
            [DeviceUniqueData_DecryptDeviceUniqueData] = SealKey_DecryptDeviceUniqueData,
            [DeviceUniqueData_ImportLotusKey]          = SealKey_ImportLotusKey,
            [DeviceUniqueData_ImportEsDeviceKey]       = SealKey_ImportEsDeviceKey,
            [DeviceUniqueData_ImportSslKey]            = SealKey_ImportSslKey,
            [DeviceUniqueData_ImportEsClientCertKey]   = SealKey_ImportEsClientCertKey,
        };

        constexpr const u8 CalibrationKeySource[AesKeySize] = {
            0xE2, 0xD6, 0xB8, 0x7A, 0x11, 0x9C, 0xB8, 0x80, 0xE8, 0x22, 0x88, 0x8A, 0x46, 0xFB, 0xA1, 0x95
        };

        constexpr const u8 EsCommonKeySources[EsCommonKeyType_Count][AesKeySize] = {
            [EsCommonKeyType_TitleKey]   = { 0x1E, 0xDC, 0x7B, 0x3B, 0x60, 0xE6, 0xB4, 0xD8, 0x78, 0xB8, 0x17, 0x15, 0x98, 0x5E, 0x62, 0x9B },
            [EsCommonKeyType_ArchiveKey] = { 0x3B, 0x78, 0xF2, 0x61, 0x0F, 0x9D, 0x5A, 0xE2, 0x7B, 0x4E, 0x45, 0xAF, 0xCB, 0x0B, 0x67, 0x4D },
        };

        constexpr const u8 EsSealKeySource[AesKeySize] = {
            0xCB, 0xB7, 0x6E, 0x38, 0xA1, 0xCB, 0x77, 0x0F, 0xB2, 0xA5, 0xB2, 0x9D, 0xD8, 0x56, 0x9F, 0x76
        };

        constexpr const u8 SecureDataSource[AesKeySize] = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        constexpr const u8 SecureDataCounters[][AesKeySize] = {
            [SecureData_Calibration]                = { 0x3C, 0xD5, 0x92, 0xEC, 0x68, 0x31, 0x4A, 0x06, 0xD4, 0x1B, 0x0C, 0xD9, 0xF6, 0x2E, 0xD9, 0xE9 },
            [SecureData_SafeMode]                   = { 0x50, 0x81, 0xCF, 0x77, 0x18, 0x11, 0xD7, 0x0D, 0x13, 0x29, 0x60, 0xED, 0x4B, 0x21, 0x3E, 0xFC },
            [SecureData_UserSystemProperEncryption] = { 0x98, 0xCB, 0x4C, 0xEB, 0x15, 0xF1, 0x4A, 0x5A, 0x7A, 0x86, 0xB6, 0xF1, 0x94, 0x66, 0xF4, 0x9D },
        };

        constexpr const u8 SecureDataTweaks[][AesKeySize] = {
            [SecureData_Calibration]                = { 0xAC, 0xCA, 0x9A, 0xCA, 0xFF, 0x2E, 0xB9, 0x22, 0xCC, 0x1F, 0x4F, 0xAD, 0xDD, 0x77, 0x21, 0x1E },
            [SecureData_SafeMode]                   = { 0x6E, 0xF8, 0x2A, 0x1A, 0xE0, 0x4F, 0xC3, 0x20, 0x08, 0x7B, 0xBA, 0x50, 0xC0, 0xCD, 0x7B, 0x39 },
            [SecureData_UserSystemProperEncryption] = { 0x6D, 0x02, 0x56, 0x2D, 0xF4, 0x3D, 0x0A, 0x15, 0xB1, 0x34, 0x5C, 0xC2, 0x84, 0x4C, 0xD4, 0x28 },
        };

        constexpr const u8 *GetSecureDataCounter(SecureData which) {
            switch (which) {
                case SecureData_Calibration:
                    return SecureDataCounters[SecureData_Calibration];
                case SecureData_SafeMode:
                    return SecureDataCounters[SecureData_SafeMode];
                case SecureData_UserSystem:
                case SecureData_UserSystemProperEncryption:
                    return SecureDataCounters[SecureData_UserSystemProperEncryption];
                default:
                    return nullptr;
            }
        }

        constexpr const u8 *GetSecureDataTweak(SecureData which) {
            switch (which) {
                case SecureData_Calibration:
                    return SecureDataTweaks[SecureData_Calibration];
                case SecureData_SafeMode:
                    return SecureDataTweaks[SecureData_SafeMode];
                case SecureData_UserSystem:
                case SecureData_UserSystemProperEncryption:
                    return SecureDataTweaks[SecureData_UserSystemProperEncryption];
                default:
                    return nullptr;
            }
        }

        constexpr uintptr_t LinkedListAddressMinimum   = secmon::MemoryRegionDram.GetAddress();
        constexpr size_t    LinkedListAddressRangeSize = 4_MB - 2_KB;
        constexpr uintptr_t LinkedListAddressMaximum   = LinkedListAddressMinimum + LinkedListAddressRangeSize;

        constexpr size_t    LinkedListSize = 12;

        constexpr bool IsValidLinkedListAddress(uintptr_t address) {
            return LinkedListAddressMinimum <= address && address <= (LinkedListAddressMaximum - LinkedListSize);
        }

        constinit bool g_is_compute_aes_completed = false;

        void SecurityEngineDoneHandler() {
            /* Check that the compute succeeded. */
            se::ValidateAesOperationResult();

            /* End the asynchronous operation. */
            g_is_compute_aes_completed = true;
            EndAsyncOperation();
        }

        SmcResult GetComputeAesResult(void *dst, size_t size) {
            /* Arguments are unused. */
            AMS_UNUSED(dst);
            AMS_UNUSED(size);

            /* Check that the operation is completed. */
            SMC_R_UNLESS(g_is_compute_aes_completed, Busy);

            /* Unlock the security engine and succeed. */
            UnlockSecurityEngine();
            return SmcResult::Success;
        }

        int PrepareMasterKey(int generation) {
            if (generation == GetKeyGeneration()) {
                return pkg1::AesKeySlot_Master;
            }

            constexpr int Slot = pkg1::AesKeySlot_Smc;
            LoadMasterKey(Slot, generation);

            return Slot;
        }

        int PrepareDeviceMasterKey(int generation) {
            if (generation == pkg1::KeyGeneration_1_0_0 && GetSocType() == fuse::SocType_Erista) {
                return pkg1::AesKeySlot_Device;
            }
            if (generation == GetKeyGeneration()) {
                return pkg1::AesKeySlot_DeviceMaster;
            }

            constexpr int Slot = pkg1::AesKeySlot_Smc;
            LoadDeviceMasterKey(Slot, generation);

            return Slot;
        }

        void GetSecureDataImpl(u8 *dst, SecureData which, bool tweak) {
            /* Compute the appropriate AES-CTR. */
            se::ComputeAes128Ctr(dst, AesKeySize, pkg1::AesKeySlot_Device, SecureDataSource, AesKeySize, GetSecureDataCounter(which), AesKeySize);

            /* Tweak, if we should. */
            if (tweak) {
                const u8 * const tweak = GetSecureDataTweak(which);

                for (size_t i = 0; i < AesKeySize; ++i) {
                    dst[i] ^= tweak[i];
                }
            }
        }

        SmcResult GenerateAesKekImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 kek_source[AesKeySize];
            std::memcpy(kek_source, std::addressof(args.r[1]), AesKeySize);

            const int generation = std::max<int>(static_cast<int>(args.r[3]) - 1, pkg1::KeyGeneration_1_0_0);

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

        SmcResult ComputeAesImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 iv[se::AesBlockSize];

            const util::BitPack32 option = { static_cast<u32>(args.r[1]) };
            std::memcpy(iv, std::addressof(args.r[2]), sizeof(iv));
            const u32 input_address  = args.r[4];
            const u32 output_address = args.r[5];
            const u32 size           = args.r[6];

            const int  slot        = option.Get<ComputeAesOption::KeySlot>();
            const auto cipher_mode = option.Get<ComputeAesOption::CipherModeIndex>();

            /* Validate arguments. */
            SMC_R_UNLESS(pkg1::IsUserAesKeySlot(slot),             InvalidArgument);
            SMC_R_UNLESS(util::IsAligned(size, se::AesBlockSize),  InvalidArgument);
            SMC_R_UNLESS(IsValidLinkedListAddress(input_address),  InvalidArgument);
            SMC_R_UNLESS(IsValidLinkedListAddress(output_address), InvalidArgument);

            /* We're starting an aes operation, so reset the completion status. */
            g_is_compute_aes_completed = false;

            /* Dispatch the correct aes operation asynchronously. */
            switch (cipher_mode) {
                case CipherMode_CbcEncryption: se::EncryptAes128CbcAsync(output_address, slot, input_address, size, iv, sizeof(iv), SecurityEngineDoneHandler); break;
                case CipherMode_CbcDecryption: se::DecryptAes128CbcAsync(output_address, slot, input_address, size, iv, sizeof(iv), SecurityEngineDoneHandler); break;
                case CipherMode_Ctr:           se::ComputeAes128CtrAsync(output_address, slot, input_address, size, iv, sizeof(iv), SecurityEngineDoneHandler); break;
                case CipherMode_Cmac:
                    return SmcResult::NotImplemented;
                default:
                    return SmcResult::InvalidArgument;
            }

            return SmcResult::Success;
        }

        SmcResult GenerateSpecificAesKeyImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 key_source[AesKeySize];
            std::memcpy(key_source, std::addressof(args.r[1]), sizeof(key_source));

            const int generation = GetTargetFirmware() >= TargetFirmware_4_0_0 ? std::max<int>(static_cast<int>(args.r[3]) - 1, pkg1::KeyGeneration_1_0_0) : pkg1::KeyGeneration_1_0_0;
            const auto which     = static_cast<SpecificAesKey>(args.r[4]);

            /* Validate arguments. */
            SMC_R_UNLESS(pkg1::IsValidKeyGeneration(generation), InvalidArgument);
            SMC_R_UNLESS(which < SpecificAesKey_Count,           InvalidArgument);

            /* Generate the specific aes key. */
            u8 output_key[AesKeySize];
            if (fuse::GetPatchVersion() >= fuse::PatchVersion_Odnx02A2) {
                const int slot = PrepareDeviceMasterKey(generation);
                se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, slot, CalibrationKeySource, sizeof(CalibrationKeySource));
                se::DecryptAes128(output_key, sizeof(output_key), pkg1::AesKeySlot_Smc, key_source, sizeof(key_source));
            } else {
                GetSecureDataImpl(output_key, SecureData_Calibration, which == SpecificAesKey_CalibrationEncryption1);
            }

            /* Copy the key to output. */
            std::memcpy(std::addressof(args.r[1]), output_key, sizeof(output_key));
            return SmcResult::Success;
        }

        SmcResult ComputeCmacImpl(SmcArguments &args) {
            /* Decode arguments. */
            const int      slot          = args.r[1];
            const uintptr_t data_address = args.r[2];
            const size_t    data_size    = args.r[3];

            /* Declare buffer for user data. */
            alignas(8) u8 user_data[CmacSizeMax];

            /* Validate arguments. */
            SMC_R_UNLESS(pkg1::IsUserAesKeySlot(slot),   InvalidArgument);
            SMC_R_UNLESS(data_size <= sizeof(user_data), InvalidArgument);

            /* Map the user data, and copy to stack. */
            {
                UserPageMapper mapper(data_address);
                SMC_R_UNLESS(mapper.Map(),                                            InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(user_data, data_address, data_size), InvalidArgument);
            }

            /* Ensure the SE sees consistent data. */
            hw::FlushDataCache(user_data, data_size);
            hw::DataSynchronizationBarrierInnerShareable();

            /* Compute the mac. */
            {
                u8 mac[se::AesBlockSize];
                se::ComputeAes128Cmac(mac, sizeof(mac), slot, user_data, data_size);

                std::memcpy(std::addressof(args.r[1]), mac, sizeof(mac));
            }

            return SmcResult::Success;
        }

        SmcResult LoadPreparedAesKeyImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 access_key[AesKeySize];

            const int slot = args.r[1];
            std::memcpy(access_key, std::addressof(args.r[2]), sizeof(access_key));

            /* Validate arguments. */
            SMC_R_UNLESS(pkg1::IsUserAesKeySlot(slot), InvalidArgument);

            /* Derive the seal key. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, pkg1::AesKeySlot_RandomForUserWrap, EsSealKeySource, sizeof(EsSealKeySource));

            /* Unseal the key. */
            se::SetEncryptedAesKey128(slot, pkg1::AesKeySlot_Smc, access_key, sizeof(access_key));

            return SmcResult::Success;
        }

        SmcResult PrepareEsCommonTitleKeyImpl(SmcArguments &args) {
            /* Declare variables. */
            u8 key_source[se::AesBlockSize];
            u8 key[se::AesBlockSize];
            u8 access_key[se::AesBlockSize];

            /* Decode arguments. */
            std::memcpy(key_source, std::addressof(args.r[1]), sizeof(key_source));
            const int generation = GetTargetFirmware() >= TargetFirmware_3_0_0 ? std::max<int>(pkg1::KeyGeneration_1_0_0, static_cast<int>(args.r[3]) - 1) : pkg1::KeyGeneration_1_0_0;

            /* Validate arguments. */
            SMC_R_UNLESS(pkg1::IsValidKeyGeneration(generation), InvalidArgument);
            SMC_R_UNLESS(generation <= GetKeyGeneration(),       InvalidArgument);

            /* Derive the key. */
            DecryptWithEsCommonKey(key, sizeof(key), key_source, sizeof(key_source), EsCommonKeyType_TitleKey, generation);

            /* Prepare the aes key. */
            PrepareEsAesKey(access_key, sizeof(access_key), key, sizeof(key));

            /* Copy the access key to output. */
            std::memcpy(std::addressof(args.r[1]), access_key, sizeof(access_key));

            return SmcResult::Success;
        }

        constexpr size_t GetDiscountedMinimumDeviceUniqueDataSize(bool enforce_device_unique) {
            if (enforce_device_unique) {
                return 0;
            } else {
                return DeviceUniqueDataTotalMetaSize - DeviceUniqueDataIvSize;
            }
        }

        SmcResult ValidateDeviceUniqueDataSize(DeviceUniqueData mode, size_t data_size, bool enforce_device_unique) {
            /* Determine the discounted size towards the minimum. */
            const size_t discounted_size = GetDiscountedMinimumDeviceUniqueDataSize(enforce_device_unique);
            SMC_R_UNLESS(enforce_device_unique || fuse::GetPatchVersion() < fuse::PatchVersion_Odnx02A2, InvalidArgument);

            switch (mode) {
                case DeviceUniqueData_DecryptDeviceUniqueData:
                    {
                        SMC_R_UNLESS(DeviceUniqueDataTotalMetaSize - discounted_size < data_size && data_size <= DeviceUniqueDataSizeMax, InvalidArgument);
                    }
                    break;
                case DeviceUniqueData_ImportLotusKey:
                case DeviceUniqueData_ImportEsDeviceKey:
                case DeviceUniqueData_ImportSslKey:
                case DeviceUniqueData_ImportEsClientCertKey:
                    {
                        SMC_R_UNLESS(DeviceUniqueDataSizeMin - discounted_size <= data_size && data_size <= DeviceUniqueDataSizeMax, InvalidArgument);
                    }
                    break;
                default:
                    return SmcResult::InvalidArgument;
            }

            return SmcResult::Success;
        }

        SmcResult DecryptDeviceUniqueDataImpl(const u8 *access_key, const u8 *key_source, const DeviceUniqueData mode, const uintptr_t data_address, const size_t data_size, bool enforce_device_unique) {
            /* Validate arguments. */
            SMC_R_TRY(ValidateDeviceUniqueDataSize(mode, data_size, enforce_device_unique));

            /* Decrypt the device unique data. */
            alignas(8) u8 work_buffer[DeviceUniqueDataSizeMax];
            ON_SCOPE_EXIT { crypto::ClearMemory(work_buffer, sizeof(work_buffer)); };
            {
                /* Map and copy in the encrypted data. */
                UserPageMapper mapper(data_address);
                SMC_R_UNLESS(mapper.Map(),                                              InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(work_buffer, data_address, data_size), InvalidArgument);

                /* Determine the seal key to use. */
                const auto seal_key_type         = DeviceUniqueDataToSealKey[mode];
                const u8 * const seal_key_source = SealKeySources[seal_key_type];

                /* Decrypt the data. */
                if (!DecryptDeviceUniqueData(work_buffer, data_size, nullptr, seal_key_source, se::AesBlockSize, access_key, se::AesBlockSize, key_source, se::AesBlockSize, work_buffer, data_size, enforce_device_unique)) {
                    return SmcResult::InvalidArgument;
                }

                /* Either output the key, or import it. */
                switch (mode) {
                    case DeviceUniqueData_DecryptDeviceUniqueData:
                        {
                            SMC_R_UNLESS(mapper.CopyToUser(data_address, work_buffer, data_size), InvalidArgument);
                        }
                        break;
                    case DeviceUniqueData_ImportLotusKey:
                    case DeviceUniqueData_ImportSslKey:
                        ImportRsaKeyExponent(ConvertToImportRsaKey(mode), work_buffer, se::RsaSize);
                        break;
                    case DeviceUniqueData_ImportEsDeviceKey:
                    case DeviceUniqueData_ImportEsClientCertKey:
                        ImportRsaKeyExponent(ConvertToImportRsaKey(mode), work_buffer, se::RsaSize);
                        ImportRsaKeyModulusProvisionally(ConvertToImportRsaKey(mode), work_buffer + se::RsaSize, se::RsaSize);
                        CommitRsaKeyModulus(ConvertToImportRsaKey(mode));
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
            }

            return SmcResult::Success;
        }

        SmcResult DecryptDeviceUniqueDataImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 access_key[se::AesBlockSize];
            u8 key_source[se::AesBlockSize];

            std::memcpy(access_key, std::addressof(args.r[1]), sizeof(access_key));
            const util::BitPack32 option = { static_cast<u32>(args.r[3]) };
            const uintptr_t data_address = args.r[4];
            const size_t    data_size    = args.r[5];
            std::memcpy(key_source, std::addressof(args.r[6]), sizeof(key_source));

            const auto mode     = GetTargetFirmware() >= TargetFirmware_5_0_0 ? option.Get<DecryptDeviceUniqueDataOption::DeviceUniqueDataIndex>() : DeviceUniqueData_DecryptDeviceUniqueData;
            const auto reserved = option.Get<DecryptDeviceUniqueDataOption::Reserved>();

            const bool enforce_device_unique = GetTargetFirmware() >= TargetFirmware_5_0_0 ? true : option.Get<DecryptDeviceUniqueDataOption::EnforceDeviceUnique>();

            /* Validate arguments. */
            SMC_R_UNLESS(reserved == 0, InvalidArgument);

            /* Decrypt the device unique data. */
            return DecryptDeviceUniqueDataImpl(access_key, key_source, mode, data_address, data_size, enforce_device_unique);
        }

        SmcResult DecryptAndImportEsDeviceKeyImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 access_key[se::AesBlockSize];
            u8 key_source[se::AesBlockSize];

            std::memcpy(access_key, std::addressof(args.r[1]), sizeof(access_key));
            const util::BitPack32 option = { static_cast<u32>(args.r[3]) };
            const uintptr_t data_address = args.r[4];
            const size_t    data_size    = args.r[5];
            std::memcpy(key_source, std::addressof(args.r[6]), sizeof(key_source));

            const auto mode     = DeviceUniqueData_ImportEsDeviceKey;
            const auto reserved = option.Get<DecryptDeviceUniqueDataOption::Reserved>();

            const bool enforce_device_unique = option.Get<DecryptDeviceUniqueDataOption::EnforceDeviceUnique>();

            /* Validate arguments. */
            SMC_R_UNLESS(reserved == 0, InvalidArgument);

            /* Ensure that the key is exactly the correct size. */
            if (enforce_device_unique) {
                SMC_R_UNLESS(data_size == util::AlignUp(2 * se::RsaSize + sizeof(u32), se::AesBlockSize) + DeviceUniqueDataTotalMetaSize, InvalidArgument);
            } else {
                SMC_R_UNLESS(data_size == util::AlignUp(2 * se::RsaSize + sizeof(u32), se::AesBlockSize) + DeviceUniqueDataIvSize,        InvalidArgument);
            }

            /* Decrypt the device unique data. */
            return DecryptDeviceUniqueDataImpl(access_key, key_source, mode, data_address, data_size, enforce_device_unique);
        }

        SmcResult DecryptAndImportLotusKeyImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 access_key[se::AesBlockSize];
            u8 key_source[se::AesBlockSize];

            std::memcpy(access_key, std::addressof(args.r[1]), sizeof(access_key));
            const util::BitPack32 option = { static_cast<u32>(args.r[3]) };
            const uintptr_t data_address = args.r[4];
            const size_t    data_size    = args.r[5];
            std::memcpy(key_source, std::addressof(args.r[6]), sizeof(key_source));

            const auto mode     = DeviceUniqueData_ImportLotusKey;
            const auto reserved = option.Get<DecryptDeviceUniqueDataOption::Reserved>();

            const bool enforce_device_unique = option.Get<DecryptDeviceUniqueDataOption::EnforceDeviceUnique>();

            /* Validate arguments. */
            SMC_R_UNLESS(reserved == 0, InvalidArgument);

            /* Ensure that the key is exactly the correct size. */
            if (enforce_device_unique) {
                SMC_R_UNLESS(data_size == se::RsaSize + DeviceUniqueDataTotalMetaSize, InvalidArgument);
            } else {
                SMC_R_UNLESS(data_size == se::RsaSize + DeviceUniqueDataIvSize,        InvalidArgument);
            }

            /* Decrypt the device unique data. */
            return DecryptDeviceUniqueDataImpl(access_key, key_source, mode, data_address, data_size, enforce_device_unique);
        }

        SmcResult ReencryptDeviceUniqueDataImpl(SmcArguments &args) {
            /* Decode arguments. */
            u8 access_key_dec[se::AesBlockSize];
            u8 access_key_enc[se::AesBlockSize];
            u8 key_source_dec[se::AesBlockSize];
            u8 key_source_enc[se::AesBlockSize];

            const uintptr_t access_key_dec_address = args.r[1];
            const uintptr_t access_key_enc_address = args.r[2];
            const util::BitPack32 option = { static_cast<u32>(args.r[3]) };
            const uintptr_t data_address = args.r[4];
            const size_t    data_size    = args.r[5];
            const uintptr_t key_source_dec_address = args.r[6];
            const uintptr_t key_source_enc_address = args.r[7];

            const auto mode     = option.Get<DecryptDeviceUniqueDataOption::DeviceUniqueDataIndex>();
            const auto reserved = option.Get<DecryptDeviceUniqueDataOption::Reserved>();

            const bool enforce_device_unique = true;

            /* Validate arguments. */
            SMC_R_UNLESS(reserved == 0, InvalidArgument);
            SMC_R_TRY(ValidateDeviceUniqueDataSize(mode, data_size, enforce_device_unique));

            /* Decrypt the device unique data. */
            alignas(8) u8 work_buffer[DeviceUniqueDataSizeMax];
            ON_SCOPE_EXIT { crypto::ClearMemory(work_buffer, sizeof(work_buffer)); };
            {
                /* Map and copy in the encrypted data. */
                UserPageMapper mapper(data_address);
                SMC_R_UNLESS(mapper.Map(),                                                                        InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(work_buffer, data_address, data_size),                           InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(access_key_dec, access_key_dec_address, sizeof(access_key_dec)), InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(access_key_enc, access_key_enc_address, sizeof(access_key_enc)), InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(key_source_dec, key_source_dec_address, sizeof(key_source_dec)), InvalidArgument);
                SMC_R_UNLESS(mapper.CopyFromUser(key_source_enc, key_source_enc_address, sizeof(key_source_enc)), InvalidArgument);

                /* Decrypt the data. */
                u8 device_id_high;
                {
                    /* Determine the seal key to use. */
                    const u8 * const seal_key_source = SealKeySources[SealKey_ReencryptDeviceUniqueData];

                    if (!DecryptDeviceUniqueData(work_buffer, data_size, std::addressof(device_id_high), seal_key_source, se::AesBlockSize, access_key_dec, sizeof(access_key_dec), key_source_dec, sizeof(key_source_dec), work_buffer, data_size, enforce_device_unique)) {
                        return SmcResult::InvalidArgument;
                    }
                }

                /* Reencrypt the data. */
                {
                    /* Determine the seal key to use. */
                    const auto seal_key_type         = DeviceUniqueDataToSealKey[mode];
                    const u8 * const seal_key_source = SealKeySources[seal_key_type];

                    /* Encrypt the data. */
                    EncryptDeviceUniqueData(work_buffer, data_size, seal_key_source, se::AesBlockSize, access_key_enc, sizeof(access_key_enc), key_source_enc, sizeof(key_source_enc), work_buffer, data_size - DeviceUniqueDataTotalMetaSize, device_id_high);
                }

                /* Copy the reencrypted data back to user. */
                SMC_R_UNLESS(mapper.CopyToUser(data_address, work_buffer, data_size), InvalidArgument);
            }

            return SmcResult::Success;
        }

        SmcResult GetSecureDataImpl(SmcArguments &args) {
            /* Decode arguments. */
            const auto which = static_cast<SecureData>(args.r[1]);

            /* Validate arguments/conditions. */
            SMC_R_UNLESS(fuse::GetPatchVersion() < fuse::PatchVersion_Odnx02A2, NotImplemented);
            SMC_R_UNLESS(which < SecureData_Count,                              NotImplemented);

            /* Use a temporary buffer. */
            u8 secure_data[AesKeySize];
            GetSecureDataImpl(secure_data, which, false);

            /* Copy out. */
            std::memcpy(std::addressof(args.r[1]), secure_data, sizeof(secure_data));
            return SmcResult::Success;
        }

    }

    /* Aes functionality. */
    SmcResult SmcGenerateAesKek(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, GenerateAesKekImpl);
    }

    SmcResult SmcLoadAesKey(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, LoadAesKeyImpl);
    }

    SmcResult SmcComputeAes(SmcArguments &args) {
        return LockSecurityEngineAndInvokeAsync(args, ComputeAesImpl, GetComputeAesResult);
    }

    SmcResult SmcGenerateSpecificAesKey(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, GenerateSpecificAesKeyImpl);
    }

    SmcResult SmcComputeCmac(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, ComputeCmacImpl);
    }

    SmcResult SmcLoadPreparedAesKey(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, LoadPreparedAesKeyImpl);
    }

    SmcResult SmcPrepareEsCommonTitleKey(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, PrepareEsCommonTitleKeyImpl);
    }

    /* Device unique data functionality. */
    SmcResult SmcDecryptDeviceUniqueData(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, DecryptDeviceUniqueDataImpl);
    }

    SmcResult SmcReencryptDeviceUniqueData(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, ReencryptDeviceUniqueDataImpl);
    }

    /* Legacy APIs. */
    SmcResult SmcDecryptAndImportEsDeviceKey(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, DecryptAndImportEsDeviceKeyImpl);
    }

    SmcResult SmcDecryptAndImportLotusKey(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, DecryptAndImportLotusKeyImpl);
    }

    /* Es encryption utilities. */
    void DecryptWithEsCommonKey(void *dst, size_t dst_size, const void *src, size_t src_size, EsCommonKeyType type, int generation) {
        /* Validate pre-conditions. */
        AMS_ABORT_UNLESS(dst_size == AesKeySize);
        AMS_ABORT_UNLESS(src_size == AesKeySize);
        AMS_ABORT_UNLESS(0 <= type && type < EsCommonKeyType_Count);

        /* Prepare the master key for the generation. */
        const int slot = PrepareMasterKey(generation);

        /* Derive the es common key. */
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, slot, EsCommonKeySources[type], AesKeySize);

        /* Decrypt the input using the common key. */
        se::DecryptAes128(dst, dst_size, pkg1::AesKeySlot_Smc, src, src_size);
    }

    void PrepareEsAesKey(void *dst, size_t dst_size, const void *src, size_t src_size) {
        /* Validate pre-conditions. */
        AMS_ABORT_UNLESS(dst_size == AesKeySize);
        AMS_ABORT_UNLESS(src_size == AesKeySize);

        /* Derive the seal key. */
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_Smc, pkg1::AesKeySlot_RandomForUserWrap, EsSealKeySource, sizeof(EsSealKeySource));

        /* Seal the key. */
        se::EncryptAes128(dst, dst_size, pkg1::AesKeySlot_Smc, src, src_size);
    }

    /* 'Tis the last rose of summer, / Left blooming alone;    */
    /* Oh! who would inhabit         / This bleak world alone? */
    SmcResult SmcGetSecureData(SmcArguments &args) {
        return LockSecurityEngineAndInvoke(args, GetSecureDataImpl);
    }

}
