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
 
#ifndef EXOSPHERE_TITLEKEY_H
#define EXOSPHERE_TITLEKEY_H

#include <stdint.h>

#define TITLEKEY_TYPE_MAX 0x1

void tkey_set_expected_label_hash(uint64_t *label_hash);
void tkey_set_master_key_rev(unsigned int master_key_rev);
void tkey_set_type(unsigned int type);

size_t tkey_rsa_oaep_unwrap(void *dst, size_t dst_size, void *src, size_t src_size);

void tkey_aes_unwrap(void *dst, size_t dst_size, const void *src, size_t src_size);

#endif