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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include <inttypes.h>

#include "utils.h"
#include "fs_utils.h"
#include "nxboot.h"
#include "nxfs.h"
#include "bct.h"
#include "car.h"
#include "di.h"
#include "mc.h"
#include "se.h"
#include "pmc.h"
#include "emc.h"
#include "fuse.h"
#include "i2c.h"
#include "ips.h"
#include "stratosphere.h"
#include "max77620.h"
#include "cluster.h"
#include "flow.h"
#include "timers.h"
#include "key_derivation.h"
#include "masterkey.h"
#include "package1.h"
#include "package2.h"
#include "smmu.h"
#include "tsec.h"
#include "lp0.h"
#include "loader.h"
#include "exocfg.h"
#include "../../../fusee/common/display/video_fb.h"
#include "../../../fusee/common/ini.h"
#include "../../../fusee/common/log.h"
#include "splash_screen.h"

#define u8 uint8_t
#define u32 uint32_t
#include "exosphere_bin.h"
#include "mariko_fatal_bin.h"
#include "mesosphere_bin.h"
#include "sept_secondary_00_enc.h"
#include "sept_secondary_01_enc.h"
#include "sept_secondary_dev_00_enc.h"
#include "sept_secondary_dev_01_enc.h"
#include "warmboot_bin.h"
#include "emummc_kip.h"
#undef u8
#undef u32

extern const uint8_t warmboot_bin[];

extern int fusee_is_experimental(void);

static const uint8_t retail_pkc_modulus[0x100] = {
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

static const uint8_t dev_pkc_modulus[0x100] = {
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

/* Determine the current SoC for Mariko specific code. */
static bool is_soc_mariko() {
    return (fuse_get_soc_type() == 1);
}

static int emummc_ini_handler(void *user, const char *section, const char *name, const char *value) {
    emummc_config_t *emummc_cfg = (emummc_config_t *)user;
    if (strcmp(section, "emummc") == 0) {
        if (strcmp(name, EMUMMC_ENABLED_KEY) == 0) {
            int tmp = 0;
            sscanf(value, "%d", &tmp);
            emummc_cfg->enabled = (tmp != 0);
        }
        if (strcmp(name, EMUMMC_SECTOR_KEY) == 0) {
            uintptr_t sector = 0;
            sscanf(value, "%x", &sector);
            emummc_cfg->sector = sector;
        } else if (strcmp(name, EMUMMC_ID_KEY) == 0) {
            sscanf(value, "%lx", &emummc_cfg->id);
        } else if (strcmp(name, EMUMMC_PATH_KEY) == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
            strncpy(emummc_cfg->path, value, sizeof(emummc_cfg->path) - 1);
#pragma GCC diagnostic pop
            emummc_cfg->path[sizeof(emummc_cfg->path) - 1]  = '\0';
        } else if (strcmp(name, EMUMMC_NINTENDO_PATH_KEY) == 0) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
            strncpy(emummc_cfg->nintendo_path, value, sizeof(emummc_cfg->nintendo_path) - 1);
#pragma GCC diagnostic pop
            emummc_cfg->nintendo_path[sizeof(emummc_cfg->nintendo_path) - 1]  = '\0';
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

static int exosphere_ini_handler(void *user, const char *section, const char *name, const char *value) {
    exosphere_parse_cfg_t *parse_cfg = (exosphere_parse_cfg_t *)user;
    int tmp = 0;
    if (strcmp(section, "exosphere") == 0) {
        if (strcmp(name, EXOSPHERE_DEBUGMODE_PRIV_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp == 1) {
                parse_cfg->debugmode = 1;
            } else if (tmp == 0) {
                parse_cfg->debugmode = 0;
            }
        } else if (strcmp(name, EXOSPHERE_DEBUGMODE_USER_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp == 1) {
                parse_cfg->debugmode_user = 1;
            } else if (tmp == 0) {
                parse_cfg->debugmode_user = 0;
            }
        } else if (strcmp(name, EXOSPHERE_DISABLE_USERMODE_EXCEPTION_HANDLERS_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp == 1) {
                parse_cfg->disable_user_exception_handlers = 1;
            } else if (tmp == 0) {
                parse_cfg->disable_user_exception_handlers = 0;
            }
        } else if (strcmp(name, EXOSPHERE_ENABLE_USERMODE_PMU_ACCESS_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp == 1) {
                parse_cfg->enable_user_pmu_access = 1;
            } else if (tmp == 0) {
                parse_cfg->enable_user_pmu_access = 0;
            }
        } else if (strcmp(name, EXOSPHERE_BLANK_PRODINFO_SYSMMC_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp == 1) {
                parse_cfg->blank_prodinfo_sysmmc = 1;
            } else if (tmp == 0) {
                parse_cfg->blank_prodinfo_sysmmc = 0;
            }
        } else if (strcmp(name, EXOSPHERE_BLANK_PRODINFO_EMUMMC_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp == 1) {
                parse_cfg->blank_prodinfo_emummc = 1;
            } else if (tmp == 0) {
                parse_cfg->blank_prodinfo_emummc = 0;
            }
        } else if (strcmp(name, EXOSPHERE_ALLOW_WRITING_TO_CAL_SYSMMC_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp == 1) {
                parse_cfg->allow_writing_to_cal_sysmmc = 1;
            } else if (tmp == 0) {
                parse_cfg->allow_writing_to_cal_sysmmc = 0;
            }
        } else if (strcmp(name, EXOSPHERE_LOG_PORT_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (0 <= tmp && tmp < 4) {
                parse_cfg->log_port = tmp;
            } else {
                parse_cfg->log_port = 0;
            }
        } else if (strcmp(name, EXOSPHERE_LOG_BAUD_RATE_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp > 0) {
                parse_cfg->log_baud_rate = tmp;
            } else {
                parse_cfg->log_baud_rate = 115200;
            }
        } else if (strcmp(name, EXOSPHERE_LOG_INVERTED_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp == 1) {
                parse_cfg->log_inverted = 1;
            } else if (tmp == 0) {
                parse_cfg->log_inverted = 0;
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

static int stratosphere_ini_handler(void *user, const char *section, const char *name, const char *value) {
    stratosphere_cfg_t *strat_cfg = (stratosphere_cfg_t *)user;
    int tmp = 0;
    if (strcmp(section, "stratosphere") == 0) {
        if (strcmp(name, STRATOSPHERE_NOGC_KEY) == 0) {
            strat_cfg->has_nogc_config = true;
            sscanf(value, "%d", &tmp);
            strat_cfg->enable_nogc = tmp != 0;
        } else if (strcmp(name, STRATOSPHERE_DISABLE_NCM_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            strat_cfg->ncm_disabled = tmp != 0;
            if (strat_cfg->ncm_disabled) {
                stratosphere_disable_ncm();
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

static bool is_nca_present(const char *nca_name) {
    char path[0x100];
    snprintf(path, sizeof(path), "system:/contents/registered/%s.nca", nca_name);

    return is_valid_concatenation_file(path);
}


static uint32_t nxboot_get_specific_target_firmware(uint32_t target_firmware){
    #define CHECK_NCA(NCA_ID, VERSION) do { if (is_nca_present(NCA_ID)) { return ATMOSPHERE_TARGET_FIRMWARE_##VERSION; } } while(0)

    if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_11_0_0) {
        CHECK_NCA("56211c7a5ed20a5332f5cdda67121e37", 11_0_1);
        CHECK_NCA("594c90bcdbcccad6b062eadba0cd0e7e", 11_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_10_0_0) {
        CHECK_NCA("26325de4db3909e0ef2379787c7e671d", 10_2_0);
        CHECK_NCA("5077973537f6735b564dd7475b779f87", 10_1_1); /* Exclusive to China. */
        CHECK_NCA("fd1faed0ca750700d254c0915b93d506", 10_1_0);
        CHECK_NCA("34728c771299443420820d8ae490ea41", 10_0_4);
        CHECK_NCA("5b1df84f88c3334335bbb45d8522cbb4", 10_0_3);
        CHECK_NCA("e951bc9dedcd54f65ffd83d4d050f9e0", 10_0_2);
        CHECK_NCA("36ab1acf0c10a2beb9f7d472685f9a89", 10_0_1);
        CHECK_NCA("5625cdc21d5f1ca52f6c36ba261505b9", 10_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_9_1_0) {
        CHECK_NCA("09ef4d92bb47b33861e695ba524a2c17", 9_2_0);
        CHECK_NCA("c5fbb49f2e3648c8cfca758020c53ecb", 9_1_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_9_0_0) {
        CHECK_NCA("fd1ffb82dc1da76346343de22edbc97c", 9_0_1);
        CHECK_NCA("a6af05b33f8f903aab90c8b0fcbcc6a4", 9_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_8_1_0) {
        CHECK_NCA("724d9b432929ea43e787ad81bf09ae65", 8_1_1); /* 8.1.1-100 from Lite */
        CHECK_NCA("e9bb0602e939270a9348bddd9b78827b", 8_1_1); /* 8.1.1-12  from chinese gamecard */
        CHECK_NCA("7eedb7006ad855ec567114be601b2a9d", 8_1_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_8_0_0) {
        CHECK_NCA("6c5426d27c40288302ad616307867eba", 8_0_1);
        CHECK_NCA("4fe7b4abcea4a0bcc50975c1a926efcb", 8_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_7_0_0) {
        CHECK_NCA("e6b22c40bb4fa66a151f1dc8db5a7b5c", 7_0_1);
        CHECK_NCA("c613bd9660478de69bc8d0e2e7ea9949", 7_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_6_2_0) {
        CHECK_NCA("6dfaaf1a3cebda6307aa770d9303d9b6", 6_2_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_6_0_0) {
        CHECK_NCA("1d21680af5a034d626693674faf81b02", 6_1_0);
        CHECK_NCA("663e74e45ffc86fbbaeb98045feea315", 6_0_1);
        CHECK_NCA("258c1786b0f6844250f34d9c6f66095b", 6_0_0); /* Release     6.0.0-5.0 */
        CHECK_NCA("286e30bafd7e4197df6551ad802dd815", 6_0_0); /* Pre-Release 6.0.0-4.0 */
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_5_0_0) {
        CHECK_NCA("fce3b0ea366f9c95fe6498b69274b0e7", 5_1_0);
        CHECK_NCA("c5758b0cb8c6512e8967e38842d35016", 5_0_2);
        CHECK_NCA("53eb605d4620e8fd50064b24fd57783a", 5_0_1);
        CHECK_NCA("09a2f9c16ce1c121ae6d231b35d17515", 5_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_4_0_0) {
        CHECK_NCA("77e1ae7661ad8a718b9b13b70304aeea", 4_1_0);
        CHECK_NCA("d0e5d20e3260f3083bcc067483b71274", 4_0_1);
        CHECK_NCA("483a24ee3fd7149f9112d1931166a678", 4_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_3_0_0) {
        CHECK_NCA("704129fc89e1fcb85c37b3112e51b0fc", 3_0_2);
        CHECK_NCA("1fb00543307337d523ccefa9923e0c50", 3_0_1);
        CHECK_NCA("6ebd3447473bade18badbeb5032af87d", 3_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_2_0_0) {
        CHECK_NCA("d1c991c53a8a9038f8c3157a553d876d", 2_3_0);
        CHECK_NCA("7f90353dff2d7ce69e19e07ebc0d5489", 2_2_0);
        CHECK_NCA("e9b3e75fce00e52fe646156634d229b4", 2_1_0);
        CHECK_NCA("7a1f79f8184d4b9bae1755090278f52c", 2_0_0);
    } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_1_0_0) {
        CHECK_NCA("a1b287e07f8455e8192f13d0e45a2aaf", 1_0_0); /* 1.0.0 from Factory */
        CHECK_NCA("117f7b9c7da3e8cef02340596af206b3", 1_0_0); /* 1.0.0 from Gamecard */
    } else {
        fatal_error("[NXBOOT] Unknown Target Firmware!");
    }

    #undef CHECK_NCA

    /* If we didn't find a more specific firmware, return our package1 approximation. */
    return target_firmware;
}

static uint32_t nxboot_get_target_firmware(const void *package1loader) {
    const package1loader_header_t *package1loader_header = (const package1loader_header_t *)package1loader;
    switch (package1loader_header->version) {
        case 0x01:          /* 1.0.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_1_0_0;
        case 0x02:          /* 2.0.0 - 2.3.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_2_0_0;
        case 0x04:          /* 3.0.0 and 3.0.1 - 3.0.2 */
            return ATMOSPHERE_TARGET_FIRMWARE_3_0_0;
        case 0x07:          /* 4.0.0 - 4.1.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_4_0_0;
        case 0x0B:          /* 5.0.0 - 5.1.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_5_0_0;
        case 0x0E: {        /* 6.0.0 - 6.2.0 */
            if (memcmp(package1loader_header->build_timestamp, "20180802", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_6_0_0;
            } else if (memcmp(package1loader_header->build_timestamp, "20181107", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_6_2_0;
            } else {
                fatal_error("[NXBOOT] Unable to identify package1!\n");
            }
        }
        case 0x0F:          /* 7.0.0 - 7.0.1 */
            return ATMOSPHERE_TARGET_FIRMWARE_7_0_0;
        case 0x10: {        /* 8.0.0 - 9.0.0 */
            if (memcmp(package1loader_header->build_timestamp, "20190314", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_8_0_0;
            } else if (memcmp(package1loader_header->build_timestamp, "20190531", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_8_1_0;
            } else if (memcmp(package1loader_header->build_timestamp, "20190809", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_9_0_0;
            } else if (memcmp(package1loader_header->build_timestamp, "20191021", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_9_1_0;
            } else if (memcmp(package1loader_header->build_timestamp, "20200303", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_10_0_0;
            } else if (memcmp(package1loader_header->build_timestamp, "20201030", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_11_0_0;
            } else {
                fatal_error("[NXBOOT] Unable to identify package1!\n");
            }
        }
        default:
            fatal_error("[NXBOOT] Unable to identify package1!\n");
    }
}

static bool nxboot_configure_emummc(exo_emummc_config_t *exo_emummc_config) {
    emummc_config_t emummc_cfg = {.enabled = false, .id = 0, .sector = 0, .path = "", .nintendo_path = ""};

    /* Initialize some defaults. */
    memset(exo_emummc_config, 0, sizeof(*exo_emummc_config));
    exo_emummc_config->base_cfg.magic      = MAGIC_EMUMMC_CONFIG;
    exo_emummc_config->base_cfg.type       = EMUMMC_TYPE_NONE;
    exo_emummc_config->base_cfg.id         = 0;
    exo_emummc_config->base_cfg.fs_version = FS_VER_1_0_0; /* Will be filled out later. */

    char *emummc_ini = calloc(1, 0x10000);
    if (!read_from_file(emummc_ini, 0xFFFF, "emummc/emummc.ini")) {
        free(emummc_ini);
        return false;
    }

    /* Load emummc settings from emummc.ini file. */
    if (ini_parse_string(emummc_ini, emummc_ini_handler, &emummc_cfg) < 0) {
        fatal_error("[NXBOOT] Failed to parse emummc.ini!\n");
    }

    free(emummc_ini);

    /* Initialize values from emummc config. */
    exo_emummc_config->base_cfg.id         = emummc_cfg.id;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
    strncpy(exo_emummc_config->emu_dir_path, emummc_cfg.nintendo_path, sizeof(exo_emummc_config->emu_dir_path));
#pragma GCC diagnostic pop
    exo_emummc_config->emu_dir_path[sizeof(exo_emummc_config->emu_dir_path) - 1] = '\0';

    if (emummc_cfg.enabled) {
        if (emummc_cfg.sector > 0) {
            exo_emummc_config->base_cfg.type  = EMUMMC_TYPE_PARTITION;
            exo_emummc_config->partition_cfg.start_sector = emummc_cfg.sector;

            /* Mount emulated NAND from SD card partition. */
            if (nxfs_mount_emummc_partition(emummc_cfg.sector) < 0) {
                fatal_error("[NXBOOT] Failed to mount EmuMMC from SD card partition!\n");
            }
        } else if (is_valid_folder(emummc_cfg.path)) {
            exo_emummc_config->base_cfg.type  = EMUMMC_TYPE_FILES;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
            strncpy(exo_emummc_config->file_cfg.path, emummc_cfg.path, sizeof(exo_emummc_config->file_cfg.path));
#pragma GCC diagnostic pop
            exo_emummc_config->file_cfg.path[sizeof(exo_emummc_config->file_cfg.path) - 1] = '\0';

            int num_parts = 0;
            uint64_t part_limit = 0;
            char emummc_path[0x100 + 1] = {0};
            char emummc_boot0_path[0x300 + 1] = {0};
            char emummc_boot1_path[0x300 + 1] = {0};
            char emummc_rawnand_path[0x300 + 1] = {0};

            /* Prepare base folder path. */
            snprintf(emummc_path, sizeof(emummc_path) - 1, "%s/%s", emummc_cfg.path, "eMMC");

            /* Check if eMMC folder is present. */
            if (!is_valid_folder(emummc_path)) {
                fatal_error("[NXBOOT] Failed to find EmuMMC eMMC folder!\n");
            }

            /* Prepare expected file paths. */
            snprintf(emummc_boot0_path, sizeof(emummc_boot0_path) - 1, "%s/%s", emummc_path, "boot0");
            snprintf(emummc_boot1_path, sizeof(emummc_boot1_path) - 1, "%s/%s", emummc_path, "boot1");

            /* Check if boot0 and boot1 image files are present. */
            if (!is_valid_file(emummc_boot0_path) || !is_valid_file(emummc_boot1_path)) {
                fatal_error("[NXBOOT] Failed to find EmuMMC boot0/boot1 image files!\n");
            }

            /* Find raw image files (single or multi part). */
            for (int i = 0; i < 64; i++) {
                snprintf(emummc_rawnand_path, sizeof(emummc_rawnand_path) - 1, "%s/%02d", emummc_path, i);
                if (is_valid_file(emummc_rawnand_path)) {
                    if (i == 0) {
                        /* The size of the first file should tell us the part limit. */
                        part_limit = get_file_size(emummc_rawnand_path);
                    }
                    num_parts++;
                } else {
                    /* No more image files. */
                    break;
                }
            }

            /* Check if at least one raw image file is present. */
            if ((num_parts == 0) || (part_limit == 0)) {
                fatal_error("[NXBOOT] Failed to find EmuMMC raw image files!\n");
            }

            /* Mount emulated NAND from files. */
            if (nxfs_mount_emummc_file(emummc_path, num_parts, part_limit) < 0) {
                fatal_error("[NXBOOT] Failed to mount EmuMMC from files!\n");
            }
        } else {
            fatal_error("[NXBOOT] Invalid EmuMMC setting!\n");
        }
    }

    return emummc_cfg.enabled;
}

static void nxboot_configure_exosphere(uint32_t target_firmware, unsigned int keygen_type, exo_emummc_config_t *exo_emummc_cfg) {
    exosphere_config_t exo_cfg = {0};

    exo_cfg.magic = MAGIC_EXOSPHERE_CONFIG;
    exo_cfg.target_firmware = target_firmware;
    memcpy(&exo_cfg.emummc_cfg, exo_emummc_cfg, sizeof(*exo_emummc_cfg));

    const bool is_emummc = exo_emummc_cfg->base_cfg.magic == MAGIC_EMUMMC_CONFIG && exo_emummc_cfg->base_cfg.type != EMUMMC_TYPE_NONE;

    if (keygen_type) {
        exo_cfg.flags[0] = EXOSPHERE_FLAG_PERFORM_620_KEYGEN;
    } else {
        exo_cfg.flags[0] = 0;
    }

    /* Setup exosphere parse configuration with defaults. */
    exosphere_parse_cfg_t parse_cfg = {
        .debugmode                          = 1,
        .debugmode_user                     = 0,
        .disable_user_exception_handlers    = 0,
        .enable_user_pmu_access             = 0,
        .blank_prodinfo_sysmmc              = 0,
        .blank_prodinfo_emummc              = 0,
        .allow_writing_to_cal_sysmmc        = 0,
        .log_port                           = 0,
        .log_baud_rate                      = 115200,
        .log_inverted                       = 0,
    };

    /* If we have an ini to read, parse it. */
    char *exosphere_ini = calloc(1, 0x10000);
    if (read_from_file(exosphere_ini, 0xFFFF, "exosphere.ini")) {
        if (ini_parse_string(exosphere_ini, exosphere_ini_handler, &parse_cfg) < 0) {
            fatal_error("[NXBOOT] Failed to parse exosphere.ini!\n");
        }
    }
    free(exosphere_ini);

    /* Apply parse config. */
    if (parse_cfg.debugmode)                           exo_cfg.flags[0] |= EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV;
    if (parse_cfg.debugmode_user)                      exo_cfg.flags[0] |= EXOSPHERE_FLAG_IS_DEBUGMODE_USER;
    if (parse_cfg.disable_user_exception_handlers)     exo_cfg.flags[0] |= EXOSPHERE_FLAG_DISABLE_USERMODE_EXCEPTION_HANDLERS;
    if (parse_cfg.enable_user_pmu_access)              exo_cfg.flags[0] |= EXOSPHERE_FLAG_ENABLE_USERMODE_PMU_ACCESS;
    if (parse_cfg.blank_prodinfo_sysmmc && !is_emummc) exo_cfg.flags[0] |= EXOSPHERE_FLAG_BLANK_PRODINFO;
    if (parse_cfg.blank_prodinfo_emummc &&  is_emummc) exo_cfg.flags[0] |= EXOSPHERE_FLAG_BLANK_PRODINFO;
    if (parse_cfg.allow_writing_to_cal_sysmmc)         exo_cfg.flags[0] |= EXOSPHERE_FLAG_ALLOW_WRITING_TO_CAL_SYSMMC;

    exo_cfg.log_port      = parse_cfg.log_port;
    exo_cfg.log_baud_rate = parse_cfg.log_baud_rate;
    if (parse_cfg.log_inverted) exo_cfg.log_flags |= EXOSPHERE_LOG_FLAG_INVERTED;

    /* Apply lcd vendor. */
    exo_cfg.lcd_vendor = display_get_lcd_vendor();

    if ((exo_cfg.target_firmware < ATMOSPHERE_TARGET_FIRMWARE_MIN) || (exo_cfg.target_firmware > ATMOSPHERE_TARGET_FIRMWARE_MAX)) {
        fatal_error("[NXBOOT] Invalid Exosphere target firmware!\n");
    }

    *(MAILBOX_EXOSPHERE_CONFIGURATION) = exo_cfg;
}

static void nxboot_configure_stratosphere(uint32_t target_firmware) {
    stratosphere_cfg_t strat_cfg = {0};
    if (ini_parse_string(get_loader_ctx()->bct0, stratosphere_ini_handler, &strat_cfg) < 0) {
        fatal_error("[NXBOOT] Failed to parse BCT.ini!\n");
    }

    /* Enable NOGC patches if the user requested it, or if the user is booting into 4.0.0+ with 3.0.2- fuses. */
    if (strat_cfg.has_nogc_config) {
        if (strat_cfg.enable_nogc) {
            kip_patches_set_enable_nogc();
        }
    } else {
        /* Check if fuses are < 4.0.0, but firmware is >= 4.0.0 */
        if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_4_0_0 && !(fuse_get_reserved_odm(7) & ~0x0000000F)) {
            kip_patches_set_enable_nogc();
        }
        /* Check if the fuses are < 9.0.0, but firmware is >= 9.0.0 */
        if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_9_0_0 && !(fuse_get_reserved_odm(7) & ~0x000003FF)) {
            kip_patches_set_enable_nogc();
        }
        /* Check if the fuses are < 11.0.0, but firmware is >= 11.0.0 */
        if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_11_0_0 && !(fuse_get_reserved_odm(7) & ~0x00001FFF)) {
            kip_patches_set_enable_nogc();
        }
    }
}

static void nxboot_set_bootreason(void *bootreason_base) {
    boot_reason_t boot_reason = {0};
    FILE *boot0;
    nvboot_config_table *bct;
    nv_bootloader_info *bootloader_info;

    /* Allocate memory for the BCT. */
    bct = malloc(sizeof(nvboot_config_table));
    if (bct == NULL) {
        fatal_error("[NXBOOT] Out of memory!\n");
    }

    /* Open boot0. */
    boot0 = fopen("boot0:/", "rb");
    if (boot0 == NULL) {
        fatal_error("[NXBOOT] Failed to open boot0!\n");
    }

    /* Read the BCT. */
    if (fread(bct, sizeof(nvboot_config_table), 1, boot0) == 0) {
        fatal_error("[NXBOOT] Failed to read the BCT!\n");
    }

    /* Check if we need to use BCT 2 instead. */
    if (package1_is_custom_public_key(bct, is_soc_mariko())) {
        if (fseek(boot0, 0x4000 * 2, SEEK_SET) != 0) {
            fatal_error("[NXBOOT] Failed to seek to BCT!\n");
        }
        if (fread(bct, sizeof(nvboot_config_table), 1, boot0) == 0) {
            fatal_error("[NXBOOT] Failed to read the BCT!\n");
        }
    }

    /* Close boot0. */
    fclose(boot0);

    /* Populate bootloader parameters. */
    bootloader_info = &bct->bootloader[0];
    boot_reason.bootloader_version = bootloader_info->version;
    boot_reason.bootloader_start_block = bootloader_info->start_blk;
    boot_reason.bootloader_start_page = bootloader_info->start_page;
    boot_reason.bootloader_attribute = bootloader_info->attribute;

    uint8_t power_key_intr = 0;
    uint8_t rtc_intr = 0;
    i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_ONOFFIRQ, &power_key_intr, 1);
    i2c_query(I2C_5, MAX77620_RTC_I2C_ADDR, MAX77620_REG_RTCINT, &rtc_intr, 1);

    /* Set PMIC value. */
    boot_reason.boot_reason_value = ((rtc_intr << 0x08) | power_key_intr);

    if (power_key_intr & 0x80)
        boot_reason.boot_reason_state = 0x01;       /* BootReason_AcOk */
    else if (power_key_intr & 0x08)
        boot_reason.boot_reason_state = 0x02;       /* BootReason_OnKey */
    else if (rtc_intr & 0x02)
        boot_reason.boot_reason_state = 0x03;       /* BootReason_RtcAlarm1 */
    else if (rtc_intr & 0x04)
        boot_reason.boot_reason_state = 0x04;       /* BootReason_RtcAlarm2 */

    /* Set in memory. */
    memcpy(bootreason_base, &boot_reason, sizeof(boot_reason));

    /* Clean up. */
    free(bct);
}

static void nxboot_move_bootconfig() {
    FILE *bcfile;
    void *bootconfig;
    uint32_t bootconfig_addr;
    uint32_t bootconfig_size;

    /* Allocate memory for reading BootConfig. */
    bootconfig = memalign(0x1000, 0x4000);
    if (bootconfig == NULL) {
        fatal_error("[NXBOOT] Out of memory!\n");
    }

    /* Get BootConfig from the Package2 partition. */
    bcfile = fopen("bcpkg21:/", "rb");
    if (bcfile == NULL) {
        fatal_error("[NXBOOT] Failed to open BootConfig from eMMC!\n");
    }
    if (fread(bootconfig, 0x4000, 1, bcfile) < 1) {
        fclose(bcfile);
        fatal_error("[NXBOOT] Failed to read BootConfig!\n");
    }
    fclose(bcfile);

    /* Select the actual BootConfig size and destination address. */
    /* NOTE: Nintendo relies on BPMP's inability to data abort and tries to copy 0x1000 bytes. */
    bootconfig_addr = 0x4003F800;
    bootconfig_size = 0x1000;

    /* Copy the BootConfig into IRAM. */
    memset((void *)bootconfig_addr, 0, bootconfig_size);
    memcpy((void *)bootconfig_addr, bootconfig, bootconfig_size);

    /* Clean up. */
    free(bootconfig);
}

static bool get_and_clear_has_run_sept(void) {
    bool has_run_sept = (MAKE_EMC_REG(EMC_SCRATCH0) & 0x80000000) != 0;
    MAKE_EMC_REG(EMC_SCRATCH0) &= ~0x80000000;
    return has_run_sept;
}

static void get_mariko_warmboot_path(char *dst, size_t dst_size, uint32_t version) {
    snprintf(dst, dst_size, "warmboot_mariko/wb_%02" PRIx32 ".bin", version);
}

/* This is the main function responsible for booting Horizon. */
static nx_keyblob_t __attribute__((aligned(16))) g_keyblobs[32];
uint32_t nxboot_main(void) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    loader_ctx_t *loader_ctx = get_loader_ctx();
    const bool is_experimental = fusee_is_experimental();
    bool is_mariko = is_soc_mariko();
    package2_header_t *package2;
    size_t package2_size;
    void *tsec_fw;
    size_t tsec_fw_size;
    const void *sept_secondary_enc = NULL;
    size_t sept_secondary_enc_size = 0;
    void *warmboot_fw;
    size_t warmboot_fw_size;
    void *warmboot_memaddr;
    void *package1loader;
    size_t package1loader_size;
    void *mesosphere;
    size_t mesosphere_size;
    void *emummc;
    size_t emummc_size;
    uint32_t available_revision;
    FILE *boot0, *pk2file;
    void *exosphere_memaddr;
    exo_emummc_config_t exo_emummc_cfg;

    /* Configure emummc or mount the real NAND. */
    if (!nxboot_configure_emummc(&exo_emummc_cfg)) {
        emummc = NULL;
        emummc_size = 0;
        if (nxfs_mount_emmc() < 0) {
            fatal_error("[NXBOOT] Failed to mount eMMC!\n");
        }
    } else {
        emummc_size = get_file_size("atmosphere/emummc.kip");
        if (emummc_size != 0) {
            /* Allocate memory for the emummc KIP. */
            emummc = memalign(0x100, emummc_size);

            if (emummc == NULL) {
                fatal_error("[NXBOOT] Out of memory!\n");
            }
            if (read_from_file(emummc, emummc_size, "atmosphere/emummc.kip") != emummc_size) {
                fatal_error("[NXBOOT] Could not read the emummc kip!\n");
            }
        } else {
            /* Use embedded copy. */
            emummc_size = emummc_kip_size;
            emummc = memalign(0x100, emummc_size);

            if (emummc == NULL) {
                fatal_error("[NXBOOT] Out of memory!\n");
            }

            memcpy(emummc, emummc_kip, emummc_size);
        }
    }

    /* Allocate memory for reading Package2. */
    package2 = memalign(0x1000, PACKAGE2_SIZE_MAX);
    if (package2 == NULL) {
        fatal_error("[NXBOOT] Out of memory!\n");
    }

    /* Read Package2 from a file, otherwise from its partition(s). */
    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Reading package2...\n");
    if (loader_ctx->package2_path[0] != '\0') {
        pk2file = fopen(loader_ctx->package2_path, "rb");
        if (pk2file == NULL) {
            fatal_error("[NXBOOT] Failed to open Package2 from %s: %s!\n", loader_ctx->package2_path, strerror(errno));
        }
    } else {
        pk2file = fopen("bcpkg21:/", "rb");
        if (pk2file == NULL) {
            fatal_error("[NXBOOT] Failed to open Package2 from eMMC: %s!\n", strerror(errno));
        }
        if (fseek(pk2file, 0x4000, SEEK_SET) != 0) {
            fclose(pk2file);
            fatal_error("[NXBOOT] Failed to seek Package2 in eMMC: %s!\n", strerror(errno));
        }
    }

    setvbuf(pk2file, NULL, _IONBF, 0); /* Workaround. */
    if (fread(package2, sizeof(package2_header_t), 1, pk2file) < 1) {
        fclose(pk2file);
        fatal_error("[NXBOOT] Failed to read Package2!\n");
    }
    package2_size = package2_meta_get_size(&package2->metadata);
    if ((package2_size > PACKAGE2_SIZE_MAX) || (package2_size <= sizeof(package2_header_t))) {
        fclose(pk2file);
        fatal_error("[NXBOOT] Package2 is too big or too small!\n");
    }
    if (fread(package2->data, package2_size - sizeof(package2_header_t), 1, pk2file) < 1) {
        fclose(pk2file);
        fatal_error("[NXBOOT] Failed to read Package2!\n");
    }
    fclose(pk2file);

    /* Read and parse boot0. */
    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Reading boot0...\n");
    boot0 = fopen("boot0:/", "rb");
    if (boot0 == NULL) {
        fatal_error("[NXBOOT] Failed to open boot0: %s!\n", strerror(errno));
    }
    if (is_mariko) {
        if (package1_read_and_parse_boot0_mariko(&package1loader, &package1loader_size, boot0) == -1) {
            fatal_error("[NXBOOT] Couldn't parse boot0: %s!\n", strerror(errno));
        }
    } else {
        if (package1_read_and_parse_boot0_erista(&package1loader, &package1loader_size, g_keyblobs, &available_revision, boot0) == -1) {
            fatal_error("[NXBOOT] Couldn't parse boot0: %s!\n", strerror(errno));
        }
    }
    fclose(boot0);

    /* Find the system's target firmware. */
    uint32_t target_firmware = nxboot_get_target_firmware(package1loader);

    if (!target_firmware) {
        fatal_error("[NXBOOT] Failed to detect target firmware!\n");
    } else {
        print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Detected target firmware %ld!\n", target_firmware);
    }

    /* Handle TSEC and Sept (Erista only). */
    uint8_t tsec_key[0x10] = {0};
    uint8_t tsec_root_keys[0x20][0x10] = {0};
    if (!is_mariko) {
        /* Read the TSEC firmware from a file, otherwise from PK1L. */
        if (loader_ctx->tsecfw_path[0] != '\0') {
            tsec_fw_size = get_file_size(loader_ctx->tsecfw_path);
            if ((tsec_fw_size != 0) && (tsec_fw_size != 0xF00 && tsec_fw_size != 0x2900 && tsec_fw_size != 0x3000 && tsec_fw_size != 0x3300)) {
                fatal_error("[NXBOOT] TSEC firmware from %s has a wrong size!\n", loader_ctx->tsecfw_path);
            } else if (tsec_fw_size == 0) {
                fatal_error("[NXBOOT] Could not read the TSEC firmware from %s!\n", loader_ctx->tsecfw_path);
            }

            /* Allocate memory for the TSEC firmware. */
            tsec_fw = memalign(0x100, tsec_fw_size);

            if (tsec_fw == NULL) {
                fatal_error("[NXBOOT] Out of memory!\n");
            }
            if (read_from_file(tsec_fw, tsec_fw_size, loader_ctx->tsecfw_path) != tsec_fw_size) {
                fatal_error("[NXBOOT] Could not read the TSEC firmware from %s!\n", loader_ctx->tsecfw_path);
            }

            if (tsec_fw_size == 0x3000) {
                if (fuse_get_hardware_state() != 0) {
                    sept_secondary_enc = sept_secondary_00_enc;
                    sept_secondary_enc_size = sept_secondary_00_enc_size;
                } else {
                    sept_secondary_enc = sept_secondary_dev_00_enc;
                    sept_secondary_enc_size = sept_secondary_dev_00_enc_size;
                }
            } else if (tsec_fw_size == 0x3300) {
                if (fuse_get_hardware_state() != 0) {
                    sept_secondary_enc = sept_secondary_01_enc;
                    sept_secondary_enc_size = sept_secondary_01_enc_size;
                } else {
                    sept_secondary_enc = sept_secondary_dev_01_enc;
                    sept_secondary_enc_size = sept_secondary_dev_01_enc_size;
                }
            } else {
                fatal_error("[NXBOOT] Unable to identify sept revision to run.");
            }
        } else {
            if (!package1_get_tsec_fw(&tsec_fw, package1loader, package1loader_size)) {
                fatal_error("[NXBOOT] Failed to read the TSEC firmware from Package1loader!\n");
            }
            if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_8_1_0) {
                if (fuse_get_hardware_state() != 0) {
                    sept_secondary_enc = sept_secondary_01_enc;
                    sept_secondary_enc_size = sept_secondary_01_enc_size;
                } else {
                    sept_secondary_enc = sept_secondary_dev_01_enc;
                    sept_secondary_enc_size = sept_secondary_dev_01_enc_size;
                }
                tsec_fw_size = 0x3300;
            } else if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_7_0_0) {
                if (fuse_get_hardware_state() != 0) {
                    sept_secondary_enc = sept_secondary_00_enc;
                    sept_secondary_enc_size = sept_secondary_00_enc_size;
                } else {
                    sept_secondary_enc = sept_secondary_dev_00_enc;
                    sept_secondary_enc_size = sept_secondary_dev_00_enc_size;
                }
                tsec_fw_size = 0x3000;
            } else if (target_firmware == ATMOSPHERE_TARGET_FIRMWARE_6_2_0) {
                tsec_fw_size = 0x2900;
            } else {
                tsec_fw_size = 0xF00;
            }
        }

        print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Loaded firmware from eMMC...\n");

        /* Get the TSEC keys. */
        if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_7_0_0) {
            /* Detect whether we need to run sept-secondary in order to derive keys. */
            if (!get_and_clear_has_run_sept()) {
                reboot_to_sept(tsec_fw, tsec_fw_size, sept_secondary_enc, sept_secondary_enc_size);
            } else {
                if (mkey_detect_revision(fuse_get_hardware_state() != 0) != 0) {
                    fatal_error("[NXBOOT] Sept derived incorrect keys!\n");
                }
            }
            get_and_clear_has_run_sept();
        } else if (target_firmware == ATMOSPHERE_TARGET_FIRMWARE_6_2_0) {
            uint8_t tsec_keys[0x20] = {0};

            /* Emulate the TSEC payload on 6.2.0+. */
            smmu_emulate_tsec((void *)tsec_keys, package1loader, package1loader_size, package1loader);

            /* Copy back the keys. */
            memcpy((void *)tsec_key, (void *)tsec_keys, 0x10);
            memcpy((void *)tsec_root_keys, (void *)tsec_keys + 0x10, 0x10);
        } else {
            /* Run the TSEC payload and get the key. */
            if (tsec_get_key(tsec_key, 1, tsec_fw, tsec_fw_size) != 0) {
                fatal_error("[NXBOOT] Failed to get TSEC key!\n");
            }
        }
    }

    /* Display splash screen. */
    display_splash_screen_bmp(loader_ctx->custom_splash_path, (void *)0xC0000000);

    /* Derive keydata. */
    unsigned int keygen_type = 0;
    if (is_mariko) {
        if (derive_nx_keydata_mariko(target_firmware) != 0) {
            fatal_error("[NXBOOT] Mariko key derivation failed!\n");
        }
    } else if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_7_0_0) {   /* If on 7.0.0+, sept has already derived keys for us (Erista only). */
        if (derive_nx_keydata_erista(target_firmware, g_keyblobs, available_revision, tsec_key, tsec_root_keys, &keygen_type) != 0) {
            fatal_error("[NXBOOT] Erista key derivation failed!\n");
        }
    }

    /* Derive new device keys. */
    derive_new_device_keys(fuse_get_hardware_state() != 0, KEYSLOT_SWITCH_5XNEWDEVICEKEYGENKEY, target_firmware);

    /* Set the system partition's keys. */
    if (fsdev_register_keys("system", target_firmware, BisPartition_UserSystem) != 0) {
        fatal_error("[NXBOOT] Failed to set SYSTEM partition keys!\n");
    }

    /* Mount the system partition. */
    if (fsdev_register_device("system") != 0) {
        fatal_error("[NXBOOT] Failed to register SYSTEM partition!\n");
    }

    /* Lightly validate the system partition. */
    if (!is_valid_folder("system:/Contents")) {
        fatal_error("[NXBOOT] SYSTEM partition seems corrupted!\n");
    }

    /* Make the target firmware more specific. */
    target_firmware = nxboot_get_specific_target_firmware(target_firmware);

    /* Setup boot configuration for Exosphère. */
    nxboot_configure_exosphere(target_firmware, keygen_type, &exo_emummc_cfg);

    /* Initialize BootReason on older firmware versions (Erista only). */
    memset((void *)MAILBOX_NX_BOOTLOADER_BASE, 0, 0x200);
    if (!is_mariko && target_firmware < ATMOSPHERE_TARGET_FIRMWARE_4_0_0) {
        print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Initializing BootReason...\n");
        nxboot_set_bootreason((void *)MAILBOX_NX_BOOTLOADER_BOOT_REASON_BASE);
    }

    /* Read the warmboot firmware from a file, otherwise from Atmosphere's implementation (Erista only) or from cache (Mariko only). */
    uint32_t warmboot_fuses = 0;
    if (is_mariko) {
        /* Get warmboot firmware from package1. */
        const package1_header_t *pk11_header = (const package1_header_t *)((uintptr_t)package1loader + (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_6_2_0 ? 0x7000 : 0x4000));

        warmboot_fw = package1_get_warmboot_fw(pk11_header);
        warmboot_fw_size = *(const uint32_t *)warmboot_fw;

        if (!(0x800 <= warmboot_fw_size && warmboot_fw_size < 0x1000)) {
            fatal_error("[NXBOOT] Warmboot firmware seems invalid!\n");
        }

        const uint32_t expected_fuses = fuse_get_expected_fuse_count(target_firmware);
        const uint32_t burnt_fuses    = fuse_get_burnt_fuse_count();

        warmboot_fuses = expected_fuses;

        char warmboot_fn[0x80];
        get_mariko_warmboot_path(warmboot_fn, sizeof(warmboot_fn), expected_fuses);

        if (!is_valid_file(warmboot_fn)) {
            dump_to_file(warmboot_fw, warmboot_fw_size, warmboot_fn);
        }

        if (burnt_fuses > expected_fuses) {
            warmboot_fw = NULL;
            warmboot_fw_size = 0;
            for (uint32_t attempt = burnt_fuses; attempt <= 32; ++attempt) {
                get_mariko_warmboot_path(warmboot_fn, sizeof(warmboot_fn), attempt);
                if (is_valid_file(warmboot_fn)) {
                    warmboot_fw_size = get_file_size(warmboot_fn);

                    /* Allocate memory for the warmboot firmware. */
                    warmboot_fw = malloc(warmboot_fw_size);

                    if (warmboot_fw == NULL) {
                        fatal_error("[NXBOOT] Out of memory!\n");
                    }
                    if (read_from_file(warmboot_fw, warmboot_fw_size, warmboot_fn) != warmboot_fw_size) {
                        fatal_error("[NXBOOT] Could not read the warmboot firmware from %s!\n", warmboot_fn);
                    }

                    warmboot_fuses = attempt;
                    break;
                }
            }
        }

        if (warmboot_fw == NULL) {
            fatal_error("[NXBOOT] Failed to determine mariko warmboot firmware\n");
        }
    } else {
        if (loader_ctx->warmboot_path[0] != '\0') {
            warmboot_fw_size = get_file_size(loader_ctx->warmboot_path);
            if (warmboot_fw_size == 0) {
                fatal_error("[NXBOOT] Could not read the warmboot firmware from %s!\n", loader_ctx->warmboot_path);
            }

            /* Allocate memory for the warmboot firmware. */
            warmboot_fw = malloc(warmboot_fw_size);

            if (warmboot_fw == NULL) {
                fatal_error("[NXBOOT] Out of memory!\n");
            }
            if (read_from_file(warmboot_fw, warmboot_fw_size, loader_ctx->warmboot_path) != warmboot_fw_size) {
                fatal_error("[NXBOOT] Could not read the warmboot firmware from %s!\n", loader_ctx->warmboot_path);
            }
        } else {
            /* Use Atmosphere's warmboot firmware implementation. */
            warmboot_fw_size = warmboot_bin_size;
            warmboot_fw = malloc(warmboot_fw_size);

            if (warmboot_fw == NULL) {
                fatal_error("[NXBOOT] Out of memory!\n");
            }

            memcpy(warmboot_fw, warmboot_bin, warmboot_fw_size);

            if (warmboot_fw_size == 0) {
                fatal_error("[NXBOOT] Could not read the warmboot firmware from Package1!\n");
            }
        }
    }

    /* Patch warmboot firmware for atmosphere (Erista only). */
    if (!is_mariko && (warmboot_fw != NULL) && (warmboot_fw_size >= sizeof(warmboot_ams_header_t))) {
        warmboot_ams_header_t *ams_header = (warmboot_ams_header_t *)warmboot_fw;
        if (ams_header->ams_metadata.magic == WARMBOOT_MAGIC) {
            /* Set target firmware */
            ams_header->ams_metadata.target_firmware = target_firmware;

            /* Set RSA modulus */
            const uint8_t *pkc_modulus = fuse_get_hardware_state() != 0 ? retail_pkc_modulus : dev_pkc_modulus;
            memcpy(ams_header->rsa_modulus, pkc_modulus, sizeof(ams_header->rsa_modulus));
        }
    }

    /* Select the right address for the warmboot firmware. */
    warmboot_memaddr = (void *)0x4003E000;

    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Copying warmboot firmware...\n");

    /* Copy the warmboot firmware and set the address in PMC if necessary. */
    if (warmboot_fw && (warmboot_fw_size > 0)) {
        memcpy(warmboot_memaddr, warmboot_fw, warmboot_fw_size);
        if (!is_mariko && (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_4_0_0)) {
            pmc->scratch1 = (uint32_t)warmboot_memaddr;
        }
    }

    /* Handle warmboot security check. */
    if (is_mariko) {
        switch (warmboot_fuses) {
            case 7:
                pmc->secure_scratch32 = 0x87;
                break;
            case 8:
                pmc->secure_scratch32 = 0xA8;
                break;
            default:
                pmc->secure_scratch32 = (0x108 + 0x21 * (warmboot_fuses - 8));
                break;
        }

        pmc->sec_disable3 |= BIT(16);
    } else {
        /* Set 3.0.0/3.0.1/3.0.2 warmboot security check. */
        if (target_firmware == ATMOSPHERE_TARGET_FIRMWARE_3_0_0) {
            pmc->secure_scratch32 = 0xE3;       /* Warmboot 3.0.0 security check.*/
        } else if (target_firmware == ATMOSPHERE_TARGET_FIRMWARE_3_0_1 || target_firmware == ATMOSPHERE_TARGET_FIRMWARE_3_0_2) {
            pmc->secure_scratch32 = 0x104;      /* Warmboot 3.0.1/3.0.2 security check. */
        }
    }

    /* Configure mesosphere. */
    {
        size_t sd_meso_size = get_file_size("atmosphere/mesosphere.bin");
        if (sd_meso_size != 0) {
            if (sd_meso_size > PACKAGE2_SIZE_MAX) {
                fatal_error("Error: atmosphere/mesosphere.bin is too large!\n");
            }
            mesosphere = malloc(sd_meso_size);
            if (mesosphere == NULL) {
                fatal_error("Error: failed to allocate mesosphere!\n");
            }
            if (read_from_file(mesosphere, sd_meso_size, "atmosphere/mesosphere.bin") != sd_meso_size) {
                fatal_error("Error: failed to read atmosphere/mesosphere.bin!\n");
            }
            mesosphere_size = sd_meso_size;
        } else if (is_experimental) {
            mesosphere_size = mesosphere_bin_size;
            mesosphere = malloc(mesosphere_size);
            if (mesosphere == NULL) {
                fatal_error("[NXBOOT] Out of memory!\n");
            }
            memcpy(mesosphere, mesosphere_bin, mesosphere_size);
            if (mesosphere_size == 0) {
                fatal_error("[NXBOOT] Could not read embedded mesosphere!\n");
            }
        } else {
            mesosphere = NULL;
            mesosphere_size = 0;
        }
    }

    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Rebuilding package2...\n");

    /* Parse stratosphere config. */
    nxboot_configure_stratosphere(MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware);

    print(SCREEN_LOG_LEVEL_INFO, u8"[NXBOOT] Configured Stratosphere...\n");

    /* Patch package2, adding Thermosphère + custom KIPs. */
    package2_rebuild_and_copy(package2, MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware, mesosphere, mesosphere_size, emummc, emummc_size);

    /* Set detected FS version. */
    MAILBOX_EXOSPHERE_CONFIGURATION->emummc_cfg.base_cfg.fs_version = stratosphere_get_fs_version();

    print(SCREEN_LOG_LEVEL_INFO, u8"[NXBOOT] Reading Exosphère...\n");

    /* Select the right address for Exosphère. */
    exosphere_memaddr = (void *)0x40030000;

    /* Copy Exosphère to a good location or read it directly to it. */
    if (loader_ctx->exosphere_path[0] != '\0') {
        size_t exosphere_size = get_file_size(loader_ctx->exosphere_path);
        if (exosphere_size == 0) {
            fatal_error(u8"[NXBOOT] Could not read Exosphère from %s!\n", loader_ctx->exosphere_path);
        } else if (exosphere_size > 0x10000) {
            /* The maximum is actually a bit less than that. */
            fatal_error(u8"[NXBOOT] Exosphère from %s is too big!\n", loader_ctx->exosphere_path);
        }
        if (read_from_file(exosphere_memaddr, exosphere_size, loader_ctx->exosphere_path) != exosphere_size) {
            fatal_error(u8"[NXBOOT] Could not read Exosphère from %s!\n", loader_ctx->exosphere_path);
        }
    } else {
        memcpy(exosphere_memaddr, exosphere_bin, exosphere_bin_size);
    }

    /* Copy the Mariko's Exosphère fatal program to a good location. */
    if (is_mariko) {
        void * const mariko_fatal_dst = (void *)0x80020000;
        memset(mariko_fatal_dst, 0, 0x20000);

        const size_t sd_mf_size = get_file_size("atmosphere/mariko_fatal.bin");
        if (sd_mf_size != 0) {
            if (sd_mf_size > 0x20000) {
                fatal_error("Error: atmosphere/mariko_fatal.bin is too large!\n");
            }
            if (read_from_file(mariko_fatal_dst, sd_mf_size, "atmosphere/mariko_fatal.bin") != sd_mf_size) {
                fatal_error("Error: failed to read atmosphere/mariko_fatal.bin");
            }
        } else {
            memcpy(mariko_fatal_dst, mariko_fatal_bin, mariko_fatal_bin_size);
        }
    }

    /* Move BootConfig. */
    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Moving BootConfig...\n");
    nxboot_move_bootconfig();

    /* Clean up. */
    free(package1loader);
    if (loader_ctx->tsecfw_path[0] != '\0') {
        free(tsec_fw);
    }
    if (loader_ctx->warmboot_path[0] != '\0') {
        free(warmboot_fw);
    }
    free(package2);

    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT] Powering on the CCPLEX...\n");

    /* Wait for the splash screen to have been displayed for as long as it should be. */
    splash_screen_wait_delay();

    /* Set reset for USBD, USB2, AHBDMA, and APBDMA (Erista only). */
    if (!is_mariko) {
        rst_enable(CARDEVICE_USBD);
        rst_enable(CARDEVICE_USB2);
        rst_enable(CARDEVICE_AHBDMA);
        rst_enable(CARDEVICE_APBDMA);
    }

    /* Return the memory address for booting CPU0. */
    return (uint32_t)exosphere_memaddr;
}
