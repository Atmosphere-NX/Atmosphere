/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

        struct HblOverrideConfig {
            OverrideKey override_key;
            ncm::ProgramId program_id;
            bool override_any_app;
        };

        struct ContentSpecificOverrideConfig {
            OverrideKey override_key;
            OverrideKey cheat_enable_key;
        };

        /* Override globals. */
        OverrideKey g_default_override_key = {
            .key_combination = KEY_L,
            .override_by_default = true,
        };

        OverrideKey g_default_cheat_enable_key = {
            .key_combination = KEY_L,
            .override_by_default = true,
        };

        HblOverrideConfig g_hbl_override_config = {
            .override_key = {
                .key_combination = KEY_R,
                .override_by_default = false,
            },
            .program_id = ncm::ProgramId::AppletPhotoViewer,
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
                cfg.key_combination = KEY_A;
            } else if (strcasecmp(value, "B") == 0) {
                cfg.key_combination = KEY_B;
            } else if (strcasecmp(value, "X") == 0) {
                cfg.key_combination = KEY_X;
            } else if (strcasecmp(value, "Y") == 0) {
                cfg.key_combination = KEY_Y;
            } else if (strcasecmp(value, "LS") == 0) {
                cfg.key_combination = KEY_LSTICK;
            } else if (strcasecmp(value, "RS") == 0) {
                cfg.key_combination = KEY_RSTICK;
            } else if (strcasecmp(value, "L") == 0) {
                cfg.key_combination = KEY_L;
            } else if (strcasecmp(value, "R") == 0) {
                cfg.key_combination = KEY_R;
            } else if (strcasecmp(value, "ZL") == 0) {
                cfg.key_combination = KEY_ZL;
            } else if (strcasecmp(value, "ZR") == 0) {
                cfg.key_combination = KEY_ZR;
            } else if (strcasecmp(value, "PLUS") == 0) {
                cfg.key_combination = KEY_PLUS;
            } else if (strcasecmp(value, "MINUS") == 0) {
                cfg.key_combination = KEY_MINUS;
            } else if (strcasecmp(value, "DLEFT") == 0) {
                cfg.key_combination = KEY_DLEFT;
            } else if (strcasecmp(value, "DUP") == 0) {
                cfg.key_combination = KEY_DUP;
            } else if (strcasecmp(value, "DRIGHT") == 0) {
                cfg.key_combination = KEY_DRIGHT;
            } else if (strcasecmp(value, "DDOWN") == 0) {
                cfg.key_combination = KEY_DDOWN;
            } else if (strcasecmp(value, "SL") == 0) {
                cfg.key_combination = KEY_SL;
            } else if (strcasecmp(value, "SR") == 0) {
                cfg.key_combination = KEY_SR;
            }

            return cfg;
        }

        int LoaderIniHandler(void *user, const char *section, const char *name, const char *value) {
            /* Taken and modified, with love, from Rajkosto's implementation. */
            if (strcasecmp(section, "hbl_config") == 0) {
                /* TODO: Consider deprecating "title_id" string in the future." */
                if (strcasecmp(name, "program_id") == 0 || strcasecmp(name, "title_id") == 0) {
                    u64 override_program_id = strtoul(value, NULL, 16);
                    if (override_program_id != 0) {
                        g_hbl_override_config.program_id = {override_program_id};
                    }
                } else if (strcasecmp(name, "path") == 0) {
                    while (*value == '/' || *value == '\\') {
                        value++;
                    }
                    std::snprintf(g_hbl_sd_path, sizeof(g_hbl_sd_path) - 1, "/%s", value);
                    g_hbl_sd_path[sizeof(g_hbl_sd_path) - 1] = '\0';
                } else if (strcasecmp(name, "override_key") == 0) {
                    g_hbl_override_config.override_key = ParseOverrideKey(value);
                } else if (strcasecmp(name, "override_any_app") == 0) {
                    if (strcasecmp(value, "true") == 0 || strcasecmp(value, "1") == 0) {
                        g_hbl_override_config.override_any_app = true;
                    } else if (strcasecmp(value, "false") == 0 || strcasecmp(value, "0") == 0) {
                        g_hbl_override_config.override_any_app = false;
                    } else {
                        /* I guess we default to not changing the value? */
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
                }
            } else {
                return 0;
            }
            return 1;
        }

        bool IsOverrideKeyHeld(const OverrideKey *cfg) {
            u64 kHeld = 0;
            bool keys_triggered = (R_SUCCEEDED(hid::GetKeysHeld(&kHeld)) && ((kHeld & cfg->key_combination) != 0));
            return IsSdCardInitialized() && (cfg->override_by_default ^ keys_triggered);
        }

        void ParseIniFile(util::ini::Handler handler, const char *path, void *user_ctx) {
            /* Mount the SD card. */
            FsFileSystem sd_fs = {};
            if (R_FAILED(fsOpenSdCardFileSystem(&sd_fs))) {
                return;
            }
            ON_SCOPE_EXIT { serviceClose(&sd_fs.s); };

            /* Open the file. */
            FsFile config_file;
            if (R_FAILED(fsFsOpenFile(&sd_fs, path, FsOpenMode_Read, &config_file))) {
                return;
            }
            ON_SCOPE_EXIT { fsFileClose(&config_file); };

            /* Parse the config. */
            util::ini::ParseFile(&config_file, user_ctx, handler);
        }

        void RefreshLoaderConfiguration() {
            ParseIniFile(LoaderIniHandler, "/atmosphere/loader.ini", nullptr);
        }

        ContentSpecificOverrideConfig GetContentOverrideConfig(ncm::ProgramId program_id) {
            char path[FS_MAX_PATH];
            std::snprintf(path, sizeof(path) - 1, "/atmosphere/contents/%016lx/config.ini", static_cast<u64>(program_id));

            ContentSpecificOverrideConfig config = {
                .override_key = g_default_override_key,
                .cheat_enable_key = g_default_cheat_enable_key,
            };
            ParseIniFile(ContentSpecificIniHandler, path, &config);
            return config;
        }

    }

    bool IsHblOverrideKeyHeld(ncm::ProgramId program_id) {
        /* If the SD card isn't initialized, we can't override. */
        if (!IsSdCardInitialized()) {
            return false;
        }

        /* For system modules and anything launched before the home menu, always override. */
        if (program_id < ncm::ProgramId::AppletStart || !pm::info::HasLaunchedProgram(ncm::ProgramId::AppletQlaunch)) {
            return true;
        }

        /* Unconditionally refresh loader.ini contents. */
        RefreshLoaderConfiguration();

        /* Check HBL config. */
        return IsHblProgramId(program_id) && IsOverrideKeyHeld(&g_hbl_override_config.override_key);
    }

    bool IsProgramOverrideKeyHeld(ncm::ProgramId program_id) {
        /* If the SD card isn't initialized, we can't override. */
        if (!IsSdCardInitialized()) {
            return false;
        }

        /* For system modules and anything launched before the home menu, always override. */
        if (program_id < ncm::ProgramId::AppletStart || !pm::info::HasLaunchedProgram(ncm::ProgramId::AppletQlaunch)) {
            return true;
        }

        /* Unconditionally refresh loader.ini contents. */
        RefreshLoaderConfiguration();

        const auto content_cfg = GetContentOverrideConfig(program_id);
        return IsOverrideKeyHeld(&content_cfg.override_key);
    }

    void GetOverrideKeyHeldStatus(bool *out_hbl, bool *out_program, ncm::ProgramId program_id) {

        /* If the SD card isn't initialized, we can't override. */
        if (!IsSdCardInitialized()) {
            *out_hbl = false;
            *out_program = false;
            return;
        }

        /* For system modules and anything launched before the home menu, always override. */
        if (program_id < ncm::ProgramId::AppletStart || !pm::info::HasLaunchedProgram(ncm::ProgramId::AppletQlaunch)) {
            *out_hbl = false;
            *out_program = true;
            return;
        }

        /* Unconditionally refresh loader.ini contents. */
        RefreshLoaderConfiguration();

        /* Set HBL output. */
        *out_hbl = IsHblProgramId(program_id) && IsOverrideKeyHeld(&g_hbl_override_config.override_key);

        /* Set content specific output. */
        const auto content_cfg = GetContentOverrideConfig(program_id);
        *out_program = IsOverrideKeyHeld(&content_cfg.override_key);
    }

    bool IsCheatEnableKeyHeld(ncm::ProgramId program_id) {
        /* If the SD card isn't initialized, don't apply cheats. */
        if (!IsSdCardInitialized()) {
            return false;
        }

        /* Don't apply cheats to HBL. */
        if (IsHblOverrideKeyHeld(program_id)) {
            return false;
        }

        const auto content_cfg = GetContentOverrideConfig(program_id);
        return IsOverrideKeyHeld(&content_cfg.cheat_enable_key);
    }

    /* HBL Configuration utilities. */
    bool IsHblProgramId(ncm::ProgramId program_id) {
        return (g_hbl_override_config.override_any_app && ncm::IsApplicationProgramId(program_id)) || (program_id == g_hbl_override_config.program_id);
    }

    const char *GetHblPath() {
        return g_hbl_sd_path;
    }

}
