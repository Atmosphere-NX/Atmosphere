#include <stdint.h>

#include "utils.h"
#include "cache.h"
#include "smc_api.h"
#include "smc_user.h"
#include "se.h"
#include "userpage.h"
#include "titlekey.h"

/* Globals. */
int g_crypt_aes_done = 0;
int g_exp_mod_done = 0;

uint8_t g_rsa_oaep_exponent[0x100];
uint8_t g_rsa_private_exponent[0x100];


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
    set_aes_keyslot(9, sealed_kek, 0x10);
    
    /* Unwrap the key. */
    decrypt_data_into_keyslot(keyslot, 9, wrapped_key, 0x10);
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

uint32_t user_rsa_oaep(smc_args_t *args) {
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
    set_rsa_keyslot(0, modulus, 0x100, g_rsa_oaep_exponent, 0x100);
    se_exp_mod(0, input, 0x100, exp_mod_done_handler);
    
    return 0;
}

uint32_t user_unwrap_rsa_wrapped_titlekey(smc_args_t *args) {
    uint8_t modulus[0x100];
    uint8_t wrapped_key[0x100];
    
    upage_ref_t page_ref;
            
    void *user_wrapped_key = (void *)args->X[1];
    void *user_modulus = (void *)args->X[2];
    unsigned int master_key_rev = (unsigned int)args->X[7];
    
    /* TODO: Validate Master Key Revision. */

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
    
    /* Expected salt occupies args->X[3] to args->X[6]. */
    tkey_set_expected_salt(&args->X[3]);
    
    tkey_set_master_key_rev(master_key_rev);
    
    /* Hardcode RSA keyslot 0. */
    set_rsa_keyslot(0, modulus, 0x100, g_rsa_private_exponent, 0x100);
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
    tkey_unseal(keyslot, sealed_titlekey, 0x10);
    return 0;

}

uint32_t user_unwrap_aes_wrapped_titlekey(smc_args_t *args) {
    uint64_t aes_wrapped_titlekey[2];
    uint8_t titlekey[0x10];
    uint64_t sealed_titlekey[2];
    
    aes_wrapped_titlekey[0] = args->X[1];
    aes_wrapped_titlekey[1] = args->X[2];
    unsigned int master_key_rev = (unsigned int)args->X[3];
    
    
    /* TODO: Validate Master Key Revision. */
    tkey_set_master_key_rev(master_key_rev);
    
    
    tkey_aes_unwrap(titlekey, 0x10, aes_wrapped_titlekey, 0x10);
    tkey_seal(sealed_titlekey, 0x10, titlekey, 0x10);
    
    args->X[1] = sealed_titlekey[0];
    args->X[2] = sealed_titlekey[1];

}