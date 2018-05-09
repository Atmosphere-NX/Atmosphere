#ifndef FUSEE_RAW_MMC_DEV_H
#define FUSEE_RAW_MMC_DEV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "sdmmc.h"

#define RAWMMC_MAX_DEVICES 16

/* Note: only XTS, CTR and ECB are supported */
typedef void (*rawmmc_crypt_func_t)(void *dst, const void *src, size_t offset, size_t len, const uint8_t *iv, uint64_t flags);

int rawmmcdev_mount_device(
    const char *name,
    struct mmc *mmc,
    enum sdmmc_partition partition,
    size_t offset,
    size_t size,
    rawmmc_crypt_func_t read_crypt_func,
    rawmmc_crypt_func_t write_crypt_func,
    uint64_t crypt_flags,
    const uint8_t *iv
);

int rawmmcdev_mount_unencrypted_device(
    const char *name,
    struct mmc *mmc,
    enum sdmmc_partition partition,
    size_t offset,
    size_t size
);

int rawmmcdev_unmount_device(const char *name);
int rawmmcdev_unmount_all(void);

#endif
