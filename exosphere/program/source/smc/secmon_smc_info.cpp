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
#include "../secmon_map.hpp"
#include "../secmon_misc.hpp"
#include "../secmon_page_mapper.hpp"
#include "../secmon_user_power_management.hpp"
#include "secmon_smc_info.hpp"
#include "secmon_smc_power_management.hpp"

namespace ams::secmon::smc {

    namespace {

        struct KernelConfiguration {
            /* Secure Monitor view. */
            using Flags1             = util::BitPack32::Field< 0, 8>;
            using Flags0             = util::BitPack32::Field< 8, 8>;
            using PhysicalMemorySize = util::BitPack32::Field<16, 2>;

            /* Kernel view, from libmesosphere. */
            using DebugFillMemory             = util::BitPack32::Field<0,                                 1, bool>;
            using EnableUserExceptionHandlers = util::BitPack32::Field<DebugFillMemory::Next,             1, bool>;
            using EnableUserPmuAccess         = util::BitPack32::Field<EnableUserExceptionHandlers::Next, 1, bool>;
            using IncreaseThreadResourceLimit = util::BitPack32::Field<EnableUserPmuAccess::Next,         1, bool>;
            using Reserved4                   = util::BitPack32::Field<IncreaseThreadResourceLimit::Next, 4, u32>;
            using UseSecureMonitorPanicCall   = util::BitPack32::Field<Reserved4::Next,                   1, bool>;
            using Reserved9                   = util::BitPack32::Field<UseSecureMonitorPanicCall::Next,   7, u32>;
            using MemorySize                  = util::BitPack32::Field<Reserved9::Next,                   2, u32>; /* smc::MemorySize = pkg1::MemorySize */
        };

        constexpr const pkg1::MemorySize DramIdToMemorySize[fuse::DramId_Count] = {
            [fuse::DramId_IcosaSamsung4GB]    = pkg1::MemorySize_4GB,
            [fuse::DramId_IcosaHynix4GB]      = pkg1::MemorySize_4GB,
            [fuse::DramId_IcosaMicron4GB]     = pkg1::MemorySize_4GB,
            [fuse::DramId_FiveHynix1y4GB]     = pkg1::MemorySize_4GB,
            [fuse::DramId_IcosaSamsung6GB]    = pkg1::MemorySize_6GB,
            [fuse::DramId_CopperHynix4GB]     = pkg1::MemorySize_4GB,
            [fuse::DramId_CopperMicron4GB]    = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaX1X2Samsung4GB] = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaSansung4GB]     = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaSamsung8GB]     = pkg1::MemorySize_8GB,
            [fuse::DramId_IowaHynix4GB]       = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaMicron4GB]      = pkg1::MemorySize_4GB,
            [fuse::DramId_HoagSamsung4GB]     = pkg1::MemorySize_4GB,
            [fuse::DramId_HoagSamsung8GB]     = pkg1::MemorySize_8GB,
            [fuse::DramId_HoagHynix4GB]       = pkg1::MemorySize_4GB,
            [fuse::DramId_HoagMicron4GB]      = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaSamsung4GBY]    = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaSamsung1y4GBX]  = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaSamsung1y8GBX]  = pkg1::MemorySize_8GB,
            [fuse::DramId_HoagSamsung1y4GBX]  = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaSamsung1y4GBY]  = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaSamsung1y8GBY]  = pkg1::MemorySize_8GB,
            [fuse::DramId_FiveSamsung1y4GB]   = pkg1::MemorySize_4GB,
            [fuse::DramId_HoagSamsung1y8GBX]  = pkg1::MemorySize_8GB,
            [fuse::DramId_FiveSamsung1y4GBX]  = pkg1::MemorySize_4GB,
            [fuse::DramId_IowaMicron1y4GB]    = pkg1::MemorySize_4GB,
            [fuse::DramId_HoagMicron1y4GB]    = pkg1::MemorySize_4GB,
            [fuse::DramId_FiveMicron1y4GB]    = pkg1::MemorySize_4GB,
            [fuse::DramId_FiveSamsung1y8GBX]  = pkg1::MemorySize_8GB,
        };

        constexpr const pkg1::MemoryMode MemoryModes[] = {
            pkg1::MemoryMode_Auto,

            pkg1::MemoryMode_4GB,
            pkg1::MemoryMode_4GBAppletDev,
            pkg1::MemoryMode_4GBSystemDev,

            pkg1::MemoryMode_6GB,
            pkg1::MemoryMode_6GBAppletDev,

            pkg1::MemoryMode_8GB,
        };

        constexpr bool IsValidMemoryMode(pkg1::MemoryMode mode) {
            for (const auto known_mode : MemoryModes) {
                if (mode == known_mode) {
                    return true;
                }
            }
            return false;
        }

        pkg1::MemoryMode SanitizeMemoryMode(pkg1::MemoryMode mode) {
            if (IsValidMemoryMode(mode)) {
                return mode;
            }
            return pkg1::MemoryMode_Auto;
        }

        pkg1::MemorySize GetAvailableMemorySize(pkg1::MemorySize size) {
            return std::min(GetPhysicalMemorySize(), size);
        }

        pkg1::MemoryMode GetMemoryMode(pkg1::MemoryMode mode) {
            /* Sanitize the mode. */
            mode = SanitizeMemoryMode(mode);

            /* If the mode is auto, construct the memory mode. */
            if (mode == pkg1::MemoryMode_Auto) {
                return pkg1::MakeMemoryMode(GetPhysicalMemorySize(), pkg1::MemoryArrange_Normal);
            } else {
                const auto mode_size    = GetMemorySize(mode);
                const auto mode_arrange = GetMemoryArrange(mode);
                const auto size         = GetAvailableMemorySize(mode_size);
                const auto arrange      = (size == mode_size) ? mode_arrange : pkg1::MemoryArrange_Normal;
                return pkg1::MakeMemoryMode(size, arrange);
            }
        }

        u32 GetMemoryMode() {
            /* Unless development function is enabled, we're 4 GB. */
            u32 memory_mode = pkg1::MemoryMode_4GB;

            if (const auto &bcd = GetBootConfig().data; bcd.IsDevelopmentFunctionEnabled()) {
                memory_mode = GetMemoryMode(bcd.GetMemoryMode());
            }

            return memory_mode;
        }

        u32 GetKernelConfiguration() {
            pkg1::MemorySize memory_size = pkg1::MemorySize_4GB;
            util::BitPack32 value = {};

            if (const auto &bcd = GetBootConfig().data; bcd.IsDevelopmentFunctionEnabled()) {
                memory_size = GetMemorySize(GetMemoryMode(bcd.GetMemoryMode()));

                value.Set<KernelConfiguration::Flags1>(bcd.GetKernelFlags1());
                value.Set<KernelConfiguration::Flags0>(bcd.GetKernelFlags0());
            }

            value.Set<KernelConfiguration::PhysicalMemorySize>(memory_size);

            /* Exosphere extensions. */
            const auto &sc = GetSecmonConfiguration();

            if (!sc.DisableUserModeExceptionHandlers()) {
                value.Set<KernelConfiguration::EnableUserExceptionHandlers>(true);
            }

            if (sc.EnableUserModePerformanceCounterAccess()) {
                value.Set<KernelConfiguration::EnableUserPmuAccess>(true);
            }

            return value.value;
        }

        constinit u64 g_payload_address = 0;

        SmcResult GetConfig(SmcArguments &args, bool kern) {
            switch (static_cast<ConfigItem>(args.r[1])) {
                case ConfigItem::DisableProgramVerification:
                    args.r[1] = GetBootConfig().signed_data.IsProgramVerificationDisabled();
                    break;
                case ConfigItem::DramId:
                    args.r[1] = fuse::GetDramId();
                    break;
                case ConfigItem::SecurityEngineInterruptNumber:
                    args.r[1] = SecurityEngineUserInterruptId;
                    break;
                case ConfigItem::FuseVersion:
                    args.r[1] = fuse::GetExpectedFuseVersion(GetTargetFirmware());
                    break;
                case ConfigItem::HardwareType:
                    args.r[1] = fuse::GetHardwareType();
                    break;
                case ConfigItem::HardwareState:
                    args.r[1] = fuse::GetHardwareState();
                    break;
                case ConfigItem::IsRecoveryBoot:
                    args.r[1] = IsRecoveryBoot();
                    break;
                case ConfigItem::DeviceId:
                    args.r[1] = fuse::GetDeviceId();
                    break;
                case ConfigItem::BootReason:
                    {
                        /* This was removed in firmware 4.0.0. */
                        if (GetTargetFirmware() >= TargetFirmware_4_0_0) {
                            return SmcResult::InvalidArgument;
                        }

                        args.r[1] = GetDeprecatedBootReason();
                    }
                    break;
                case ConfigItem::MemoryMode:
                    args.r[1] = GetMemoryMode();
                    break;
                case ConfigItem::IsDevelopmentFunctionEnabled:
                    args.r[1] = GetSecmonConfiguration().IsDevelopmentFunctionEnabled(kern) || GetBootConfig().data.IsDevelopmentFunctionEnabled();
                    break;
                case ConfigItem::KernelConfiguration:
                    args.r[1] = GetKernelConfiguration();
                    break;
                case ConfigItem::IsChargerHiZModeEnabled:
                    args.r[1] = IsChargerHiZModeEnabled();
                    break;
                case ConfigItem::QuestState:
                    args.r[1] = fuse::GetQuestState();
                    break;
                case ConfigItem::RegulatorType:
                    args.r[1] = fuse::GetRegulator();
                    break;
                case ConfigItem::DeviceUniqueKeyGeneration:
                    args.r[1] = fuse::GetDeviceUniqueKeyGeneration();
                    break;
                case ConfigItem::Package2Hash:
                    {
                        /* Only allow getting the package2 hash in recovery boot. */
                        if (!IsRecoveryBoot()) {
                            return SmcResult::InvalidArgument;
                        }

                        /* Get the hash. */
                        se::Sha256Hash tmp_hash;
                        GetPackage2Hash(std::addressof(tmp_hash));

                        /* Copy it out. */
                        static_assert(sizeof(args) - sizeof(args.r[0]) >= sizeof(tmp_hash));
                        std::memcpy(std::addressof(args.r[1]), std::addressof(tmp_hash), sizeof(tmp_hash));
                    }
                    break;
                case ConfigItem::ExosphereApiVersion:
                    /* Get information about the current exosphere version. */
                    args.r[1] = (static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MAJOR & 0xFF) << 56) |
                                (static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MINOR & 0xFF) << 48) |
                                (static_cast<u64>(ATMOSPHERE_RELEASE_VERSION_MICRO & 0xFF) << 40) |
                                (static_cast<u64>(GetKeyGeneration())                      << 32) |
                                (static_cast<u64>(GetTargetFirmware())                     << 00);
                    break;
                case ConfigItem::ExosphereNeedsReboot:
                    /* We are executing, so we aren't in the process of rebooting. */
                    args.r[1] = 0;
                    break;
                case ConfigItem::ExosphereNeedsShutdown:
                    /* We are executing, so we aren't in the process of shutting down. */
                    args.r[1] = 0;
                    break;
                case ConfigItem::ExosphereGitCommitHash:
                    /* Get information about the current exosphere git commit hash. */
                    args.r[1] = ATMOSPHERE_GIT_HASH;
                    break;
                case ConfigItem::ExosphereHasRcmBugPatch:
                    /* Get information about whether this unit has the RCM bug patched. */
                    args.r[1] = fuse::HasRcmVulnerabilityPatch();
                    break;
                case ConfigItem::ExosphereBlankProdInfo:
                    /* Get whether this unit should simulate a "blanked" PRODINFO. */
                    args.r[1] = GetSecmonConfiguration().ShouldUseBlankCalibrationBinary();
                    break;
                case ConfigItem::ExosphereAllowCalWrites:
                    /* Get whether this unit should allow writing to the calibration partition. */
                    args.r[1] = (GetEmummcConfiguration().IsEmummcActive() || GetSecmonConfiguration().AllowWritingToCalibrationBinarySysmmc());
                    break;
                case ConfigItem::ExosphereEmummcType:
                    /* Get what kind of emummc this unit has active. */
                    /* NOTE: This may return values other than 1 in the future. */
                    args.r[1] = (GetEmummcConfiguration().IsEmummcActive() ? 1 : 0);
                    break;
                case ConfigItem::ExospherePayloadAddress:
                    /* Gets the physical address of the reboot payload buffer, if one exists. */
                    if (g_payload_address != 0) {
                        args.r[1] = g_payload_address;
                    } else {
                        return SmcResult::NotInitialized;
                    }
                    break;
                case ConfigItem::ExosphereLogConfiguration:
                    /* Get the log configuration. */
                    args.r[1] = (static_cast<u64>(static_cast<u8>(secmon::GetLogPort())) << 32) | static_cast<u64>(secmon::GetLogBaudRate());
                    break;
                default:
                    return SmcResult::InvalidArgument;
            }

            return SmcResult::Success;
        }

        SmcResult SetConfig(SmcArguments &args) {
            const auto soc_type = GetSocType();

            switch (static_cast<ConfigItem>(args.r[1])) {
                case ConfigItem::IsChargerHiZModeEnabled:
                    /* Configure the HiZ mode. */
                    SetChargerHiZModeEnabled(static_cast<bool>(args.r[3]));
                    break;
                case ConfigItem::ExosphereNeedsReboot:
                    if (soc_type == fuse::SocType_Erista) {
                        switch (static_cast<UserRebootType>(args.r[3])) {
                            case UserRebootType_None:
                                break;
                            case UserRebootType_ToRcm:
                                PerformUserRebootToRcm();
                                break;
                            case UserRebootType_ToPayload:
                                PerformUserRebootToPayload();
                                break;
                            case UserRebootType_ToFatalError:
                                PerformUserRebootToFatalError();
                                break;
                            default:
                                return SmcResult::InvalidArgument;
                        }
                    } else /* if (soc_type == fuse::SocType_Mariko) */ {
                        switch (static_cast<UserRebootType>(args.r[3])) {
                            case UserRebootType_ToFatalError:
                                PerformUserRebootToFatalError();
                                break;
                            default:
                                return SmcResult::InvalidArgument;
                        }
                    }
                    break;
                case ConfigItem::ExosphereNeedsShutdown:
                    if (soc_type == fuse::SocType_Erista) {
                        if (args.r[3] != 0) {
                            PerformUserShutDown();
                        }
                    } else /* if (soc_type == fuse::SocType_Mariko) */ {
                        return SmcResult::NotImplemented;
                    }
                    break;
                case ConfigItem::ExospherePayloadAddress:
                    if (g_payload_address == 0) {
                        if (secmon::IsPhysicalMemoryAddress(args.r[2])) {
                            g_payload_address = args.r[2];
                        } else {
                            return SmcResult::InvalidArgument;
                        }
                    } else {
                        return SmcResult::Busy;
                    }
                    break;
                default:
                    return SmcResult::InvalidArgument;
            }

            return SmcResult::Success;
        }

    }

    SmcResult SmcGetConfigUser(SmcArguments &args) {
        return GetConfig(args, false);
    }

    SmcResult SmcGetConfigKern(SmcArguments &args) {
        return GetConfig(args, true);
    }

    SmcResult SmcSetConfig(SmcArguments &args) {
        return SetConfig(args);
    }

    /* This is an atmosphere extension smc. */
    SmcResult SmcGetEmummcConfig(SmcArguments &args) {
        /* Decode arguments. */
        const auto mmc               = static_cast<EmummcMmc>(args.r[1]);
        const uintptr_t user_address = args.r[2];
        const uintptr_t user_offset  = user_address % 4_KB;

        /* Validate arguments. */
        /* NOTE: In the future, configuration for non-NAND storage may be implemented. */
        SMC_R_UNLESS(mmc == EmummcMmc_Nand,                             NotImplemented);
        SMC_R_UNLESS(user_offset + 2 * sizeof(EmummcFilePath) <= 4_KB, InvalidArgument);

        /* Get the emummc config. */
        const auto &cfg = GetEmummcConfiguration();
        static_assert(sizeof(cfg.file_cfg)     == sizeof(EmummcFilePath));
        static_assert(sizeof(cfg.emu_dir_path) == sizeof(EmummcFilePath));

        /* Clear the output. */
        constexpr size_t InlineOutputSize = sizeof(args) - sizeof(args.r[0]);
        u8 * const inline_output = static_cast<u8 *>(static_cast<void *>(std::addressof(args.r[1])));
        std::memset(inline_output, 0, InlineOutputSize);

        /* Copy out the configuration. */
        {
            /* Map the user output page. */
            AtmosphereUserPageMapper mapper(user_address);
            SMC_R_UNLESS(mapper.Map(), InvalidArgument);

            /* Copy the base configuration. */
            static_assert(sizeof(cfg.base_cfg) <= InlineOutputSize);
            std::memcpy(inline_output, std::addressof(cfg.base_cfg), sizeof(cfg.base_cfg));

            /* Copy out type-specific data. */
            switch (cfg.base_cfg.type) {
                case EmummcType_None:
                    /* No additional configuration needs to be copied. */
                    break;
                case EmummcType_Partition:
                    /* Copy the partition config. */
                    static_assert(sizeof(cfg.base_cfg) + sizeof(cfg.partition_cfg) <= InlineOutputSize);
                    std::memcpy(inline_output + sizeof(cfg.base_cfg), std::addressof(cfg.partition_cfg), sizeof(cfg.partition_cfg));
                    break;
                case EmummcType_File:
                    /* Copy the file config. */
                    SMC_R_UNLESS(mapper.CopyToUser(user_address, std::addressof(cfg.file_cfg), sizeof(cfg.file_cfg)), InvalidArgument);
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }

            /* Copy the redirection directory path to the user page. */
            SMC_R_UNLESS(mapper.CopyToUser(user_address + sizeof(EmummcFilePath), std::addressof(cfg.emu_dir_path), sizeof(cfg.emu_dir_path)), InvalidArgument);
        }

        return SmcResult::Success;
    }

    /* For exosphere's usage. */
    pkg1::MemorySize GetPhysicalMemorySize() {
        const auto dram_id = fuse::GetDramId();
        AMS_ABORT_UNLESS(dram_id < fuse::DramId_Count);
        return DramIdToMemorySize[dram_id];
    }

}
