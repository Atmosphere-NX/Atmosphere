#include <stdint.h>
#include <string.h>

#include "utils.h"
#include "cache.h"
#include "configitem.h"
#include "masterkey.h"
#include "smc_api.h"
#include "smc_user.h"
#include "se.h"
#include "sealedkeys.h"
#include "userpage.h"
#include "titlekey.h"

/* Globals. */
int g_crypt_aes_done = 0;
int g_exp_mod_done = 0;

uint8_t g_secure_exp_mod_exponent[0x100];
uint8_t g_rsa_oaep_exponent[0x100];


void set_exp_mod_done(int done) {
    g_exp_mod_done = done & 1;
}

int get_exp_mod_done(void) {
    return g_exp_mod_done;
}

uint32_t exp_mod_done_handler(void) {  
    set_exp_mod_done(1);
    
    se_trigger_interrupt();
    
    return 0;
}

uint32_t user_exp_mod(smc_args_t *args) {
    uint8_t modulus[0x100];
    uint8_t exponent[0x100];
    uint8_t input[0x100];
    
    upage_ref_t page_ref;
    
    /* Validate size. */
    if (args->X[4] == 0 || args->X[4] > 0x100 || (args->X[4] & 3) != 0) {
        return 2;
    }
    
    size_t exponent_size = (size_t)args->X[4];
    
    void *user_input = (void *)args->X[1];
    void *user_exponent = (void *)args->X[2];
    void *user_modulus = (void *)args->X[3];

    /* Copy user data into secure memory. */
    if (upage_init(&page_ref, user_input) == 0) {
        return 2;
    }
    if (user_copy_to_secure(&page_ref, input, user_input, 0x100) == 0) {
        return 2;
    }
    if (user_copy_to_secure(&page_ref, exponent, user_exponent, exponent_size) == 0) {
        return 2;
    }    
    if (user_copy_to_secure(&page_ref, modulus, user_modulus, 0x100) == 0) {
        return 2;
    }
    
    set_exp_mod_done(0);
    /* Hardcode RSA keyslot 0. */
    set_rsa_keyslot(0, modulus, 0x100, exponent, exponent_size);
    se_exp_mod(0, input, 0x100, exp_mod_done_handler);
    
    return 0;
}

uint32_t user_get_random_bytes(smc_args_t *args) {
    uint8_t random_bytes[0x40];
    if (args->X[1] > 0x38) {
        return 2;
    }
    
    size_t size = (size_t)args->X[1];
    
    flush_dcache_range(random_bytes, random_bytes + size);
    se_generate_random(KEYSLOT_SWITCH_RNGKEY, random_bytes, size);
    flush_dcache_range(random_bytes, random_bytes + size);
    
    memcpy(&args->X[1], random_bytes, size);
    return 0;
}

uint32_t user_generate_aes_kek(smc_args_t *args) {
    uint64_t wrapped_kek[2];
    uint8_t kek_source[0x10];
    uint64_t kek[2];
    uint64_t sealed_kek[2];
    
    wrapped_kek[0] = args->X[1];
    wrapped_kek[1] = args->X[2];
    
    unsigned int master_key_rev = (unsigned int)args->X[3];
    
    if (master_key_rev > 0) {
        master_key_rev -= 1; /* GenerateAesKek offsets by one. */
    }
    
    if (master_key_rev >= MASTERKEY_REVISION_MAX) {
        return 2;
    }
    
    uint64_t packed_options = args->X[4];
    if (packed_options > 0xFF) {
        return 2;
    }
    
    /* Switched the output based on how the system was booted. */
    uint8_t mask_id = (uint8_t)((packed_options >> 1) & 3);
    
    /* Switches the output based on how it will be used. */
    uint8_t usecase = (uint8_t)((packed_options >> 5) & 3);
    
    /* Switched the output based on whether it should be console unique. */
    int is_personalized = (int)(packed_options & 1);
    
    uint64_t is_recovery_boot = configitem_is_recovery_boot();
    
    /* Mask 2 is only allowed when booted from recovery. */
    if (mask_id == 2 && is_recovery_boot == 0) {
        return 2;
    }
    /* Mask 1 is only allowed when booted normally. */
    if (mask_id == 1 && is_recovery_boot != 0) {
        return 2;
    }
    
    /* Masks 0, 3 are allowed all the time. */
    
    const uint8_t kek_seeds[4][0x10] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0xA2, 0xAB, 0xBF, 0x9C, 0x92, 0x2F, 0xBB, 0xE3, 0x78, 0x79, 0x9B, 0xC0, 0xCC, 0xEA, 0xA5, 0x74},
        {0x57, 0xE2, 0xD9, 0x45, 0xE4, 0x92, 0xF4, 0xFD, 0xC3, 0xF9, 0x86, 0x38, 0x89, 0x78, 0x9F, 0x3C},
        {0xE5, 0x4D, 0x9A, 0x02, 0xF0, 0x4F, 0x5F, 0xA8, 0xAD, 0x76, 0x0A, 0xF6, 0x32, 0x95, 0x59, 0xBB}
    };
    const uint8_t kek_masks[4][0x10] = {
        {0x4D, 0x87, 0x09, 0x86, 0xC4, 0x5D, 0x20, 0x72, 0x2F, 0xBA, 0x10, 0x53, 0xDA, 0x92, 0xE8, 0xA9},
        {0x25, 0x03, 0x31, 0xFB, 0x25, 0x26, 0x0B, 0x79, 0x8C, 0x80, 0xD2, 0x69, 0x98, 0xE2, 0x22, 0x77},
        {0x76, 0x14, 0x1D, 0x34, 0x93, 0x2D, 0xE1, 0x84, 0x24, 0x7B, 0x66, 0x65, 0x55, 0x04, 0x65, 0x81},
        {0xAF, 0x3D, 0xB7, 0xF3, 0x08, 0xA2, 0xD8, 0xA2, 0x08, 0xCA, 0x18, 0xA8, 0x69, 0x46, 0xC9, 0x0B}
    };
    
    /* Create kek source. */
    for (unsigned int i = 0; i < 0x10; i++) {
        kek_source[i] = kek_seeds[usecase][i] ^ kek_masks[mask_id][i];
    }
    
    unsigned int keyslot;
    if (is_personalized) {
        /* Behavior changed in 4.0.0. */
        if (mkey_get_revision() >= MASTERKEY_REVISION_400_CURRENT) {
            if (master_key_rev >= 1) {
                keyslot = KEYSLOT_SWITCH_DEVICEKEY; /* New device key, 4.x. */
            } else {
                keyslot = KEYSLOT_SWITCH_4XOLDDEVICEKEY; /* Old device key, 4.x. */
            }
        } else {
            keyslot = KEYSLOT_SWITCH_DEVICEKEY;
        }
    } else {
        keyslot = mkey_get_keyslot(master_key_rev);
    }
    
    /* Derive kek. */
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, keyslot, kek_source, 0x10);
    se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_TEMPKEY, kek, 0x10, wrapped_kek, 0x10);
    
    
    /* Seal kek. */
    seal_key(sealed_kek, 0x10, kek, 0x10, usecase);
    
    args->X[1] = sealed_kek[0];
    args->X[2] = sealed_kek[1];
    
    return 0;
}

uint32_t user_load_aes_key(smc_args_t *args) {
    uint64_t sealed_kek[2];
    uint64_t wrapped_key[2];
    
    uint32_t keyslot = (uint32_t)args->X[1];
    if (keyslot > 3) {
        return 2;
    }
    
    /* Copy keydata */
    sealed_kek[0] = args->X[2];
    sealed_kek[1] = args->X[3];
    wrapped_key[0] = args->X[4];
    wrapped_key[1] = args->X[5];
    
    /* TODO: Unseal the kek. */
    unseal_key(KEYSLOT_SWITCH_TEMPKEY, sealed_kek, 0x10, CRYPTOUSECASE_AES);
    
    /* Unwrap the key. */
    decrypt_data_into_keyslot(keyslot, KEYSLOT_SWITCH_TEMPKEY, wrapped_key, 0x10);
    return 0;
}


void set_crypt_aes_done(int done) {
    g_crypt_aes_done = done & 1;
}

int get_crypt_aes_done(void) {
    return g_crypt_aes_done;
}

uint32_t crypt_aes_done_handler(void) {
    se_check_for_error();
    
    set_crypt_aes_done(1);
    
    se_trigger_interrupt();
    
    return 0;
}

uint32_t user_crypt_aes(smc_args_t *args) {
    uint32_t keyslot = args->X[1] & 3;
    uint32_t mode = (args->X[1] >> 4) & 3;
    
    uint64_t iv_ctr[2];
    iv_ctr[0] = args->X[2];
    iv_ctr[1] = args->X[3];
    
    uint32_t in_ll_paddr = (uint32_t)(args->X[4]);
    uint32_t out_ll_paddr = (uint32_t)(args->X[5]);
    
    size_t size = args->X[6];
    if (size & 0xF) {
        panic();
    }
    
    set_crypt_aes_done(0);

    uint64_t result = 0;
    switch (mode) {
        case 0: /* CBC Encryption */
            se_aes_cbc_encrypt_insecure(keyslot, out_ll_paddr, in_ll_paddr, size, iv_ctr, crypt_aes_done_handler);
            result = 0;
            break;
        case 1: /* CBC Decryption */
            se_aes_cbc_decrypt_insecure(keyslot, out_ll_paddr, in_ll_paddr, size, iv_ctr, crypt_aes_done_handler);
            result = 0;
            break;
        case 2: /* CTR "Encryption" */
            se_aes_ctr_crypt_insecure(keyslot, out_ll_paddr, in_ll_paddr, size, iv_ctr, crypt_aes_done_handler);
            result = 0;
            break;
        case 3:
        default:
            result = 1;
            break;
    }
    
    return result;
}

uint32_t user_generate_specific_aes_key(smc_args_t *args) {
    uint64_t wrapped_key[2];
    uint8_t key[0x10];
    unsigned int master_key_rev;
    int should_mask;
    
    wrapped_key[0] = args->X[1];
    wrapped_key[1] = args->X[2];
    if (args->X[4] > MASTERKEY_REVISION_MAX) {
        return 2;
    }
    master_key_rev = (unsigned int)(args->X[4]);
    if (args->X[3] > 1) {
        return 2;
    }
    should_mask = (int)(args->X[3]);
    
    unsigned int keyslot;
    
    /* Behavior changed in 4.0.0. */
    if (mkey_get_revision() >= MASTERKEY_REVISION_400_CURRENT) {
        if (master_key_rev >= 2) {
            keyslot = KEYSLOT_SWITCH_DEVICEKEY; /* New device key, 4.x. */
        } else {
            keyslot = KEYSLOT_SWITCH_4XOLDDEVICEKEY; /* Old device key, 4.x. */
        }
    } else {
        keyslot = KEYSLOT_SWITCH_DEVICEKEY;
    }
    
    if (0 /* TODO: GET_BOOTROM_PATCH_VERSION < 0x7F */) {
        /* On dev units, use a fixed "all-zeroes" seed. */
        /* Yes, this data really is all-zero in actual TrustZone .rodata. */
        uint8_t dev_specific_aes_key_source[0x10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        uint8_t dev_specific_aes_key_ctr[0x10] = {0x3C, 0xD5, 0x92, 0xEC, 0x68, 0x31, 0x4A, 0x06, 0xD4, 0x1B, 0x0C, 0xD9, 0xF6, 0x2E, 0xD9, 0xE9};
        uint8_t dev_specific_aes_key_mask[0x10] = {0xAC, 0xCA, 0x9A, 0xCA, 0xFF, 0x2E, 0xB9, 0x22, 0xCC, 0x1F, 0x4F, 0xAD, 0xDD, 0x77, 0x21, 0x1E};
        
        cache_flush(key, key + 0x10);
        se_aes_ctr_crypt(keyslot, key, 0x10, dev_specific_aes_key_source, 0x10, dev_specific_aes_key_ctr, 0x10);
        cache_flush(key, key + 0x10);
        
        if (should_mask) {
            for (unsigned int i = 0; i < 0x10; i++) {
                key[i] ^= dev_specific_aes_key_mask[i];
            }
        }
    } else {
        /* On retail, standard kek->key decryption. */
        uint8_t retail_specific_aes_key_source[0x10] = {0xE2, 0xD6, 0xB8, 0x7A, 0x11, 0x9C, 0xB8, 0x80, 0xE8, 0x22, 0x88, 0x8A, 0x46, 0xFB, 0xA1, 0x95};
        decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, keyslot, retail_specific_aes_key_source, 0x10);
        se_aes_ecb_decrypt_block(KEYSLOT_SWITCH_TEMPKEY, key, 0x10, wrapped_key, 0x10);
    }
    
    
    args->X[1] = key[0];
    args->X[2] = key[1];
    return 0;
}

uint32_t user_compute_cmac(smc_args_t *args) {
    uint32_t keyslot = (uint32_t)args->X[1];
    void *user_address = (void *)args->X[2];
    size_t size = (size_t)args->X[3];
        
    uint8_t user_data[0x400];
    uint64_t result_cmac[2];
    upage_ref_t page_ref;
    
    /* Validate keyslot and size. */
    if (keyslot > 3 || args->X[3] > 0x400) {
        return 2;
    }
    
    if (upage_init(&page_ref, user_address) == 0 || user_copy_to_secure(&page_ref, user_data, user_address, size) == 0) {
        return 2;
    }
    
    flush_dcache_range(user_data, user_data + size);
    se_compute_aes_128_cmac(keyslot, result_cmac, 0x10, user_data, size);
    
    /* Copy CMAC out. */
    args->X[1] = result_cmac[0];
    args->X[2] = result_cmac[1];
    
    return 0;
}

uint32_t user_load_rsa_oaep_key(smc_args_t *args) {
    uint64_t sealed_kek[2];
    uint64_t wrapped_key[2];
    int is_personalized;
    
    uint8_t user_data[0x400];
    void *user_address;
    size_t size;
    upage_ref_t page_ref;

    
    /* Copy keydata */
    sealed_kek[0] = args->X[1];
    sealed_kek[1] = args->X[2];
    if (args->X[3] > 1) {
        return 2;
    }
    is_personalized = (int)args->X[3];
    user_address = (void *)args->X[4];
    size = (size_t)args->X[5];
    wrapped_key[0] = args->X[6];
    wrapped_key[1] = args->X[7];
    
    if (is_personalized && size != 0x240) {
        return 2;
    }
    if (!is_personalized && (size != 0x220 /* TODO: || GET_BOOTROM_PATCH_VERSION >= 0x7F */)) {
        return 2;
    }
    
    if (upage_init(&page_ref, user_address) == 0 || user_copy_to_secure(&page_ref, user_data, user_address, size) == 0) {
        return 2;
    }
    
    /* Ensure that our private key is 0x100 bytes. */
    if (gcm_decrypt_key(user_data, size, user_data, size, sealed_kek, 0x10, wrapped_key, 0x10, CRYPTOUSECASE_RSAOAEP, is_personalized) < 0x100) {
        return 2;
    }
    
    memcpy(g_rsa_oaep_exponent, user_data, 0x100);
    return 0;
}

uint32_t user_decrypt_rsa_private_key(smc_args_t *args) {
    uint64_t sealed_kek[2];
    uint64_t wrapped_key[2];
    int is_personalized;
    
    uint8_t user_data[0x400];
    void *user_address;
    size_t size;
    upage_ref_t page_ref;

    
    /* Copy keydata */
    sealed_kek[0] = args->X[1];
    sealed_kek[1] = args->X[2];
    if (args->X[3] > 1) {
        return 2;
    }
    is_personalized = (int)args->X[3];
    user_address = (void *)args->X[4];
    size = (size_t)args->X[5];
    wrapped_key[0] = args->X[6];
    wrapped_key[1] = args->X[7];
    
    if (size > 0x240) {
        return 2;
    }
    
    if (is_personalized && size < 0x31) {
        return 2;
    }
    if (!is_personalized && (size < 0x11 /* TODO: || GET_BOOTROM_PATCH_VERSION >= 0x7F */)) {
        return 2;
    }
    
    if (upage_init(&page_ref, user_address) == 0 || user_copy_to_secure(&page_ref, user_data, user_address, size) == 0) {
        return 2;
    }
    
    size_t out_size;
    
    if ((out_size = gcm_decrypt_key(user_data, size, user_data, size, sealed_kek, 0x10, wrapped_key, 0x10, CRYPTOUSECASE_RSAPRIVATE, is_personalized)) == 0) {
        return 2;
    }
    
    if (secure_copy_to_user(&page_ref, user_address, user_data, size) == 0) {
        return 2;
    }

    args->X[1] = out_size;
    return 0;
}

uint32_t user_load_secure_exp_mod_key(smc_args_t *args) {
    uint64_t sealed_kek[2];
    uint64_t wrapped_key[2];
    int is_personalized;
    
    uint8_t user_data[0x400];
    void *user_address;
    size_t size;
    upage_ref_t page_ref;

    
    /* Copy keydata */
    sealed_kek[0] = args->X[1];
    sealed_kek[1] = args->X[2];
    if (args->X[3] > 1) {
        return 2;
    }
    is_personalized = (int)args->X[3];
    user_address = (void *)args->X[4];
    size = (size_t)args->X[5];
    wrapped_key[0] = args->X[6];
    wrapped_key[1] = args->X[7];
    
    if (is_personalized && size != 0x130) {
        return 2;
    }
    if (!is_personalized && (size != 0x110 /* TODO: || GET_BOOTROM_PATCH_VERSION >= 0x7F */)) {
        return 2;
    }
    
    if (upage_init(&page_ref, user_address) == 0 || user_copy_to_secure(&page_ref, user_data, user_address, size) == 0) {
        return 2;
    }
    
    size_t out_size;
    
    /* Ensure that our key is non-zero bytes. */
    if ((out_size = gcm_decrypt_key(user_data, size, user_data, size, sealed_kek, 0x10, wrapped_key, 0x10, CRYPTOUSECASE_SECUREEXPMOD, is_personalized)) == 0) {
        return 2;
    }
    
    /* Copy key to global. */
    if (out_size <= 0x100) {
        memcpy(g_secure_exp_mod_exponent, user_data, out_size);
    } else {
        memcpy(g_secure_exp_mod_exponent, user_data, 0x100);
    }
    
    return 0;
}

uint32_t user_secure_exp_mod(smc_args_t *args) {
    uint8_t modulus[0x100];
    uint8_t input[0x100];
    
    upage_ref_t page_ref;
            
    void *user_input = (void *)args->X[1];
    void *user_modulus = (void *)args->X[2];

    /* Copy user data into secure memory. */
    if (upage_init(&page_ref, user_input) == 0) {
        return 2;
    }
    if (user_copy_to_secure(&page_ref, input, user_input, 0x100) == 0) {
        return 2;
    }
    if (user_copy_to_secure(&page_ref, modulus, user_modulus, 0x100) == 0) {
        return 2;
    }
    
    set_exp_mod_done(0);
    /* Hardcode RSA keyslot 0. */
    set_rsa_keyslot(0, modulus, 0x100, g_secure_exp_mod_exponent, 0x100);
    se_exp_mod(0, input, 0x100, exp_mod_done_handler);
    
    return 0;
}

uint32_t user_unwrap_rsa_oaep_wrapped_titlekey(smc_args_t *args) {
    uint8_t modulus[0x100];
    uint8_t wrapped_key[0x100];
    
    upage_ref_t page_ref;
            
    void *user_wrapped_key = (void *)args->X[1];
    void *user_modulus = (void *)args->X[2];
    unsigned int master_key_rev = (unsigned int)args->X[7];
    
    if (master_key_rev >= MASTERKEY_REVISION_MAX) {
        return 2;
    }

    /* Copy user data into secure memory. */
    if (upage_init(&page_ref, user_wrapped_key) == 0) {
        return 2;
    }
    if (user_copy_to_secure(&page_ref, wrapped_key, user_wrapped_key, 0x100) == 0) {
        return 2;
    }
    if (user_copy_to_secure(&page_ref, modulus, user_modulus, 0x100) == 0) {
        return 2;
    }
    
    set_exp_mod_done(0);
    
    /* Expected label_hash occupies args->X[3] to args->X[6]. */
    tkey_set_expected_label_hash(&args->X[3]);
    
    tkey_set_master_key_rev(master_key_rev);
    
    /* Hardcode RSA keyslot 0. */
    set_rsa_keyslot(0, modulus, 0x100, g_rsa_oaep_exponent, 0x100);
    se_exp_mod(0, wrapped_key, 0x100, exp_mod_done_handler);
    
    return 0;

}

uint32_t user_load_titlekey(smc_args_t *args) {
    uint64_t sealed_titlekey[2];
    
    uint32_t keyslot = (uint32_t)args->X[1];
    if (keyslot > 3) {
        return 2;
    }
    
    /* Copy keydata */
    sealed_titlekey[0] = args->X[2];
    sealed_titlekey[1] = args->X[3];
    
    /* Unseal the key. */
    unseal_titlekey(keyslot, sealed_titlekey, 0x10);
    return 0;

}

uint32_t user_unwrap_aes_wrapped_titlekey(smc_args_t *args) {
    uint64_t aes_wrapped_titlekey[2];
    uint8_t titlekey[0x10];
    uint64_t sealed_titlekey[2];
    
    aes_wrapped_titlekey[0] = args->X[1];
    aes_wrapped_titlekey[1] = args->X[2];
    unsigned int master_key_rev = (unsigned int)args->X[3];
    
    
    if (master_key_rev >= MASTERKEY_REVISION_MAX) {
        return 2;
    }
    
    tkey_set_master_key_rev(master_key_rev);
    
    
    tkey_aes_unwrap(titlekey, 0x10, aes_wrapped_titlekey, 0x10);
    seal_titlekey(sealed_titlekey, 0x10, titlekey, 0x10);
    
    args->X[1] = sealed_titlekey[0];
    args->X[2] = sealed_titlekey[1];
}
