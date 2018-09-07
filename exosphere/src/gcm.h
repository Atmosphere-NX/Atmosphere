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
 
#ifndef EXOSPHERE_GCM_H
#define EXOSPHERE_GCM_H

#include <stdbool.h>
#include <stdint.h>

size_t gcm_decrypt_key(void *dst, size_t dst_size,
                       const void *src, size_t src_size,
                       const void *sealed_kek, size_t kek_size,
                       const void *wrapped_key, size_t key_size,
                       unsigned int usecase, bool is_personalized, 
                       uint8_t *out_deviceid_high);


void gcm_encrypt_key(void *dst, size_t dst_size,
                       const void *src, size_t src_size,
                       const void *sealed_kek, size_t kek_size,
                       const void *wrapped_key, size_t key_size,
                       unsigned int usecase, uint64_t deviceid_high);

#endif
