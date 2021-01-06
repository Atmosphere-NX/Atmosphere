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

#ifndef FUSEE_FS_UTILS_H
#define FUSEE_FS_UTILS_H

#include <stdbool.h>
#include <stdint.h>

#include "../../../fusee/common/sdmmc/sdmmc.h"
#include "utils.h"

extern sdmmc_t g_sd_sdmmc;
extern sdmmc_device_t g_sd_device;

bool mount_sd(void);
void unmount_sd(void);
uint32_t get_file_size(const char *filename);
int read_from_file(void *dst, uint32_t dst_size, const char *filename);
int write_to_file(void *src, uint32_t src_size, const char *filename);

#endif
