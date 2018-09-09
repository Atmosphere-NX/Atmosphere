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
 
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "arm.h"
#include "exocfg.h"

#include "titlekey.h"
#include "masterkey.h"
#include "se.h"

static uint64_t g_tkey_expected_label_hash[4];
static unsigned int g_tkey_master_key_rev = MASTERKEY_REVISION_MAX;
static unsigned int g_tkey_type = 0;

/* Set the expected db prefix. */
void tkey_set_expected_label_hash(uint64_t *label_hash) {
    for (unsigned int i = 0; i < 4; i++) {
        g_tkey_expected_label_hash[i] = label_hash[i];
    }
}

void tkey_set_master_key_rev(unsigned int master_key_rev) {
    if (master_key_rev >= MASTERKEY_REVISION_MAX) {
        generic_panic();
    }
    g_tkey_master_key_rev = master_key_rev;
}

static void tkey_validate_type(unsigned int type) {
    if (type > TITLEKEY_TYPE_MAX || (type > 0 && exosphere_get_target_firmware() < EXOSPHERE_TARGET_FIRMWARE_600)) {
        generic_panic();
    }
}

void tkey_set_type(unsigned int type) {
    tkey_validate_type(type);
    g_tkey_type = type;
}

/* Reference for MGF1 can be found here: https://en.wikipedia.org/wiki/Mask_generation_function#MGF1 */
void calculate_mgf1_and_xor(void *masked, size_t masked_size, const void *seed, size_t seed_size) {
    uint8_t cur_hash[0x20];
    uint8_t hash_buf[0xE4];
    if (seed_size >= 0xE0) {
        generic_panic();
    }

    size_t hash_buf_size = seed_size + 4;
    if (seed_size != 0) {
        memcpy(hash_buf, seed, seed_size);
    }
    uint32_t round_num = 0;

    uint8_t *p_out = (uint8_t *)masked;

    while (masked_size) {
        size_t cur_size = masked_size;
        if (cur_size > 0x20) {
            cur_size = 0x20;
        }

        write32be(hash_buf, seed_size, round_num++);

        flush_dcache_range(hash_buf, hash_buf + hash_buf_size);
        se_calculate_sha256(cur_hash, hash_buf, hash_buf_size);

        for (unsigned int i = 0; i < cur_size; i++) {
            *p_out ^= cur_hash[i];
            p_out++;
        }

        masked_size -= cur_size;
    }
}

size_t tkey_rsa_oaep_unwrap(void *dst, size_t dst_size, void *src, size_t src_size) {
    if (src_size != 0x100) {
        generic_panic();
    }

    /* RSA Wrapped titlekeys use RSA-OAEP. */
    /* Message is of the form prefix || maskedSalt || maskedDB. */
    /* maskedSalt = salt ^ MGF1(maskedDB) */
    /* maskedDB = DB ^ MGF1(salt) */
    /* Salt is random and not validated in any way. */
    /* DB is of the form label_hash || 00....01 || wrapped_titlekey. */
    /* label_hash is, in practice, a constant in es .rodata. */
    /* I have no idea why Nintendo did this, it should be either nonconstant (in tik) or in tz .rodata. */

    uint8_t *message = (uint8_t *)src;

    /* Prefix should always be zero. */
    if (*message != 0) {
        return 0;
    }


    uint8_t *salt = message + 1;
    uint8_t *db = message + 0x21;

    /* This will be passed to smc_unwrap_rsa_oaep_wrapped_titlekey. */
    uint8_t *expected_label_hash = (uint8_t *)(&g_tkey_expected_label_hash[0]);

    /* Unmask the salt. */
    calculate_mgf1_and_xor(salt, 0x20, db, 0xDF);
    /* Unmask the DB. */
    calculate_mgf1_and_xor(db, 0xDF, salt, 0x20);

    /* Validate expected salt. */
    for (unsigned int i = 0; i < 0x20; i++) {
        if (expected_label_hash[i] != db[i]) {
            return 0;
        }
    }

    /* Don't validate salt from message[1:0x21] at all. */

    /* Advance pointer to DB, since we've validated the salt prefix. */
    db += 0x20;

    /* DB must be of the form 0000...01 || wrapped_titlekey */
    if (*db != 0) {
        return 0;
    }

    /* Locate wrapped_titlekey inside DB. */
    size_t wrapped_key_offset_in_db = 0;
    while (wrapped_key_offset_in_db < 0xBF) {
        if (db[wrapped_key_offset_in_db] == 0) {
            wrapped_key_offset_in_db++;
        } else if (db[wrapped_key_offset_in_db] == 1) {
            wrapped_key_offset_in_db++;
            break;
        } else {
            /* Invalid wrapped titlekey prefix. */
            return 0;
        }
    }

    /* Validate size... */
    size_t wrapped_titlekey_size = 0xBF - wrapped_key_offset_in_db;
    if (wrapped_titlekey_size > dst_size || wrapped_titlekey_size == 0) {
        return 0;
    }

    /* Extract the wrapped key. */
    memcpy(dst, &db[wrapped_key_offset_in_db], wrapped_titlekey_size);
    return wrapped_titlekey_size;
}

static const uint8_t titlekek_sources[TITLEKEY_TYPE_MAX+1][0x10] = {
    {0x1E, 0xDC, 0x7B, 0x3B, 0x60, 0xE6, 0xB4, 0xD8, 0x78, 0xB8, 0x17, 0x15, 0x98, 0x5E, 0x62, 0x9B},
    {0x3B, 0x78, 0xF2, 0x61, 0x0F, 0x9D, 0x5A, 0xE2, 0x7B, 0x4E, 0x45, 0xAF, 0xCB, 0x0B, 0x67, 0x4D}
};

void tkey_aes_unwrap(void *dst, size_t dst_size, const void *src, size_t src_size) {
    if (g_tkey_master_key_rev >= MASTERKEY_REVISION_MAX || dst_size != 0x10 || src_size != 0x10) {
        generic_panic();
    }
    
    /* Generate the appropriate titlekek into keyslot 9. */
    unsigned int master_keyslot = mkey_get_keyslot(g_tkey_master_key_rev);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, master_keyslot, titlekek_sources[g_tkey_type], 0x10);

    /* Unwrap the titlekey using the titlekek. */
    se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_TEMPKEY, dst, 0x10, src, 0x10);
}
