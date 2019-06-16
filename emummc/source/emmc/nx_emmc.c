/*
 * Copyright (c) 2018 naehrwert
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

#include <string.h>
#include "../utils/types.h"
#include "nx_emmc.h"

int nx_emmc_part_read(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	// The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;
	return sdmmc_storage_read(storage, part->lba_start + sector_off, num_sectors, buf);
}

int nx_emmc_part_write(sdmmc_storage_t *storage, emmc_part_t *part, u32 sector_off, u32 num_sectors, void *buf)
{
	// The last LBA is inclusive.
	if (part->lba_start + sector_off > part->lba_end)
		return 0;
	return sdmmc_storage_write(storage, part->lba_start + sector_off, num_sectors, buf);
}
