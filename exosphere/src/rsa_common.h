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

#ifndef EXOSPHERE_RSA_COMMON_H
#define EXOSPHERE_RSA_COMMON_H
#include <stdint.h>

typedef union {
    struct {
        uint8_t user_data[0x100];
    } storage_exp_mod;
    struct {
        uint32_t master_key_rev;
        uint32_t type;
        uint64_t expected_label_hash[4];
    } unwrap_titlekey;
} rsa_shared_data_t __attribute__((aligned(4)));

_Static_assert(sizeof(rsa_shared_data_t) == 0x100);

extern rsa_shared_data_t g_rsa_shared_data;

#endif
