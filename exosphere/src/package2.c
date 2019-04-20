/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
 
#include <string.h>

#include "utils.h"
#include "memory_map.h"
#include "bootup.h"
#include "cpu_context.h"
#include "package2.h"
#include "configitem.h"
#include "se.h"
#include "interrupt.h"
#include "masterkey.h"
#include "arm.h"
#include "pmc.h"
#include "randomcache.h"
#include "timers.h"
#include "bootconfig.h"
#include "sysctr0.h"
#include "exocfg.h"
#include "smc_api.h"

extern void *__start_cold_addr;
extern size_t __bin_size;

static const uint8_t new_device_key_sources[MASTERKEY_NUM_NEW_DEVICE_KEYS][0x10] = {
    {0x8B, 0x4E, 0x1C, 0x22, 0x42, 0x07, 0xC8, 0x73, 0x56, 0x94, 0x08, 0x8B, 0xCC, 0x47, 0x0F, 0x5D}, /* 4.x   New Device Key Source. */
    {0x6C, 0xEF, 0xC6, 0x27, 0x8B, 0xEC, 0x8A, 0x91, 0x99, 0xAB, 0x24, 0xAC, 0x4F, 0x1C, 0x8F, 0x1C}, /* 5.x   New Device Key Source. */
    {0x70, 0x08, 0x1B, 0x97, 0x44, 0x64, 0xF8, 0x91, 0x54, 0x9D, 0xC6, 0x84, 0x8F, 0x1A, 0xB2, 0xE4}, /* 6.x   New Device Key Source. */
    {0x8E, 0x09, 0x1F, 0x7A, 0xBB, 0xCA, 0x6A, 0xFB, 0xB8, 0x9B, 0xD5, 0xC1, 0x25, 0x9C, 0xA9, 0x17}, /* 6.2.0 New Device Key Source. */
    {0x8F, 0x77, 0x5A, 0x96, 0xB0, 0x94, 0xFD, 0x8D, 0x28, 0xE4, 0x19, 0xC8, 0x16, 0x1C, 0xDB, 0x3D}, /* 7.0.0 New Device Key Source. */
};

static const uint8_t new_device_keygen_sources[MASTERKEY_NUM_NEW_DEVICE_KEYS][0x10] = {
    {0x88, 0x62, 0x34, 0x6E, 0xFA, 0xF7, 0xD8, 0x3F, 0xE1, 0x30, 0x39, 0x50, 0xF0, 0xB7, 0x5D, 0x5D}, /* 4.x   New Device Keygen Source. */
    {0x06, 0x1E, 0x7B, 0xE9, 0x6D, 0x47, 0x8C, 0x77, 0xC5, 0xC8, 0xE7, 0x94, 0x9A, 0xA8, 0x5F, 0x2E}, /* 5.x   New Device Keygen Source. */
    {0x99, 0xFA, 0x98, 0xBD, 0x15, 0x1C, 0x72, 0xFD, 0x7D, 0x9A, 0xD5, 0x41, 0x00, 0xFD, 0xB2, 0xEF}, /* 6.x   New Device Keygen Source. */
    {0x81, 0x3C, 0x6C, 0xBF, 0x5D, 0x21, 0xDE, 0x77, 0x20, 0xD9, 0x6C, 0xE3, 0x22, 0x06, 0xAE, 0xBB}, /* 6.2.0 New Device Keygen Source. */
    {0x86, 0x61, 0xB0, 0x16, 0xFA, 0x7A, 0x9A, 0xEA, 0xF6, 0xF5, 0xBE, 0x1A, 0x13, 0x5B, 0x6D, 0x9E}, /* 7.0.0 New Device Keygen Source. */
};

static const uint8_t new_device_keygen_sources_dev[MASTERKEY_NUM_NEW_DEVICE_KEYS][0x10] = {
    {0xD6, 0xBD, 0x9F, 0xC6, 0x18, 0x09, 0xE1, 0x96, 0x20, 0x39, 0x60, 0xD2, 0x89, 0x83, 0x31, 0x34}, /* 4.x   New Device Keygen Source. */
    {0x59, 0x2D, 0x20, 0x69, 0x33, 0xB5, 0x17, 0xBA, 0xCF, 0xB1, 0x4E, 0xFD, 0xE4, 0xC2, 0x7B, 0xA8}, /* 5.x   New Device Keygen Source. */
    {0xF6, 0xD8, 0x59, 0x63, 0x8F, 0x47, 0xCB, 0x4A, 0xD8, 0x74, 0x05, 0x7F, 0x88, 0x92, 0x33, 0xA5}, /* 6.x   New Device Keygen Source. */
    {0x20, 0xAB, 0xF2, 0x0F, 0x05, 0xE3, 0xDE, 0x2E, 0xA1, 0xFB, 0x37, 0x5E, 0x8B, 0x22, 0x1A, 0x38}, /* 6.2.0 New Device Keygen Source. */
    {0x60, 0xAE, 0x56, 0x68, 0x11, 0xE2, 0x0C, 0x99, 0xDE, 0x05, 0xAE, 0x68, 0x78, 0x85, 0x04, 0xAE}, /* 6.2.0 New Device Keygen Source. */
};

static const uint8_t new_master_kek_sources[MASTERKEY_REVISION_700_CURRENT - MASTERKEY_REVISION_600_610][0x10] = {
    {0x37, 0x4B, 0x77, 0x29, 0x59, 0xB4, 0x04, 0x30, 0x81, 0xF6, 0xE5, 0x8C, 0x6D, 0x36, 0x17, 0x9A}, /* 6.2.0 Master Kek Source. */
    {0x9A, 0x3E, 0xA9, 0xAB, 0xFD, 0x56, 0x46, 0x1C, 0x9B, 0xF6, 0x48, 0x7F, 0x5C, 0xFA, 0x09, 0x5C}, /* 7.0.0 Master Kek Source. */
};

static const uint8_t keyblob_key_seed_00[0x10] = {
    0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3
};

static const uint8_t devicekey_seed[0x10] = {
    0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78
};

static const uint8_t devicekey_4x_seed[0x10] = {
    0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28
};

static const uint8_t masterkey_seed[0x10] = {
    0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C
};

static const uint8_t devicekek_4x_seed[0x10] = {
    0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66
};

static void derive_new_device_keys(unsigned int keygen_keyslot) {
    uint8_t work_buffer[0x10];
    bool is_retail = configitem_is_retail();
    for (unsigned int revision = 0; revision < MASTERKEY_NUM_NEW_DEVICE_KEYS; revision++) {
        se_aes_ecb_decrypt_block(keygen_keyslot, work_buffer, 0x10, new_device_key_sources[revision], 0x10);
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, mkey_get_keyslot(0), is_retail ? new_device_keygen_sources[revision] : new_device_keygen_sources_dev[revision], 0x10);
        if (revision < MASTERKEY_NUM_NEW_DEVICE_KEYS - 1) {
            se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_TEMPKEY, work_buffer, 0x10, work_buffer, 0x10);
            set_old_devkey(revision + MASTERKEY_REVISION_400_410, work_buffer);
        } else {
            decrypt_data_into_keyslot(KEYSLOT_SWITCH_DEVICEKEY, KEYSLOT_SWITCH_TEMPKEY, work_buffer, 0x10);
        }
    }
    set_aes_keyslot_flags(KEYSLOT_SWITCH_DEVICEKEY, 0xFF);
    clear_aes_keyslot(keygen_keyslot);
}

/* Hardware init, sets up the RNG and SESSION keyslots, derives new DEVICE key. */
static void setup_se(void) {
    uint8_t work_buffer[0x10];

    /* Sanity check the Security Engine. */
    se_verify_flags_cleared();

    /* Initialize interrupts. */
    intr_initialize_gic_nonsecure();

    /* Perform some sanity initialization. */
    volatile tegra_se_t *se = se_get_regs();
    se->_0x0 &= 0xFFFEFFFF; /* Clear bit 16. */
    (void)(se->FLAGS_REG);
    __dsb_sy();
    
    se->_0x4 = 0;
    se->AES_KEY_READ_DISABLE_REG = 0;
    se->RSA_KEY_READ_DISABLE_REG = 0;
    se->_0x0 &= 0xFFFFFFFB;

    /* Currently unknown what each flag does. */
    for (unsigned int i = 0; i < KEYSLOT_AES_MAX; i++) {
        set_aes_keyslot_flags(i, 0x15);
    }

    for (unsigned int i = 4; i < KEYSLOT_AES_MAX; i++) {
        set_aes_keyslot_flags(i, 0x40);
    }

    for (unsigned int i = 0; i < KEYSLOT_RSA_MAX; i++) {
        set_rsa_keyslot_flags(i, 0x41);
    }
    
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_620 && exosphere_should_perform_620_keygen()) {
        unsigned int master_kek_source_ind;
        switch (exosphere_get_target_firmware()) {
            case ATMOSPHERE_TARGET_FIRMWARE_620:
                master_kek_source_ind = MASTERKEY_REVISION_620 - MASTERKEY_REVISION_620;
                break;
            case ATMOSPHERE_TARGET_FIRMWARE_700:
            case ATMOSPHERE_TARGET_FIRMWARE_800:
                master_kek_source_ind = MASTERKEY_REVISION_700_CURRENT - MASTERKEY_REVISION_620;
                break;
            default:
                generic_panic();
                break;
        }
        /* Start by generating device keys. */
        se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_6XTSECKEY, work_buffer, 0x10, keyblob_key_seed_00, 0x10);
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_4XOLDDEVICEKEY, KEYSLOT_SWITCH_6XSBK, work_buffer, 0x10);
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_4XNEWCONSOLEKEYGENKEY, KEYSLOT_SWITCH_4XOLDDEVICEKEY, devicekey_4x_seed, 0x10);
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_4XOLDDEVICEKEY, KEYSLOT_SWITCH_4XOLDDEVICEKEY, devicekey_seed, 0x10);
        
        /* Next, generate the master kek, and from there master key/device kek. We use different keyslots than Nintendo, here. */
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_6XTSECROOTKEY, KEYSLOT_SWITCH_6XTSECROOTKEY, new_master_kek_sources[master_kek_source_ind], 0x10);
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_MASTERKEY, KEYSLOT_SWITCH_6XTSECROOTKEY, masterkey_seed, 0x10);
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_5XNEWDEVICEKEYGENKEY, KEYSLOT_SWITCH_6XTSECROOTKEY, devicekek_4x_seed, 0x10);
        clear_aes_keyslot(KEYSLOT_SWITCH_6XTSECROOTKEY);  
    }

    /* Detect Master Key revision. */
    mkey_detect_revision();

    /* Derive new device keys. */
    switch (exosphere_get_target_firmware()) {
        case ATMOSPHERE_TARGET_FIRMWARE_100:
        case ATMOSPHERE_TARGET_FIRMWARE_200:
        case ATMOSPHERE_TARGET_FIRMWARE_300:
            break;
        case ATMOSPHERE_TARGET_FIRMWARE_400:
            derive_new_device_keys(KEYSLOT_SWITCH_4XNEWDEVICEKEYGENKEY);
            break;
        case ATMOSPHERE_TARGET_FIRMWARE_500:
        case ATMOSPHERE_TARGET_FIRMWARE_600:
        case ATMOSPHERE_TARGET_FIRMWARE_620:
        case ATMOSPHERE_TARGET_FIRMWARE_700:
        case ATMOSPHERE_TARGET_FIRMWARE_800:
            derive_new_device_keys(KEYSLOT_SWITCH_5XNEWDEVICEKEYGENKEY);
            break;
    }

    se_initialize_rng(KEYSLOT_SWITCH_DEVICEKEY);

    /* Generate random data, transform with device key to get RNG key. */
    se_generate_random(KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_RNGKEY, KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    set_aes_keyslot_flags(KEYSLOT_SWITCH_RNGKEY, 0xFF);

    /* Repeat for Session key. */
    se_generate_random(KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_SESSIONKEY, KEYSLOT_SWITCH_DEVICEKEY, work_buffer, 0x10);
    set_aes_keyslot_flags(KEYSLOT_SWITCH_SESSIONKEY, 0xFF);

    /* Generate test vector for our keys. */
    se_generate_stored_vector();  
}

static void setup_boot_config(void) {
    /* Load boot config only if dev unit. */
    if (configitem_is_retail()) {
        bootconfig_clear();
    } else {
        void *bootconfig_ptr = NX_BOOTLOADER_BOOTCONFIG_POINTER;
        if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_600) {
            bootconfig_ptr = NX_BOOTLOADER_BOOTCONFIG_POINTER_6X;
        }
        flush_dcache_range((uint8_t *)bootconfig_ptr, (uint8_t *)bootconfig_ptr + sizeof(bootconfig_t));
        bootconfig_load_and_verify((bootconfig_t *)bootconfig_ptr);
    }
}

static void package2_crypt_ctr(unsigned int master_key_rev, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size) {
    /* Derive package2 key. */
    static const uint8_t package2_key_source[0x10] = {0xFB, 0x8B, 0x6A, 0x9C, 0x79, 0x00, 0xC8, 0x49, 0xEF, 0xD2, 0x4D, 0x85, 0x4D, 0x30, 0xA0, 0xC7};
    flush_dcache_range((uint8_t *)dst, (uint8_t *)dst + dst_size);
    flush_dcache_range((uint8_t *)src, (uint8_t *)src + src_size);
    unsigned int keyslot = mkey_get_keyslot(master_key_rev);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_PACKAGE2KEY, keyslot, package2_key_source, 0x10);

    /* Perform Encryption. */
    se_aes_ctr_crypt(KEYSLOT_SWITCH_PACKAGE2KEY, dst, dst_size, src, src_size, ctr, ctr_size);
}

static void verify_header_signature(package2_header_t *header) {
    const uint8_t *modulus;

    if (configitem_is_retail()) {
        static const uint8_t package2_modulus_retail[0x100] = {
            0x8D, 0x13, 0xA7, 0x77, 0x6A, 0xE5, 0xDC, 0xC0, 0x3B, 0x25, 0xD0, 0x58, 0xE4, 0x20, 0x69, 0x59,
            0x55, 0x4B, 0xAB, 0x70, 0x40, 0x08, 0x28, 0x07, 0xA8, 0xA7, 0xFD, 0x0F, 0x31, 0x2E, 0x11, 0xFE,
            0x47, 0xA0, 0xF9, 0x9D, 0xDF, 0x80, 0xDB, 0x86, 0x5A, 0x27, 0x89, 0xCD, 0x97, 0x6C, 0x85, 0xC5,
            0x6C, 0x39, 0x7F, 0x41, 0xF2, 0xFF, 0x24, 0x20, 0xC3, 0x95, 0xA6, 0xF7, 0x9D, 0x4A, 0x45, 0x74,
            0x8B, 0x5D, 0x28, 0x8A, 0xC6, 0x99, 0x35, 0x68, 0x85, 0xA5, 0x64, 0x32, 0x80, 0x9F, 0xD3, 0x48,
            0x39, 0xA2, 0x1D, 0x24, 0x67, 0x69, 0xDF, 0x75, 0xAC, 0x12, 0xB5, 0xBD, 0xC3, 0x29, 0x90, 0xBE,
            0x37, 0xE4, 0xA0, 0x80, 0x9A, 0xBE, 0x36, 0xBF, 0x1F, 0x2C, 0xAB, 0x2B, 0xAD, 0xF5, 0x97, 0x32,
            0x9A, 0x42, 0x9D, 0x09, 0x8B, 0x08, 0xF0, 0x63, 0x47, 0xA3, 0xE9, 0x1B, 0x36, 0xD8, 0x2D, 0x8A,
            0xD7, 0xE1, 0x54, 0x11, 0x95, 0xE4, 0x45, 0x88, 0x69, 0x8A, 0x2B, 0x35, 0xCE, 0xD0, 0xA5, 0x0B,
            0xD5, 0x5D, 0xAC, 0xDB, 0xAF, 0x11, 0x4D, 0xCA, 0xB8, 0x1E, 0xE7, 0x01, 0x9E, 0xF4, 0x46, 0xA3,
            0x8A, 0x94, 0x6D, 0x76, 0xBD, 0x8A, 0xC8, 0x3B, 0xD2, 0x31, 0x58, 0x0C, 0x79, 0xA8, 0x26, 0xE9,
            0xD1, 0x79, 0x9C, 0xCB, 0xD4, 0x2B, 0x6A, 0x4F, 0xC6, 0xCC, 0xCF, 0x90, 0xA7, 0xB9, 0x98, 0x47,
            0xFD, 0xFA, 0x4C, 0x6C, 0x6F, 0x81, 0x87, 0x3B, 0xCA, 0xB8, 0x50, 0xF6, 0x3E, 0x39, 0x5D, 0x4D,
            0x97, 0x3F, 0x0F, 0x35, 0x39, 0x53, 0xFB, 0xFA, 0xCD, 0xAB, 0xA8, 0x7A, 0x62, 0x9A, 0x3F, 0xF2,
            0x09, 0x27, 0x96, 0x3F, 0x07, 0x9A, 0x91, 0xF7, 0x16, 0xBF, 0xC6, 0x3A, 0x82, 0x5A, 0x4B, 0xCF,
            0x49, 0x50, 0x95, 0x8C, 0x55, 0x80, 0x7E, 0x39, 0xB1, 0x48, 0x05, 0x1E, 0x21, 0xC7, 0x24, 0x4F
        };
        modulus = package2_modulus_retail;
    } else {
        static const uint8_t package2_modulus_dev[0x100] = {
            0xB3, 0x65, 0x54, 0xFB, 0x0A, 0xB0, 0x1E, 0x85, 0xA7, 0xF6, 0xCF, 0x91, 0x8E, 0xBA, 0x96, 0x99,
            0x0D, 0x8B, 0x91, 0x69, 0x2A, 0xEE, 0x01, 0x20, 0x4F, 0x34, 0x5C, 0x2C, 0x4F, 0x4E, 0x37, 0xC7,
            0xF1, 0x0B, 0xD4, 0xCD, 0xA1, 0x7F, 0x93, 0xF1, 0x33, 0x59, 0xCE, 0xB1, 0xE9, 0xDD, 0x26, 0xE6,
            0xF3, 0xBB, 0x77, 0x87, 0x46, 0x7A, 0xD6, 0x4E, 0x47, 0x4A, 0xD1, 0x41, 0xB7, 0x79, 0x4A, 0x38,
            0x06, 0x6E, 0xCF, 0x61, 0x8F, 0xCD, 0xC1, 0x40, 0x0B, 0xFA, 0x26, 0xDC, 0xC0, 0x34, 0x51, 0x83,
            0xD9, 0x3B, 0x11, 0x54, 0x3B, 0x96, 0x27, 0x32, 0x9A, 0x95, 0xBE, 0x1E, 0x68, 0x11, 0x50, 0xA0,
            0x6B, 0x10, 0xA8, 0x83, 0x8B, 0xF5, 0xFC, 0xBC, 0x90, 0x84, 0x7A, 0x5A, 0x5C, 0x43, 0x52, 0xE6,
            0xC8, 0x26, 0xE9, 0xFE, 0x06, 0xA0, 0x8B, 0x53, 0x0F, 0xAF, 0x1E, 0xC4, 0x1C, 0x0B, 0xCF, 0x50,
            0x1A, 0xA4, 0xF3, 0x5C, 0xFB, 0xF0, 0x97, 0xE4, 0xDE, 0x32, 0x0A, 0x9F, 0xE3, 0x5A, 0xAA, 0xB7,
            0x44, 0x7F, 0x5C, 0x33, 0x60, 0xB9, 0x0F, 0x22, 0x2D, 0x33, 0x2A, 0xE9, 0x69, 0x79, 0x31, 0x42,
            0x8F, 0xE4, 0x3A, 0x13, 0x8B, 0xE7, 0x26, 0xBD, 0x08, 0x87, 0x6C, 0xA6, 0xF2, 0x73, 0xF6, 0x8E,
            0xA7, 0xF2, 0xFE, 0xFB, 0x6C, 0x28, 0x66, 0x0D, 0xBD, 0xD7, 0xEB, 0x42, 0xA8, 0x78, 0xE6, 0xB8,
            0x6B, 0xAE, 0xC7, 0xA9, 0xE2, 0x40, 0x6E, 0x89, 0x20, 0x82, 0x25, 0x8E, 0x3C, 0x6A, 0x60, 0xD7,
            0xF3, 0x56, 0x8E, 0xEC, 0x8D, 0x51, 0x8A, 0x63, 0x3C, 0x04, 0x78, 0x23, 0x0E, 0x90, 0x0C, 0xB4,
            0xE7, 0x86, 0x3B, 0x4F, 0x8E, 0x13, 0x09, 0x47, 0x32, 0x0E, 0x04, 0xB8, 0x4D, 0x5B, 0xB0, 0x46,
            0x71, 0xB0, 0x5C, 0xF4, 0xAD, 0x63, 0x4F, 0xC5, 0xE2, 0xAC, 0x1E, 0xC4, 0x33, 0x96, 0x09, 0x7B
        };
        modulus = package2_modulus_dev;
    }

    /* This is normally only allowed on dev units, but we'll allow it anywhere. */
    bool is_unsigned = EXOSPHERE_LOOSEN_PACKAGE2_RESTRICTIONS_FOR_DEBUG || bootconfig_is_package2_unsigned();
    if (!is_unsigned && se_rsa2048_pss_verify(header->signature, 0x100, modulus, 0x100, header->encrypted_header, 0x100) == 0) {
        panic(0xF0000001); /* Invalid PK21 signature. */
    }
}

static uint32_t get_package2_size(package2_meta_t *metadata) {
    return metadata->ctr_dwords[0] ^ metadata->ctr_dwords[2] ^ metadata->ctr_dwords[3];
}

static bool validate_package2_metadata(package2_meta_t *metadata) {
    if (metadata->magic != MAGIC_PK21) {
        return false;
    }
    

    /* Package2 size, version number is stored XORed in header CTR. */
    /* Nintendo, what the fuck? */
    uint32_t package_size = metadata->ctr_dwords[0] ^ metadata->ctr_dwords[2] ^ metadata->ctr_dwords[3];
    uint8_t header_version = (uint8_t)((metadata->ctr_dwords[1] ^ (metadata->ctr_dwords[1] >> 16) ^ (metadata->ctr_dwords[1] >> 24)) & 0xFF);

    /* Ensure package isn't too big or too small. */
    if (package_size <= sizeof(package2_header_t) || package_size > PACKAGE2_SIZE_MAX) {
        return false;
    }

    /* Validate that we're working with a header we know how to handle. */
    if (header_version > MASTERKEY_REVISION_MAX) {
        return false;
    }

    /* Require aligned entrypoint. */
    if (metadata->entrypoint & 3) {
        return false;
    }

    /* Validate section size sanity. */
    if (metadata->section_sizes[0] + metadata->section_sizes[1] + metadata->section_sizes[2] + sizeof(package2_header_t) != package_size) {
        return false;
    }

    bool entrypoint_found = false;

    /* Header has space for 4 sections, but only 3 are validated/potentially loaded on hardware. */
    size_t cur_section_offset = 0;
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        /* Validate section size alignment. */
        if (metadata->section_sizes[section] & 3) {
            return false;
        }

        /* Validate section does not overflow. */
        if (check_32bit_additive_overflow(metadata->section_offsets[section], metadata->section_sizes[section])) {
            return false;
        }

        /* Check for entrypoint presence. */
        uint32_t section_end = metadata->section_offsets[section] + metadata->section_sizes[section];
        if (metadata->section_offsets[section] <= metadata->entrypoint && metadata->entrypoint < section_end) {
            entrypoint_found = true;
        }

        /* Ensure no overlap with later sections. */
        if (metadata->section_sizes[section] != 0) {
            for (unsigned int later_section = section + 1; later_section < PACKAGE2_SECTION_MAX; later_section++) {
                if (metadata->section_sizes[later_section] == 0) {
                    continue;
                }
                uint32_t later_section_end = metadata->section_offsets[later_section] + metadata->section_sizes[later_section];
                if (overlaps(metadata->section_offsets[section], section_end, metadata->section_offsets[later_section], later_section_end)) {
                    return false;
                }
            }
        }

        bool check_hash = EXOSPHERE_LOOSEN_PACKAGE2_RESTRICTIONS_FOR_DEBUG == 0;
        /* Validate section hashes. */
        if (metadata->section_sizes[section]) {
            void *section_data = (void *)((uint8_t *)NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS + sizeof(package2_header_t) + cur_section_offset);
            uint8_t calculated_hash[0x20];
            flush_dcache_range((uint8_t *)section_data, (uint8_t *)section_data + metadata->section_sizes[section]);
            se_calculate_sha256(calculated_hash, section_data, metadata->section_sizes[section]);
            if (check_hash && memcmp(calculated_hash, metadata->section_hashes[section], sizeof(metadata->section_hashes[section])) != 0) {
                return false;
            }
            cur_section_offset += metadata->section_sizes[section];
        }

    }

    /* Ensure that entrypoint is present in one of our sections. */
    if (!entrypoint_found) {
        return false;
    }

    /* Perform version checks. */
    /* We will be compatible with all package2s released before current, but not newer ones. */
    if (metadata->version_max >= PACKAGE2_MINVER_THEORETICAL && metadata->version_min < PACKAGE2_MAXVER_700_CURRENT) {
        return true;
    }

    return false;
}

/* Decrypts package2 header, and returns the master key revision required. */
static uint32_t decrypt_and_validate_header(package2_header_t *header) {
    package2_meta_t metadata;

    if (bootconfig_is_package2_plaintext() == 0) {
        uint32_t mkey_rev;

        /* Try to decrypt for all possible master keys. */
        for (mkey_rev = 0; mkey_rev <= mkey_get_revision(); mkey_rev++) {
            package2_crypt_ctr(mkey_rev, &metadata, sizeof(package2_meta_t), &header->metadata, sizeof(package2_meta_t), header->metadata.ctr, sizeof(header->metadata.ctr));
            /* Copy the ctr (which stores information) into the decrypted metadata. */
            memcpy(metadata.ctr, header->metadata.ctr, sizeof(header->metadata.ctr));
            /* See if this is the correct key. */
            if (validate_package2_metadata(&metadata)) {
                header->metadata = metadata;
                return mkey_rev;
            }
        }

        /* Ensure we successfully decrypted the header. */
        if (mkey_rev > mkey_get_revision()) {
            panic(0xFAF00003);
        }
    } else if (!validate_package2_metadata(&header->metadata)) {
        panic(0xFAF0003);
    }
    return 0;
}

static void load_package2_sections(package2_meta_t *metadata, uint32_t master_key_rev) {
    /* By default, copy data directly from where NX_BOOTLOADER puts it. */
    void *load_buf = NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS;

    /* Check whether any of our sections overlap this region. If they do, we must relocate and copy from elsewhere. */
    bool needs_relocation = false;
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        uint64_t section_start = DRAM_BASE_PHYSICAL + (uint64_t)metadata->section_offsets[section];
        uint64_t section_end = section_start + (uint64_t)metadata->section_sizes[section];
        if (overlaps(section_start, section_end, (uint64_t)(NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS), (uint64_t)(NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS) + PACKAGE2_SIZE_MAX)) {
            needs_relocation = true;
        }
    }
    if (needs_relocation) {
        /* This code should *always* succeed in finding a carveout within seven loops, */
        /* due to the section size limit, and section number limit. */
        /* However, Nintendo tries panics after 8 loops if a safe section is not found. */
        /* This should never be the case, mathematically. */
        /* We will replicate this behavior. */
        bool found_safe_carveout = false;
        uint64_t potential_base_start = DRAM_BASE_PHYSICAL;
        uint64_t potential_base_end = potential_base_start + PACKAGE2_SIZE_MAX;
        for (unsigned int i = 0; i < 8; i++) {
            int is_safe = 1;
            for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
                uint64_t section_start = DRAM_BASE_PHYSICAL + (uint64_t)metadata->section_offsets[section];
                uint64_t section_end = section_start + (uint64_t)metadata->section_sizes[section];
                if (overlaps(section_start, section_end, potential_base_start, potential_base_end)) {
                    is_safe = 0;
                }
            }
            found_safe_carveout |= is_safe;
            if (found_safe_carveout) {
                break;
            }
            potential_base_start += PACKAGE2_SIZE_MAX;
            potential_base_end += PACKAGE2_SIZE_MAX;
        }
        if (!found_safe_carveout) {
            generic_panic();
        }
        /* Relocate to new carveout. */
        memcpy((void *)potential_base_start, load_buf, PACKAGE2_SIZE_MAX);
        memset(load_buf, 0, PACKAGE2_SIZE_MAX);
        load_buf = (void *)potential_base_start;
    }

    size_t cur_section_offset = 0;
    /* Copy each section to its appropriate location, decrypting if necessary. */
    for (unsigned int section = 0; section < PACKAGE2_SECTION_MAX; section++) {
        if (metadata->section_sizes[section] == 0) {
            continue;
        }

        void *dst_start = (void *)(DRAM_BASE_PHYSICAL + (uint64_t)metadata->section_offsets[section]);
        void *src_start = load_buf + sizeof(package2_header_t) + cur_section_offset;
        size_t size = (size_t)metadata->section_sizes[section];

        if (bootconfig_is_package2_plaintext() && size != 0) {
            memcpy(dst_start, src_start, size);
        } else if (size != 0) {
            package2_crypt_ctr(master_key_rev, dst_start, size, src_start, size, metadata->section_ctrs[section], 0x10);
        }
        cur_section_offset += size;
    }

    /* Clear the encrypted package2 from memory. */
    memset(load_buf, 0, PACKAGE2_SIZE_MAX);
}

static void copy_warmboot_bin_to_dram() {
    uint8_t *warmboot_src;
    switch (exosphere_get_target_firmware()) {
        case ATMOSPHERE_TARGET_FIRMWARE_100:
        case ATMOSPHERE_TARGET_FIRMWARE_200:
        case ATMOSPHERE_TARGET_FIRMWARE_300:
        default:
            generic_panic();
            break;
        case ATMOSPHERE_TARGET_FIRMWARE_400:
        case ATMOSPHERE_TARGET_FIRMWARE_500:
            warmboot_src = (uint8_t *)0x4003B000;
            break;
        case ATMOSPHERE_TARGET_FIRMWARE_600:
        case ATMOSPHERE_TARGET_FIRMWARE_620:
            warmboot_src = (uint8_t *)0x4003D800;
            break;
        case ATMOSPHERE_TARGET_FIRMWARE_700:
        case ATMOSPHERE_TARGET_FIRMWARE_800:
            warmboot_src = (uint8_t *)0x4003E000;
            break;
    }
    uint8_t *warmboot_dst = (uint8_t *)0x8000D000;
    const size_t warmboot_size = 0x2000;
    
    /* Flush cache, to ensure warmboot is where we need it to be. */
    flush_dcache_range(warmboot_src, warmboot_src + warmboot_size);
    __dsb_sy();
    
    /* Copy warmboot. */
    for (size_t i = 0; i < warmboot_size; i += sizeof(uint32_t)) {
        write32le(warmboot_dst, i, read32le(warmboot_src, i));
    }
    
    /* Flush cache, to ensure warmboot is where we need it to be. */
    flush_dcache_range(warmboot_dst, warmboot_dst + warmboot_size);
    __dsb_sy();
}

static void sync_with_nx_bootloader(int state) {
    while (MAILBOX_NX_BOOTLOADER_SETUP_STATE(exosphere_get_target_firmware()) < state) {
        wait(100);
    }
}

static void identity_unmap_dram(void) {
    uintptr_t *mmu_l1_tbl = (uintptr_t *)(TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGEMENT_ID_SECMON_EVT) + 0x800 - 64);

    mmu_unmap_range(1, mmu_l1_tbl, IDENTITY_GET_MAPPING_ADDRESS(IDENTITY_MAPPING_DRAM), IDENTITY_GET_MAPPING_SIZE(IDENTITY_MAPPING_DRAM));
    tlb_invalidate_all_inner_shareable();
}

uintptr_t get_pk2ldr_stack_address(void) {
    return TZRAM_GET_SEGMENT_ADDRESS(TZRAM_SEGMENT_ID_PK2LDR) + 0x2000;
}

/* This function is called during coldboot init, and validates a package2. */
/* This package2 is read into memory by a concurrent BPMP bootloader. */
void load_package2(coldboot_crt0_reloc_list_t *reloc_list) {
    /* Load Exosphere-specific config. */
    exosphere_load_config();
    configitem_set_debugmode_override(exosphere_should_override_debugmode_user() != 0, exosphere_should_override_debugmode_priv() != 0);

    /* Setup the Security Engine. */
    setup_se();
    
    /* Perform initial PMC register writes, if relevant. */
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_400) {
        MAKE_REG32(PMC_BASE + 0x054) = 0x8000D000; 
        MAKE_REG32(PMC_BASE + 0x0A0) &= 0xFFF3FFFF; 
        MAKE_REG32(PMC_BASE + 0x818) &= 0xFFFFFFFE; 
        MAKE_REG32(PMC_BASE + 0x334) |= 0x10;
        switch (exosphere_get_target_firmware()) {
            case ATMOSPHERE_TARGET_FIRMWARE_400:
                MAKE_REG32(PMC_BASE + 0x360) = 0x105;
                break;
            case ATMOSPHERE_TARGET_FIRMWARE_500:
                MAKE_REG32(PMC_BASE + 0x360) = 6;
                break;
            case ATMOSPHERE_TARGET_FIRMWARE_600:
                MAKE_REG32(PMC_BASE + 0x360) = 0x87;
                break;
            case ATMOSPHERE_TARGET_FIRMWARE_620:
                MAKE_REG32(PMC_BASE + 0x360) = 0xA8;
                break;
            case ATMOSPHERE_TARGET_FIRMWARE_700:
            case ATMOSPHERE_TARGET_FIRMWARE_800:
                MAKE_REG32(PMC_BASE + 0x360) = 0x129;
                break;
        }
    }

    wait(1000);

    bootup_misc_mmio();

    setup_current_core_state();

    /* Save boot reason to global. */
    bootconfig_load_boot_reason((volatile boot_reason_t *)(MAILBOX_NX_BOOTLOADER_BOOT_REASON(exosphere_get_target_firmware())));

    /* Initialize cache'd random bytes for kernel. */
    randomcache_init();

    /* memclear the initial copy of Exosphere running in IRAM (relocated to TZRAM by earlier code). */
    /* memset((void *)reloc_list->reloc_base, 0, reloc_list->loaded_bin_size); */
    
    /* Let NX Bootloader know that we're running. */
    MAILBOX_NX_BOOTLOADER_IS_SECMON_AWAKE(exosphere_get_target_firmware()) = 1;

    /* Wait for 1 second, to allow time for NX_BOOTLOADER to draw to the screen. This is useful for debugging. */
    /* wait(1000000); */

    /* Synchronize with NX BOOTLOADER. */
    sync_with_nx_bootloader(NX_BOOTLOADER_STATE_MOVED_BOOTCONFIG);

    /* Load Boot Config into global. */
    setup_boot_config();
    
    /* Set sysctr0 registers based on bootconfig. */
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_400) {
        uint64_t sysctr0_val = bootconfig_get_value_for_sysctr0();
        MAKE_SYSCTR0_REG(0x8) = (uint32_t)((sysctr0_val >> 0) & 0xFFFFFFFFULL);
        MAKE_SYSCTR0_REG(0xC) = (uint32_t)((sysctr0_val >> 32) & 0xFFFFFFFFULL);
        MAKE_SYSCTR0_REG(0x0) = 3;
    }

    /* Synchronize with NX BOOTLOADER. */
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_400) {
        sync_with_nx_bootloader(NX_BOOTLOADER_STATE_DRAM_INITIALIZED_4X);
        copy_warmboot_bin_to_dram();
        if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_600) {
            setup_dram_magic_numbers();
        }
        sync_with_nx_bootloader(NX_BOOTLOADER_STATE_LOADED_PACKAGE2_4X);
    } else {
        sync_with_nx_bootloader(NX_BOOTLOADER_STATE_LOADED_PACKAGE2);
    }

    /* Make PMC (2.x+), MC (4.x+) registers secure-only */
    secure_additional_devices();
    
    /* Remove the identity mapping for iRAM-C+D & TZRAM */
    /* For our crt0 to work, this doesn't actually unmap TZRAM */
    identity_unmap_iram_cd_tzram();

    /* Load header from NX_BOOTLOADER-initialized DRAM. */
    package2_header_t header;
    flush_dcache_range((uint8_t *)NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS, (uint8_t *)NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS + sizeof(header));
    memcpy(&header, NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS, sizeof(header));
    flush_dcache_range((uint8_t *)&header, (uint8_t *)&header + sizeof(header));
    
    /* Perform signature checks. */
    /* Special exosphere patching enable: All-zeroes signature + decrypted header implies unsigned and decrypted package2. */
    if (header.signature[0] == 0 && memcmp(header.signature, header.signature + 1, sizeof(header.signature) - 1) == 0 && header.metadata.magic == MAGIC_PK21) {
        bootconfig_set_package2_plaintext_and_unsigned();
    }

    verify_header_signature(&header);

    /* Decrypt header, get key revision required. */
    uint32_t package2_mkey_rev = decrypt_and_validate_header(&header);
    
    /* Copy hash, if necessary. */
    if (bootconfig_is_recovery_boot()) {
        bootconfig_set_package2_hash_for_recovery(NX_BOOTLOADER_PACKAGE2_LOAD_ADDRESS, get_package2_size(&header.metadata));
    }

    /* Load Package2 Sections. */
    load_package2_sections(&header.metadata, package2_mkey_rev);
    
    /* Clean up cache. */
    flush_dcache_all();
    invalidate_icache_all(); /* non-broadcasting */

    /* Set CORE0 entrypoint for Package2. */
    set_core_entrypoint_and_argument(0, DRAM_BASE_PHYSICAL + header.metadata.entrypoint, 0);

    /* Remove the DRAM identity mapping. */
    if (0) {
        identity_unmap_dram();
    } 

    /* Synchronize with NX BOOTLOADER. */
    if (exosphere_get_target_firmware() >= ATMOSPHERE_TARGET_FIRMWARE_400) {
        sync_with_nx_bootloader(NX_BOOTLOADER_STATE_FINISHED_4X);
        setup_4x_mmio();
    } else {
        sync_with_nx_bootloader(NX_BOOTLOADER_STATE_FINISHED);
    }

    /* Prepare the SMC API with version-dependent SMCs. */
    set_version_specific_smcs();

    /* Update SCR_EL3 depending on value in Bootconfig. */
    set_extabt_serror_taken_to_el3(bootconfig_take_extabt_serror_to_el3());
}
