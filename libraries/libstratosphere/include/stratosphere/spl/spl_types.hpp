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

#pragma once
#include <vapours.hpp>

namespace ams::spl {

    namespace smc {

        enum class FunctionId : u32 {
            SetConfig                         = 0xC3000401,
            GetConfig                         = 0xC3000002,
            GetResult                         = 0xC3000003,
            GetResultData                     = 0xC3000404,
            ModularExponentiate               = 0xC3000E05,
            GenerateRandomBytes               = 0xC3000006,
            GenerateAesKek                    = 0xC3000007,
            LoadAesKey                        = 0xC3000008,
            ComputeAes                        = 0xC3000009,
            GenerateSpecificAesKey            = 0xC300000A,
            ComputeCmac                       = 0xC300040B,
            ReencryptDeviceUniqueData         = 0xC300D60C,
            DecryptDeviceUniqueData           = 0xC300100D,

            ModularExponentiateWithStorageKey = 0xC300060F,
            PrepareEsDeviceUniqueKey          = 0xC3000610,
            LoadPreparedAesKey                = 0xC3000011,
            PrepareCommonEsTitleKey           = 0xC3000012,

            /* Deprecated functions. */
            LoadEsDeviceKey                   = 0xC300100C,
            DecryptAndStoreGcKey              = 0xC300100E,

            /* Atmosphere functions. */
            AtmosphereIramCopy                = 0xF0000201,
            AtmosphereReadWriteRegister       = 0xF0000002,

            AtmosphereGetEmummcConfig         = 0xF0000404,
        };

        enum class Result {
            Success               = 0,
            NotImplemented        = 1,
            InvalidArgument       = 2,
            InProgress            = 3,
            NoAsyncOperation      = 4,
            InvalidAsyncOperation = 5,
            NotPermitted          = 6,
            NotInitialized        = 7,
        };

        constexpr inline ::ams::Result ConvertResult(Result smc_result) {
            /* smc::Result::Success becomes ResultSuccess() directly. */
            R_SUCCEED_IF(smc_result == smc::Result::Success);

            /* Convert to the list of known SecureMonitorErrors. */
            const auto converted = R_MAKE_NAMESPACE_RESULT(::ams::spl, static_cast<u32>(smc_result));
            R_UNLESS(spl::ResultSecureMonitorError::Includes(converted), spl::ResultUnknownSecureMonitorError());

            /* Return the error. */
            return converted;
        }

        enum class CipherMode {
            CbcEncrypt = 0,
            CbcDecrypt = 1,
            Ctr        = 2,
        };

        enum class DeviceUniqueDataMode {
            DecryptDeviceUniqueData         = 0,
            DecryptAndStoreGcKey            = 1,
            DecryptAndStoreEsDeviceKey      = 2,
            DecryptAndStoreSslKey           = 3,
            DecryptAndStoreDrmDeviceCertKey = 4,
        };

        enum class ModularExponentiateWithStorageKeyMode {
            Gc            = 0,
            Ssl           = 1,
            DrmDeviceCert = 2,
        };

        enum class EsCommonKeyType {
            TitleKey   = 0,
            ArchiveKey = 1,
        };

        struct AsyncOperationKey {
            u64 value;
        };
    }

    enum class HardwareType {
        Icosa   = 0,
        Copper  = 1,
        Hoag    = 2,
        Iowa    = 3,
        Calcio  = 4,
        _Five_  = 5,
    };

    enum SocType {
        SocType_Erista = 0,
        SocType_Mariko = 1,
    };

    enum HardwareState {
        HardwareState_Development = 0,
        HardwareState_Production  = 1,
    };

    enum MemoryArrangement {
        MemoryArrangement_Standard             = 0,
        MemoryArrangement_StandardForAppletDev = 1,
        MemoryArrangement_StandardForSystemDev = 2,
        MemoryArrangement_Expanded             = 3,
        MemoryArrangement_ExpandedForAppletDev = 4,

        /* Note: MemoryArrangement_Dynamic is not official. */
        /* Atmosphere uses it to maintain compatibility with firmwares prior to 6.0.0, */
        /* which removed the explicit retrieval of memory arrangement from PM. */
        MemoryArrangement_Dynamic              = 5,
        MemoryArrangement_Count,
    };

    struct BootReasonValue {
        union {
            struct {
                u8 power_intr;
                u8 rtc_intr;
                u8 nv_erc;
                u8 boot_reason;
            };
            u32 value;
        };
    };
    static_assert(sizeof(BootReasonValue) == sizeof(u32), "BootReasonValue definition!");

    enum BootReason {
        BootReason_Unknown   = 0,
        BootReason_AcOk      = 1,
        BootReason_OnKey     = 2,
        BootReason_RtcAlarm1 = 3,
        BootReason_RtcAlarm2 = 4,
    };

    #pragma pack(push, 1)

    struct AesKey {
        union {
            u8 data[AES_128_KEY_SIZE];
            u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
        };
    };
    static_assert(alignof(AesKey) == alignof(u8), "AesKey definition!");

    struct IvCtr {
        union {
            u8 data[AES_128_KEY_SIZE];
            u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
        };
    };
    static_assert(alignof(IvCtr) == alignof(u8), "IvCtr definition!");

    struct Cmac {
        union {
            u8 data[AES_128_KEY_SIZE];
            u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
        };
    };
    static_assert(alignof(Cmac) == alignof(u8), "Cmac definition!");

    struct AccessKey {
        union {
            u8 data[AES_128_KEY_SIZE];
            u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
        };
    };
    static_assert(alignof(AccessKey) == alignof(u8), "AccessKey definition!");

    struct KeySource {
        union {
            u8 data[AES_128_KEY_SIZE];
            u64 data64[AES_128_KEY_SIZE / sizeof(u64)];
        };
    };
    static_assert(alignof(AccessKey) == alignof(u8), "KeySource definition!");
    #pragma pack(pop)

    enum class ConfigItem : u32 {
        /* Standard config items. */
        DisableProgramVerification    = 1,
        DramId                        = 2,
        SecurityEngineInterruptNumber = 3,
        FuseVersion                   = 4,
        HardwareType                  = 5,
        HardwareState                 = 6,
        IsRecoveryBoot                = 7,
        DeviceId                      = 8,
        BootReason                    = 9,
        MemoryMode                    = 10,
        IsDevelopmentFunctionEnabled  = 11,
        KernelConfiguration           = 12,
        IsChargerHiZModeEnabled       = 13,
        QuestState                    = 14,
        RegulatorType                 = 15,
        DeviceUniqueKeyGeneration     = 16,
        Package2Hash                  = 17,

        /* Extension config items for exosphere. */
        ExosphereApiVersion     = 65000,
        ExosphereNeedsReboot    = 65001,
        ExosphereNeedsShutdown  = 65002,
        ExosphereGitCommitHash  = 65003,
        ExosphereHasRcmBugPatch = 65004,
        ExosphereBlankProdInfo  = 65005,
        ExosphereAllowCalWrites = 65006,
        ExosphereEmummcType     = 65007,
        ExospherePayloadAddress = 65008,
    };

}

/* Extensions to libnx spl config item enum. */
constexpr inline SplConfigItem SplConfigItem_ExosphereApiVersion     = static_cast<SplConfigItem>(65000);
constexpr inline SplConfigItem SplConfigItem_ExosphereNeedsReboot    = static_cast<SplConfigItem>(65001);
constexpr inline SplConfigItem SplConfigItem_ExosphereNeedsShutdown  = static_cast<SplConfigItem>(65002);
constexpr inline SplConfigItem SplConfigItem_ExosphereGitCommitHash  = static_cast<SplConfigItem>(65003);
constexpr inline SplConfigItem SplConfigItem_ExosphereHasRcmBugPatch = static_cast<SplConfigItem>(65004);
constexpr inline SplConfigItem SplConfigItem_ExosphereBlankProdInfo  = static_cast<SplConfigItem>(65005);
constexpr inline SplConfigItem SplConfigItem_ExosphereAllowCalWrites = static_cast<SplConfigItem>(65006);
constexpr inline SplConfigItem SplConfigItem_ExosphereEmummcType     = static_cast<SplConfigItem>(65007);
constexpr inline SplConfigItem SplConfigItem_ExospherePayloadAddress = static_cast<SplConfigItem>(65008);
