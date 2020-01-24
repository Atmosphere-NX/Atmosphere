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
 
#ifndef FUSEE_IPS_H
#define FUSEE_IPS_H

#include <stdint.h>
#include "utils.h"
#include "kip.h"
#include "exocfg.h"

#define FS_TITLE_ID 0x0100000000000000ull

void apply_kernel_ips_patches(void *kernel, size_t kernel_size);
kip1_header_t *apply_kip_ips_patches(kip1_header_t *kip, size_t kip_size, emummc_fs_ver_t *out_fs_ver);
kip1_header_t *kip1_uncompress(kip1_header_t *kip, size_t *size);

void kip_patches_set_enable_nogc(void);

#endif
