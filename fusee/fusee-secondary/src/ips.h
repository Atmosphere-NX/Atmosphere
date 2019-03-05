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
 
#ifndef FUSEE_IPS_H
#define FUSEE_IPS_H

#include "utils.h"
#include "kip.h"
#include <stdint.h>

void apply_kernel_ips_patches(void *kernel, size_t kernel_size);
kip1_header_t *apply_kip_ips_patches(kip1_header_t *kip, size_t kip_size);

void kip_patches_set_enable_nogc(void);

#endif
