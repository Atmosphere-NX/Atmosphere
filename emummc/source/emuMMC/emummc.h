/*
 * Copyright (c) 2019 m4xw <m4x@m4xw.net>
 * Copyright (c) 2019 Atmosphere-NX
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

#ifndef __EMUMMC_H__
#define __EMUMMC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "../emmc/nx_sd.h"
#include "../emmc/sdmmc.h"
#include "../soc/i2c.h"
#include "../soc/gpio.h"
#include "../utils/util.h"
#include "../FS/FS.h"
#include "../libs/fatfs/ff.h"

// FS typedefs
typedef sdmmc_accessor_t *(*_sdmmc_accessor_gc)();
typedef sdmmc_accessor_t *(*_sdmmc_accessor_sd)();
typedef sdmmc_accessor_t *(*_sdmmc_accessor_nand)();
typedef void (*_lock_mutex)(void *mtx);
typedef void (*_unlock_mutex)(void *mtx);

bool sdmmc_initialize(void);
void sdmmc_finalize(void);
int sdmmc_nand_get_active_partition_index();
sdmmc_accessor_t *sdmmc_accessor_get(int mmc_id);

void mutex_lock_handler(int mmc_id);
void mutex_unlock_handler(int mmc_id);

// Hooks
uint64_t sdmmc_wrapper_controller_open(int mmc_id);
uint64_t sdmmc_wrapper_controller_close(int mmc_id);
uint64_t sdmmc_wrapper_read(void *buf, uint64_t bufSize, int mmc_id, unsigned int sector, unsigned int num_sectors);
uint64_t sdmmc_wrapper_write(int mmc_id, unsigned int sector, unsigned int num_sectors, void *buf, uint64_t bufSize);

typedef struct _file_based_ctxt
{
	FATFS sd_fs;
	uint64_t parts;
	uint64_t part_size;
	FIL fp_boot0;
	DWORD clmt_boot0[0x400];
	FIL fp_boot1;
	DWORD clmt_boot1[0x400];
	FIL fp_gpp[32];
	DWORD clmt_gpp[0x8000];
} file_based_ctxt;

#ifdef __cplusplus
}
#endif

#endif /* __EMUMMC_H__ */
