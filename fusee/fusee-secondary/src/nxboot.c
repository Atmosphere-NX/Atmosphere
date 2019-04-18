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
 
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <malloc.h>
#include "utils.h"
#include "fs_utils.h"
#include "nxboot.h"
#include "nxfs.h"
#include "bct.h"
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
#include "display/video_fb.h"
#include "lib/ini.h"
#include "splash_screen.h"

#define u8 uint8_t
#define u32 uint32_t
#include "exosphere_bin.h"
#include "sept_secondary_enc.h"
#include "lp0fw_bin.h"
#include "lib/log.h"
#undef u8
#undef u32

extern const uint8_t lp0fw_bin[];
extern const uint32_t lp0fw_bin_size;

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

static int exosphere_ini_handler(void *user, const char *section, const char *name, const char *value) {
    exosphere_config_t *exo_cfg = (exosphere_config_t *)user;
    int tmp = 0;
    if (strcmp(section, "exosphere") == 0) {
        if (strcmp(name, EXOSPHERE_TARGETFW_KEY) == 0) {
            sscanf(value, "%d", &exo_cfg->target_firmware);
        } 
        if (strcmp(name, EXOSPHERE_DEBUGMODE_PRIV_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp) {
                exo_cfg->flags |= EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV;
            } else {
                exo_cfg->flags &= ~(EXOSPHERE_FLAG_IS_DEBUGMODE_PRIV);
            }
        } 
        if (strcmp(name, EXOSPHERE_DEBUGMODE_USER_KEY) == 0) {
            sscanf(value, "%d", &tmp);
            if (tmp) {
                exo_cfg->flags |= EXOSPHERE_FLAG_IS_DEBUGMODE_USER;
            } else {
                exo_cfg->flags &= ~(EXOSPHERE_FLAG_IS_DEBUGMODE_USER);
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
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

static uint32_t nxboot_get_target_firmware(const void *package1loader) {
    const package1loader_header_t *package1loader_header = (const package1loader_header_t *)package1loader;
    switch (package1loader_header->version) {
        case 0x01:          /* 1.0.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_100;
        case 0x02:          /* 2.0.0 - 2.3.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_200;
        case 0x04:          /* 3.0.0 and 3.0.1 - 3.0.2 */
            return ATMOSPHERE_TARGET_FIRMWARE_300;
        case 0x07:          /* 4.0.0 - 4.1.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_400;
        case 0x0B:          /* 5.0.0 - 5.1.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_500;
        case 0x0E: {        /* 6.0.0 - 6.2.0 */
            if (memcmp(package1loader_header->build_timestamp, "20180802", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_600;
            } else if (memcmp(package1loader_header->build_timestamp, "20181107", 8) == 0) {
                return ATMOSPHERE_TARGET_FIRMWARE_620;
            } else {
                fatal_error("[NXBOOT]: Unable to identify package1!\n");
            }
        }
        case 0x0F:          /* 7.0.0 - 7.0.1 */
            return ATMOSPHERE_TARGET_FIRMWARE_700;
        case 0x10:          /* 8.0.0 */
            return ATMOSPHERE_TARGET_FIRMWARE_800;
        default:
            fatal_error("[NXBOOT]: Unable to identify package1!\n");
    }
}

static void nxboot_configure_exosphere(uint32_t target_firmware, unsigned int keygen_type) {
    exosphere_config_t exo_cfg = {0};

    exo_cfg.magic = MAGIC_EXOSPHERE_CONFIG;
    exo_cfg.target_firmware = target_firmware;
    if (keygen_type) {
        exo_cfg.flags = EXOSPHERE_FLAGS_DEFAULT | EXOSPHERE_FLAG_PERFORM_620_KEYGEN;
    } else {
        exo_cfg.flags = EXOSPHERE_FLAGS_DEFAULT;
    }

    if (ini_parse_string(get_loader_ctx()->bct0, exosphere_ini_handler, &exo_cfg) < 0) {
        fatal_error("[NXBOOT]: Failed to parse BCT.ini!\n");
    }

    if ((exo_cfg.target_firmware < ATMOSPHERE_TARGET_FIRMWARE_MIN) || (exo_cfg.target_firmware > ATMOSPHERE_TARGET_FIRMWARE_MAX)) {
        fatal_error("[NXBOOT]: Invalid Exosphere target firmware!\n");
    }

    *(MAILBOX_EXOSPHERE_CONFIGURATION) = exo_cfg;
}

static void nxboot_configure_stratosphere(uint32_t target_firmware) {
    stratosphere_cfg_t strat_cfg = {0};
    if (ini_parse_string(get_loader_ctx()->bct0, stratosphere_ini_handler, &strat_cfg) < 0) {
        fatal_error("[NXBOOT]: Failed to parse BCT.ini!\n");
    }
    
    /* Enable NOGC patches if the user requested it, or if the user is booting into 4.0.0+ with 3.0.2- fuses. */
    if (strat_cfg.has_nogc_config) {
        if (strat_cfg.enable_nogc) {
            kip_patches_set_enable_nogc();
        }
    } else {
        /* Check if fuses are < 4.0.0, but firmware is >= 4.0.0 */
        if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_400 && !(fuse_get_reserved_odm(7) & ~0x0000000F)) {
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
        fatal_error("[NXBOOT]: Out of memory!\n");
    }
    
    /* Open boot0. */
    boot0 = fopen("boot0:/", "rb");
    if (boot0 == NULL) {
        fatal_error("[NXBOOT]: Failed to open boot0!\n");
    }

    /* Read the BCT. */
    if (fread(bct, sizeof(nvboot_config_table), 1, boot0) == 0) {
        fatal_error("[NXBOOT]: Failed to read the BCT!\n");
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
    
    /* TODO: Find out what these mean. */
    if (power_key_intr & 0x80)
        boot_reason.boot_reason_state = 0x01;
    else if (power_key_intr & 0x08)
        boot_reason.boot_reason_state = 0x02;
    else if (rtc_intr & 0x02)
        boot_reason.boot_reason_state = 0x03;
    else if (rtc_intr & 0x04)
        boot_reason.boot_reason_state = 0x04;
    
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
        fatal_error("[NXBOOT]: Out of memory!\n");
    }
    
    /* Get BootConfig from the Package2 partition. */
    bcfile = fopen("bcpkg21:/", "rb");
    if (bcfile == NULL) {
        fatal_error("[NXBOOT]: Failed to open BootConfig from eMMC!\n");
    }
    if (fread(bootconfig, 0x4000, 1, bcfile) < 1) {
        fclose(bcfile);
        fatal_error("[NXBOOT]: Failed to read BootConfig!\n");
    }
    fclose(bcfile);
    
    /* Select the actual BootConfig size and destination address. */
    bootconfig_addr = (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < ATMOSPHERE_TARGET_FIRMWARE_600) ? 0x4003D000 : 0x4003F800;
    bootconfig_size = (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < ATMOSPHERE_TARGET_FIRMWARE_400) ? 0x3000 : 0x1000;
    
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

/* This is the main function responsible for booting Horizon. */
static nx_keyblob_t __attribute__((aligned(16))) g_keyblobs[32];
uint32_t nxboot_main(void) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    loader_ctx_t *loader_ctx = get_loader_ctx();
    package2_header_t *package2;
    size_t package2_size;
    void *tsec_fw;
    size_t tsec_fw_size;
    void *warmboot_fw;
    size_t warmboot_fw_size;
    void *warmboot_memaddr;
    void *package1loader;
    size_t package1loader_size;
    uint32_t available_revision;
    FILE *boot0, *pk2file;
    void *exosphere_memaddr;

    /* Allocate memory for reading Package2. */
    package2 = memalign(0x1000, PACKAGE2_SIZE_MAX);
    if (package2 == NULL) {
        fatal_error("[NXBOOT]: Out of memory!\n");
    }

    /* Read Package2 from a file, otherwise from its partition(s). */
    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Reading package2...\n");
    if (loader_ctx->package2_path[0] != '\0') {
        pk2file = fopen(loader_ctx->package2_path, "rb");
        if (pk2file == NULL) {
            fatal_error("[NXBOOT]: Failed to open Package2 from %s: %s!\n", loader_ctx->package2_path, strerror(errno));
        }
    } else {
        pk2file = fopen("bcpkg21:/", "rb");
        if (pk2file == NULL) {
            fatal_error("[NXBOOT]: Failed to open Package2 from eMMC: %s!\n", strerror(errno));
        }
        if (fseek(pk2file, 0x4000, SEEK_SET) != 0) {
            fclose(pk2file);
            fatal_error("[NXBOOT]: Failed to seek Package2 in eMMC: %s!\n", strerror(errno));
        }
    }

    setvbuf(pk2file, NULL, _IONBF, 0); /* Workaround. */
    if (fread(package2, sizeof(package2_header_t), 1, pk2file) < 1) {
        fclose(pk2file);
        fatal_error("[NXBOOT]: Failed to read Package2!\n");
    }
    package2_size = package2_meta_get_size(&package2->metadata);
    if ((package2_size > PACKAGE2_SIZE_MAX) || (package2_size <= sizeof(package2_header_t))) {
        fclose(pk2file);
        fatal_error("[NXBOOT]: Package2 is too big or too small!\n");
    }
    if (fread(package2->data, package2_size - sizeof(package2_header_t), 1, pk2file) < 1) {
        fclose(pk2file);
        fatal_error("[NXBOOT]: Failed to read Package2!\n");
    }
    fclose(pk2file);
    
    /* Read and parse boot0. */
    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Reading boot0...\n");
    boot0 = fopen("boot0:/", "rb");
    if ((boot0 == NULL) || (package1_read_and_parse_boot0(&package1loader, &package1loader_size, g_keyblobs, &available_revision, boot0) == -1)) {
        fatal_error("[NXBOOT]: Couldn't parse boot0: %s!\n", strerror(errno));
    }
    fclose(boot0);
    
    /* Find the system's target firmware. */
    uint32_t target_firmware = nxboot_get_target_firmware(package1loader);
    if (!target_firmware)
        fatal_error("[NXBOOT]: Failed to detect target firmware!\n");
    else
        print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Detected target firmware %ld!\n", target_firmware);

    /* Read the TSEC firmware from a file, otherwise from PK1L. */
    if (loader_ctx->tsecfw_path[0] != '\0') {
        tsec_fw_size = get_file_size(loader_ctx->tsecfw_path);
        if ((tsec_fw_size != 0) && (tsec_fw_size != 0xF00 && tsec_fw_size != 0x2900 && tsec_fw_size != 0x3000)) {
            fatal_error("[NXBOOT]: TSEC firmware from %s has a wrong size!\n", loader_ctx->tsecfw_path);
        } else if (tsec_fw_size == 0) {
            fatal_error("[NXBOOT]: Could not read the TSEC firmware from %s!\n", loader_ctx->tsecfw_path);
        }
        
        /* Allocate memory for the TSEC firmware. */
        tsec_fw = memalign(0x100, tsec_fw_size);
        
        if (tsec_fw == NULL) {
            fatal_error("[NXBOOT]: Out of memory!\n");
        }
        if (read_from_file(tsec_fw, tsec_fw_size, loader_ctx->tsecfw_path) != tsec_fw_size) {
            fatal_error("[NXBOOT]: Could not read the TSEC firmware from %s!\n", loader_ctx->tsecfw_path);
        }
    } else {
        if (!package1_get_tsec_fw(&tsec_fw, package1loader, package1loader_size)) {
            fatal_error("[NXBOOT]: Failed to read the TSEC firmware from Package1loader!\n");
        }
        if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_700) { 
            tsec_fw_size = 0x3000;
        } else if (target_firmware == ATMOSPHERE_TARGET_FIRMWARE_620) {
            tsec_fw_size = 0x2900;
        } else {
            tsec_fw_size = 0xF00;
        }
    }

    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Loaded firmware from eMMC...\n");

    /* Get the TSEC keys. */
    uint8_t tsec_key[0x10] = {0};
    uint8_t tsec_root_keys[0x20][0x10] = {0};
    if (target_firmware >= ATMOSPHERE_TARGET_FIRMWARE_700) {
        /* Detect whether we need to run sept-secondary in order to derive keys. */
        if (!get_and_clear_has_run_sept()) {
            reboot_to_sept(tsec_fw, tsec_fw_size, sept_secondary_enc, sept_secondary_enc_size);
        } else {
            if (mkey_detect_revision(fuse_get_retail_type() != 0) != 0) {
                fatal_error("[NXBOOT]: Sept derived incorrect keys!\n");
            }
        }
        get_and_clear_has_run_sept();
    } else if (target_firmware == ATMOSPHERE_TARGET_FIRMWARE_620) {
        uint8_t tsec_keys[0x20] = {0};
        
        /* Emulate the TSEC payload on 6.2.0+. */
        smmu_emulate_tsec((void *)tsec_keys, package1loader, package1loader_size, package1loader);
        
        /* Copy back the keys. */
        memcpy((void *)tsec_key, (void *)tsec_keys, 0x10);
        memcpy((void *)tsec_root_keys, (void *)tsec_keys + 0x10, 0x10);
    } else {
        /* Run the TSEC payload and get the key. */
        if (tsec_get_key(tsec_key, 1, tsec_fw, tsec_fw_size) != 0) {
            fatal_error("[NXBOOT]: Failed to get TSEC key!\n");
        }
    }
    
    //fatal_error("Ran sept!");
    /* Display splash screen. */
    display_splash_screen_bmp(loader_ctx->custom_splash_path, (void *)0xC0000000);
    
    /* Derive keydata. If on 7.0.0+, sept has already derived keys for us. */
    unsigned int keygen_type = 0;
    if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_700) {
        if (derive_nx_keydata(target_firmware, g_keyblobs, available_revision, tsec_key, tsec_root_keys, &keygen_type) != 0) {
            fatal_error("[NXBOOT]: Key derivation failed!\n");
        }
    }

    /* Setup boot configuration for Exosphère. */
    nxboot_configure_exosphere(target_firmware, keygen_type);

    /* Initialize Boot Reason on older firmware versions. */
    if (target_firmware < ATMOSPHERE_TARGET_FIRMWARE_400) {
        print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Initializing Boot Reason...\n");
        nxboot_set_bootreason((void *)MAILBOX_NX_BOOTLOADER_BOOT_REASON_BASE(target_firmware));
    }

    /* Read the warmboot firmware from a file, otherwise from Atmosphere's implementation. */
    if (loader_ctx->warmboot_path[0] != '\0') {
        warmboot_fw_size = get_file_size(loader_ctx->warmboot_path);
        if (warmboot_fw_size == 0) {
            fatal_error("[NXBOOT]: Could not read the warmboot firmware from %s!\n", loader_ctx->warmboot_path);
        }

        /* Allocate memory for the warmboot firmware. */
        warmboot_fw = malloc(warmboot_fw_size);

        if (warmboot_fw == NULL) {
            fatal_error("[NXBOOT]: Out of memory!\n");
        }
        if (read_from_file(warmboot_fw, warmboot_fw_size, loader_ctx->warmboot_path) != warmboot_fw_size) {
            fatal_error("[NXBOOT]: Could not read the warmboot firmware from %s!\n", loader_ctx->warmboot_path);
        }
    } else {
        /* Use Atmosphere's warmboot firmware implementation. */
        warmboot_fw_size = lp0fw_bin_size;
        warmboot_fw = malloc(warmboot_fw_size);

        if (warmboot_fw == NULL) {
            fatal_error("[NXBOOT]: Out of memory!\n");
        }
        
        memcpy(warmboot_fw, lp0fw_bin, warmboot_fw_size);
        
        if (warmboot_fw_size == 0) {
            fatal_error("[NXBOOT]: Could not read the warmboot firmware from Package1!\n");
        }
    }
    
    /* Patch warmboot firmware for atmosphere. */
    if (warmboot_fw != NULL && warmboot_fw_size >= sizeof(warmboot_ams_header_t)) {
        warmboot_ams_header_t *ams_header = (warmboot_ams_header_t *)warmboot_fw;
        if (ams_header->ams_metadata.magic == WARMBOOT_MAGIC) {
            /* Set target firmware */
            ams_header->ams_metadata.target_firmware = target_firmware;
            
            /* Set RSA modulus */
            const uint8_t *pkc_modulus = fuse_get_retail_type() != 0 ? retail_pkc_modulus : dev_pkc_modulus;
            memcpy(ams_header->rsa_modulus, pkc_modulus, sizeof(ams_header->rsa_modulus));
        }
    }
        

    /* Select the right address for the warmboot firmware. */
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < ATMOSPHERE_TARGET_FIRMWARE_400) {
        warmboot_memaddr = (void *)0x8000D000;
    } else if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < ATMOSPHERE_TARGET_FIRMWARE_600) {
        warmboot_memaddr = (void *)0x4003B000;
    } else if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < ATMOSPHERE_TARGET_FIRMWARE_700) {
        warmboot_memaddr = (void *)0x4003D800;
    } else {
        warmboot_memaddr = (void *)0x4003E000;
    }

    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Copying warmboot firmware...\n");

    /* Copy the warmboot firmware and set the address in PMC if necessary. */
    if (warmboot_fw && (warmboot_fw_size > 0)) {
        memcpy(warmboot_memaddr, warmboot_fw, warmboot_fw_size);
        if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < ATMOSPHERE_TARGET_FIRMWARE_400)
            pmc->scratch1 = (uint32_t)warmboot_memaddr;
    }

    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Rebuilding package2...\n");
    
    /* Parse stratosphere config. */
    nxboot_configure_stratosphere(MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware);

    print(SCREEN_LOG_LEVEL_INFO, u8"[NXBOOT]: Configured Stratosphere...\n");

    /* Patch package2, adding Thermosphère + custom KIPs. */
    package2_rebuild_and_copy(package2, MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware);

    print(SCREEN_LOG_LEVEL_INFO, u8"[NXBOOT]: Reading Exosphère...\n");

    /* Select the right address for Exosphère. */
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware < ATMOSPHERE_TARGET_FIRMWARE_400) {
        exosphere_memaddr = (void *)0x4002D000;
    } else {
        exosphere_memaddr = (void *)0x4002B000;
    }

    /* Copy Exosphère to a good location or read it directly to it. */
    if (loader_ctx->exosphere_path[0] != '\0') {
        size_t exosphere_size = get_file_size(loader_ctx->exosphere_path);
        if (exosphere_size == 0) {
            fatal_error(u8"[NXBOOT]: Could not read Exosphère from %s!\n", loader_ctx->exosphere_path);
        } else if (exosphere_size > 0x10000) {
            /* The maximum is actually a bit less than that. */
            fatal_error(u8"[NXBOOT]: Exosphère from %s is too big!\n", loader_ctx->exosphere_path);
        }

        if (read_from_file(exosphere_memaddr, exosphere_size, loader_ctx->exosphere_path) != exosphere_size) {
            fatal_error(u8"[NXBOOT]: Could not read Exosphère from %s!\n", loader_ctx->exosphere_path);
        }
    } else {
        memcpy(exosphere_memaddr, exosphere_bin, exosphere_bin_size);
    }

    /* Move BootConfig. */
    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Moving BootConfig...\n");
    nxboot_move_bootconfig();

    /* Set 3.0.0/3.0.1/3.0.2 warmboot security check. */
    if (MAILBOX_EXOSPHERE_CONFIGURATION->target_firmware == ATMOSPHERE_TARGET_FIRMWARE_300) {
        const package1loader_header_t *package1loader_header = (const package1loader_header_t *)package1loader;
        if (!strcmp(package1loader_header->build_timestamp, "20170519101410"))
            pmc->secure_scratch32 = 0xE3;       /* Warmboot 3.0.0 security check.*/
        else if (!strcmp(package1loader_header->build_timestamp, "20170710161758"))
            pmc->secure_scratch32 = 0x104;      /* Warmboot 3.0.1/3.0.2 security check. */
    }

    /* Clean up. */
    free(package1loader);
    if (loader_ctx->tsecfw_path[0] != '\0') {
        free(tsec_fw);
    }
    if (loader_ctx->warmboot_path[0] != '\0') {
        free(warmboot_fw);
    }
    free(package2);

    print(SCREEN_LOG_LEVEL_INFO, "[NXBOOT]: Powering on the CCPLEX...\n");
    
    /* Unmount everything. */
    nxfs_unmount_all();
    
    /* Return the memory address for booting CPU0. */
    return (uint32_t)exosphere_memaddr;
}
