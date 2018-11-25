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
 
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "configitem.h"
#include "masterkey.h"
#include "se.h"

static unsigned int g_mkey_revision = 0;
static bool g_determined_mkey_revision = false;

static uint8_t g_old_masterkeys[MASTERKEY_REVISION_MAX][0x10];
static uint8_t g_old_devicekeys[MASTERKEY_NUM_NEW_DEVICE_KEYS - 1][0x10];


/* TODO: Extend with new vectors, as needed. */
/* Dev unit keys. */
static const uint8_t mkey_vectors_dev[MASTERKEY_REVISION_MAX][0x10] =
{
    {0x46, 0x22, 0xB4, 0x51, 0x9A, 0x7E, 0xA7, 0x7F, 0x62, 0xA1, 0x1F, 0x8F, 0xC5, 0x3A, 0xDB, 0xFE}, /* Zeroes encrypted with Master Key 00. */
    {0x39, 0x33, 0xF9, 0x31, 0xBA, 0xE4, 0xA7, 0x21, 0x2C, 0xDD, 0xB7, 0xD8, 0xB4, 0x4E, 0x37, 0x23}, /* Master key 00 encrypted with Master key 01. */
    {0x97, 0x29, 0xB0, 0x32, 0x43, 0x14, 0x8C, 0xA6, 0x85, 0xE9, 0x5A, 0x94, 0x99, 0x39, 0xAC, 0x5D}, /* Master key 01 encrypted with Master key 02. */
    {0x2C, 0xCA, 0x9C, 0x31, 0x1E, 0x07, 0xB0, 0x02, 0x97, 0x0A, 0xD8, 0x03, 0xA2, 0x76, 0x3F, 0xA3}, /* Master key 02 encrypted with Master key 03. */
    {0x9B, 0x84, 0x76, 0x14, 0x72, 0x94, 0x52, 0xCB, 0x54, 0x92, 0x9B, 0xC4, 0x8C, 0x5B, 0x0F, 0xBA}, /* Master key 03 encrypted with Master key 04. */
    {0x78, 0xD5, 0xF1, 0x20, 0x3D, 0x16, 0xE9, 0x30, 0x32, 0x27, 0x34, 0x6F, 0xCF, 0xE0, 0x27, 0xDC}, /* Master key 04 encrypted with Master key 05. */
    {0x6F, 0xD2, 0x84, 0x1D, 0x05, 0xEC, 0x40, 0x94, 0x5F, 0x18, 0xB3, 0x81, 0x09, 0x98, 0x8D, 0x4E}, /* Master key 05 encrypted with Master key 06. */
};

/* Retail unit keys. */
static const uint8_t mkey_vectors[MASTERKEY_REVISION_MAX][0x10] =
{
    {0x0C, 0xF0, 0x59, 0xAC, 0x85, 0xF6, 0x26, 0x65, 0xE1, 0xE9, 0x19, 0x55, 0xE6, 0xF2, 0x67, 0x3D}, /* Zeroes encrypted with Master Key 00. */
    {0x29, 0x4C, 0x04, 0xC8, 0xEB, 0x10, 0xED, 0x9D, 0x51, 0x64, 0x97, 0xFB, 0xF3, 0x4D, 0x50, 0xDD}, /* Master key 00 encrypted with Master key 01. */
    {0xDE, 0xCF, 0xEB, 0xEB, 0x10, 0xAE, 0x74, 0xD8, 0xAD, 0x7C, 0xF4, 0x9E, 0x62, 0xE0, 0xE8, 0x72}, /* Master key 01 encrypted with Master key 02. */
    {0x0A, 0x0D, 0xDF, 0x34, 0x22, 0x06, 0x6C, 0xA4, 0xE6, 0xB1, 0xEC, 0x71, 0x85, 0xCA, 0x4E, 0x07}, /* Master key 02 encrypted with Master key 03. */
    {0x6E, 0x7D, 0x2D, 0xC3, 0x0F, 0x59, 0xC8, 0xFA, 0x87, 0xA8, 0x2E, 0xD5, 0x89, 0x5E, 0xF3, 0xE9}, /* Master key 03 encrypted with Master key 04. */
    {0xEB, 0xF5, 0x6F, 0x83, 0x61, 0x9E, 0xF8, 0xFA, 0xE0, 0x87, 0xD7, 0xA1, 0x4E, 0x25, 0x36, 0xEE}, /* Master key 04 encrypted with Master key 05. */
    {0x1E, 0x1E, 0x22, 0xC0, 0x5A, 0x33, 0x3C, 0xB9, 0x0B, 0xA9, 0x03, 0x04, 0xBA, 0xDB, 0x07, 0x57}, /* Master key 05 encrypted with Master key 06. */
};

bool check_mkey_revision(unsigned int revision, bool is_retail) {
    uint8_t final_vector[0x10];

    unsigned int check_keyslot = KEYSLOT_SWITCH_MASTERKEY;
    if (revision > 0) {
        /* Generate old master key array. */
        for (unsigned int i = revision; i > 0; i--) {
            se_aes_ecb_decrypt_block(check_keyslot, g_old_masterkeys[i-1], 0x10, is_retail ? mkey_vectors[i] : mkey_vectors_dev[i], 0x10);
            set_aes_keyslot(KEYSLOT_SWITCH_TEMPKEY, g_old_masterkeys[i-1], 0x10);
            check_keyslot = KEYSLOT_SWITCH_TEMPKEY;
        }
    }

    se_aes_ecb_decrypt_block(check_keyslot, final_vector, 0x10, mkey_vectors[0], 0x10);
    for (unsigned int i = 0; i < 0x10; i++) {
        if (final_vector[i] != 0) {
            return false;
        }
    }
    return true;
}

void mkey_detect_revision(void) {
    if (g_determined_mkey_revision) {
        generic_panic();
    }
    
    for (unsigned int rev = 0; rev < MASTERKEY_REVISION_MAX; rev++) {
        if (check_mkey_revision(rev, configitem_is_retail())) {
            g_determined_mkey_revision = true;
            g_mkey_revision = rev;
            break;
        }
    }
    
    /* We must have determined the master key, or we're not running on a Switch. */
    if (!g_determined_mkey_revision) {
        /* Panic in bright red. */
        panic(0x00F00060);
    }
}

unsigned int mkey_get_revision(void) {
    if (!g_determined_mkey_revision) {
        generic_panic();
    }

    return g_mkey_revision;
}

unsigned int mkey_get_keyslot(unsigned int revision) {
    if (!g_determined_mkey_revision || revision >= MASTERKEY_REVISION_MAX) {
        generic_panic();
    }

    if (revision > g_mkey_revision) {
        generic_panic();
    }

    if (revision == g_mkey_revision) {
        return KEYSLOT_SWITCH_MASTERKEY;
    } else {
        /* Load into a temp keyslot. */
        set_aes_keyslot(KEYSLOT_SWITCH_TEMPKEY, g_old_masterkeys[revision], 0x10);
        return KEYSLOT_SWITCH_TEMPKEY;
    }
}


void set_old_devkey(unsigned int revision, const uint8_t *key) {
    if (revision < MASTERKEY_REVISION_400_410 || MASTERKEY_REVISION_MAX <= revision) {
        generic_panic();
    }

    memcpy(g_old_devicekeys[revision - MASTERKEY_REVISION_400_410], key, 0x10);
}

unsigned int devkey_get_keyslot(unsigned int revision) {
    if (!g_determined_mkey_revision || revision >= MASTERKEY_REVISION_MAX) {
        generic_panic();
    }

    if (revision > g_mkey_revision) {
        generic_panic();
    }

    if (revision >= 1) {
        if (revision == MASTERKEY_REVISION_MAX) {
            return KEYSLOT_SWITCH_DEVICEKEY;
        } else {
            /* Load into a temp keyslot. */
            set_aes_keyslot(KEYSLOT_SWITCH_TEMPKEY, g_old_devicekeys[revision - MASTERKEY_REVISION_400_410], 0x10);
            return KEYSLOT_SWITCH_TEMPKEY;
        }
    } else {
        return KEYSLOT_SWITCH_4XOLDDEVICEKEY;
    }
}