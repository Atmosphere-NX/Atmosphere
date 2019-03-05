/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include "utils.h"
#include "sdmmc/sdmmc.h"

size_t get_file_size(const char *filename);
size_t read_from_file(void *dst, size_t dst_size, const char *filename);
size_t dump_to_file(const void *src, size_t src_size, const char *filename);

#endif
