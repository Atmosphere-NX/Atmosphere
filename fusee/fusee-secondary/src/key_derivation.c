#include "key_derivation.h"
#include "masterkey.h"
#include "se.h"
#include "exocfg.h"
#include "fuse.h"

static const u8 keyblob_seeds[MASTERKEY_REVISION_MAX][0x10] =
{
    {0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3}, /* Keyblob seed 00. */
    {0x0C, 0x25, 0x61, 0x5D, 0x68, 0x4C, 0xEB, 0x42, 0x1C, 0x23, 0x79, 0xEA, 0x82, 0x25, 0x12, 0xAC}, /* Keyblob seed 01. */
    {0x33, 0x76, 0x85, 0xEE, 0x88, 0x4A, 0xAE, 0x0A, 0xC2, 0x8A, 0xFD, 0x7D, 0x63, 0xC0, 0x43, 0x3B}, /* Keyblob seed 02. */
    {0x2D, 0x1F, 0x48, 0x80, 0xED, 0xEC, 0xED, 0x3E, 0x3C, 0xF2, 0x48, 0xB5, 0x65, 0x7D, 0xF7, 0xBE}, /* Keyblob seed 03. */
    {0xBB, 0x5A, 0x01, 0xF9, 0x88, 0xAF, 0xF5, 0xFC, 0x6C, 0xFF, 0x07, 0x9E, 0x13, 0x3C, 0x39, 0x80}, /* Keyblob seed 04. */
};

static const u8 keyblob_mac_seed[0x10] = 
{
    0x59, 0xC7, 0xFB, 0x6F, 0xBE, 0x9B, 0xBE, 0x87, 0x65, 0x6B, 0x15, 0xC0, 0x53, 0x73, 0x36, 0xA5
};

static const uint8_t masterkey_seed[0x10] = 
{
    0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C
};

static const uint8_t devicekey_seed[0x10] = 
{
    0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78
};

static const uint8_t devicekey_4x_seed[0x10] = 
{
    0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28
};

static const uint8_t masterkey_4x_seed[0x10] = 
{
    0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66
};

void get_tsec_key(void *dst) {
    /* TODO: Implement this method. Attempt to read TSEC fw from NAND, or from SD if that fails. */
}

void get_keyblob(void *dst, u32 revision) {
    if (revision >= 0x20) {
        generic_panic();
    }
    
    /* TODO: Read the appropriate keyblob from eMMC Boot0 partition. */
}

bool safe_memcmp(u8 *a, u8 *b, u32 sz) {
    u8 different = 0;
    for (u32 i = 0; i < sz; i++) {
        different |= a[i] ^ b[i];
    }
    return different != 0;
}

/* Derive all Switch keys. */
void derive_nx_keydata(u32 target_firmware) {
    u8 work_buffer[0x10];
    nx_keyblob_t keyblob;
    
    /* TODO: Set keyslot flags properly in preparation of derivation. */
    set_aes_keyslot_flags(0xE, 0x15);
    set_aes_keyslot_flags(0xD, 0x15);
    
    /* Set TSEC key. */
    get_tsec_key(work_buffer);
    set_aes_keyslot(0xD, work_buffer, 0x10);
    
    
    /* Get keyblob, always try to set up the highest possible master key. */
    /* TODO: Should we iterate, trying lower keys on failure? */
    get_keyblob(&keyblob, MASTERKEY_REVISION_500_CURRENT);
    
    /* Derive both keyblob key 1, and keyblob key latest. */
    se_aes_ecb_decrypt_block(0xD, work_buffer, 0x10, keyblob_seeds[MASTERKEY_REVISION_100_230], 0x10);
    decrypt_data_into_keyslot(0xF, 0xE, work_buffer, 0x10);
    se_aes_ecb_decrypt_block(0xD, work_buffer, 0x10, keyblob_seeds[MASTERKEY_REVISION_500_CURRENT], 0x10);
    decrypt_data_into_keyslot(0xD, 0xE, work_buffer, 0x10);
    
    /* Clear the SBK. */
    clear_aes_keyslot(0xE);
    se_aes_ecb_decrypt_block(0xD, work_buffer, 0x10, keyblob_mac_seed, 0x10);
    decrypt_data_into_keyslot(0xB, 0xD, keyblob_mac_seed, 0x10);
    
    /* Validate keyblob. */
    se_compute_aes_128_cmac(0xB, work_buffer, 0x10, keyblob.mac + sizeof(keyblob.mac), sizeof(keyblob) - sizeof(keyblob.mac));
    if (safe_memcmp(keyblob.mac, work_buffer, 0x10)) {
        generic_panic();
    }
    
    /* Decrypt keyblob. */
    se_aes_ctr_crypt(0xD, keyblob.data, sizeof(keyblob.data), keyblob.data, sizeof(keyblob.data), keyblob.ctr, sizeof(keyblob.ctr));
    
    /* Get needed data. */
    set_aes_keyslot(0xC, keyblob.keys[0], 0x10);
    /* We don't need the Package1 Key, but for reference: set_aes_keyslot(0xB, keyblob.keys[8], 0x10); */
    
    /* Clear keyblob. */
    memset(keyblob.data, 0, sizeof(keyblob.data));    
    
    /* Derive keys for Exosphere, lock critical keyslots. */
    switch (target_firmware) {
        case EXOSPHERE_TARGET_FIRMWARE_100:
        case EXOSPHERE_TARGET_FIRMWARE_200:
        case EXOSPHERE_TARGET_FIRMWARE_300: 
            decrypt_data_into_keyslot(0xD, 0xF, devicekey_seed, 0x10);
            decrypt_data_into_keyslot(0xC, 0xC, masterkey_seed, 0x10);
            break;
        case EXOSPHERE_TARGET_FIRMWARE_400: 
            decrypt_data_into_keyslot(0xD, 0xF, devicekey_4x_seed, 0x10);
            decrypt_data_into_keyslot(0xF, 0xF, devicekey_seed, 0x10);
            decrypt_data_into_keyslot(0xE, 0xC, masterkey_4x_seed, 0x10);
            decrypt_data_into_keyslot(0xC, 0xC, masterkey_seed, 0x10);
            break;
        case EXOSPHERE_TARGET_FIRMWARE_500: 
            decrypt_data_into_keyslot(0xA, 0xF, devicekey_4x_seed, 0x10);
            decrypt_data_into_keyslot(0xF, 0xF, devicekey_seed, 0x10);
            decrypt_data_into_keyslot(0xE, 0xC, masterkey_4x_seed, 0x10);
            decrypt_data_into_keyslot(0xC, 0xC, masterkey_seed, 0x10);
            break;
        default:
            generic_panic();
    }
    
    /* Setup master key revision, derive older master keys for use. */
    mkey_detect_revision();
}

/* Sets final keyslot flags, for handover to TZ/Exosphere. Setting these will prevent the BPMP from using the device key or master key. */
void finalize_nx_keydata(u32 target_firmware) {
    set_aes_keyslot_flags(0xC, 0xFF);
    set_aes_keyslot_flags((target_firmware >= EXOSPHERE_TARGET_FIRMWARE_400) ? (KEYSLOT_SWITCH_4XOLDDEVICEKEY) : (KEYSLOT_SWITCH_DEVICEKEY), 0xFF);
}

void fusee_generate_specific_aes_key(void *dst, const void *wrapped_key, bool should_mask, u32 target_firmware) {
    unsigned int keyslot = (target_firmware >= EXOSPHERE_TARGET_FIRMWARE_400) ? (KEYSLOT_SWITCH_4XOLDDEVICEKEY) : (KEYSLOT_SWITCH_DEVICEKEY);
    if (fuse_get_bootrom_patch_version() < 0x7F) {
        /* On dev units, use a fixed "all-zeroes" seed. */
        /* Yes, this data really is all-zero in actual TrustZone .rodata. */
        static const u8 dev_specific_aes_key_source[0x10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        static const u8 dev_specific_aes_key_ctr[0x10] = {0x3C, 0xD5, 0x92, 0xEC, 0x68, 0x31, 0x4A, 0x06, 0xD4, 0x1B, 0x0C, 0xD9, 0xF6, 0x2E, 0xD9, 0xE9};
        static const u8 dev_specific_aes_key_mask[0x10] = {0xAC, 0xCA, 0x9A, 0xCA, 0xFF, 0x2E, 0xB9, 0x22, 0xCC, 0x1F, 0x4F, 0xAD, 0xDD, 0x77, 0x21, 0x1E};

        se_aes_ctr_crypt(keyslot, dst, 0x10, dev_specific_aes_key_source, 0x10, dev_specific_aes_key_ctr, 0x10);

        if (should_mask) {
            for (unsigned int i = 0; i < 0x10; i++) {
                ((u8 *)dst)[i] ^= dev_specific_aes_key_mask[i];
            }
        }
    } else {
        /* On retail, standard kek->key decryption. */
        static const u8 retail_specific_aes_key_source[0x10] = {0xE2, 0xD6, 0xB8, 0x7A, 0x11, 0x9C, 0xB8, 0x80, 0xE8, 0x22, 0x88, 0x8A, 0x46, 0xFB, 0xA1, 0x95};
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, keyslot, retail_specific_aes_key_source, 0x10);
        se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_TEMPKEY, dst, 0x10, wrapped_key, 0x10);
    }
}

void fusee_generate_personalized_aes_key_for_bis(void *dst, const void *wrapped_kek, const void *wrapped_key, u32 target_firmware) {
    static const u8 kek_source[0x10] = {
        0x4D, 0x87, 0x09, 0x86, 0xC4, 0x5D, 0x20, 0x72, 0x2F, 0xBA, 0x10, 0x53, 0xDA, 0x92, 0xE8, 0xA9
    };
    static const u8 key_source[0x10] = {
        0x89, 0x61, 0x5E, 0xE0, 0x5C, 0x31, 0xB6, 0x80, 0x5F, 0xE5, 0x8F, 0x3D, 0xA2, 0x4F, 0x7A, 0xA8
    };

    unsigned int keyslot = (target_firmware >= EXOSPHERE_TARGET_FIRMWARE_400) ? (KEYSLOT_SWITCH_4XOLDDEVICEKEY) : (KEYSLOT_SWITCH_DEVICEKEY);
    /* Derive kek. */
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, keyslot, kek_source, 0x10);
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, KEYSLOT_SWITCH_TEMPKEY, wrapped_kek, 0x10);
    /* Derive key. */
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, KEYSLOT_SWITCH_TEMPKEY, key_source, 0x10);
    se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_TEMPKEY, dst, 0x10, wrapped_key, 0x10);
}

void derive_bis_key(void *dst, BisPartition_t partition_id, u32 target_firmware) {
    static const u8 key_source_for_bis[3][2][0x10] = {
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
    
    static const u8 bis_kek_source[0x10] = {0x34, 0xC1, 0xA0, 0xC4, 0x82, 0x58, 0xF8, 0xB4, 0xFA, 0x9E, 0x5E, 0x6A, 0xDA, 0xFC, 0x7E, 0x4F};
    switch (partition_id) {
        case BisPartition_Calibration:
            fusee_generate_specific_aes_key(dst, key_source_for_bis[partition_id][0], false, target_firmware);
            fusee_generate_specific_aes_key(dst + 0x10, key_source_for_bis[partition_id][1], false, target_firmware);
            break;
        case BisPartition_Safe:
        case BisPartition_UserSystem:
            fusee_generate_personalized_aes_key_for_bis(dst, bis_kek_source, key_source_for_bis[partition_id][0], target_firmware);
            fusee_generate_personalized_aes_key_for_bis(dst + 0x10, bis_kek_source, key_source_for_bis[partition_id][1], target_firmware);
            break;
        default:
            generic_panic();
    }
}