#ifndef FUSEE_FS_UTILS_H
#define FUSEE_FS_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#include "sdmmc/sdmmc.h"
#include "utils.h"

sdmmc_t g_sd_sdmmc;
sdmmc_device_t g_sd_device;

bool mount_sd(void);
void unmount_sd(void);
uint32_t get_file_size(const char *filename);
int read_from_file(void *dst, uint32_t dst_size, const char *filename);
int write_to_file(void *src, uint32_t src_size, const char *filename);

#endif
