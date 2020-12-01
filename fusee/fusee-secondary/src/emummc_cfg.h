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

#ifndef EXOSPHERE_EMUMMC_CONFIG_H
#define EXOSPHERE_EMUMMC_CONFIG_H

#include <stdint.h>
#include <vapours/ams_version.h>

/* "EFS0" */
#define MAGIC_EMUMMC_CONFIG (0x30534645)

#define EMUMMC_FILE_PATH_MAX (0x80)

typedef enum {
   EMUMMC_TYPE_NONE = 0,
   EMUMMC_TYPE_PARTITION = 1,
   EMUMMC_TYPE_FILES = 2,
} emummc_type_t;

typedef enum {
    EMUMMC_MMC_NAND = 0,
    EMUMMC_MMC_SD = 1,
    EMUMMC_MMC_GC = 2,
} emummc_mmc_t;

typedef enum {
    FS_VER_1_0_0 = 0,

    FS_VER_2_0_0,
    FS_VER_2_0_0_EXFAT,

    FS_VER_2_1_0,
    FS_VER_2_1_0_EXFAT,

    FS_VER_3_0_0,
    FS_VER_3_0_0_EXFAT,

    FS_VER_3_0_1,
    FS_VER_3_0_1_EXFAT,

    FS_VER_4_0_0,
    FS_VER_4_0_0_EXFAT,

    FS_VER_4_1_0,
    FS_VER_4_1_0_EXFAT,

    FS_VER_5_0_0,
    FS_VER_5_0_0_EXFAT,

    FS_VER_5_1_0,
    FS_VER_5_1_0_EXFAT,

    FS_VER_6_0_0,
    FS_VER_6_0_0_EXFAT,

    FS_VER_7_0_0,
    FS_VER_7_0_0_EXFAT,

    FS_VER_8_0_0,
    FS_VER_8_0_0_EXFAT,

    FS_VER_8_1_0,
    FS_VER_8_1_0_EXFAT,

    FS_VER_9_0_0,
    FS_VER_9_0_0_EXFAT,

    FS_VER_9_1_0,
    FS_VER_9_1_0_EXFAT,

    FS_VER_10_0_0,
    FS_VER_10_0_0_EXFAT,

    FS_VER_10_2_0,
    FS_VER_10_2_0_EXFAT,

    FS_VER_11_0_0,
    FS_VER_11_0_0_EXFAT,

    FS_VER_MAX,
} emummc_fs_ver_t;


typedef struct {
    uint32_t magic;
    uint32_t type;
    uint32_t id;
    uint32_t fs_version;
} emummc_base_config_t;

typedef struct {
    uint64_t start_sector;
} emummc_partition_config_t;

typedef struct {
    char path[EMUMMC_FILE_PATH_MAX];
} emummc_file_config_t;

typedef struct {
    emummc_base_config_t base_cfg;
    union {
        emummc_partition_config_t partition_cfg;
        emummc_file_config_t file_cfg;
    };
    char emu_dir_path[EMUMMC_FILE_PATH_MAX];
} exo_emummc_config_t;

_Static_assert(sizeof(exo_emummc_config_t) == 0x110, "exo_emummc_config_t definition!");

#endif
