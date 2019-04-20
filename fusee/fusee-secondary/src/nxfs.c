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
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include "nxfs.h"
#include "mc.h"
#include "gpt.h"
#include "se.h"
#include "utils.h"
#include "sdmmc/sdmmc.h"

static bool g_ahb_redirect_enabled = false;
static bool g_sd_device_initialized = false;
static bool g_emmc_device_initialized = false;

static sdmmc_t g_sd_sdmmc = {0};
static sdmmc_t g_emmc_sdmmc = {0};

static sdmmc_device_t g_sd_device = {0};
static sdmmc_device_t g_emmc_device = {0};

typedef struct mmc_partition_info_t {
    sdmmc_device_t *device;
    SdmmcControllerNum controller;
    SdmmcPartitionNum partition;
} mmc_partition_info_t;

static mmc_partition_info_t g_sd_mmcpart = {&g_sd_device, SDMMC_1, SDMMC_PARTITION_USER};
static mmc_partition_info_t g_emmc_boot0_mmcpart = {&g_emmc_device, SDMMC_4, SDMMC_PARTITION_BOOT0};
static mmc_partition_info_t g_emmc_boot1_mmcpart = {&g_emmc_device, SDMMC_4, SDMMC_PARTITION_BOOT1};
static mmc_partition_info_t g_emmc_user_mmcpart = {&g_emmc_device, SDMMC_4, SDMMC_PARTITION_USER};

SdmmcPartitionNum g_current_emmc_partition = SDMMC_PARTITION_INVALID;

static int mmc_partition_initialize(device_partition_t *devpart) {
    mmc_partition_info_t *mmcpart = (mmc_partition_info_t *)devpart->device_struct;
    
    /* Allocate the crypto work buffer. */
    if ((devpart->read_cipher != NULL) || (devpart->write_cipher != NULL)) {
        devpart->crypto_work_buffer = memalign(16, devpart->sector_size * 16);
        if (devpart->crypto_work_buffer == NULL) {
            return ENOMEM;
        } else {
            devpart->crypto_work_buffer_num_sectors = devpart->sector_size * 16;
        }
    } else {
        devpart->crypto_work_buffer = NULL;
        devpart->crypto_work_buffer_num_sectors = 0;
    }

    /* Enable AHB redirection if necessary. */
    if (!g_ahb_redirect_enabled) {
        mc_enable_ahb_redirect();
        g_ahb_redirect_enabled = true;
    }

    /* Initialize hardware. */
    if (mmcpart->device == &g_sd_device) {
        if (!g_sd_device_initialized) {
            int rc = sdmmc_device_sd_init(mmcpart->device, &g_sd_sdmmc, SDMMC_BUS_WIDTH_4BIT, SDMMC_SPEED_SDR104) ? 0 : EIO;
            if (rc)
                return rc;
            g_sd_device_initialized = true;
        }
        devpart->initialized = true;
        return 0;
    } else if (mmcpart->device == &g_emmc_device) {
        if (!g_emmc_device_initialized) {
            int rc = sdmmc_device_mmc_init(mmcpart->device, &g_emmc_sdmmc, SDMMC_BUS_WIDTH_8BIT, SDMMC_SPEED_HS400) ? 0 : EIO;
            if (rc)
                return rc;
            g_emmc_device_initialized = true;
        }
        devpart->initialized = true;
        return 0;
    }

    return 0;
}

static void mmc_partition_finalize(device_partition_t *devpart) {
    mmc_partition_info_t *mmcpart = (mmc_partition_info_t *)devpart->device_struct;
    
    /* Finalize hardware. */
    if (mmcpart->device == &g_sd_device) {
        if (g_sd_device_initialized) {
            sdmmc_device_finish(&g_sd_device);
            g_sd_device_initialized = false;
        }
        devpart->initialized = false;
    } else if (mmcpart->device == &g_emmc_device) {
        if (g_emmc_device_initialized) {
            sdmmc_device_finish(&g_emmc_device);
            g_emmc_device_initialized = false;
        }
        devpart->initialized = false;
    }
    
    /* Disable AHB redirection if necessary. */
    if (g_ahb_redirect_enabled) {
        mc_disable_ahb_redirect();
        g_ahb_redirect_enabled = false;
    }
    
    /* Free the crypto work buffer. */
    if (devpart->crypto_work_buffer != NULL) {
        free(devpart->crypto_work_buffer);
    }
}

static int mmc_partition_read(device_partition_t *devpart, void *dst, uint64_t sector, uint64_t num_sectors) {
    mmc_partition_info_t *mmcpart = (mmc_partition_info_t *)devpart->device_struct;
    
    if ((mmcpart->device == &g_emmc_device) && (g_current_emmc_partition != mmcpart->partition)) {
        if (!sdmmc_mmc_select_partition(mmcpart->device, mmcpart->partition))
            return EIO;
        g_current_emmc_partition = mmcpart->partition;
    }
    
    return sdmmc_device_read(mmcpart->device, (uint32_t)(devpart->start_sector + sector), (uint32_t)num_sectors, dst) ? 0 : EIO;
}

static int mmc_partition_write(device_partition_t *devpart, const void *src, uint64_t sector, uint64_t num_sectors) {
    mmc_partition_info_t *mmcpart = (mmc_partition_info_t *)devpart->device_struct;
    
    if ((mmcpart->device == &g_emmc_device) && (g_current_emmc_partition != mmcpart->partition)) {
        if (!sdmmc_mmc_select_partition(mmcpart->device, mmcpart->partition))
            return EIO;
        g_current_emmc_partition = mmcpart->partition;
    }

    return sdmmc_device_write(mmcpart->device, (uint32_t)(devpart->start_sector + sector), (uint32_t)num_sectors, (void *)src) ? 0 : EIO;
}

static int nxfs_bis_crypto_decrypt(device_partition_t *devpart, uint64_t sector, uint64_t num_sectors) {
    unsigned int keyslot_a = 4; /* These keyslots are never used by exosphere, and should be safe. */
    unsigned int keyslot_b = 5;
    size_t size = num_sectors * devpart->sector_size;
    switch (devpart->crypto_mode) {
        case DevicePartitionCryptoMode_Ctr:
            set_aes_keyslot(keyslot_a, devpart->keys[0], 0x10);
            se_aes_ctr_crypt(keyslot_a, devpart->crypto_work_buffer, size, devpart->crypto_work_buffer, size, devpart->iv, 0x10);
            return 0;
        case DevicePartitionCryptoMode_Xts:
            set_aes_keyslot(keyslot_a, devpart->keys[0], 0x10);
            set_aes_keyslot(keyslot_b, devpart->keys[1], 0x10);
            se_aes_128_xts_nintendo_decrypt(keyslot_a, keyslot_b, sector, devpart->crypto_work_buffer, devpart->crypto_work_buffer, size, devpart->sector_size);
            return 0;
        case DevicePartitionCryptoMode_None:
        default:
            return 0;
    }
}

static int nxfs_bis_crypto_encrypt(device_partition_t *devpart, uint64_t sector, uint64_t num_sectors) {
    unsigned int keyslot_a = 4; /* These keyslots are never used by exosphere, and should be safe. */
    unsigned int keyslot_b = 5;
    size_t size = num_sectors * devpart->sector_size;
    switch (devpart->crypto_mode) {
        case DevicePartitionCryptoMode_Ctr:
            set_aes_keyslot(keyslot_a, devpart->keys[0], 0x10);
            se_aes_ctr_crypt(keyslot_a, devpart->crypto_work_buffer, size, devpart->crypto_work_buffer, size, devpart->iv, 0x10);
            return 0;
        case DevicePartitionCryptoMode_Xts:
            set_aes_keyslot(keyslot_a, devpart->keys[0], 0x10);
            set_aes_keyslot(keyslot_b, devpart->keys[1], 0x10);
            se_aes_128_xts_nintendo_encrypt(keyslot_a, keyslot_b, sector, devpart->crypto_work_buffer, devpart->crypto_work_buffer, size, devpart->sector_size);
            return 0;
        case DevicePartitionCryptoMode_None:
        default:
            return 0;
    }
}

static const device_partition_t g_mmc_devpart_template = {
    .sector_size = 512,
    .initializer = mmc_partition_initialize,
    .finalizer = mmc_partition_finalize,
    .reader = mmc_partition_read,
    .writer = mmc_partition_write,
};

static int nxfs_mount_partition_gpt_callback(const efi_entry_t *entry, void *param, size_t entry_offset, FILE *disk) {
    (void)entry_offset;
    (void)disk;
    device_partition_t *parent = (device_partition_t *)param;
    device_partition_t devpart = *parent;
    char name_buffer[128];
    const uint16_t *utf16name = entry->name;
    uint32_t name_len;
    int rc;

    static const struct {
        const char *partition_name;
        const char *mount_point;
        bool is_fat;
        bool is_encrypted;
        bool register_immediately;
    } known_partitions[] = {
        {"PRODINFO", "prodinfo", false, true, false},
        {"PRODINFOF", "prodinfof", true, true, false},
        {"BCPKG2-1-Normal-Main", "bcpkg21", false, false, true},
        {"BCPKG2-2-Normal-Sub", "bcpkg22", false, false, false},
        {"BCPKG2-3-SafeMode-Main", "bcpkg23", false, false, false},
        {"BCPKG2-4-SafeMode-Sub", "bcpkg24", false, false, false},
        {"BCPKG2-5-Repair-Main", "bcpkg25", false, false, false},
        {"BCPKG2-6-Repair-Sub", "bcpkg26", false, false, false},
        {"SAFE", "safe", true, true, false},
        {"SYSTEM", "system", true, true, false},
        {"USER", "user", true, true, false},
    };

    /* Convert the partition name to ASCII, for comparison. */
    for (name_len = 0; name_len < sizeof(entry->name) && *utf16name != 0; name_len++) {
        name_buffer[name_len] = (char)*utf16name++;
    }
    name_buffer[name_len] = '\0';

    /* Mount the partition, if we know about it. */
    for (size_t i = 0; i < sizeof(known_partitions)/sizeof(known_partitions[0]); i++) {
        if (strcmp(name_buffer, known_partitions[i].partition_name) == 0) {
            devpart.start_sector += entry->first_lba;
            devpart.num_sectors = (entry->last_lba + 1) - entry->first_lba;
            if (parent->num_sectors < devpart.num_sectors) {
                errno = EINVAL;
                return -1;
            }

            if (known_partitions[i].is_encrypted) {
                devpart.read_cipher = nxfs_bis_crypto_decrypt;
                devpart.write_cipher = nxfs_bis_crypto_encrypt;
                devpart.crypto_mode = DevicePartitionCryptoMode_Xts;
            }

            if (known_partitions[i].is_fat) {
                rc = fsdev_mount_device(known_partitions[i].mount_point, &devpart, false);
                if (rc == -1) {
                    return -1;
                }
                if (known_partitions[i].register_immediately) {
                    rc = fsdev_register_device(known_partitions[i].mount_point);
                    if (rc == -1) {
                        return -1;
                    }
                }
            } else {
                rc = rawdev_mount_device(known_partitions[i].mount_point, &devpart, false);
                if (rc == -1) {
                    return -1;
                }
                if (known_partitions[i].register_immediately) {
                    rc = rawdev_register_device(known_partitions[i].mount_point);
                    if (rc == -1) {
                        return -1;
                    }
                }
            }
        }
    }

    return 0;
}

int nxfs_mount_all(void) {
    device_partition_t model;
    int rc;
    FILE *rawnand;

    /* Initialize the SD card and its primary partition. */
    model = g_mmc_devpart_template;
    model.device_struct = &g_sd_mmcpart;
    model.start_sector = 0;
    model.num_sectors = 1u << 30; /* arbitrary numbers of sectors. TODO: find the size of the SD in sectors. */
    
    rc = fsdev_mount_device("sdmc", &model, true);
    
    if (rc == -1) {
        return -1;
    }
    
    rc = fsdev_register_device("sdmc");
    
    if (rc == -1) {
        return -1;
    }

    /* Boot0. */
    model = g_mmc_devpart_template;
    model.device_struct = &g_emmc_boot0_mmcpart;
    model.start_sector = 0;
    model.num_sectors = 0x184000 / model.sector_size;
    
    rc = rawdev_mount_device("boot0", &model, true);
    
    if (rc == -1) {
        return -1;
    }
    
    rc = rawdev_register_device("boot0");
    
    if (rc == -1) {
        return -1;
    }
    
    /* Boot1. */
    model = g_mmc_devpart_template;
    model.device_struct = &g_emmc_boot1_mmcpart;
    model.start_sector = 0;
    model.num_sectors = 0x80000 / model.sector_size;
    
    rc = rawdev_mount_device("boot1", &model, false);
    
    if (rc == -1) {
        return -1;
    }

    /* Don't register boot1 for now. */

    /* Raw NAND (excluding boot partitions), and its partitions. */
    model = g_mmc_devpart_template;
    model = g_mmc_devpart_template;
    model.device_struct = &g_emmc_user_mmcpart;
    model.start_sector = 0;
    model.num_sectors = (256ull << 30) / model.sector_size;
    
    rc = rawdev_mount_device("rawnand", &model, false);
    
    if (rc == -1) {
        return -1;
    }
    
    rc = rawdev_register_device("rawnand");
    if (rc == -1) {
        return -1;
    }
    
    rawnand = fopen("rawnand:/", "rb");
    
    if (rawnand == NULL) {
        return -1;
    }
    
    rc = gpt_iterate_through_entries(rawnand, model.sector_size, nxfs_mount_partition_gpt_callback, &model);
    fclose(rawnand);
    
    if (rc == 0) {
        rc = fsdev_set_default_device("sdmc");
    }
    
    return rc;
}

int nxfs_unmount_all(void) {
    return ((fsdev_unmount_all() || rawdev_unmount_all()) ? -1 : 0);
}
