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

#ifndef FUSEE_KEYDERIVATION_H
#define FUSEE_KEYDERIVATION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum BisPartition {
    BisPartition_Calibration = 0,
    BisPartition_Safe = 1,
    BisPartition_UserSystem = 2,
} BisPartition;

typedef struct {
    union {
        uint8_t keys[9][0x10];
        struct {
            uint8_t master_kek[0x10];
            uint8_t _keys[7][0x10];
            uint8_t package1_key[0x10];
        };
    };
} nx_dec_keyblob_t;

typedef struct nx_keyblob_t {
    uint8_t mac[0x10];
    uint8_t ctr[0x10];
    union {
        uint8_t data[0x90];
        nx_dec_keyblob_t dec_blob;
    };
} nx_keyblob_t;

int derive_nx_keydata(uint32_t target_firmware, const nx_keyblob_t *keyblobs, uint32_t available_revision, const void *tsec_key, void *tsec_root_key, unsigned int *out_keygen_type);
int load_package1_key(uint32_t revision);
void derive_bis_key(void *dst, BisPartition partition_id, uint32_t target_firmware);

#endif
