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
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool sdmmc_read_sd_card(void *dst, size_t size, size_t sector_index, size_t sector_count);
bool sdmmc_write_sd_card(size_t sector_index, size_t sector_count, const void *src, size_t size);

#ifdef __cplusplus
}
#endif