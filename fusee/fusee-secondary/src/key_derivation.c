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

#include <stdio.h>
#include "../../../fusee/common/log.h"
#include "key_derivation.h"
#include "masterkey.h"
#include "se.h"
#include "exocfg.h"
#include "fuse.h"
#include "extkeys.h"
#include "utils.h"

#define AL16 ALIGN(16)

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

static const uint8_t AL16 keyblob_seed_00[0x10] = {
    0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3
};

static const uint8_t AL16 master_kek_seed_erista[0x10] = { /* TODO: Update on next change of keys. */
    0x84, 0x67, 0xB6, 0x7F, 0x13, 0x11, 0xAE, 0xE6, 0x58, 0x9B, 0x19, 0xAF, 0x13, 0x6C, 0x80, 0x7A /* Erista MasterKek seed 0B. */
};

static const uint8_t AL16 master_devkey_seed_erista[0x10] = {
    0xAA, 0xFD, 0xBC, 0xBB, 0x25, 0xC3, 0xA4, 0xEF, 0xE3, 0xEE, 0x58, 0x53, 0xB7, 0xF8, 0xDD, 0xD6
};

static const uint8_t AL16 master_kek_seed_mariko[0x10] = { /* TODO: Update on next change of keys. */
    0xE5, 0x41, 0xAC, 0xEC, 0xD1, 0xA7, 0xD1, 0xAB, 0xED, 0x03, 0x77, 0xF1, 0x27, 0xCA, 0xF8, 0xF1, /* Mariko MasterKek seed 0B. */
};

/* Derive all Switch keys. */
int derive_nx_keydata_erista(uint32_t target_firmware) {
    uint8_t AL16 work_buffer[0x10];

    /* Get whether we're using dev keys. */
    const bool is_retail = fuse_get_hardware_state() != 0;

    /* Derive Keyblob Key 00. */
    se_aes_ecb_decrypt_block(0xC, work_buffer, 0x10, keyblob_seed_00, 0x10);
    decrypt_data_into_keyslot(0xF, 0xE, work_buffer, 0x10);

    /* Derive master kek. */
    decrypt_data_into_keyslot(0xE, is_retail ? 0xD : 0xB, master_kek_seed_erista, 0x10);

    /* Derive master key, device master key. */
    decrypt_data_into_keyslot(0xD, 0xE, masterkey_seed, 0x10);
    decrypt_data_into_keyslot(0xE, 0xE, masterkey_4x_seed, 0x10);

    /* Derive device keys. */
    decrypt_data_into_keyslot(0xA, 0xF, devicekey_4x_seed, 0x10);
    decrypt_data_into_keyslot(0xF, 0xF, devicekey_seed, 0x10);

    /* Derive firmware specific device key. */
    se_aes_ecb_decrypt_block(0xA, work_buffer, 0x10, master_devkey_seed_erista, 0x10);
    decrypt_data_into_keyslot(0xC, 0xE, work_buffer, 0x10);

    /* Clear keyslots 0xB/0xE. */
    clear_aes_keyslot(0xB);
    clear_aes_keyslot(0xE);

    /* Setup master key revision, derive older master keys for use. */
    return mkey_detect_revision(fuse_get_hardware_state() != 0);
}

int derive_nx_keydata_mariko(uint32_t target_firmware) {
    /* Derive the device and master keys. */
    /* NOTE: Keyslots 7 and 10 are chosen here so we don't overwrite critical key material (KEK, BEK, SBK and SSK). */
    decrypt_data_into_keyslot(0xA, 0xE, devicekey_4x_seed, 0x10);
    decrypt_data_into_keyslot(0x7, 0xC, master_kek_seed_mariko, 0x10);
    decrypt_data_into_keyslot(0x7, 0x7, masterkey_seed, 0x10);

    /* Setup master key revision, derive older master keys for use. */
    return mkey_detect_revision(fuse_get_hardware_state() != 0);
}

static void generate_specific_aes_key(void *dst, const void *wrapped_key, bool should_mask, uint32_t target_firmware, uint32_t generation) {
    unsigned int keyslot = devkey_get_keyslot(generation);

    if (fuse_get_soc_type() == 0 && fuse_get_bootrom_patch_version() < 0x7F) {
        /* On dev units, use a fixed "all-zeroes" seed. */
        /* Yes, this data really is all-zero in actual TrustZone .rodata. */
        static const uint8_t AL16 dev_specific_aes_key_source[0x10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        static const uint8_t AL16 dev_specific_aes_key_ctr[0x10] = {0x3C, 0xD5, 0x92, 0xEC, 0x68, 0x31, 0x4A, 0x06, 0xD4, 0x1B, 0x0C, 0xD9, 0xF6, 0x2E, 0xD9, 0xE9};
        static const uint8_t AL16 dev_specific_aes_key_mask[0x10] = {0xAC, 0xCA, 0x9A, 0xCA, 0xFF, 0x2E, 0xB9, 0x22, 0xCC, 0x1F, 0x4F, 0xAD, 0xDD, 0x77, 0x21, 0x1E};

        se_aes_ctr_crypt(keyslot, dst, 0x10, dev_specific_aes_key_source, 0x10, dev_specific_aes_key_ctr, 0x10);

        if (should_mask) {
            for (unsigned int i = 0; i < 0x10; i++) {
                ((uint8_t *)dst)[i] ^= dev_specific_aes_key_mask[i];
            }
        }
    } else {
        /* On retail, standard kek->key decryption. */
        static const uint8_t AL16 retail_specific_aes_key_source[0x10] = {0xE2, 0xD6, 0xB8, 0x7A, 0x11, 0x9C, 0xB8, 0x80, 0xE8, 0x22, 0x88, 0x8A, 0x46, 0xFB, 0xA1, 0x95};
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, keyslot, retail_specific_aes_key_source, 0x10);
        se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_TEMPKEY, dst, 0x10, wrapped_key, 0x10);
    }
}

static void generate_personalized_aes_key_for_bis(void *dst, const void *wrapped_kek, const void *wrapped_key, uint32_t target_firmware, uint32_t generation) {
    static const uint8_t AL16 kek_source[0x10] = {
        0x4D, 0x87, 0x09, 0x86, 0xC4, 0x5D, 0x20, 0x72, 0x2F, 0xBA, 0x10, 0x53, 0xDA, 0x92, 0xE8, 0xA9
    };
    static const uint8_t AL16 key_source[0x10] = {
        0x89, 0x61, 0x5E, 0xE0, 0x5C, 0x31, 0xB6, 0x80, 0x5F, 0xE5, 0x8F, 0x3D, 0xA2, 0x4F, 0x7A, 0xA8
    };

    unsigned int keyslot = devkey_get_keyslot(generation);
    /* Derive kek. */
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, keyslot, kek_source, 0x10);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, KEYSLOT_SWITCH_TEMPKEY, wrapped_kek, 0x10);
    /* Derive key. */
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, KEYSLOT_SWITCH_TEMPKEY, key_source, 0x10);
    se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_TEMPKEY, dst, 0x10, wrapped_key, 0x10);
}

void derive_bis_key(void *dst, BisPartition partition_id, uint32_t target_firmware) {
    static const uint8_t AL16 key_source_for_bis[3][2][0x10] = {
        {
            {0xF8, 0x3F, 0x38, 0x6E, 0x2C, 0xD2, 0xCA, 0x32, 0xA8, 0x9A, 0xB9, 0xAA, 0x29, 0xBF, 0xC7, 0x48},
            {0x7D, 0x92, 0xB0, 0x3A, 0xA8, 0xBF, 0xDE, 0xE1, 0xA7, 0x4C, 0x3B, 0x6E, 0x35, 0xCB, 0x71, 0x06}
        },
        {
            {0x41, 0x00, 0x30, 0x49, 0xDD, 0xCC, 0xC0, 0x65, 0x64, 0x7A, 0x7E, 0xB4, 0x1E, 0xED, 0x9C, 0x5F},
            {0x44, 0x42, 0x4E, 0xDA, 0xB4, 0x9D, 0xFC, 0xD9, 0x87, 0x77, 0x24, 0x9A, 0xDC, 0x9F, 0x7C, 0xA4}
        },
        {
            {0x52, 0xC2, 0xE9, 0xEB, 0x09, 0xE3, 0xEE, 0x29, 0x32, 0xA1, 0x0C, 0x1F, 0xB6, 0xA0, 0x92, 0x6C},
            {0x4D, 0x12, 0xE1, 0x4B, 0x2A, 0x47, 0x4C, 0x1C, 0x09, 0xCB, 0x03, 0x59, 0xF0, 0x15, 0xF4, 0xE4}
        }
    };

    uint32_t bis_key_generation = fuse_get_device_unique_key_generation();
    if (bis_key_generation > 0) {
        bis_key_generation -= 1;
    }

    static const uint8_t AL16 bis_kek_source[0x10] = {0x34, 0xC1, 0xA0, 0xC4, 0x82, 0x58, 0xF8, 0xB4, 0xFA, 0x9E, 0x5E, 0x6A, 0xDA, 0xFC, 0x7E, 0x4F};
    switch (partition_id) {
        case BisPartition_Calibration:
            generate_specific_aes_key(dst, key_source_for_bis[partition_id][0], false, target_firmware, bis_key_generation);
            generate_specific_aes_key(dst + 0x10, key_source_for_bis[partition_id][1], false, target_firmware, bis_key_generation);
            break;
        case BisPartition_Safe:
        case BisPartition_UserSystem:
            generate_personalized_aes_key_for_bis(dst, bis_kek_source, key_source_for_bis[partition_id][0], target_firmware, bis_key_generation);
            generate_personalized_aes_key_for_bis(dst + 0x10, bis_kek_source, key_source_for_bis[partition_id][1], target_firmware, bis_key_generation);
            break;
        default:
            generic_panic();
    }
}
