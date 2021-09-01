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
#include "fusee_key_derivation.hpp"
#include "fusee_secondary_archive.hpp"
#include "fusee_setup_horizon.hpp"
#include "fusee_ini.hpp"
#include "fusee_emummc.hpp"
#include "fusee_mmc.hpp"
#include "fusee_fatal.hpp"
#include "fusee_package2.hpp"
#include "fusee_malloc.hpp"
#include "fs/fusee_fs_api.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t CLKRST  = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t MC      = secmon::MemoryRegionPhysicalDeviceMemoryController.GetAddress();

        constinit secmon::EmummcConfiguration g_emummc_cfg = {};

        void DeriveAllKeys(const fuse::SocType soc_type) {
            /* If on erista, run the TSEC keygen firmware. */
            if (soc_type == fuse::SocType_Erista) {
                clkrst::SetBpmpClockRate(clkrst::BpmpClockRate_408MHz);

                if (!tsec::RunTsecFirmware(GetSecondaryArchive().tsec_keygen, sizeof(GetSecondaryArchive().tsec_keygen))) {
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
                        x |= c - '0';
                    } else if ('a' <= c && c <= 'f') {
                        x |= c - 'a';
                    } else if ('A' <= c && c <= 'F') {
                        x |= c - 'A';
                    }
                }
            }
        }

        bool IsDirectoryExist(const char *path) {
            fs::DirectoryEntryType entry_type;
            bool archive;
            return R_SUCCEEDED(fs::GetEntryType(std::addressof(entry_type), std::addressof(archive), path)) && entry_type == fs::DirectoryEntryType_Directory;
        }

        [[maybe_unused]] bool IsFileExist(const char *path) {
            fs::DirectoryEntryType entry_type;
            bool archive;
            return R_SUCCEEDED(fs::GetEntryType(std::addressof(entry_type), std::addressof(archive), path)) && entry_type == fs::DirectoryEntryType_File;
        }

        bool IsConcatenationFileExist(const char *path) {
            fs::DirectoryEntryType entry_type;
            bool archive;
            return R_SUCCEEDED(fs::GetEntryType(std::addressof(entry_type), std::addressof(archive), path)) && ((entry_type == fs::DirectoryEntryType_File) || (entry_type == fs::DirectoryEntryType_Directory && archive));
        }

        constinit char g_nca_path[0x40] = "sys:/contents/registered/xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.nca";

        bool IsNcaExist(const char *nca_name) {
            std::memcpy(g_nca_path + 0x19, nca_name, 0x20);

            return IsConcatenationFileExist(g_nca_path);
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
                                enabled = entry.value[0] == '1';
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

                const u8 iv[0x10] = {};
                se::DecryptAes128Cbc(package1 + 0x20, 0x40000 - (0x20 + 0x170), pkg1::AesKeySlot_MarikoBek, package1 + 0x20, 0x40000 - (0x20 + 0x170), iv, sizeof(iv));

                hw::InvalidateDataCache(package1 + 0x20, 0x40000 - (0x20 + 0x170));
            }

            if (std::memcmp(package1, package1 + 0x20, 0x20) != 0) {
                ShowFatalError("Package1 seems corrupt!\n");
            }

            return package1;
        }

        ams::TargetFirmware GetTargetFirmware(const u8 *package1) {
            /* Get first an approximation of the target firmware. */
            ams::TargetFirmware target_firmware = ams::TargetFirmware_Current;
            switch (package1[0x1F]) {
                case 0x01:
                    target_firmware = ams::TargetFirmware_1_0_0;
                    break;
                case 0x02:
                    target_firmware = ams::TargetFirmware_2_0_0;
                    break;
                case 0x04:
                    target_firmware = ams::TargetFirmware_3_0_0;
                    break;
                case 0x07:
                    target_firmware = ams::TargetFirmware_4_0_0;
                    break;
                case 0x0B:
                    target_firmware = ams::TargetFirmware_5_0_0;
                    break;
                case 0x0E:
                    if (std::memcmp(package1 + 0x10, "20180802", 8) == 0) {
                        target_firmware = ams::TargetFirmware_6_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20181107", 8) == 0) {
                        target_firmware = ams::TargetFirmware_6_2_0;
                    } else {
                        ShowFatalError("Unable to identify package1!\n");
                    }
                    break;
                case 0x0F:
                    target_firmware = ams::TargetFirmware_7_0_0;
                    break;
                case 0x10:
                    if (std::memcmp(package1 + 0x10, "20190314", 8) == 0) {
                        target_firmware = ams::TargetFirmware_8_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20190531", 8) == 0) {
                        target_firmware = ams::TargetFirmware_8_1_0;
                    } else if (std::memcmp(package1 + 0x10, "20190809", 8) == 0) {
                        target_firmware = ams::TargetFirmware_9_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20191021", 8) == 0) {
                        target_firmware = ams::TargetFirmware_9_1_0;
                    } else if (std::memcmp(package1 + 0x10, "20200303", 8) == 0) {
                        target_firmware = ams::TargetFirmware_10_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20201030", 8) == 0) {
                        target_firmware = ams::TargetFirmware_11_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20210129", 8) == 0) {
                        target_firmware = ams::TargetFirmware_12_0_0;
                    } else if (std::memcmp(package1 + 0x10, "20210422", 8) == 0) {
                        target_firmware = ams::TargetFirmware_12_0_2;
                    } else if (std::memcmp(package1 + 0x10, "20210607", 8) == 0) {
                        target_firmware = ams::TargetFirmware_12_1_0;
                    } else {
                        ShowFatalError("Unable to identify package1!\n");
                    }
                    break;
                default:
                    ShowFatalError("Unable to identify package1!\n");
                    break;
            }

            #define CHECK_NCA(NCA_ID, VERSION) do { if (IsNcaExist(NCA_ID)) { return ams::TargetFirmware_##VERSION; } } while(0)

            if (target_firmware >= ams::TargetFirmware_12_1_0) {
                CHECK_NCA("9d9d83d68d9517f245f3e8cd7f93c416", 12_1_0);
            } else if (target_firmware >= ams::TargetFirmware_12_0_2) {
                CHECK_NCA("a1863a5c0e1cedd442f5e60b0422dc15", 12_0_3);
                CHECK_NCA("63d928b5a3016fe8cc0e76d2f06f4e98", 12_0_2);
            } else if (target_firmware >= ams::TargetFirmware_12_0_0) {
                CHECK_NCA("e65114b456f9d0b566a80e53bade2d89", 12_0_1);
                CHECK_NCA("bd4185843550fbba125b20787005d1d2", 12_0_0);
            } else if (target_firmware >= ams::TargetFirmware_11_0_0) {
                CHECK_NCA("56211c7a5ed20a5332f5cdda67121e37", 11_0_1);
                CHECK_NCA("594c90bcdbcccad6b062eadba0cd0e7e", 11_0_0);
            } else if (target_firmware >= ams::TargetFirmware_10_0_0) {
                CHECK_NCA("26325de4db3909e0ef2379787c7e671d", 10_2_0);
                CHECK_NCA("5077973537f6735b564dd7475b779f87", 10_1_1); /* Exclusive to China. */
                CHECK_NCA("fd1faed0ca750700d254c0915b93d506", 10_1_0);
                CHECK_NCA("34728c771299443420820d8ae490ea41", 10_0_4);
                CHECK_NCA("5b1df84f88c3334335bbb45d8522cbb4", 10_0_3);
                CHECK_NCA("e951bc9dedcd54f65ffd83d4d050f9e0", 10_0_2);
                CHECK_NCA("36ab1acf0c10a2beb9f7d472685f9a89", 10_0_1);
                CHECK_NCA("5625cdc21d5f1ca52f6c36ba261505b9", 10_0_0);
            } else if (target_firmware >= ams::TargetFirmware_9_1_0) {
                CHECK_NCA("09ef4d92bb47b33861e695ba524a2c17", 9_2_0);
                CHECK_NCA("c5fbb49f2e3648c8cfca758020c53ecb", 9_1_0);
            } else if (target_firmware >= ams::TargetFirmware_9_0_0) {
                CHECK_NCA("fd1ffb82dc1da76346343de22edbc97c", 9_0_1);
                CHECK_NCA("a6af05b33f8f903aab90c8b0fcbcc6a4", 9_0_0);
            } else if (target_firmware >= ams::TargetFirmware_8_1_0) {
                CHECK_NCA("724d9b432929ea43e787ad81bf09ae65", 8_1_1); /* 8.1.1-100 from Lite */
                CHECK_NCA("e9bb0602e939270a9348bddd9b78827b", 8_1_1); /* 8.1.1-12  from chinese gamecard */
                CHECK_NCA("7eedb7006ad855ec567114be601b2a9d", 8_1_0);
            } else if (target_firmware >= ams::TargetFirmware_8_0_0) {
                CHECK_NCA("6c5426d27c40288302ad616307867eba", 8_0_1);
                CHECK_NCA("4fe7b4abcea4a0bcc50975c1a926efcb", 8_0_0);
            } else if (target_firmware >= ams::TargetFirmware_7_0_0) {
                CHECK_NCA("e6b22c40bb4fa66a151f1dc8db5a7b5c", 7_0_1);
                CHECK_NCA("c613bd9660478de69bc8d0e2e7ea9949", 7_0_0);
            } else if (target_firmware >= ams::TargetFirmware_6_2_0) {
                CHECK_NCA("6dfaaf1a3cebda6307aa770d9303d9b6", 6_2_0);
            } else if (target_firmware >= ams::TargetFirmware_6_0_0) {
                CHECK_NCA("1d21680af5a034d626693674faf81b02", 6_1_0);
                CHECK_NCA("663e74e45ffc86fbbaeb98045feea315", 6_0_1);
                CHECK_NCA("258c1786b0f6844250f34d9c6f66095b", 6_0_0); /* Release     6.0.0-5.0 */
                CHECK_NCA("286e30bafd7e4197df6551ad802dd815", 6_0_0); /* Pre-Release 6.0.0-4.0 */
            } else if (target_firmware >= ams::TargetFirmware_5_0_0) {
                CHECK_NCA("fce3b0ea366f9c95fe6498b69274b0e7", 5_1_0);
                CHECK_NCA("c5758b0cb8c6512e8967e38842d35016", 5_0_2);
                CHECK_NCA("53eb605d4620e8fd50064b24fd57783a", 5_0_1);
                CHECK_NCA("09a2f9c16ce1c121ae6d231b35d17515", 5_0_0);
            } else if (target_firmware >= ams::TargetFirmware_4_0_0) {
                CHECK_NCA("77e1ae7661ad8a718b9b13b70304aeea", 4_1_0);
                CHECK_NCA("d0e5d20e3260f3083bcc067483b71274", 4_0_1);
                CHECK_NCA("483a24ee3fd7149f9112d1931166a678", 4_0_0);
            } else if (target_firmware >= ams::TargetFirmware_3_0_0) {
                CHECK_NCA("704129fc89e1fcb85c37b3112e51b0fc", 3_0_2);
                CHECK_NCA("1fb00543307337d523ccefa9923e0c50", 3_0_1);
                CHECK_NCA("6ebd3447473bade18badbeb5032af87d", 3_0_0);
            } else if (target_firmware >= ams::TargetFirmware_2_0_0) {
                CHECK_NCA("d1c991c53a8a9038f8c3157a553d876d", 2_3_0);
                CHECK_NCA("7f90353dff2d7ce69e19e07ebc0d5489", 2_2_0);
                CHECK_NCA("e9b3e75fce00e52fe646156634d229b4", 2_1_0);
                CHECK_NCA("7a1f79f8184d4b9bae1755090278f52c", 2_0_0);
            } else if (target_firmware >= ams::TargetFirmware_1_0_0) {
                CHECK_NCA("a1b287e07f8455e8192f13d0e45a2aaf", 1_0_0); /* 1.0.0 from Factory */
                CHECK_NCA("117f7b9c7da3e8cef02340596af206b3", 1_0_0); /* 1.0.0 from Gamecard */
            } else {
                ShowFatalError("Unable to determine target firmware!\n");
            }

            #undef CHECK_NCA

            /* If we didn't find a more specific firmware, return our package1 approximation. */
            return target_firmware;
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
                constexpr s64 Package2Offset = __builtin_offsetof(pkg2::StorageLayout, package2_header);

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
        const auto target_firmware = GetTargetFirmware(package1);
        AMS_UNUSED(target_firmware);

        /* Read/decrypt package2. */
        u8 * const package2 = LoadBootConfigAndPackage2();
        AMS_UNUSED(package2);

        /* TODO: Setup warmboot firmware. */

        /* TODO: Setup exosphere. */

        /* TODO: Start CPU. */
        /* NOTE: Security Engine unusable past this point. */

        /* TODO: Build modified package2. */
        WaitForReboot();
    }

}