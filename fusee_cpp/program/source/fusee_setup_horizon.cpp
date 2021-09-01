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
#include "fs/fusee_fs_api.hpp"

namespace ams::nxboot {

    namespace {

        constexpr inline const uintptr_t CLKRST  = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t MC      = secmon::MemoryRegionPhysicalDeviceMemoryController.GetAddress();

        constinit secmon::EmummcConfiguration g_emummc_cfg = {};

        void DisableArc() {
            /* Disable ARC_CLK_OVR_ON. */
            reg::ReadWrite(CLKRST + CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD, CLK_RST_REG_BITS_ENUM(LVL2_CLK_GATE_OVRD_ARC_CLK_OVR_ON, OFF));

            /* Disable the ARC. */
            reg::ReadWrite(MC + MC_IRAM_REG_CTRL, MC_REG_BITS_ENUM(IRAM_REG_CTRL_IRAM_CFG_WRITE_ACCESS, DISABLED));

            /* Set IRAM BOM/TOP to close all redirection access. */
            reg::Write(MC + MC_IRAM_BOM, 0xFFFFF000);
            reg::Write(MC + MC_IRAM_TOM, 0x00000000);

            /* Read to ensure our configuration takes. */
            reg::Read(MC + MC_IRAM_REG_CTRL);
        }

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

        [[maybe_unused]] bool IsConcatenationFileExist(const char *path) {
            fs::DirectoryEntryType entry_type;
            bool archive;
            return R_SUCCEEDED(fs::GetEntryType(std::addressof(entry_type), std::addressof(archive), path)) && ((entry_type == fs::DirectoryEntryType_File) || (entry_type == fs::DirectoryEntryType_Directory && archive));
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

    }

    void SetupAndStartHorizon() {
        /* Get soc/hardware type. */
        const auto soc_type = fuse::GetSocType();
        const auto hw_type  = fuse::GetHardwareType();

        /* Derive all keys. */
        DeriveAllKeys(soc_type);

        /* Determine whether we're using emummc. */
        const bool emummc_enabled = ConfigureEmummc();

        /* Initialize emummc. */
        InitializeEmummc(emummc_enabled, g_emummc_cfg);

        AMS_UNUSED(hw_type);
        ShowFatalError("SetupAndStartHorizon not fully implemented\n");

        /* Disable the ARC. */
        DisableArc();
    }

}