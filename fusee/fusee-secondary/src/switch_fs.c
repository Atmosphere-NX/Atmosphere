#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include "switch_fs.h"
#include "gpt.h"
#include "sdmmc.h"
#include "se.h"
#include "hwinit.h"

static bool g_ahb_redirect_enabled = false;
static bool g_sd_mmc_initialized = false;
static bool g_nand_mmc_initialized = false;

static struct mmc g_sd_mmc = {0};
static struct mmc g_nand_mmc = {0};

typedef struct mmc_partition_info_t {
    struct mmc *mmc;
    enum sdmmc_controller controller;
    enum sdmmc_partition mmc_partition;
} mmc_partition_info_t;

static int mmc_partition_initialize(device_partition_t *devpart) {
    mmc_partition_info_t *mmcpart = (mmc_partition_info_t *)devpart->device_struct;

    if (devpart->read_cipher != NULL || devpart->write_cipher != NULL) {
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

    if (!g_ahb_redirect_enabled) {
        mc_enable_ahb_redirect();
        g_ahb_redirect_enabled = true;
    }

    if (mmcpart->mmc == &g_sd_mmc && !g_sd_mmc_initialized) {
        int rc = sdmmc_init(mmcpart->mmc, mmcpart->controller);
        if (rc == 0) {
            sdmmc_set_write_enable(mmcpart->mmc, SDMMC_WRITE_ENABLED);
            devpart->initialized = g_sd_mmc_initialized = true;
        }
        return rc;
    } else if (mmcpart->mmc == &g_nand_mmc && !g_nand_mmc_initialized) {
        int rc = sdmmc_init(mmcpart->mmc, mmcpart->controller);
        if (rc == 0) {
            devpart->initialized = g_nand_mmc_initialized = true;
        }
        return rc;
    }

    return 0;
}

static void mmc_partition_finalize(device_partition_t *devpart) {
    free(devpart->crypto_work_buffer);
}

static enum sdmmc_partition g_current_emmc_partition = SDMMC_PARTITION_USER;

static int mmc_partition_read(device_partition_t *devpart, void *dst, uint64_t sector, uint64_t num_sectors) {
    mmc_partition_info_t *mmcpart = (mmc_partition_info_t *)devpart->device_struct;
    if (mmcpart->mmc == &g_nand_mmc && g_current_emmc_partition != mmcpart->mmc_partition) {
        int rc = sdmmc_select_partition(mmcpart->mmc, mmcpart->mmc_partition);
        if (rc != 0 && rc != ENOTTY) {
            return rc;
        }
        g_current_emmc_partition = mmcpart->mmc_partition;
    }
    return sdmmc_read(mmcpart->mmc, dst, (uint32_t)(devpart->start_sector + sector), (uint32_t)num_sectors);
}

static int mmc_partition_write(device_partition_t *devpart, const void *src, uint64_t sector, uint64_t num_sectors) {
    mmc_partition_info_t *mmcpart = (mmc_partition_info_t *)devpart->device_struct;
    if (mmcpart->mmc == &g_nand_mmc && g_current_emmc_partition != mmcpart->mmc_partition) {
        int rc = sdmmc_select_partition(mmcpart->mmc, mmcpart->mmc_partition);
        if (rc != 0 && rc != ENOTTY) {
            return rc;
        }
        g_current_emmc_partition = mmcpart->mmc_partition;
    }

    return sdmmc_write(mmcpart->mmc, src, (uint32_t)(devpart->start_sector + sector), (uint32_t)num_sectors);
}

static int switchfs_bis_crypto_decrypt(device_partition_t *devpart, uint64_t sector, uint64_t num_sectors) {
    unsigned int keyslot = (unsigned int)devpart->crypto_flags;
    (void)keyslot;
    (void)sector;
    (void)num_sectors;
    /*devpart->crypto_work_buffer*/
    /* TODO */
    return 0;
}

static int switchfs_bis_crypto_encrypt(device_partition_t *devpart, uint64_t sector, uint64_t num_sectors) {
    unsigned int keyslot = (unsigned int)devpart->crypto_flags;
    (void)keyslot;
    (void)sector;
    (void)num_sectors;
    /*devpart->crypto_work_buffer*/
    /* TODO */
    return 0;
}

static mmc_partition_info_t g_sd_mmcpart = { &g_sd_mmc, SWITCH_MICROSD, SDMMC_PARTITION_USER };
static mmc_partition_info_t g_nand_boot0_mmcpart = { &g_nand_mmc, SWITCH_EMMC, SDMMC_PARTITION_BOOT0 };
static mmc_partition_info_t g_nand_boot1_mmcpart = { &g_nand_mmc, SWITCH_EMMC, SDMMC_PARTITION_BOOT1 };
static mmc_partition_info_t g_nand_user_mmcpart = { &g_nand_mmc, SWITCH_EMMC, SDMMC_PARTITION_USER };

static const device_partition_t g_mmc_devpart_template = {
    .sector_size = 512,
    .initializer = mmc_partition_initialize,
    .finalizer = mmc_partition_finalize,
    .reader = mmc_partition_read,
    .writer = mmc_partition_write,
};

static int switchfs_mount_partition_gpt_callback(const efi_entry_t *entry, void *param, size_t entry_offset, FILE *disk) {
    (void)entry_offset;
    (void)disk;
    device_partition_t *parent = (device_partition_t *)param;
    device_partition_t devpart = *parent;
    char name_buffer[64];
    const uint16_t *utf16name = entry->name;
    uint32_t name_len;
    int rc;

    static const struct {
        const char *partition_name;
        const char *mount_point;
        bool is_fat;
        bool is_encrypted;
    } known_partitions[] = {
        {"PRODINFO", "prodinfo", false, true},
        {"PRODINFOF", "prodinfof", true, true},
        {"BCPKG2-1-Normal-Main", "bcpkg21", false, false},
        {"BCPKG2-2-Normal-Sub", "bcpkg22", false, false},
        {"BCPKG2-3-SafeMode-Main", "bcpkg23", false, false},
        {"BCPKG2-4-SafeMode-Sub", "bcpkg24", false, false},
        {"BCPKG2-5-Repair-Main", "bcpkg25", false, false},
        {"BCPKG2-6-Repair-Sub", "bcpkg26", false, false},
        {"SAFE", "safe", true, true},
        {"SYSTEM", "system", true, true},
        {"USER", "user", true, true},
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
            devpart.num_sectors = entry->last_lba - entry->first_lba;
            if (parent->num_sectors < devpart.num_sectors) {
                errno = EINVAL;
                return -1;
            }

            if (known_partitions[i].is_encrypted) {
                devpart.read_cipher = switchfs_bis_crypto_decrypt;
                devpart.write_cipher = switchfs_bis_crypto_encrypt;
            }

            if (known_partitions[i].is_fat) {
                rc = fsdev_mount_device(known_partitions[i].partition_name, &devpart, false);
                if (rc == -1) {
                    return -1;
                }
            } else {
                rc = rawdev_mount_device(known_partitions[i].partition_name, &devpart, false);
                if (rc == -1) {
                    return -1;
                }
            }
        }
    }

    return 0;
}

int switchfs_mount_all(void) {
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

    /* Boot0. */
    model = g_mmc_devpart_template;
    model.device_struct = &g_nand_boot0_mmcpart;
    model.start_sector = 0;
    model.num_sectors = 0x184000 / model.sector_size;
    rc = rawdev_mount_device("boot0", &model, true);
    if (rc == -1) {
        return -1;
    }

    /* Boot1. */
    model = g_mmc_devpart_template;
    model.device_struct = &g_nand_boot1_mmcpart;
    model.start_sector = 0;
    model.num_sectors = 0x80000 / model.sector_size;
    rc = rawdev_mount_device("boot1", &model, false);
    if (rc == -1) {
        return -1;
    }

    /* Raw NAND (excluding boot partitions), and its partitions. */
    model = g_mmc_devpart_template;
    model = g_mmc_devpart_template;
    model.device_struct = &g_nand_user_mmcpart;
    model.start_sector = 0;
    model.num_sectors = (32ull << 30) / model.sector_size;
    rc = rawdev_mount_device("rawnand", &model, false);
    if (rc == -1) {
        return -1;
    }
    rawnand = fopen("rawnand:/", "rb");
    rc = gpt_iterate_through_entries(rawnand, model.sector_size, switchfs_mount_partition_gpt_callback, &model);
    fclose(rawnand);
    if (rc != 0) {
        rc = fsdev_set_default_device("sdmc");
    }
    return rc;
}

int switchfs_unmount_all(void) {
    return fsdev_unmount_all() != 0 || rawdev_unmount_all() != 0 ? -1 : 0;
}
