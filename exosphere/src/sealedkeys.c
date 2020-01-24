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
 
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "exocfg.h"
#include "sealedkeys.h"
#include "se.h"

static const uint8_t g_titlekey_seal_key_source[0x10] = {
    0xCB, 0xB7, 0x6E, 0x38, 0xA1, 0xCB, 0x77, 0x0F, 0xB2, 0xA5, 0xB2, 0x9D, 0xD8, 0x56, 0x9F, 0x76
};

static const uint8_t g_seal_key_sources[CRYPTOUSECASE_MAX_5X][0x10] = {
    {0xF4, 0x0C, 0x16, 0x26, 0x0D, 0x46, 0x3B, 0xE0, 0x8C, 0x6A, 0x56, 0xE5, 0x82, 0xD4, 0x1B, 0xF6},
    {0x7F, 0x54, 0x2C, 0x98, 0x1E, 0x54, 0x18, 0x3B, 0xBA, 0x63, 0xBD, 0x4C, 0x13, 0x5B, 0xF1, 0x06},
    {0xC7, 0x3F, 0x73, 0x60, 0xB7, 0xB9, 0x9D, 0x74, 0x0A, 0xF8, 0x35, 0x60, 0x1A, 0x18, 0x74, 0x63},
    {0x0E, 0xE0, 0xC4, 0x33, 0x82, 0x66, 0xE8, 0x08, 0x39, 0x13, 0x41, 0x7D, 0x04, 0x64, 0x2B, 0x6D},
    {0xE1, 0xA8, 0xAA, 0x6A, 0x2D, 0x9C, 0xDE, 0x43, 0x0C, 0xDE, 0xC6, 0x17, 0xF6, 0xC7, 0xF1, 0xDE},
    {0x74, 0x20, 0xF6, 0x46, 0x77, 0xB0, 0x59, 0x2C, 0xE8, 0x1B, 0x58, 0x64, 0x47, 0x41, 0x37, 0xD9},
    {0xAA, 0x19, 0x0F, 0xFA, 0x4C, 0x30, 0x3B, 0x2E, 0xE6, 0xD8, 0x9A, 0xCF, 0xE5, 0x3F, 0xB3, 0x4B}
};

bool usecase_is_invalid(unsigned int usecase) {
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_500) {
        return usecase >= CRYPTOUSECASE_MAX_5X;
    } else {
        return usecase >= CRYPTOUSECASE_MAX;
    }
}

void seal_key_internal(void *dst, const void *src, const uint8_t *seal_key_source) {
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, KEYSLOT_SWITCH_SESSIONKEY, seal_key_source, 0x10);
    se_aes_128_ecb_encrypt_block(KEYSLOT_SWITCH_TEMPKEY, dst, 0x10, src, 0x10);
}

void unseal_key_internal(unsigned int keyslot, const void *src, const uint8_t *seal_key_source) {
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, KEYSLOT_SWITCH_SESSIONKEY, seal_key_source, 0x10);
    decrypt_data_into_keyslot(keyslot, KEYSLOT_SWITCH_TEMPKEY, src, 0x10);
}


void seal_titlekey(void *dst, size_t dst_size, const void *src, size_t src_size) {
    if (dst_size != 0x10 || src_size != 0x10) {
        generic_panic();
    }
    
    seal_key_internal(dst, src, g_titlekey_seal_key_source);

}

void unseal_titlekey(unsigned int keyslot, const void *src, size_t src_size) {
    if (src_size != 0x10) {
        generic_panic();
    }
    
    unseal_key_internal(keyslot, src, g_titlekey_seal_key_source);
}


void seal_key(void *dst, size_t dst_size, const void *src, size_t src_size, unsigned int usecase) {
    if (usecase_is_invalid(usecase) || dst_size != 0x10 || src_size != 0x10) {
        generic_panic();
    }
    
    
    seal_key_internal(dst, src, g_seal_key_sources[usecase]);
}

void unseal_key(unsigned int keyslot, const void *src, size_t src_size, unsigned int usecase) {
    if (usecase_is_invalid(usecase) || src_size != 0x10) {
        generic_panic();
    }
    
    unseal_key_internal(keyslot, src, g_seal_key_sources[usecase]);
}