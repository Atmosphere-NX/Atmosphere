#include <string.h>

#include "utils.h"
/*#include "interrupt.h"*/
#include "se.h"

void trigger_se_blocking_op(unsigned int op, void *dst, size_t dst_size, const void *src, size_t src_size);

/* Globals for driver. */
static unsigned int g_se_modulus_sizes[KEYSLOT_RSA_MAX];
static unsigned int g_se_exp_sizes[KEYSLOT_RSA_MAX];

/* Initialize a SE linked list. */
void NOINLINE ll_init(volatile se_ll_t *ll, void *buffer, size_t size) {
    ll->num_entries = 0; /* 1 Entry. */

    if (buffer != NULL) {
        ll->addr_info.address = (uint32_t) get_physical_address(buffer);
        ll->addr_info.size = (uint32_t) size;
    } else {
        ll->addr_info.address = 0;
        ll->addr_info.size = 0;
    }
}

void se_check_error_status_reg(void) {
    if (SECURITY_ENGINE->ERR_STATUS_REG) {
        generic_panic();
    }
}

void se_check_for_error(void) {
    if (SECURITY_ENGINE->INT_STATUS_REG & 0x10000 || SECURITY_ENGINE->FLAGS_REG & 3 || SECURITY_ENGINE->ERR_STATUS_REG) {
        generic_panic();
    }
}

void se_verify_flags_cleared(void) {
    if (SECURITY_ENGINE->FLAGS_REG & 3) {
        generic_panic();
    }
}

/* Set the flags for an AES keyslot. */
void set_aes_keyslot_flags(unsigned int keyslot, unsigned int flags) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    /* Misc flags. */
    if (flags & ~0x80) {
        SECURITY_ENGINE->AES_KEYSLOT_FLAGS[keyslot] = ~flags;
    }

    /* Disable keyslot reads. */
    if (flags & 0x80) {
        SECURITY_ENGINE->AES_KEY_READ_DISABLE_REG &= ~(1 << keyslot);
    }
}

/* Set the flags for an RSA keyslot. */
void set_rsa_keyslot_flags(unsigned int keyslot, unsigned int flags) {
    if (keyslot >= KEYSLOT_RSA_MAX) {
        generic_panic();
    }

    /* Misc flags. */
    if (flags & ~0x80) {
        /* TODO: Why are flags assigned this way? */
        SECURITY_ENGINE->RSA_KEYSLOT_FLAGS[keyslot] = (((flags >> 4) & 4) | (flags & 3)) ^ 7;
    }

    /* Disable keyslot reads. */
    if (flags & 0x80) {
        SECURITY_ENGINE->RSA_KEY_READ_DISABLE_REG &= ~(1 << keyslot);
    }
}

void clear_aes_keyslot(unsigned int keyslot) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    /* Zero out the whole keyslot and IV. */
    for (unsigned int i = 0; i < 0x10; i++) {
        SECURITY_ENGINE->AES_KEYTABLE_ADDR = (keyslot << 4) | i;
        SECURITY_ENGINE->AES_KEYTABLE_DATA = 0;
    }
}

void clear_rsa_keyslot(unsigned int keyslot) {
    if (keyslot >= KEYSLOT_RSA_MAX) {
        generic_panic();
    }

    /* Zero out the whole keyslot. */
    for (unsigned int i = 0; i < 0x40; i++) {
        /* Select Keyslot Modulus[i] */
        SECURITY_ENGINE->RSA_KEYTABLE_ADDR = (keyslot << 7) | i | 0x40;
        SECURITY_ENGINE->RSA_KEYTABLE_DATA = 0;
    }
    for (unsigned int i = 0; i < 0x40; i++) {
        /* Select Keyslot Expontent[i] */
        SECURITY_ENGINE->RSA_KEYTABLE_ADDR = (keyslot << 7) | i;
        SECURITY_ENGINE->RSA_KEYTABLE_DATA = 0;
    }
}

void set_aes_keyslot(unsigned int keyslot, const void *key, size_t key_size) {
    if (keyslot >= KEYSLOT_AES_MAX || key_size > KEYSIZE_AES_MAX) {
        generic_panic();
    }

    for (size_t i = 0; i < (key_size >> 2); i++) {
        SECURITY_ENGINE->AES_KEYTABLE_ADDR = (keyslot << 4) | i;
        SECURITY_ENGINE->AES_KEYTABLE_DATA = read32le(key, 4 * i);
    }
}

void set_rsa_keyslot(unsigned int keyslot, const void  *modulus, size_t modulus_size, const void *exponent, size_t exp_size) {
    if (keyslot >= KEYSLOT_RSA_MAX || modulus_size > KEYSIZE_RSA_MAX || exp_size > KEYSIZE_RSA_MAX) {
        generic_panic();
    }

    for (size_t i = 0; i < (modulus_size >> 2); i++) {
        SECURITY_ENGINE->RSA_KEYTABLE_ADDR = (keyslot << 7) | 0x40 | i;
        SECURITY_ENGINE->RSA_KEYTABLE_DATA = read32be(modulus, (4 * (modulus_size >> 2)) - (4 * i) - 4);
    }

    for (size_t i = 0; i < (exp_size >> 2); i++) {
        SECURITY_ENGINE->RSA_KEYTABLE_ADDR = (keyslot << 7) | i;
        SECURITY_ENGINE->RSA_KEYTABLE_DATA = read32be(exponent, (4 * (exp_size >> 2)) - (4 * i) - 4);
    }

    g_se_modulus_sizes[keyslot] = modulus_size;
    g_se_exp_sizes[keyslot] = exp_size;
}

void set_aes_keyslot_iv(unsigned int keyslot, const void *iv, size_t iv_size) {
    if (keyslot >= KEYSLOT_AES_MAX || iv_size > 0x10) {
        generic_panic();
    }

    for (size_t i = 0; i < (iv_size >> 2); i++) {
        SECURITY_ENGINE->AES_KEYTABLE_ADDR = (keyslot << 4) | 8 | i;
        SECURITY_ENGINE->AES_KEYTABLE_DATA = read32le(iv, 4 * i);
    }
}

void clear_aes_keyslot_iv(unsigned int keyslot) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    for (size_t i = 0; i < (0x10 >> 2); i++) {
        SECURITY_ENGINE->AES_KEYTABLE_ADDR = (keyslot << 4) | 8 | i;
        SECURITY_ENGINE->AES_KEYTABLE_DATA = 0;
    }
}

void set_se_ctr(const void *ctr) {
    for (unsigned int i = 0; i < 4; i++) {
        SECURITY_ENGINE->CRYPTO_CTR_REG[i] = read32le(ctr, i * 4);
    }
}

void decrypt_data_into_keyslot(unsigned int keyslot_dst, unsigned int keyslot_src, const void *wrapped_key, size_t wrapped_key_size) {
    if (keyslot_dst >= KEYSLOT_AES_MAX || keyslot_src >= KEYSLOT_AES_MAX || wrapped_key_size > KEYSIZE_AES_MAX) {
        generic_panic();
    }

    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_DEC | DST_KEYTAB);
    SECURITY_ENGINE->CRYPTO_REG = keyslot_src << 24;
    SECURITY_ENGINE->BLOCK_COUNT_REG = 0;
    SECURITY_ENGINE->CRYPTO_KEYTABLE_DST_REG = keyslot_dst << 8;

    trigger_se_blocking_op(OP_START, NULL, 0, wrapped_key, wrapped_key_size);
}

void se_synchronous_exp_mod(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    uint8_t ALIGN(16) stack_buf[KEYSIZE_RSA_MAX];

    if (keyslot >= KEYSLOT_RSA_MAX || src_size > KEYSIZE_RSA_MAX || dst_size > KEYSIZE_RSA_MAX) {
        generic_panic();
    }

    /* Endian swap the input. */
    for (size_t i = 0; i < src_size; i++) {
        stack_buf[i] = *((uint8_t *)src + src_size - i - 1);
    }

    SECURITY_ENGINE->CONFIG_REG = (ALG_RSA | DST_RSAREG);
    SECURITY_ENGINE->RSA_CONFIG = keyslot << 24;
    SECURITY_ENGINE->RSA_KEY_SIZE_REG = (g_se_modulus_sizes[keyslot] >> 6) - 1;
    SECURITY_ENGINE->RSA_EXP_SIZE_REG = g_se_exp_sizes[keyslot] >> 2;


    trigger_se_blocking_op(OP_START, NULL, 0, stack_buf, src_size);
    se_get_exp_mod_output(dst, dst_size);
}

void se_get_exp_mod_output(void *buf, size_t size) {
    size_t num_dwords = (size >> 2);
    if (num_dwords < 1) {
        return;
    }

    uint32_t *p_out = ((uint32_t *)buf) + num_dwords - 1;
    uint32_t offset = 0;

    /* Copy endian swapped output. */
    while (num_dwords) {
        *p_out = read32be(SECURITY_ENGINE->RSA_OUTPUT, offset);
        offset += 4;
        p_out--;
        num_dwords--;
    }
}

bool se_rsa2048_pss_verify(const void *signature, size_t signature_size, const void *modulus, size_t modulus_size, const void *data, size_t data_size) {
    uint8_t message[RSA_2048_BYTES];
    uint8_t h_buf[0x24];

    /* Hardcode RSA with keyslot 0. */
    const uint8_t public_exponent[4] = {0x00, 0x01, 0x00, 0x01};
    set_rsa_keyslot(0, modulus, modulus_size, public_exponent, sizeof(public_exponent));
    se_synchronous_exp_mod(0, message, sizeof(message), signature, signature_size);

    /* Validate sanity byte. */
    if (message[RSA_2048_BYTES - 1] != 0xBC) {
        return false;
    }

    /* Copy Salt into MGF1 Hash Buffer. */
    memset(h_buf, 0, sizeof(h_buf));
    memcpy(h_buf, message + RSA_2048_BYTES - 0x20 - 0x1, 0x20);

    /* Decrypt maskedDB (via inline MGF1). */
    uint8_t seed = 0;
    uint8_t mgf1_buf[0x20];
    for (unsigned int ofs = 0; ofs < RSA_2048_BYTES - 0x20 - 1; ofs += 0x20) {
        h_buf[sizeof(h_buf) - 1] = seed++;
        se_calculate_sha256(mgf1_buf, h_buf, sizeof(h_buf));
        for (unsigned int i = ofs; i < ofs + 0x20 && i < RSA_2048_BYTES - 0x20 - 1; i++) {
            message[i] ^= mgf1_buf[i - ofs];
        }
    }

    /* Constant lmask for rsa-2048-pss. */
    message[0] &= 0x7F;

    /* Validate DB is of the form 0000...0001. */
    for (unsigned int i = 0; i < RSA_2048_BYTES - 0x20 - 0x20 - 1 - 1; i++) {
        if (message[i] != 0) {
            return false;
        }
    }
    if (message[RSA_2048_BYTES - 0x20 - 0x20 - 1 - 1] != 1) {
        return false;
    }

    /* Check hash correctness. */
    uint8_t validate_buf[8 + 0x20 + 0x20];
    uint8_t validate_hash[0x20];

    memset(validate_buf, 0, sizeof(validate_buf));
    se_calculate_sha256(&validate_buf[8], data, data_size);
    memcpy(&validate_buf[0x28], &message[RSA_2048_BYTES - 0x20 - 0x20 - 1], 0x20);
    se_calculate_sha256(validate_hash, validate_buf, sizeof(validate_buf));
    return memcmp(h_buf, validate_hash, 0x20) == 0;
}

void trigger_se_blocking_op(unsigned int op, void *dst, size_t dst_size, const void *src, size_t src_size) {
    se_ll_t in_ll;
    se_ll_t out_ll;

    ll_init(&in_ll, (void *)src, src_size);
    ll_init(&out_ll, dst, dst_size);

    /* Set the LLs. */
    SECURITY_ENGINE->IN_LL_ADDR_REG = (uint32_t) get_physical_address(&in_ll);
    SECURITY_ENGINE->OUT_LL_ADDR_REG = (uint32_t) get_physical_address(&out_ll);

    /* Set registers for operation. */
    SECURITY_ENGINE->ERR_STATUS_REG = SECURITY_ENGINE->ERR_STATUS_REG;
    SECURITY_ENGINE->INT_STATUS_REG = SECURITY_ENGINE->INT_STATUS_REG;
    SECURITY_ENGINE->OPERATION_REG = op;

    while (!(SECURITY_ENGINE->INT_STATUS_REG & 0x10)) { /* Wait a while */ }
    se_check_for_error();
}


/* Secure AES Functionality. */
void se_perform_aes_block_operation(void *dst, size_t dst_size, const void *src, size_t src_size) {
    uint8_t block[0x10] = {0};

    if (src_size > sizeof(block) || dst_size > sizeof(block)) {
        generic_panic();
    }

    /* Load src data into block. */
    if (src_size != 0) {
        memcpy(block, src, src_size);
    }

    /* Trigger AES operation. */
    SECURITY_ENGINE->BLOCK_COUNT_REG = 0;
    trigger_se_blocking_op(OP_START, block, sizeof(block), block, sizeof(block));

    /* Copy output data into dst. */
    if (dst_size != 0) {
        memcpy(dst, block, dst_size);
    }
}

void se_aes_ctr_crypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size) {
    if (keyslot >= KEYSLOT_AES_MAX || ctr_size != 0x10) {
        generic_panic();
    }
    unsigned int num_blocks = src_size >> 4;

    /* Unknown what this write does, but official code writes it for CTR mode. */
    SECURITY_ENGINE->_0x80C = 1;
    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_ENC | DST_MEMORY);
    SECURITY_ENGINE->CRYPTO_REG = (keyslot << 24) | 0x91E;
    set_se_ctr(ctr);

    /* Handle any aligned blocks. */
    size_t aligned_size = (size_t)num_blocks << 4;
    if (aligned_size) {
        SECURITY_ENGINE->BLOCK_COUNT_REG = num_blocks - 1;
        trigger_se_blocking_op(OP_START, dst, dst_size, src, aligned_size);
    }

    /* Handle final, unaligned block. */
    if (aligned_size < dst_size && aligned_size < src_size) {
        size_t last_block_size = dst_size - aligned_size;
        if (src_size < dst_size) {
            last_block_size = src_size - aligned_size;
        }
        se_perform_aes_block_operation(dst + aligned_size, last_block_size, (uint8_t *)src + aligned_size, src_size - aligned_size);
    }
}

void se_aes_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, unsigned int config_high) {
    if (keyslot >= KEYSLOT_AES_MAX || dst_size != 0x10 || src_size != 0x10) {
        generic_panic();
    }

    /* Set configuration high (256-bit vs 128-bit) based on parameter. */
    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_ENC | DST_MEMORY) | (config_high << 16);
    SECURITY_ENGINE->CRYPTO_REG = keyslot << 24 | 0x100;
    se_perform_aes_block_operation(dst, 0x10, src, 0x10);

}

void se_aes_128_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    se_aes_ecb_encrypt_block(keyslot, dst, dst_size, src, src_size, 0);
}

void se_aes_256_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    se_aes_ecb_encrypt_block(keyslot, dst, dst_size, src, src_size, 0x202);
}


void se_aes_ecb_decrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    if (keyslot >= KEYSLOT_AES_MAX || dst_size != 0x10 || src_size != 0x10) {
        generic_panic();
    }

    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_DEC | DST_MEMORY);
    SECURITY_ENGINE->CRYPTO_REG = keyslot << 24;
    se_perform_aes_block_operation(dst, 0x10, src, 0x10);
}

void shift_left_xor_rb(uint8_t *key) {
    uint8_t prev_high_bit = 0;
    for (unsigned int i = 0; i < 0x10; i++) {
        uint8_t cur_byte = key[0xF - i];
        key[0xF - i] = (cur_byte << 1) | (prev_high_bit);
        prev_high_bit = cur_byte >> 7;
    }
    if (prev_high_bit) {
        key[0xF] ^= 0x87;
    }
}

void shift_left_xor_rb_le(uint8_t *key) {
    uint8_t prev_high_bit = 0;
    for (unsigned int i = 0; i < 0x10; i++) {
        uint8_t cur_byte = key[i];
        key[i] = (cur_byte << 1) | (prev_high_bit);
        prev_high_bit = cur_byte >> 7;
    }
    if (prev_high_bit) {
        key[0x0] ^= 0x87;
    }
}

void aes_128_xts_nintendo_get_tweak(uint8_t *tweak, size_t sector) {
    for (int i = 0xF; i >= 0; i--) { /* Nintendo LE custom tweak... */
        tweak[i] = (unsigned char)(sector & 0xFF);
        sector >>= 8;
    }
}

void aes_128_xts_nintendo_xor_with_tweak(unsigned int keyslot, size_t sector, uint8_t *dst, const uint8_t *src, size_t size) {
    if ((size & 0xF) || size == 0) {
        generic_panic();
    }
    uint8_t tweak[0x10];
    aes_128_xts_nintendo_get_tweak(tweak, sector);
    se_aes_128_ecb_encrypt_block(keyslot, tweak, sizeof(tweak), tweak, sizeof(tweak));
    
    for (unsigned int block = 0; block < (size >> 4); block++) {
        for (unsigned int i = 0; i < 0x10; i++) {
            dst[(block << 4) | i] = src[(block << 4) | i] ^ tweak[i];
        }
        shift_left_xor_rb_le(tweak);
    }
}

void aes_128_xts_nintendo_crypt_sector(unsigned int keyslot_1, unsigned int keyslot_2, size_t sector, bool encrypt, void *dst, const void *src, size_t size) {
    if ((size & 0xF) || size == 0) {
        generic_panic();
    }
            
    /* XOR. */
    aes_128_xts_nintendo_xor_with_tweak(keyslot_2, sector, dst, src, size);
    
    /* Encrypt/Decrypt. */
    if (encrypt) {
        SECURITY_ENGINE->CONFIG_REG = (ALG_AES_ENC | DST_MEMORY);
        SECURITY_ENGINE->CRYPTO_REG = keyslot_1 << 24 | 0x100;
    } else {
        SECURITY_ENGINE->CONFIG_REG = (ALG_AES_DEC | DST_MEMORY);
        SECURITY_ENGINE->CRYPTO_REG = keyslot_1 << 24;
    }
    SECURITY_ENGINE->BLOCK_COUNT_REG = (size >> 4) - 1;
    trigger_se_blocking_op(OP_START, dst, size, src, size);
    
    /* XOR. */
    aes_128_xts_nintendo_xor_with_tweak(keyslot_2, sector, dst, dst, size);
}

/* Encrypt with AES-XTS (Nintendo's custom tweak). */
void se_aes_128_xts_nintendo_encrypt(unsigned int keyslot_1, unsigned int keyslot_2, size_t base_sector, void *dst, const void *src, size_t size, unsigned int sector_size) {
    if ((size & 0xF) || size == 0) {
        generic_panic();
    }
    size_t sector = base_sector;
    for (size_t ofs = 0; ofs < size; ofs += sector_size) {
        aes_128_xts_nintendo_crypt_sector(keyslot_1, keyslot_2, sector, true, dst + ofs, src + ofs, sector_size);
        sector++;
    }
}

/* Decrypt with AES-XTS (Nintendo's custom tweak). */
void se_aes_128_xts_nintendo_decrypt(unsigned int keyslot_1, unsigned int keyslot_2, size_t base_sector, void *dst, const void *src, size_t size, unsigned int sector_size) {
    if ((size & 0xF) || size == 0) {
        generic_panic();
    }
    size_t sector = base_sector;
    for (size_t ofs = 0; ofs < size; ofs += sector_size) {
        aes_128_xts_nintendo_crypt_sector(keyslot_1, keyslot_2, sector, false, dst + ofs, src + ofs, sector_size);
        sector++;
    }
}

void se_compute_aes_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size, unsigned int config_high) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    /* Generate the derived key, to be XOR'd with final output block. */
    uint8_t ALIGN(16) derived_key[0x10] = {0};
    se_aes_ecb_encrypt_block(keyslot, derived_key, sizeof(derived_key), derived_key, sizeof(derived_key), config_high);
    shift_left_xor_rb(derived_key);
    if (data_size & 0xF) {
        shift_left_xor_rb(derived_key);
    }

    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_ENC | DST_HASHREG) | (config_high << 16);
    SECURITY_ENGINE->CRYPTO_REG = (keyslot << 24) | (0x145);
    clear_aes_keyslot_iv(keyslot);


    unsigned int num_blocks = (data_size + 0xF) >> 4;
    /* Handle aligned blocks. */
    if (num_blocks > 1) {
        SECURITY_ENGINE->BLOCK_COUNT_REG = num_blocks - 2;
        trigger_se_blocking_op(OP_START, NULL, 0, data, data_size);
        SECURITY_ENGINE->CRYPTO_REG |= 0x80;
    }

    /* Create final block. */
    uint8_t ALIGN(16) last_block[0x10] = {0};
    if (data_size & 0xF) {
        memcpy(last_block, data + (data_size & ~0xF), data_size & 0xF);
        last_block[data_size & 0xF] = 0x80; /* Last block = data || 100...0 */
    } else if (data_size >= 0x10) {
        memcpy(last_block, data + data_size - 0x10, 0x10);
    }

    for (unsigned int i = 0; i < 0x10; i++) {
        last_block[i] ^= derived_key[i];
    }

    /* Perform last operation. */
    SECURITY_ENGINE->BLOCK_COUNT_REG = 0;
    trigger_se_blocking_op(OP_START, NULL, 0, last_block, sizeof(last_block));

    /* Copy output CMAC. */
    for (unsigned int i = 0; i < (cmac_size >> 2); i++) {
        ((uint32_t *)cmac)[i] = read32le(SECURITY_ENGINE->HASH_RESULT_REG, i << 2);
    }
}

void se_compute_aes_128_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size) {
    se_compute_aes_cmac(keyslot, cmac, cmac_size, data, data_size, 0);
}
void se_compute_aes_256_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size) {
    se_compute_aes_cmac(keyslot, cmac, cmac_size, data, data_size, 0x202);
}

void se_aes_256_cbc_encrypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *iv) {
    if (keyslot >= KEYSLOT_AES_MAX || src_size < 0x10) {
        generic_panic();
    }

    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_ENC | DST_MEMORY) | (0x202 << 16);
    SECURITY_ENGINE->CRYPTO_REG = (keyslot << 24) | 0x144;
    set_aes_keyslot_iv(keyslot, iv, 0x10);
    SECURITY_ENGINE->BLOCK_COUNT_REG = (src_size >> 4) - 1;
    trigger_se_blocking_op(OP_START, dst, dst_size, src, src_size);
}

/* SHA256 Implementation. */
void se_calculate_sha256(void *dst, const void *src, size_t src_size) {
    /* Setup config for SHA256, size = BITS(src_size) */
    SECURITY_ENGINE->CONFIG_REG = (ENCMODE_SHA256 | ALG_SHA | DST_HASHREG);
    SECURITY_ENGINE->SHA_CONFIG_REG = 1;
    SECURITY_ENGINE->SHA_MSG_LENGTH_REG = (unsigned int)(src_size << 3);
    SECURITY_ENGINE->_0x20C = 0;
    SECURITY_ENGINE->_0x210 = 0;
    SECURITY_ENGINE->SHA_MSG_LEFT_REG = 0;
    SECURITY_ENGINE->_0x218 = (unsigned int)(src_size << 3);
    SECURITY_ENGINE->_0x21C = 0;
    SECURITY_ENGINE->_0x220 = 0;
    SECURITY_ENGINE->_0x224 = 0;

    /* Trigger the operation. */
    trigger_se_blocking_op(OP_START, NULL, 0, src, src_size);

    /* Copy output hash. */
    for (unsigned int i = 0; i < (0x20 >> 2); i++) {
        ((uint32_t *)dst)[i] = read32be(SECURITY_ENGINE->HASH_RESULT_REG, i << 2);
    }
}

/* RNG API */
void se_initialize_rng(unsigned int keyslot) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    /* To initialize the RNG, we'll perform an RNG operation into an output buffer. */
    /* This will be discarded, when done. */
    uint8_t ALIGN(16) output_buf[0x10];

    SECURITY_ENGINE->RNG_SRC_CONFIG_REG = 3; /* Entropy enable + Entropy lock enable */
    SECURITY_ENGINE->RNG_RESEED_INTERVAL_REG = 70001;
    SECURITY_ENGINE->CONFIG_REG = (ALG_RNG | DST_MEMORY);
    SECURITY_ENGINE->CRYPTO_REG = (keyslot << 24) | 0x108;
    SECURITY_ENGINE->RNG_CONFIG_REG = 5;
    SECURITY_ENGINE->BLOCK_COUNT_REG = 0;
    trigger_se_blocking_op(OP_START, output_buf, 0x10, NULL, 0);
}

void se_generate_random(unsigned int keyslot, void *dst, size_t size) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    uint32_t num_blocks = size >> 4;
    size_t aligned_size = num_blocks << 4;
    SECURITY_ENGINE->CONFIG_REG = (ALG_RNG | DST_MEMORY);
    SECURITY_ENGINE->CRYPTO_REG = (keyslot << 24) | 0x108;
    SECURITY_ENGINE->RNG_CONFIG_REG = 4;

    if (num_blocks >= 1) {
        SECURITY_ENGINE->BLOCK_COUNT_REG = num_blocks - 1;
        trigger_se_blocking_op(OP_START, dst, aligned_size, NULL, 0);
    }
    if (size > aligned_size) {
        se_perform_aes_block_operation(dst + aligned_size, size - aligned_size, NULL, 0);
    }

}