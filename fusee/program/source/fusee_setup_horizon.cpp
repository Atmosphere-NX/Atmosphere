/*
 * Copyright (c) Atmosphère-NX
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
#include <exosphere/secmon/secmon_monitor_context.hpp>
#include "fusee_key_derivation.hpp"
#include "fusee_external_package.hpp"
#include "fusee_setup_horizon.hpp"
#include "fusee_ini.hpp"
#include "fusee_emummc.hpp"
#include "fusee_mmc.hpp"
#include "fusee_cpu.hpp"
#include "fusee_fatal.hpp"
#include "fusee_package2.hpp"
#include "fusee_malloc.hpp"
#include "fusee_secmon_sync.hpp"
#include "fusee_stratosphere.hpp"
#include "fs/fusee_fs_api.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t CLKRST  = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t PMC     = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t MC      = secmon::MemoryRegionPhysicalDeviceMemoryController.GetAddress();

        constinit secmon::EmummcConfiguration g_emummc_cfg = {};

        void DeriveAllKeys(const fuse::SocType soc_type) {
            /* If on erista, run the TSEC keygen firmware. */
            if (soc_type == fuse::SocType_Erista) {
                clkrst::SetBpmpClockRate(clkrst::BpmpClockRate_408MHz);

                if (!tsec::RunTsecFirmware(GetExternalPackage().tsec_keygen, sizeof(GetExternalPackage().tsec_keygen))) {
                    ShowFatalError("Failed to run tsec_keygen firmware!\n");
                }

                clkrst::SetBpmpClockRate(clkrst::BpmpClockRate_576MHz);
            }

            /* Derive master/device keys. */
            if (soc_type == fuse::SocType_Erista) {
                DeriveKeysErista();
            } else /* if (soc_type == fuse::SocType_Mariko) */ {
                DeriveKeysMariko();
            }
        }

        bool ParseIniSafe(IniSectionList &out_sections, const char *ini_path) {
            const auto result = ParseIniFile(out_sections, ini_path);
            if (result == ParseIniResult_Success) {
                return true;
            } else if (result == ParseIniResult_NoFile) {
                return false;
            } else {
                ShowFatalError("Failed to parse %s!\n", ini_path);
            }
        }

        u32 ParseHexInteger(const char *s) {
            u32 x = 0;
            if (s[0] == '0' && s[1] == 'x') {
                s += 2;
            }

            while (true) {
                const char c = *(s++);

                if (c == '\x00') {
                    return x;
                } else {
                    x <<= 4;

                    if ('0' <= c && c <= '9') {
                        x |= (c - '0');
                    } else if ('a' <= c && c <= 'f') {
                        x |= (c - 'a') + 10;
                    } else if ('A' <= c && c <= 'F') {
                        x |= (c - 'A') + 10;
                    }
                }
            }
        }

        u32 ParseDecimalInteger(const char *s) {
            u32 x = 0;
            while (true) {
                const char c = *(s++);

                if (c == '\x00') {
                    return x;
                } else {
                    x *= 10;

                    if ('0' <= c && c <= '9') {
                        x += c - '0';
                    }
                }
            }
        }

        bool IsDirectoryExist(const char *path) {
            fs::DirectoryEntryType entry_type;
            bool archive;
            return R_SUCCEEDED(fs::GetEntryType(std::addressof(entry_type), std::addressof(archive), path)) && entry_type == fs::DirectoryEntryType_Directory;
        }

        bool IsFileExist(const char *path) {
            fs::DirectoryEntryType entry_type;
            bool archive;
            return R_SUCCEEDED(fs::GetEntryType(std::addressof(entry_type), std::addressof(archive), path)) && entry_type == fs::DirectoryEntryType_File;
        }

        bool ConfigureEmummc() {
            /* Set magic. */
            g_emummc_cfg.base_cfg.magic = secmon::EmummcBaseConfiguration::Magic;

            /* Parse ini. */
            bool enabled = false;
            u32 id = 0;
            u32 sector = 0;
            const char *path = "";
            const char *n_path = "";
            {
                IniSectionList sections;
                if (ParseIniSafe(sections, "sdmc:/emummc/emummc.ini")) {
                    for (const auto &section : sections) {
                        /* We only care about the [emummc] section. */
                        if (std::strcmp(section.name, "emummc")) {
                            continue;
                        }

                        /* Handle individual fields. */
                        for (const auto &entry : section.kv_list) {
                            if (std::strcmp(entry.key, "enabled") == 0) {
                                enabled = entry.value[0] != '0';
                            } else if (std::strcmp(entry.key, "id") == 0) {
                                id = ParseHexInteger(entry.value);
                            } else if (std::strcmp(entry.key, "sector") == 0) {
                                sector = ParseHexInteger(entry.value);
                            } else if (std::strcmp(entry.key, "path") == 0) {
                                path = entry.value;
                            } else if (std::strcmp(entry.key, "nintendo_path") == 0) {
                                n_path = entry.value;
                            }
                        }
                    }
                }
            }

            /* Set values parsed from config. */
            g_emummc_cfg.base_cfg.id = id;
            std::strncpy(g_emummc_cfg.emu_dir_path.str, n_path, sizeof(g_emummc_cfg.emu_dir_path.str));
            g_emummc_cfg.emu_dir_path.str[sizeof(g_emummc_cfg.emu_dir_path.str) - 1] = '\x00';

            if (enabled) {
                if (sector > 0) {
                    g_emummc_cfg.base_cfg.type = secmon::EmummcType_Partition;
                    g_emummc_cfg.partition_cfg.start_sector = sector;
                } else if (path[0] != '\x00' && IsDirectoryExist(path)) {
                    g_emummc_cfg.base_cfg.type = secmon::EmummcType_File;

                    std::strncpy(g_emummc_cfg.file_cfg.path.str, path, sizeof(g_emummc_cfg.file_cfg.path.str));
                    g_emummc_cfg.file_cfg.path.str[sizeof(g_emummc_cfg.file_cfg.path.str) - 1] = '\x00';
                } else {
                    ShowFatalError("Invalid emummc setting!\n");
                }
            }

            return enabled;
        }

        u8 *LoadPackage1(fuse::SocType soc_type) {
            u8 *package1 = static_cast<u8 *>(AllocateAligned(0x40000, 0x1000));

            const Result result = ReadBoot0(0x100000, package1, 0x40000);
            if (R_FAILED(result)) {
                ShowFatalError("Failed to read boot0: 0x%08" PRIx32 "!\n", result.GetValue());
            }

            if (soc_type == fuse::SocType_Mariko) {
                package1 += 0x170;

                se::DecryptAes128Cbc(package1 + 0x20, 0x40000 - (0x20 + 0x170), pkg1::AesKeySlot_MarikoBek, package1 + 0x20, 0x40000 - (0x20 + 0x170), package1 + 0x10, se::AesBlockSize);

                hw::InvalidateDataCache(package1 + 0x20, 0x40000 - (0x20 + 0x170));

                if (std::memcmp(package1, package1 + 0x20, 0x20) != 0) {
                    ShowFatalError("Package1 seems corrupt!\n");
                }
            }

            return package1;
        }

        ams::TargetFirmware GetApproximateTargetFirmware(const u8 *package1) {
            /* Get an approximation of the target firmware. */
            switch (package1[0x1F]) {
                case 0x01:
                    return ams::TargetFirmware_1_0_0;
                case 0x02:
                    return ams::TargetFirmware_2_0_0;
                case 0x04:
                    return ams::TargetFirmware_3_0_0;
                case 0x07:
                    return ams::TargetFirmware_4_0_0;
                case 0x0B:
                    return ams::TargetFirmware_5_0_0;
                case 0x0E:
                    if (std::memcmp(package1 + 0x10, "20180802", 8) == 0) {
                        return ams::TargetFirmware_6_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20181107", 8) == 0) {
                        return ams::TargetFirmware_6_2_0;
                    }
                    break;
                case 0x0F:
                    return ams::TargetFirmware_7_0_0;
                case 0x10:
                    if (std::memcmp(package1 + 0x10, "20190314", 8) == 0) {
                        return ams::TargetFirmware_8_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20190531", 8) == 0) {
                        return ams::TargetFirmware_8_1_0;
                    } else if (std::memcmp(package1 + 0x10, "20190809", 8) == 0) {
                        return ams::TargetFirmware_9_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20191021", 8) == 0) {
                        return ams::TargetFirmware_9_1_0;
                    } else if (std::memcmp(package1 + 0x10, "20200303", 8) == 0) {
                        return ams::TargetFirmware_10_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20201030", 8) == 0) {
                        return ams::TargetFirmware_11_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20210129", 8) == 0) {
                        return ams::TargetFirmware_12_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20210422", 8) == 0) {
                        return ams::TargetFirmware_12_0_2;
                    } else if (std::memcmp(package1 + 0x10, "20210607", 8) == 0) {
                        return ams::TargetFirmware_12_1_0;
                    } else if (std::memcmp(package1 + 0x10, "20210805", 8) == 0) {
                        return ams::TargetFirmware_13_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20220105", 8) == 0) {
                        return ams::TargetFirmware_13_2_1;
                    } else if (std::memcmp(package1 + 0x10, "20220209", 8) == 0) {
                        return ams::TargetFirmware_14_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20220801", 8) == 0) {
                        return ams::TargetFirmware_15_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20230111", 8) == 0) {
                        return ams::TargetFirmware_16_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20230906", 8) == 0) {
                        return ams::TargetFirmware_17_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20240207", 8) == 0) {
                        return ams::TargetFirmware_18_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20240808", 8) == 0) {
                        return ams::TargetFirmware_19_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20250206", 8) == 0) {
                        return ams::TargetFirmware_20_0_0;
                    }
                    break;
                default:
                    break;
            }

            ShowFatalError("Unable to identify package1!\n");
        }

        u8 *LoadBootConfigAndPackage2() {
            Result result;

            /* Load boot config. */
            if (R_FAILED((result = ReadPackage2(0, secmon::MemoryRegionPhysicalIramBootConfig.GetPointer<void>(), secmon::MemoryRegionPhysicalIramBootConfig.GetSize())))) {
                ShowFatalError("Failed to read boot config: 0x%08" PRIx32 "!\n", result.GetValue());
            }

            /* Read package2 header. */
            u8 *package2;
            size_t package2_size;
            {
                constexpr s64 Package2Offset = AMS_OFFSETOF(pkg2::StorageLayout, package2_header);

                pkg2::Package2Header header;
                if (R_FAILED((result = ReadPackage2(Package2Offset, std::addressof(header), sizeof(header))))) {
                    ShowFatalError("Failed to read package2 header: 0x%08" PRIx32 "!\n", result.GetValue());
                }

                package2_size = header.meta.GetSize();
                package2 = static_cast<u8 *>(AllocateAligned(util::AlignUp(package2_size, 0x4000), 0x4000));

                if (R_FAILED((result = ReadPackage2(Package2Offset, package2, util::AlignUp(package2_size, 0x4000))))) {
                    ShowFatalError("Failed to read package2: 0x%08" PRIx32 "!\n", result.GetValue());
                }
            }

            /* Decrypt package2. */
            DecryptPackage2(package2);

            return package2;
        }

        constexpr inline const u8 PkcModulusErista[0x100] = {
            0xF7, 0x86, 0x47, 0xAB, 0x71, 0x89, 0x81, 0xB5, 0xCF, 0x0C, 0xB0, 0xE8, 0x48, 0xA7, 0xFD, 0xAD,
            0xCB, 0x4E, 0x4A, 0x52, 0x0B, 0x1A, 0x8E, 0xDE, 0x41, 0x87, 0x6F, 0xB7, 0x31, 0x05, 0x5F, 0xAA,
            0xEA, 0x97, 0x76, 0x21, 0x20, 0x2B, 0x40, 0x48, 0x76, 0x55, 0x35, 0x03, 0xFE, 0x7F, 0x67, 0x62,
            0xFD, 0x4E, 0xE1, 0x22, 0xF8, 0xF0, 0x97, 0x39, 0xEF, 0xEA, 0x47, 0x89, 0x3C, 0xDB, 0xF0, 0x02,
            0xAD, 0x0C, 0x96, 0xCA, 0x82, 0xAB, 0xB3, 0xCB, 0x98, 0xC8, 0xDC, 0xC6, 0xAC, 0x5C, 0x93, 0x3B,
            0x84, 0x3D, 0x51, 0x91, 0x9E, 0xC1, 0x29, 0x22, 0x95, 0xF0, 0xA1, 0x51, 0xBA, 0xAF, 0x5D, 0xC3,
            0xAB, 0x04, 0x1B, 0x43, 0x61, 0x7D, 0xEA, 0x65, 0x95, 0x24, 0x3C, 0x51, 0x3E, 0x8F, 0xDB, 0xDB,
            0xC1, 0xC4, 0x2D, 0x04, 0x29, 0x5A, 0xD7, 0x34, 0x6B, 0xCC, 0xF1, 0x06, 0xF9, 0xC9, 0xE1, 0xF9,
            0x61, 0x52, 0xE2, 0x05, 0x51, 0xB1, 0x3D, 0x88, 0xF9, 0xA9, 0x27, 0xA5, 0x6F, 0x4D, 0xE7, 0x22,
            0x48, 0xA5, 0xF8, 0x12, 0xA2, 0xC2, 0x5A, 0xA0, 0xBF, 0xC8, 0x76, 0x4B, 0x66, 0xFE, 0x1C, 0x73,
            0x00, 0x29, 0x26, 0xCD, 0x18, 0x4F, 0xC2, 0xB0, 0x51, 0x77, 0x2E, 0x91, 0x09, 0x1B, 0x41, 0x5D,
            0x89, 0x5E, 0xEE, 0x24, 0x22, 0x47, 0xE5, 0xE5, 0xF1, 0x86, 0x99, 0x67, 0x08, 0x28, 0x42, 0xF0,
            0x58, 0x62, 0x54, 0xC6, 0x5B, 0xDC, 0xE6, 0x80, 0x85, 0x6F, 0xE2, 0x72, 0xB9, 0x7E, 0x36, 0x64,
            0x48, 0x85, 0x10, 0xA4, 0x75, 0x38, 0x79, 0x76, 0x8B, 0x51, 0xD5, 0x87, 0xC3, 0x02, 0xC9, 0x1B,
            0x93, 0x22, 0x49, 0xEA, 0xAB, 0xA0, 0xB5, 0xB1, 0x3C, 0x10, 0xC4, 0x71, 0xF0, 0xF1, 0x81, 0x1A,
            0x3A, 0x9C, 0xFC, 0x51, 0x61, 0xB1, 0x4B, 0x18, 0xB2, 0x3D, 0xAA, 0xD6, 0xAC, 0x72, 0x26, 0xB7
        };

        constexpr inline const u8 PkcModulusDevelopmentErista[0x100] = {
            0x37, 0x84, 0x14, 0xB3, 0x78, 0xA4, 0x7F, 0xD8, 0x71, 0x45, 0xCD, 0x90, 0x51, 0x51, 0xBF, 0x2C,
            0x27, 0x03, 0x30, 0x46, 0xBE, 0x8F, 0x99, 0x3E, 0x9F, 0x36, 0x4D, 0xEB, 0xF7, 0x0E, 0x81, 0x7F,
            0xE4, 0x6B, 0xA8, 0x42, 0x8A, 0xA5, 0x4F, 0x76, 0xCC, 0xCB, 0xC5, 0x31, 0xA8, 0x5A, 0x70, 0x51,
            0x34, 0xBF, 0x1E, 0x8D, 0x6E, 0xCF, 0x05, 0x84, 0xCF, 0x8B, 0xE5, 0x9C, 0x3A, 0xA5, 0xCD, 0x1A,
            0x9C, 0xAC, 0x59, 0x30, 0x09, 0x21, 0x3C, 0xBE, 0x07, 0x5C, 0x8D, 0x1C, 0xD1, 0xA3, 0xC9, 0x8F,
            0x26, 0xE2, 0x99, 0xB2, 0x3C, 0x28, 0xAD, 0x63, 0x0F, 0xF5, 0xA0, 0x1C, 0xA2, 0x34, 0xC4, 0x0E,
            0xDB, 0xD7, 0xE1, 0xA9, 0x5E, 0xE9, 0xA5, 0xA8, 0x64, 0x3A, 0xFC, 0x48, 0xB5, 0x97, 0xDF, 0x55,
            0x7C, 0x9A, 0xD2, 0x8C, 0x32, 0x36, 0x1D, 0xC5, 0xA0, 0xC5, 0x66, 0xDF, 0x8A, 0xAD, 0x76, 0x18,
            0x46, 0x3E, 0xDF, 0xD8, 0xEF, 0xB9, 0xE5, 0xDC, 0xCD, 0x08, 0x59, 0xBC, 0x36, 0x68, 0xD6, 0xFC,
            0x3F, 0xFA, 0x11, 0x00, 0x0D, 0x50, 0xE0, 0x69, 0x0F, 0x70, 0x78, 0x7E, 0xD1, 0xA5, 0x85, 0xCD,
            0x13, 0xBC, 0x42, 0x74, 0x33, 0x0C, 0x11, 0x24, 0x1E, 0x33, 0xD5, 0x31, 0xB7, 0x3E, 0x48, 0x94,
            0xCC, 0x81, 0x29, 0x1E, 0xB1, 0xCF, 0x4C, 0x36, 0x7F, 0xE1, 0x1C, 0x15, 0xD4, 0x3F, 0xFB, 0x12,
            0xC2, 0x73, 0x22, 0x16, 0x52, 0xE0, 0x5C, 0x4C, 0x94, 0xE0, 0x87, 0x47, 0xEA, 0xD0, 0x9F, 0x42,
            0x9B, 0xAC, 0xB6, 0xB5, 0xB6, 0x34, 0xE4, 0x55, 0x49, 0xD7, 0xC0, 0xAE, 0xD4, 0x22, 0xB3, 0x5C,
            0x87, 0x64, 0x42, 0xEC, 0x11, 0x6D, 0xBC, 0x09, 0xC0, 0x80, 0x07, 0xD0, 0xBD, 0xBA, 0x45, 0xFE,
            0xD5, 0x52, 0xDA, 0xEC, 0x41, 0xA4, 0xAD, 0x7B, 0x36, 0x86, 0x18, 0xB4, 0x5B, 0xD1, 0x30, 0xBB
        };

        void LoadWarmbootFirmware(fuse::SocType soc_type, ams::TargetFirmware target_firmware, const u8 *package1) {
            u8 *warmboot_dst     = secmon::MemoryRegionPhysicalIramWarmbootBin.GetPointer<u8>();
            size_t warmboot_size = std::min(sizeof(GetExternalPackage().warmboot), secmon::MemoryRegionPhysicalIramWarmbootBin.GetSize());
            if (soc_type == fuse::SocType_Erista) {
                /* Copy the ams warmboot binary. */
                std::memcpy(warmboot_dst, GetExternalPackage().warmboot, warmboot_size);

                /* Set the rsa modulus. */
                if (fuse::GetHardwareState() == fuse::HardwareState_Production) {
                    std::memcpy(warmboot_dst + 0x10, PkcModulusErista, sizeof(PkcModulusErista));
                } else {
                    std::memcpy(warmboot_dst + 0x10, PkcModulusDevelopmentErista, sizeof(PkcModulusDevelopmentErista));
                }

                /* Set the target firmware. */
                std::memcpy(warmboot_dst + 0x248, std::addressof(target_firmware), sizeof(target_firmware));
            } else /* if (soc_type == fuse::SocType_Mariko) */ {
                /* Declare path for mariko warmboot files. */
                char warmboot_path[0x80] = "sdmc:/warmboot_mariko/wb_xx.bin";

                auto UpdateWarmbootPath = [&warmboot_path](u8 fuses) {
                    warmboot_path[0x19] = "0123456789abcdef"[(fuses >> 4) & 0xF];
                    warmboot_path[0x1A] = "0123456789abcdef"[(fuses >> 0) & 0xF];
                };

                /* Get expected/burnt fuse counts. */
                const u32 expected_fuses = fuse::GetExpectedFuseVersion(target_firmware);
                const u32 burnt_fuses    = fuse::GetFuseVersion();
                      u32 used_fuses     = expected_fuses;

                /* Get warmboot from package1. */
                const u8 *warmboot_src = nullptr;
                size_t warmboot_src_size = 0;
                {
                    const u32 *package1_pk11 = reinterpret_cast<const u32 *>(package1 + (target_firmware >= ams::TargetFirmware_6_2_0 ? 0x7000 : 0x4000));
                    if (std::memcmp(package1_pk11, "PK11", 4) != 0) {
                        ShowFatalError("Invalid package1 magic!\n");
                    }

                    const u32 *package1_pk11_data = reinterpret_cast<const u32 *>(package1_pk11 + (0x20 / sizeof(u32)));
                    for (size_t i = 0; i < 3; ++i) {
                        switch (*package1_pk11_data) {
                            case 0xD5034FDF:
                                package1_pk11_data += package1_pk11[6] / sizeof(u32);
                                break;
                            case 0xE328F0C0:
                            case 0xF0C0A7F0:
                                package1_pk11_data += package1_pk11[4] / sizeof(u32);
                                break;
                            default:
                                warmboot_src = reinterpret_cast<const u8 *>(package1_pk11_data);
                                i = 3;
                                break;
                        }
                    }

                    warmboot_src_size = *package1_pk11_data;
                    if (!(0x800 <= warmboot_src_size && warmboot_src_size < 0x1000)) {
                        ShowFatalError("Package1 warmboot firmware seems invalid!\n");
                    }

                    /* If we should, save the current warmboot firmware. */
                    UpdateWarmbootPath(expected_fuses);
                    if (!IsFileExist(warmboot_path)) {
                        fs::CreateDirectory("sdmc:/warmboot_mariko");
                        fs::CreateFile(warmboot_path, warmboot_src_size);

                        Result result;
                        fs::FileHandle file;
                        if (R_FAILED((result = fs::OpenFile(std::addressof(file), warmboot_path, fs::OpenMode_ReadWrite)))) {
                            ShowFatalError("Failed to save %s!\n", warmboot_path);
                        }

                        ON_SCOPE_EXIT { fs::CloseFile(file); };

                        if (R_FAILED((result = fs::WriteFile(file, 0, warmboot_src, warmboot_src_size, fs::WriteOption::Flush)))) {
                            ShowFatalError("Failed to save %s!\n", warmboot_path);
                        }
                    }

                    /* If we need to, find a cached warmboot firmware that we can use. */
                    if (burnt_fuses > expected_fuses) {
                        warmboot_src = nullptr;
                        warmboot_src_size = 0;
                        for (u32 attempt = burnt_fuses; attempt <= 32; ++attempt) {
                            /* Open the current cache file. */
                            UpdateWarmbootPath(attempt);

                            fs::FileHandle file;
                            if (R_FAILED(fs::OpenFile(std::addressof(file), warmboot_path, fs::OpenMode_Read))) {
                                continue;
                            }
                            ON_SCOPE_EXIT { fs::CloseFile(file); };

                            /* Get the size. */
                            s64 size;
                            if (R_FAILED(fs::GetFileSize(std::addressof(size), file)) || !(0x800 <= size && size < 0x1000)) {
                                continue;
                            }

                            /* Allocate memory. */
                            warmboot_src_size = static_cast<size_t>(size);
                            void *tmp = AllocateAligned(warmboot_src_size, 0x10);

                            /* Read the file. */
                            if (R_FAILED(fs::ReadFile(file, 0, tmp, warmboot_src_size))) {
                                continue;
                            }

                            /* Use the cached file. */
                            used_fuses   = attempt;
                            warmboot_src = static_cast<const u8 *>(tmp);
                            break;
                        }
                    }

                    /* Check that we found a firmware. */
                    if (warmboot_src == nullptr) {
                        ShowFatalError("Failed to locate warmboot firmware!\n");
                    }

                    /* Copy the warmboot firmware. */
                    std::memcpy(warmboot_dst, warmboot_src, std::min(warmboot_size, warmboot_src_size));

                    /* Set the warmboot firmware magic. */
                    switch (used_fuses) {
                        case 7:
                            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH32, 0x87);
                        case 8:
                            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH32, 0xA8);
                        default:
                            reg::Write(PMC + APBDEV_PMC_SECURE_SCRATCH32, (0x108 + 0x21 * (used_fuses - 8)));
                            break;
                    }
                    reg::SetBits(PMC + APBDEV_PMC_SEC_DISABLE3, (1 << 16));
                }
            }
        }

        void ConfigureExosphere(fuse::SocType soc_type, ams::TargetFirmware target_firmware, bool emummc_enabled, u32 fs_version) {
            /* Get monitor configuration. */
            auto &storage_ctx = *secmon::MemoryRegionPhysicalDramMonitorConfiguration.GetPointer<secmon::SecureMonitorStorageConfiguration>();
            std::memset(std::addressof(storage_ctx), 0, sizeof(storage_ctx));

            /* Set magic. */
            storage_ctx.magic = secmon::SecureMonitorStorageConfiguration::Magic;

            /* Set some defaults. */
            storage_ctx.target_firmware = target_firmware;
            storage_ctx.lcd_vendor      = GetDisplayLcdVendor();
            storage_ctx.emummc_cfg      = g_emummc_cfg;
            storage_ctx.flags[0]        = secmon::SecureMonitorConfigurationFlag_Default;
            storage_ctx.flags[1]        = secmon::SecureMonitorConfigurationFlag_None;
            storage_ctx.log_port        = uart::Port_ReservedDebug;
            storage_ctx.log_baud_rate   = 115200;

            /* Set the fs version. */
            storage_ctx.emummc_cfg.base_cfg.fs_version = fs_version;

            /* Parse fields from exosphere.ini */
            {
                IniSectionList sections;
                if (ParseIniSafe(sections, "sdmc:/exosphere.ini")) {
                    for (const auto &section : sections) {
                        /* We only care about the [exosphere] section. */
                        if (std::strcmp(section.name, "exosphere")) {
                            continue;
                        }

                        /* Handle individual fields. */
                        for (const auto &entry : section.kv_list) {
                            if (std::strcmp(entry.key, "debugmode") == 0) {
                                if (entry.value[0] == '1') {
                                    storage_ctx.flags[0] |= secmon::SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForKernel;
                                } else {
                                    storage_ctx.flags[0] &= ~secmon::SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForKernel;
                                }
                            } else if (std::strcmp(entry.key, "debugmode_user") == 0) {
                                if (entry.value[0] == '1') {
                                    storage_ctx.flags[0] |= secmon::SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForUser;
                                } else {
                                    storage_ctx.flags[0] &= ~secmon::SecureMonitorConfigurationFlag_IsDevelopmentFunctionEnabledForUser;
                                }
                            } else if (std::strcmp(entry.key, "disable_user_exception_handlers") == 0) {
                                if (entry.value[0] == '1') {
                                    storage_ctx.flags[0] |= secmon::SecureMonitorConfigurationFlag_DisableUserModeExceptionHandlers;
                                } else {
                                    storage_ctx.flags[0] &= ~secmon::SecureMonitorConfigurationFlag_DisableUserModeExceptionHandlers;
                                }
                            } else if (std::strcmp(entry.key, "enable_user_pmu_access") == 0) {
                                if (entry.value[0] == '1') {
                                    storage_ctx.flags[0] |= secmon::SecureMonitorConfigurationFlag_EnableUserModePerformanceCounterAccess;
                                } else {
                                    storage_ctx.flags[0] &= ~secmon::SecureMonitorConfigurationFlag_EnableUserModePerformanceCounterAccess;
                                }
                            } else if (std::strcmp(entry.key, "blank_prodinfo_sysmmc") == 0) {
                                if (!emummc_enabled) {
                                    if (entry.value[0] == '1') {
                                        storage_ctx.flags[0] |= secmon::SecureMonitorConfigurationFlag_ShouldUseBlankCalibrationBinary;
                                    } else {
                                        storage_ctx.flags[0] &= ~secmon::SecureMonitorConfigurationFlag_ShouldUseBlankCalibrationBinary;
                                    }
                                }
                            } else if (std::strcmp(entry.key, "blank_prodinfo_emummc") == 0) {
                                if (emummc_enabled) {
                                    if (entry.value[0] == '1') {
                                        storage_ctx.flags[0] |= secmon::SecureMonitorConfigurationFlag_ShouldUseBlankCalibrationBinary;
                                    } else {
                                        storage_ctx.flags[0] &= ~secmon::SecureMonitorConfigurationFlag_ShouldUseBlankCalibrationBinary;
                                    }
                                }
                            } else if (std::strcmp(entry.key, "allow_writing_to_cal_sysmmc") == 0) {
                                if (entry.value[0] == '1') {
                                    storage_ctx.flags[0] |= secmon::SecureMonitorConfigurationFlag_AllowWritingToCalibrationBinarySysmmc;
                                } else {
                                    storage_ctx.flags[0] &= ~secmon::SecureMonitorConfigurationFlag_AllowWritingToCalibrationBinarySysmmc;
                                }
                            } else if (std::strcmp(entry.key, "log_port") == 0) {
                                const u32 log_port = ParseDecimalInteger(entry.value);
                                if (0 <= log_port && log_port < 4) {
                                    storage_ctx.log_port = log_port;
                                }
                            } else if (std::strcmp(entry.key, "log_baud_rate") == 0) {
                                storage_ctx.log_baud_rate = ParseDecimalInteger(entry.value);
                            } else if (std::strcmp(entry.key, "log_inverted") == 0) {
                                if (entry.value[0] == '1') {
                                    storage_ctx.log_flags |= uart::Flag_Inverted;
                                }
                            }
                        }
                    }
                }
            }

            /* Parse usb setting from system_settings.ini */
            {
                IniSectionList sections;
                if (ParseIniSafe(sections, "sdmc:/atmosphere/config/system_settings.ini")) {
                    for (const auto &section : sections) {
                        /* We only care about the [usb] section. */
                        if (std::strcmp(section.name, "usb")) {
                            continue;
                        }

                        /* Handle individual fields. */
                        for (const auto &entry : section.kv_list) {
                            if (std::strcmp(entry.key, "usb30_force_enabled") == 0) {
                                if (std::strcmp(entry.value, "u8!0x1") == 0) {
                                    storage_ctx.flags[0] |= secmon::SecureMonitorConfigurationFlag_ForceEnableUsb30;
                                }
                            }
                        }
                    }
                }
            }

            /* Copy exosphere. */
            void *exosphere_dst = reinterpret_cast<void *>(0x40030000);
            bool use_sd_exo = false;
            {
                /* Try to use an sd card file, if present. */
                fs::FileHandle exo_file;
                if (R_SUCCEEDED(fs::OpenFile(std::addressof(exo_file), "sdmc:/atmosphere/exosphere.bin", fs::OpenMode_Read))) {
                    ON_SCOPE_EXIT { fs::CloseFile(exo_file); };

                    /* Note that we're using sd_exo. */
                    use_sd_exo = true;

                    Result result;

                    /* Get the size. */
                    s64 size;
                    if (R_FAILED((result = fs::GetFileSize(std::addressof(size), exo_file))) || size > sizeof(GetExternalPackage().exosphere)) {
                        ShowFatalError("Invalid SD exosphere size: 0x%08" PRIx32 ", %" PRIx64 "!\n", result.GetValue(), static_cast<u64>(size));
                    }

                    /* Read the file. */
                    if (R_FAILED((result = fs::ReadFile(exo_file, 0, exosphere_dst, size)))) {
                        ShowFatalError("Failed to read SD exosphere: 0x%08" PRIx32 "!\n", result.GetValue());
                    }
                }
            }

            if (!use_sd_exo) {
                std::memcpy(exosphere_dst, GetExternalPackage().exosphere, sizeof(GetExternalPackage().exosphere));
            }

            /* Copy mariko fatal. */
            if (soc_type == fuse::SocType_Mariko) {
                u8 *mariko_fatal_dst = secmon::MemoryRegionPhysicalMarikoProgramImage.GetPointer<u8>();
                bool use_sd_mariko_fatal = false;
                {
                    /* Try to use an sd card file, if present. */
                    fs::FileHandle mariko_program_file;
                    if (R_SUCCEEDED(fs::OpenFile(std::addressof(mariko_program_file), "sdmc:/atmosphere/mariko_fatal.bin", fs::OpenMode_Read))) {
                        ON_SCOPE_EXIT { fs::CloseFile(mariko_program_file); };

                        /* Note that we're using sd mariko fatal. */
                        use_sd_mariko_fatal = true;

                        Result result;

                        /* Get the size. */
                        s64 size;
                        if (R_FAILED((result = fs::GetFileSize(std::addressof(size), mariko_program_file))) || size > sizeof(GetExternalPackage().mariko_fatal)) {
                            ShowFatalError("Invalid SD mariko_fatal size: 0x%08" PRIx32 ", %" PRIx64 "!\n", result.GetValue(), static_cast<u64>(size));
                        }

                        /* Read the file. */
                        if (R_FAILED((result = fs::ReadFile(mariko_program_file, 0, mariko_fatal_dst, size)))) {
                            ShowFatalError("Failed to read SD mariko_fatal: 0x%08" PRIx32 "!\n", result.GetValue());
                        }

                        /* Clear the remainder. */
                        std::memset(mariko_fatal_dst + size, 0, sizeof(GetExternalPackage().mariko_fatal) - size);
                    }
                }

                if (!use_sd_mariko_fatal) {
                    std::memcpy(mariko_fatal_dst, GetExternalPackage().mariko_fatal, sizeof(GetExternalPackage().mariko_fatal));
                }
            }

            /* Setup the CPU to boot exosphere. */
            SetupCpu(reinterpret_cast<uintptr_t>(exosphere_dst));

            /* Initialize bootloader parameters. */
            InitializeSecureMonitorMailbox();

            /* Set our bootloader state. */
            SetBootloaderState(pkg1::BootloaderState_LoadedBootConfig);

            /* Ensure that the CPU will see consistent data. */
            hw::FlushEntireDataCache();
        }

        bool IsNogcEnabled(ams::TargetFirmware target_firmware) {
            /* First parse from ini. */
            {
                IniSectionList sections;
                if (ParseIniSafe(sections, "sdmc:/atmosphere/config/stratosphere.ini")) {
                    for (const auto &section : sections) {
                        /* We only care about the [stratosphere] section. */
                        if (std::strcmp(section.name, "stratosphere")) {
                            continue;
                        }

                        /* Handle individual fields. */
                        for (const auto &entry : section.kv_list) {
                            if (std::strcmp(entry.key, "nogc") == 0) {
                                return entry.value[0] == '1';
                            }
                        }
                    }
                }
            }

            /* That failed, so try to decide automatically. */
            const auto fuse_version = fuse::GetFuseVersion();
            if (target_firmware >= ams::TargetFirmware_12_0_2 && fuse_version < fuse::GetExpectedFuseVersion(ams::TargetFirmware_12_0_2)) {
                return true;
            }
            if (target_firmware >= ams::TargetFirmware_11_0_0 && fuse_version < fuse::GetExpectedFuseVersion(ams::TargetFirmware_11_0_0)) {
                return true;
            }
            if (target_firmware >= ams::TargetFirmware_9_0_0 && fuse_version < fuse::GetExpectedFuseVersion(ams::TargetFirmware_9_0_0)) {
                return true;
            }
            if (target_firmware >= ams::TargetFirmware_4_0_0 && fuse_version < fuse::GetExpectedFuseVersion(ams::TargetFirmware_4_0_0)) {
                return true;
            }

            return false;
        }

    }

    void SetupAndStartHorizon() {
        /* Get soc type. */
        const auto soc_type = fuse::GetSocType();

        /* Derive all keys. */
        DeriveAllKeys(soc_type);

        /* Determine whether we're using emummc. */
        const bool emummc_enabled = ConfigureEmummc();

        /* Initialize emummc. */
        /* NOTE: SYSTEM:/ accessible past this point. */
        InitializeEmummc(emummc_enabled, g_emummc_cfg);

        /* Read bootloader. */
        const u8 * const package1 = LoadPackage1(soc_type);

        /* Get target firmware. */
        const auto target_firmware = GetApproximateTargetFirmware(package1);

        /* Read/decrypt package2. */
        u8 * const package2 = LoadBootConfigAndPackage2();

        /* Setup warmboot firmware. */
        LoadWarmbootFirmware(soc_type, target_firmware, package1);

        /* Decide whether to use nogc patches. */
        const bool nogc_enabled = IsNogcEnabled(target_firmware);

        /* Decide what KIPs/patches we're loading. */
        const auto fs_version = ConfigureStratosphere(package2, target_firmware, emummc_enabled, nogc_enabled);

        /* Setup exosphere. */
        ConfigureExosphere(soc_type, target_firmware, emummc_enabled, fs_version);

        /* Start CPU. */
        StartCpu();

        /* Build modified package2. */
        RebuildPackage2(target_firmware, emummc_enabled);

        /* Wait for confirmation that exosphere is ready. */
        WaitSecureMonitorState(pkg1::SecureMonitorState_Initialized);
    }

}