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

#ifndef __FS_OFFSETS_H__
#define __FS_OFFSETS_H__

#include <stdint.h>
#include "FS_versions.h"

typedef struct {
    int opcode_reg;
    uint32_t adrp_offset;
    uint32_t add_rel_offset;
} fs_offsets_nintendo_path_t;

typedef struct {
    // Accessor vtable getters
    uintptr_t sdmmc_accessor_gc;
    uintptr_t sdmmc_accessor_sd;
    uintptr_t sdmmc_accessor_nand;
    // Hooks
    uintptr_t sdmmc_wrapper_read;
    uintptr_t sdmmc_wrapper_write;
    uintptr_t rtld;
    uintptr_t rtld_destination;
    uintptr_t clkrst_set_min_v_clock_rate;
    // Misc funcs
    uintptr_t lock_mutex;
    uintptr_t unlock_mutex;
    uintptr_t sdmmc_accessor_controller_open;
    uintptr_t sdmmc_accessor_controller_close;
    // Misc data
    uintptr_t sd_mutex;
    uintptr_t nand_mutex;
    uintptr_t active_partition;
    uintptr_t sdmmc_das_handle;
    // NOPs
    uintptr_t sd_das_init;
    // Nintendo Paths
    fs_offsets_nintendo_path_t nintendo_paths[];
} fs_offsets_t;

const fs_offsets_t *get_fs_offsets(enum FS_VER version);

#endif // __FS_OFFSETS_H__
