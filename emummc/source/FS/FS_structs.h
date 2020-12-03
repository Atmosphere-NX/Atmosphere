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

#ifndef __FS_STRUCTS_H__
#define __FS_STRUCTS_H__

#include "../utils/types.h"

typedef struct
{
    char *device_addr_buffer;
    uint64_t device_addr_buffer_size;
    char *device_addr_buffer_masked;
} sdmmc_dma_buffer_t;

_Static_assert(__alignof(sdmmc_dma_buffer_t) == 8, "sdmmc_dma_buffer_t definition");

typedef struct sdmmc_accessor_vt
{
    void *ctor;
    void *dtor;
    void *map_device_addr_space;
    void *unmap_device_addr_space;
    uint64_t (*sdmmc_accessor_controller_open)(void *);
    uint64_t (*sdmmc_accessor_controller_close)(void *);
    uint64_t (*read_write)(void *, uint64_t, uint64_t, void *, uint64_t, uint64_t);
    // More not included because we don't use it.
} sdmmc_accessor_vt_t;

_Static_assert(__alignof(sdmmc_accessor_vt_t) == 8, "sdmmc_accessor_vt_t definition");

typedef struct
{
    void *vtab;
    t210_sdmmc_t *io_map;
    sdmmc_dma_buffer_t dmaBuffers[3];
    // More not included because we don't use it.
} mmc_obj_t;

typedef struct
{
    sdmmc_accessor_vt_t *vtab;
    mmc_obj_t *parent;
    // More not included because we don't use it.
} sdmmc_accessor_t;

#endif // __FS_STRUCTS_H__
