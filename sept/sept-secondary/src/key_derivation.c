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

#include <stdio.h>
#include "key_derivation.h"
#include "se.h"
#include "fuse.h"
#include "utils.h"

#define AL16 ALIGN(16)

static const uint8_t AL16 keyblob_seed_00[0x10] = {
    0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3
};

static const uint8_t AL16 masterkey_seed[0x10] = {
    0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C
};

static const uint8_t AL16 devicekey_seed[0x10] = {
    0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78
};

static const uint8_t AL16 devicekey_4x_seed[0x10] = {
    0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28
};

static const uint8_t AL16 masterkey_4x_seed[0x10] = {
    0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66
};

static const uint8_t AL16 new_master_kek_seed_7x[0x10] = {
    0x9A, 0x3E, 0xA9, 0xAB, 0xFD, 0x56, 0x46, 0x1C, 0x9B, 0xF6, 0x48, 0x7F, 0x5C, 0xFA, 0x09, 0x5C
};

void derive_7x_keys(const void *tsec_key, void *tsec_root_key) {
    uint8_t AL16 work_buffer[0x10];

    /* Set keyslot flags properly in preparation of derivation. */
    set_aes_keyslot_flags(0xE, 0x15);
    set_aes_keyslot_flags(0xD, 0x15);
    
    /* Set the TSEC key. */
    set_aes_keyslot(0xD, tsec_key, 0x10);
    
    /* Derive keyblob key 0. */
    se_aes_ecb_decrypt_block(0xD, work_buffer, 0x10, keyblob_seed_00, 0x10);
    decrypt_data_into_keyslot(0xF, 0xE, work_buffer, 0x10);
    
    /* Clear the SBK. */
    clear_aes_keyslot(0xE);
    
    /* Derive the master kek. */
    set_aes_keyslot(0xC, tsec_root_key, 0x10);
    decrypt_data_into_keyslot(0xC, 0xC, new_master_kek_seed_7x, 0x10);
    
    /* Derive keys for exosphere. */
    decrypt_data_into_keyslot(0xA, 0xF, devicekey_4x_seed, 0x10);
    decrypt_data_into_keyslot(0xF, 0xF, devicekey_seed, 0x10);
    decrypt_data_into_keyslot(0xE, 0xC, masterkey_4x_seed, 0x10);
    decrypt_data_into_keyslot(0xC, 0xC, masterkey_seed, 0x10);
    
    /* Clear master kek from memory. */
    for (size_t i = 0; i < sizeof(work_buffer); i++) {
        work_buffer[i] = 0xCC;
    }
}
