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
#include <stratosphere.hpp>

namespace ams::cfg {

    namespace {

        /* Types. */
        struct OverrideKey {
            u64 key_combination;
            bool override_by_default;
        };

        struct ProgramOverrideKey {
            OverrideKey    override_key;
            ncm::ProgramId program_id;
        };

        constexpr ProgramOverrideKey InvalidProgramOverrideKey = {};

        constexpr ProgramOverrideKey DefaultAppletPhotoViewerOverrideKey = {
            .override_key = {
                .key_combination     = HidNpadButton_R,
                .override_by_default = true,
            },
            .program_id = ncm::SystemAppletId::PhotoViewer,
        };

        constexpr size_t MaxProgramOverrideKeys = 8;

        struct HblOverrideConfig {
            ProgramOverrideKey program_configs[MaxProgramOverrideKeys];
            impl::OverrideStatusFlag program_as_flags[MaxProgramOverrideKeys];
            OverrideKey override_any_app_key;
            impl::OverrideStatusFlag override_any_app_as_flag;
            bool override_any_app;
        };

        struct ContentSpecificOverrideConfig {
            OverrideKey override_key;
            OverrideKey cheat_enable_key;
            OverrideLocale locale;
        };

        /* Override globals. */
        OverrideKey g_default_override_key = {
            .key_combination = HidNpadButton_L,
            .override_by_default = true,
        };

        OverrideKey g_default_cheat_enable_key = {
            .key_combination = HidNpadButton_L,
            .override_by_default = true,
        };

        HblOverrideConfig g_hbl_override_config = {
            .program_configs = {
                DefaultAppletPhotoViewerOverrideKey,
                InvalidProgramOverrideKey,
                InvalidProgramOverrideKey,
                InvalidProgramOverrideKey,
                InvalidProgramOverrideKey,
                InvalidProgramOverrideKey,
                InvalidProgramOverrideKey,
                InvalidProgramOverrideKey,
            },
            .program_as_flags = {
                impl::OverrideStatusFlag_AddressSpace64Bit,
                impl::OverrideStatusFlag_AddressSpace64Bit,
                impl::OverrideStatusFlag_AddressSpace64Bit,
                impl::OverrideStatusFlag_AddressSpace64Bit,
                impl::OverrideStatusFlag_AddressSpace64Bit,
                impl::OverrideStatusFlag_AddressSpace64Bit,
                impl::OverrideStatusFlag_AddressSpace64Bit,
                impl::OverrideStatusFlag_AddressSpace64Bit,
            },
            .override_any_app_key = {
                .key_combination = HidNpadButton_R,
                .override_by_default = false,
            },
            .override_any_app_as_flag = impl::OverrideStatusFlag_AddressSpace64Bit,
            .override_any_app = true,
        };

        char g_hbl_sd_path[0x100] = "/atmosphere/hbl.nsp";

        /* Helpers. */
        OverrideKey ParseOverrideKey(const char *value) {
            OverrideKey cfg = {};

            /* Parse on by default. */
            if (value[0] == '!') {
                cfg.override_by_default = true;
                value++;
            }

            /* Parse key combination. */
            if (strcasecmp(value, "A") == 0) {
                cfg.key_combination = HidNpadButton_A;
            } else if (strcasecmp(value, "B") == 0) {
                cfg.key_combination = HidNpadButton_B;
            } else if (strcasecmp(value, "X") == 0) {
                cfg.key_combination = HidNpadButton_X;
            } else if (strcasecmp(value, "Y") == 0) {
                cfg.key_combination = HidNpadButton_Y;
            } else if (strcasecmp(value, "LS") == 0) {
                cfg.key_combination = HidNpadButton_StickL;
            } else if (strcasecmp(value, "RS") == 0) {
                cfg.key_combination = HidNpadButton_StickR;
            } else if (strcasecmp(value, "L") == 0) {
                cfg.key_combination = HidNpadButton_L;
            } else if (strcasecmp(value, "R") == 0) {
                cfg.key_combination = HidNpadButton_R;
            } else if (strcasecmp(value, "ZL") == 0) {
                cfg.key_combination = HidNpadButton_ZL;
            } else if (strcasecmp(value, "ZR") == 0) {
                cfg.key_combination = HidNpadButton_ZR;
            } else if (strcasecmp(value, "PLUS") == 0) {
                cfg.key_combination = HidNpadButton_Plus;
            } else if (strcasecmp(value, "MINUS") == 0) {
                cfg.key_combination = HidNpadButton_Minus;
            } else if (strcasecmp(value, "DLEFT") == 0) {
                cfg.key_combination = HidNpadButton_Left;
            } else if (strcasecmp(value, "DUP") == 0) {
                cfg.key_combination = HidNpadButton_Up;
            } else if (strcasecmp(value, "DRIGHT") == 0) {
                cfg.key_combination = HidNpadButton_Right;
            } else if (strcasecmp(value, "DDOWN") == 0) {
                cfg.key_combination = HidNpadButton_Down;
            } else if (strcasecmp(value, "SL") == 0) {
                cfg.key_combination = HidNpadButton_AnySL;
            } else if (strcasecmp(value, "SR") == 0) {
                cfg.key_combination = HidNpadButton_AnySR;
            }

            return cfg;
        }

        impl::OverrideStatusFlag ParseOverrideAddressSpace(const char *value) {
            if (strcasecmp(value, "39_bit") == 0 || strcasecmp(value, "39") == 0) {
                return impl::OverrideStatusFlag_AddressSpace64Bit;
            } else if (strcasecmp(value, "36_bit") == 0 || strcasecmp(value, "36") == 0) {
                return impl::OverrideStatusFlag_AddressSpace64BitDeprecated;
            } else if (strcasecmp(value, "32_bit") == 0 || strcasecmp(value, "32") == 0) {
                return impl::OverrideStatusFlag_AddressSpace32Bit;
            } else if (strcasecmp(value, "32_bit_without_alias") == 0 ||
                       strcasecmp(value, "32_bit_no_alias") == 0 ||
                       strcasecmp(value, "32_without_alias") == 0 ||
                       strcasecmp(value, "32_no_alias") ||
                       strcasecmp(value, "32_bit_without_map") == 0 ||
                       strcasecmp(value, "32_bit_no_map") == 0 ||
                       strcasecmp(value, "32_without_map") == 0 ||
                       strcasecmp(value, "32_no_map") == 0)
            {
                return impl::OverrideStatusFlag_AddressSpace32BitWithoutAlias;
            } else {
                /* Default to 39-bit. */
                return impl::OverrideStatusFlag_AddressSpace64Bit;
            }
        }

        inline void SetHblSpecificProgramId(size_t i, const char *value) {
            g_hbl_override_config.program_configs[i].program_id = {strtoul(value, nullptr, 16)};
        }

        inline void SetHblSpecificOverrideKey(size_t i, const char *value) {
            g_hbl_override_config.program_configs[i].override_key = ParseOverrideKey(value);
        }

        inline void SetHblSpecificAddressSpace(size_t i, const char *value) {
            g_hbl_override_config.program_as_flags[i] = ParseOverrideAddressSpace(value);
        }

        int OverrideConfigIniHandler(void *user, const char *section, const char *name, const char *value) {
            /* Taken and modified, with love, from Rajkosto's implementation. */
            if (strcasecmp(section, "hbl_config") == 0) {
                if (strcasecmp(name, "program_id") == 0 || strcasecmp(name, "program_id_0") == 0) {
                    SetHblSpecificProgramId(0, value);
                } else if (strcasecmp(name, "program_id_1") == 0) {
                    SetHblSpecificProgramId(1, value);
                } else if (strcasecmp(name, "program_id_2") == 0) {
                    SetHblSpecificProgramId(2, value);
                } else if (strcasecmp(name, "program_id_3") == 0) {
                    SetHblSpecificProgramId(3, value);
                } else if (strcasecmp(name, "program_id_4") == 0) {
                    SetHblSpecificProgramId(4, value);
                } else if (strcasecmp(name, "program_id_5") == 0) {
                    SetHblSpecificProgramId(5, value);
                } else if (strcasecmp(name, "program_id_6") == 0) {
                    SetHblSpecificProgramId(6, value);
                } else if (strcasecmp(name, "program_id_7") == 0) {
                    SetHblSpecificProgramId(7, value);
                } else if (strcasecmp(name, "override_key") == 0 || strcasecmp(name, "override_key_0") == 0) {
                    SetHblSpecificOverrideKey(0, value);
                } else if (strcasecmp(name, "override_key_1") == 0) {
                    SetHblSpecificOverrideKey(1, value);
                } else if (strcasecmp(name, "override_key_2") == 0) {
                    SetHblSpecificOverrideKey(2, value);
                } else if (strcasecmp(name, "override_key_3") == 0) {
                    SetHblSpecificOverrideKey(3, value);
                } else if (strcasecmp(name, "override_key_4") == 0) {
                    SetHblSpecificOverrideKey(4, value);
                } else if (strcasecmp(name, "override_key_5") == 0) {
                    SetHblSpecificOverrideKey(5, value);
                } else if (strcasecmp(name, "override_key_6") == 0) {
                    SetHblSpecificOverrideKey(6, value);
                } else if (strcasecmp(name, "override_key_7") == 0) {
                    SetHblSpecificOverrideKey(7, value);
                } else if (strcasecmp(name, "override_address_space") == 0 || strcasecmp(name, "override_address_space_0") == 0) {
                    SetHblSpecificAddressSpace(0, value);
                } else if (strcasecmp(name, "override_address_space_1") == 0) {
                    SetHblSpecificAddressSpace(1, value);
                } else if (strcasecmp(name, "override_address_space_2") == 0) {
                    SetHblSpecificAddressSpace(2, value);
                } else if (strcasecmp(name, "override_address_space_3") == 0) {
                    SetHblSpecificAddressSpace(3, value);
                } else if (strcasecmp(name, "override_address_space_4") == 0) {
                    SetHblSpecificAddressSpace(4, value);
                } else if (strcasecmp(name, "override_address_space_5") == 0) {
                    SetHblSpecificAddressSpace(5, value);
                } else if (strcasecmp(name, "override_address_space_6") == 0) {
                    SetHblSpecificAddressSpace(6, value);
                } else if (strcasecmp(name, "override_address_space_7") == 0) {
                    SetHblSpecificAddressSpace(7, value);
                } else if (strcasecmp(name, "override_any_app") == 0) {
                   if (strcasecmp(value, "true") == 0 || strcasecmp(value, "1") == 0) {
                        g_hbl_override_config.override_any_app = true;
                    } else if (strcasecmp(value, "false") == 0 || strcasecmp(value, "0") == 0) {
                        g_hbl_override_config.override_any_app = false;
                    } else {
                        /* I guess we default to not changing the value? */
                    }
                } else if (strcasecmp(name, "override_any_app_key") == 0) {
                    g_hbl_override_config.override_any_app_key = ParseOverrideKey(value);
                } else if (strcasecmp(name, "override_any_app_address_space") == 0) {
                    g_hbl_override_config.override_any_app_as_flag = ParseOverrideAddressSpace(value);
                } else if (strcasecmp(name, "path") == 0) {
                    while (*value == '/' || *value == '\\') {
                        value++;
                    }
                    std::snprintf(g_hbl_sd_path, sizeof(g_hbl_sd_path) - 1, "/%s", value);
                    g_hbl_sd_path[sizeof(g_hbl_sd_path) - 1] = '\0';

                    for (size_t i = 0; i < sizeof(g_hbl_sd_path); i++) {
                        if (g_hbl_sd_path[i] == '\\') {
                            g_hbl_sd_path[i] = '/';
                        }
                    }
                }
            } else if (strcasecmp(section, "default_config") == 0) {
                if (strcasecmp(name, "override_key") == 0) {
                    g_default_override_key = ParseOverrideKey(value);
                } else if (strcasecmp(name, "cheat_enable_key") == 0) {
                    g_default_cheat_enable_key = ParseOverrideKey(value);
                }
            } else {
                return 0;
            }
            return 1;
        }

        int ContentSpecificIniHandler(void *user, const char *section, const char *name, const char *value) {
            ContentSpecificOverrideConfig *config = reinterpret_cast<ContentSpecificOverrideConfig *>(user);

            if (strcasecmp(section, "override_config") == 0) {
                if (strcasecmp(name, "override_key") == 0) {
                    config->override_key = ParseOverrideKey(value);
                } else if (strcasecmp(name, "cheat_enable_key") == 0) {
                    config->cheat_enable_key = ParseOverrideKey(value);
                } else if (strcasecmp(name, "override_language") == 0) {
                    config->locale.language_code = settings::LanguageCode::Encode(value);
                } else if (strcasecmp(name, "override_region") == 0) {
                    if (strcasecmp(value, "jpn") == 0) {
                        config->locale.region_code = settings::RegionCode_Japan;
                    } else if (strcasecmp(value, "usa") == 0) {
                        config->locale.region_code = settings::RegionCode_America;
                    } else if (strcasecmp(value, "eur") == 0) {
                        config->locale.region_code = settings::RegionCode_Europe;
                    } else if (strcasecmp(value, "aus") == 0) {
                        config->locale.region_code = settings::RegionCode_Australia;
                    } else if (strcasecmp(value, "chn") == 0) {
                        config->locale.region_code = settings::RegionCode_China;
                    } else if (strcasecmp(value, "kor") == 0) {
                        config->locale.region_code = settings::RegionCode_Korea;
                    } else if (strcasecmp(value, "twn") == 0) {
                        config->locale.region_code = settings::RegionCode_Taiwan;
                    }
                }
            } else {
                return 0;
            }

            return 1;
        }

        constexpr inline bool IsOverrideMatch(const OverrideStatus &status, const OverrideKey &cfg) {
            bool keys_triggered = ((status.keys_held & cfg.key_combination) != 0);
            return (cfg.override_by_default ^ keys_triggered);
        }

        inline bool IsAnySpecificHblProgramId(ncm::ProgramId program_id) {
            for (size_t i = 0; i < MaxProgramOverrideKeys; i++) {
                if (program_id == g_hbl_override_config.program_configs[i].program_id) {
                    return true;
                }
            }
            return false;
        }

        inline bool IsSpecificHblProgramId(size_t i, ncm::ProgramId program_id) {
            return program_id == g_hbl_override_config.program_configs[i].program_id;
        }

        inline bool IsAnyApplicationHblProgramId(ncm::ProgramId program_id) {
            return g_hbl_override_config.override_any_app && ncm::IsApplicationId(program_id) && !IsAnySpecificHblProgramId(program_id);
        }

        std::atomic<u32> g_ini_mount_count;

        void GetIniMountName(char *dst) {
            std::snprintf(dst, fs::MountNameLengthMax + 1, "#ini%08x", g_ini_mount_count.fetch_add(1));
        }

        void ParseIniFile(util::ini::Handler handler, const char *path, void *user_ctx) {
            /* Mount the SD card. */
            char mount_name[fs::MountNameLengthMax + 1];
            GetIniMountName(mount_name);
            if (R_FAILED(fs::MountSdCard(mount_name))) {
                return;
            }
            ON_SCOPE_EXIT { fs::Unmount(mount_name); };

            /* Open the file. */
            fs::FileHandle file;
            {
                char full_path[fs::EntryNameLengthMax + 1];
                std::snprintf(full_path, sizeof(full_path), "%s:/%s", mount_name, path[0] == '/' ? path + 1 : path);
                if (R_FAILED(fs::OpenFile(std::addressof(file), full_path, fs::OpenMode_Read))) {
                    return;
                }
            }
            ON_SCOPE_EXIT { fs::CloseFile(file); };

            /* Parse the config. */
            util::ini::ParseFile(file, user_ctx, handler);
        }

        void RefreshOverrideConfiguration() {
            ParseIniFile(OverrideConfigIniHandler, "/atmosphere/config/override_config.ini", nullptr);
        }

        ContentSpecificOverrideConfig GetContentOverrideConfig(ncm::ProgramId program_id) {
            char path[fs::EntryNameLengthMax + 1];
            std::snprintf(path, sizeof(path), "/atmosphere/contents/%016lx/config.ini", static_cast<u64>(program_id));

            ContentSpecificOverrideConfig config = {
                .override_key = g_default_override_key,
                .cheat_enable_key = g_default_cheat_enable_key,
            };
            std::memset(&config.locale, 0xCC, sizeof(config.locale));

            ParseIniFile(ContentSpecificIniHandler, path, &config);
            return config;
        }

    }

    OverrideStatus CaptureOverrideStatus(ncm::ProgramId program_id) {
        OverrideStatus status = {};

        /* If the SD card isn't initialized, we can't override. */
        if (!IsSdCardInitialized()) {
            return status;
        }

        /* For system modules and anything launched before the home menu, always override. */
        if (program_id < ncm::SystemAppletId::Start || !pm::info::HasLaunchedProgram(ncm::SystemAppletId::Qlaunch)) {
            status.SetProgramSpecific();
            return status;
        }

        /* Unconditionally refresh override_config.ini contents. */
        RefreshOverrideConfiguration();

        /* If we can't read the key state, don't override anything. */
        if (R_FAILED(hid::GetKeysHeld(&status.keys_held))) {
            return status;
        }

        /* Detect Hbl. */
        if (IsAnyApplicationHblProgramId(program_id)  && IsOverrideMatch(status, g_hbl_override_config.override_any_app_key)) {
            status.SetHbl();
            status.flags &= ~impl::OverrideStatusFlag_AddressSpaceMask;
            status.flags |= g_hbl_override_config.override_any_app_as_flag;
        }
        for (size_t i = 0; i < MaxProgramOverrideKeys; i++) {
            if (IsSpecificHblProgramId(i, program_id) && IsOverrideMatch(status, g_hbl_override_config.program_configs[i].override_key)) {
                status.SetHbl();
                status.flags &= ~impl::OverrideStatusFlag_AddressSpaceMask;
                status.flags |= g_hbl_override_config.program_as_flags[i];
            }
        }

        /* Detect content specific keys. */
        const auto content_cfg = GetContentOverrideConfig(program_id);
        if (IsOverrideMatch(status, content_cfg.override_key)) {
            status.SetProgramSpecific();
        }

        /* Only allow cheat enable if not HBL. */
        if (!status.IsHbl() && IsOverrideMatch(status, content_cfg.cheat_enable_key)) {
            status.SetCheatEnabled();
        }

        return status;
    }

    OverrideLocale GetOverrideLocale(ncm::ProgramId program_id) {
        return GetContentOverrideConfig(program_id).locale;
    }

    /* HBL Configuration utilities. */
    const char *GetHblPath() {
        return g_hbl_sd_path;
    }

}
