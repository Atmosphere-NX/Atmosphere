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
 
#ifndef FUSEE_NX_FS_H
#define FUSEE_NX_FS_H

#include "fs_dev.h"
#include "raw_dev.h"

int nxfs_init();
int nxfs_end();

int nxfs_mount_sd();
int nxfs_mount_emmc();
int nxfs_mount_emummc_partition(uint64_t emummc_start_sector);
int nxfs_mount_emummc_file(const char *emummc_path, int num_parts, uint64_t part_limit);
int nxfs_unmount_sd();
int nxfs_unmount_emmc();
int nxfs_unmount_emummc();

#endif
