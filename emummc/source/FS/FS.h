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

#ifndef __FS_H__
#define __FS_H__

// TODO
#include "../emmc/sdmmc_t210.h"

#include "FS_versions.h"
#include "FS_offsets.h"
#include "FS_structs.h"

#define FS_SDMMC_EMMC 0
#define FS_SDMMC_SD 1
#define FS_SDMMC_GC 2

#define FS_EMMC_PARTITION_GPP 0
#define FS_EMMC_PARTITION_BOOT0 1
#define FS_EMMC_PARTITION_BOOT1 2
#define FS_EMMC_PARTITION_INVALID 3

#define BOOT_PARTITION_SIZE 0x2000
#define FS_READ_WRITE_ERROR 1048

#define NAND_PATROL_SECTOR 0xC20
#define NAND_PATROL_OFFSET 0x184000

typedef struct _fs_nand_patrol_t
{
	uint8_t hmac[0x20];
	unsigned int offset;
	unsigned int count;
	uint8_t rsvd[0x1D8];
} fs_nand_patrol_t;

#endif /* __FS_H__ */
