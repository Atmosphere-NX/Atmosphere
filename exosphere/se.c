#include <stdint.h>
#include <stddef.h>

#include "utils.h"
#include "mmu.h"
#include "cache.h"
#include "se.h"

/* Macro for the SE registers. */
#define SECURITY_ENGINE ((volatile security_engine_t *)(mmio_get_device_address(MMIO_DEVID_SE)))

void trigger_se_rsa_op(void *buf, size_t size);
void trigger_se_blocking_op(unsigned int op, void *dst, size_t dst_size, const void *src, size_t src_size);

/* Globals for driver. */
unsigned int (*g_se_callback)(void);

unsigned int g_se_modulus_sizes[KEYSLOT_RSA_MAX];
unsigned int g_se_exp_sizes[KEYSLOT_RSA_MAX];


/* Initialize a SE linked list. */
void ll_init(se_ll_t *ll, void *buffer, size_t size) {
    ll->num_entries = 0; /* 1 Entry. */
    
    if (buffer != NULL) {
        ll->addr_info.address = get_physical_address(buffer);
        ll->addr_info.size = (uint32_t) size;
    } else {
        ll->addr_info.address = 0;
        ll->addr_info.size = 0;
    }

    flush_dcache_range((uint8_t *)ll, (uint8_t *)ll + sizeof(*ll));
}

/* Gets security engine pointer. */
security_engine_t *get_security_engine_address(void) {
    return SECURITY_ENGINE;
}

void set_security_engine_callback(unsigned int (*callback)(void)) {
    if (callback == NULL || g_se_callback != NULL) {
        panic();
    }
    
    g_se_callback = callback;
}

/* Fires on Security Engine operation completion. */
void se_operation_completed(void) {
    SECURITY_ENGINE->INT_ENABLE_REG = 0;
    if (g_se_callback != NULL) {
        g_se_callback();
        g_se_callback = NULL;
    }
}


void se_check_for_error(void) {
    if (SECURITY_ENGINE->INT_STATUS_REG & 0x10000 || SECURITY_ENGINE->FLAGS_REG & 3 || SECURITY_ENGINE->ERR_STATUS_REG) {
        panic();
    }
}

void se_trigger_intrrupt(void) {
    /* TODO */
}

void se_verify_flags_cleared(void) {
    if (SECURITY_ENGINE->FLAGS_REG & 3) {
        panic();
    }
}

void se_clear_interrupts(void) {
    /* TODO */
}

/* Set the flags for an AES keyslot. */
void set_aes_keyslot_flags(unsigned int keyslot, unsigned int flags) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        panic();
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
        panic();
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
        panic();
    }
    
    /* Zero out the whole keyslot and IV. */
    for (unsigned int i = 0; i < 0x10; i++) {
        SECURITY_ENGINE->AES_KEYTABLE_ADDR = (keyslot << 4) | i;
        SECURITY_ENGINE->AES_KEYTABLE_DATA = 0;
    }
}

void clear_rsa_keyslot(unsigned int keyslot) {
    if (keyslot >= KEYSLOT_RSA_MAX) {
        panic();
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
        panic();
    }
    
    for (size_t i = 0; i < (key_size >> 2); i++) {
        SECURITY_ENGINE->AES_KEYTABLE_ADDR = (keyslot << 4) | i;
        SECURITY_ENGINE->AES_KEYTABLE_DATA = read32le(key, 4 * i);
    }
}

void set_rsa_keyslot(unsigned int keyslot, const void  *modulus, size_t modulus_size, const void *exponent, size_t exp_size) {
    if (keyslot >= KEYSLOT_RSA_MAX || modulus_size > KEYSIZE_RSA_MAX || exp_size > KEYSIZE_RSA_MAX) {
        panic();
    }
    
    for (size_t i = 0; i < (modulus_size >> 2); i++) {
        SECURITY_ENGINE->RSA_KEYTABLE_ADDR = (keyslot << 7) | 0x40 | i;
        SECURITY_ENGINE->RSA_KEYTABLE_DATA = read32be(modulus, 4 * i);
    }
    
    for (size_t i = 0; i < (exp_size >> 2); i++) {
        SECURITY_ENGINE->RSA_KEYTABLE_ADDR = (keyslot << 7) | i;
        SECURITY_ENGINE->RSA_KEYTABLE_DATA = read32be(exponent, 4 * i);
    }
    
    g_se_modulus_sizes[keyslot] = modulus_size;
    g_se_exp_sizes[keyslot] = exp_size;
}

void set_aes_keyslot_iv(unsigned int keyslot, const void *iv, size_t iv_size) {
    if (keyslot >= KEYSLOT_AES_MAX || iv_size > 0x10) {
        panic();
    }
    
    for (size_t i = 0; i < (iv_size >> 2); i++) {
        SECURITY_ENGINE->AES_KEYTABLE_ADDR = (keyslot << 4) | 8 | i;
        SECURITY_ENGINE->AES_KEYTABLE_DATA = read32le(iv, 4 * i);
    }
}

void clear_aes_keyslot_iv(unsigned int keyslot) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        panic();
    }
    
    for (size_t i = 0; i < (0x10 >> 2); i++) {
        SECURITY_ENGINE->AES_KEYTABLE_ADDR = (keyslot << 4) | 8;
        SECURITY_ENGINE->AES_KEYTABLE_DATA = 0;
    }
}

void set_se_ctr(const void *ctr) {
    for (unsigned int i = 0; i < 4; i++) {
        SECURITY_ENGINE->CRYPTO_CTR_REG[i] = read32le(ctr, i * 4);
    }
}

void decrypt_data_into_keyslot(unsigned int keyslot_dst, unsigned int keyslot_src, const void *wrapped_key, size_t wrapped_key_size) {
    if (keyslot_dst >= KEYSLOT_AES_MAX || keyslot_src >= KEYSIZE_AES_MAX || wrapped_key_size > KEYSIZE_AES_MAX) {
        panic();
    }
    
    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_DEC | DST_KEYTAB);
    SECURITY_ENGINE->CRYPTO_REG = keyslot_src << 24;
    SECURITY_ENGINE->BLOCK_COUNT_REG = 0;
    SECURITY_ENGINE->CRYPTO_KEYTABLE_DST_REG = keyslot_dst << 8;

    flush_dcache_range(wrapped_key, (const uint8_t *)wrapped_key + wrapped_key_size);
    trigger_se_aes_op(OP_START, NULL, 0, wrapped_key, wrapped_key_size);
}

void se_aes_crypt_insecure_internal(unsigned int keyslot, uint32_t out_ll_paddr, uint32_t in_ll_paddr, size_t size, unsigned int crypt_config, bool encrypt, unsigned int (*callback)(void)) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        panic();
    }
    
    if (size == 0) {
        return;
    }
    
    /* Setup Config register. */
    if (encrypt) {
        SECURITY_ENGINE->CONFIG_REG = (ALG_AES_ENC | DST_MEMORY);
    } else {
        SECURITY_ENGINE->CONFIG_REG = (ALG_AES_DEC | DST_MEMORY);
    }
    
    /* Setup Crypto register. */
    SECURITY_ENGINE->CRYPTO_REG = crypt_config | (keyslot << 24) | (encrypt << 8);
    
    /* Mark this encryption as insecure -- this makes the SE not a secure busmaster. */
    SECURITY_ENGINE->CRYPTO_REG |= 0x80000000;
    
    /* Appropriate number of blocks. */
    SECURITY_ENGINE->BLOCK_COUNT_REG = (size >> 4) - 1;
    
    /* Set the callback, for after the async operation. */
    set_security_engine_callback(callback);
    
    /* Enable SE Interrupt firing for async op. */
    SECURITY_ENGINE->INT_ENABLE_REG = 0x10;
    
    /* Setup Input/Output lists */
    SECURITY_ENGINE->IN_LL_ADDR_REG = in_ll_paddr;
    SECURITY_ENGINE->OUT_LL_ADDR_REG = out_ll_paddr;
    
    /* Set registers for operation. */
    SECURITY_ENGINE->ERR_STATUS_REG = SECURITY_ENGINE->ERR_STATUS_REG;
    SECURITY_ENGINE->INT_STATUS_REG = SECURITY_ENGINE->INT_STATUS_REG;
    SECURITY_ENGINE->OPERATION_REG = 1;
    
    /* Ensure writes go through. */
    __asm__ __volatile__ ("dsb ish" : : : "memory");
}

void se_aes_ctr_crypt_insecure(unsigned int keyslot, uint32_t out_ll_paddr, uint32_t in_ll_paddr, size_t size, const void *ctr, unsigned int (*callback)(void)) {
    /* Unknown what this write does, but official code writes it for CTR mode. */
    SECURITY_ENGINE->_0x80C = 1;
    set_se_ctr(ctr);
    se_aes_crypt_insecure_internal(keyslot, out_ll_paddr, in_ll_paddr, size, 0x81E, true, callback);
}

void se_aes_cbc_encrypt_insecure(unsigned int keyslot, uint32_t out_ll_paddr, uint32_t in_ll_paddr, size_t size, const void *iv, unsigned int (*callback)(void)) {
    set_aes_keyslot_iv(keyslot, iv, 0x10);
    se_aes_crypt_insecure_internal(keyslot, out_ll_paddr, in_ll_paddr, size, 0x44, true, callback);
}

void se_aes_cbc_decrypt_insecure(unsigned int keyslot, uint32_t out_ll_paddr, uint32_t in_ll_paddr, size_t size, const void *iv, unsigned int (*callback)(void)) {    
    set_aes_keyslot_iv(keyslot, iv, 0x10);
    se_aes_crypt_insecure_internal(keyslot, out_ll_paddr, in_ll_paddr, size, 0x66, false, callback);
}


void se_exp_mod(unsigned int keyslot, void *buf, size_t size, unsigned int (*callback)(void)) {
    uint8_t stack_buf[KEYSIZE_RSA_MAX];
    
    if (keyslot >= KEYSLOT_RSA_MAX || size > KEYSIZE_RSA_MAX) {
        panic();
    }
    
    /* Endian swap the input. */
    for (size_t i = size; i > 0; i--) {
        stack_buf[i] = *((uint8_t *)buf + size - i);
    }
    

    SECURITY_ENGINE->CONFIG_REG = (ALG_RSA | DST_RSAREG);
    SECURITY_ENGINE->RSA_CONFIG = keyslot << 24;
    SECURITY_ENGINE->RSA_KEY_SIZE_REG = (g_se_modulus_sizes[keyslot] >> 6) - 1;
    SECURITY_ENGINE->RSA_EXP_SIZE_REG = g_se_exp_sizes[keyslot] >> 2;

    set_security_engine_callback(callback);
    
    /* Enable SE Interrupt firing for async op. */
    SECURITY_ENGINE->INT_ENABLE_REG = 0x10;

    flush_dcache_range(stack_buf, stack_buf + KEYSIZE_RSA_MAX);
    trigger_se_rsa_op(stack_buf, size);

    while (!(SECURITY_ENGINE->INT_STATUS_REG & 2)) { /* Wait a while */ }
}

void se_synchronous_exp_mod(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    uint8_t stack_buf[KEYSIZE_RSA_MAX];
    
    if (keyslot >= KEYSLOT_RSA_MAX || src_size > KEYSIZE_RSA_MAX || dst_size > KEYSIZE_RSA_MAX) {
        panic();
    }

    /* Endian swap the input. */
    for (size_t i = size; i > 0; i--) {
        stack_buf[i] = *((uint8_t *)buf + size - i);
    }
    
    SECURITY_ENGINE->CONFIG_REG = (ALG_RSA | DST_RSAREG);
    SECURITY_ENGINE->RSA_CONFIG = keyslot << 24;
    SECURITY_ENGINE->RSA_KEY_SIZE_REG = (g_se_modulus_sizes[keyslot] >> 6) - 1;
    SECURITY_ENGINE->RSA_EXP_SIZE_REG = g_se_exp_sizes[keyslot] >> 2;

    
    flush_dcache_range(stack_buf, stack_buf + KEYSIZE_RSA_MAX);
    trigger_se_blocking_op(1, NULL, 0, stack_buf, src_size);
    se_get_exp_mod_output(dst, dst_size);
}

void se_get_exp_mod_output(void *buf, size_t size) {
    size_t num_dwords = (size >> 2);
    if (num_dwords < 1) {
        return;
    }
    
    uint32_t *p_out = ((uint32_t *)buf) + num_dwords - 1;
    uint32_t out_ofs = 0;
    
    /* Copy endian swapped output. */
    while (num_dwords) {
        *p_out = read32be(SECURITY_ENGINE->RSA_OUTPUT, offset);
        offset += 4;
        p_out--;
        num_dwords--;
    }
}

void trigger_se_rsa_op(void *buf, size_t size) {
    se_ll_t in_ll;
    ll_init(&in_ll, buf, size);
    
    /* Set the input LL. */
    SECURITY_ENGINE->IN_LL_ADDR_REG = get_physical_address(&in_ll);
    
    /* Set registers for operation. */
    SECURITY_ENGINE->ERR_STATUS_REG = SECURITY_ENGINE->ERR_STATUS_REG;
    SECURITY_ENGINE->INT_STATUS_REG = SECURITY_ENGINE->INT_STATUS_REG;
    SECURITY_ENGINE->OPERATION_REG = 1;
    
    /* Ensure writes go through. */
    __asm__ __volatile__ ("dsb ish" : : : "memory");
}

void trigger_se_blocking_op(unsigned int op, void *dst, size_t dst_size, const void *src, size_t src_size) {
    se_ll_t in_ll;
    se_ll_t out_ll;
    
    ll_init(&in_ll, src, src_size);
    ll_init(&out_ll, dst, dst_size);
    
    /* Set the LLs. */
    SECURITY_ENGINE->IN_LL_ADDR_REG = get_physical_address(&in_ll);
    SECURITY_ENGINE->OUT_LL_ADDR_REG = get_physical_address(&out_ll);
    
    /* Set registers for operation. */
    SECURITY_ENGINE->ERR_STATUS_REG = SECURITY_ENGINE->ERR_STATUS_REG;
    SECURITY_ENGINE->INT_STATUS_REG = SECURITY_ENGINE->INT_STATUS_REG;
    SECURITY_ENGINE->OPERATION_REG = op;
    
    while (!(SECURITY_ENGINE->INT_STATUS_REG & 0x10)) { /* Wait a while */ }
    se_check_for_error();
}


/* Secure AES Functionality. */
void se_perform_aes_block_operation(void *dst, size_t dst_size, const void *src, size_t src_size) {
    uint8_t block[0x10];

    if (src_size > sizeof(block) || dst_size > sizeof(block)) {
        panic();
    }
    
    /* Load src data into block. */
    memset(block, 0, sizeof(block));
    memcpy(block, src, src_size);
    flush_dcache_range(block, block + sizeof(block));
    
    /* Trigger AES operation. */
    SECURITY_ENGINE->BLOCK_COUNT_REG = 0;
    trigger_se_blocking_op(1, block, sizeof(block), block, sizeof(block));
    
    /* Copy output data into dst. */
    flush_dcache_range(block, block + sizeof(block));
    memcpy(dst, block, dst_size);
}

void se_aes_ctr_crypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size) {
    if (keyslot >= KEYSLOT_AES_MAX || ctr_size != 0x10) {
        panic();
    }
    
    unsigned int num_blocks = src_size >> 4;
    
    /* Unknown what this write does, but official code writes it for CTR mode. */
    SECURITY_ENGINE->_0x80C = 1;
    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_ENC | DST_MEMORY);
    SECURITY_ENGINE->CRYPTO_REG = (keyslot << 24) | 0x91E;
    set_se_ctr(ctr, ctr_size);
    
    /* Handle any aligned blocks. */
    size_t aligned_size = (size_t)num_blocks << 4;
    if (aligned_size) {
        SECURITY_ENGINE->BLOCK_COUNT_REG = num_blocks - 1;
        trigger_se_blocking_op(1, dst, dst_size, src, aligned_size);
    }
    
    /* Handle final, unaligned block. */
    if (aligned_size < dst_size && aligned_size < src_size) {
        size_t last_block_size = dst_size - aligned_size;
        if (src_size < dst_size) {
            last_block_size = src_size - aligned_size;
        }
        se_perform_aes_block_operation(dst + aligned_size, last_block_size, src + aligned_size, src - aligned_size);
    }
}

void se_aes_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, unsigned int config_high) {
    if (keyslot >= KEYSLOT_AES_MAX || dst_size != 0x10 || src_size != 0x10) {
        panic();
    }
    
    /* Set configuration high (256-bit vs 128-bit) based on parameter. */
    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_ENC | DST_MEMORY) | (config_high << 16);
    SECURITY_ENGINE->CRYPTO_REG = keyslot << 24;
    se_perform_aes_block_operation(1, dst, 0x10, src, 0x10);

}

void se_aes_128_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    se_aes_ecb_encrypt_block(keyslot, dst, dst_size, src, src_size, 0);
}

void se_aes_256_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    se_aes_ecb_encrypt_block(keyslot, dst, dst_size, src, src_size, 0x202);
}


void se_aes_ecb_decrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    if (keyslot >= KEYSLOT_AES_MAX || dst_size != 0x10 || src_size != 0x10) {
        panic();
    }
    
    SECURITY_ENGINE->CONFIG_REG = (ALG_AES_DEC | DST_MEMORY);
    SECURITY_ENGINE->CRYPTO_REG = keyslot << 24;
    se_perform_aes_block_operation(1, dst, 0x10, src, 0x10);
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

void se_compute_aes_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size, unsigned int config_high) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        panic();
    }
    
    /* Generate the derived key, to be XOR'd with final output block. */
    uint8_t derived_key[0x10];
    memset(derived_key, 0, sizeof(derived_key));
    se_aes_128_ecb_encrypt_block(keyslot, derived_key, sizeof(derived_key), derived_key, sizeof(derived_key));
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
        trigger_se_blocking_op(1, NULL, 0, data, data_size);
        SECURITY_ENGINE->CRYPTO_REG |= 0x80;
    }
    
    /* Create final block. */
    uint8_t last_block[0x10];
    memset(last_block, 0, sizeof(last_block));
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
    flush_dcache_range(last_block, last_block + sizeof(last_block));
    trigger_se_blocking_op(1, NULL, 0, last_block, sizeof(last_block));
    
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
    trigger_se_blocking_op(1, NULL, 0, src, src_size);
    
    /* Copy output hash. */
    for (unsigned int i = 0; i < (0x20 >> 2); i++) {
        ((uint32_t *)dst)[i] = read32be(SECURITY_ENGINE->HASH_RESULT_REG, i << 2);
    }
}

/* RNG API */
void se_initialize_rng(unsigned int keyslot) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        panic();
    }
    
    /* To initialize the RNG, we'll perform an RNG operation into an output buffer. */
    /* This will be discarded, when done. */
    uint8_t output_buf[0x10];
    
    SECURITY_ENGINE->RNG_SRC_CONFIG_REG = 3; /* Entropy enable + Entropy lock enable */
    SECURITY_ENGINE->RNG_RESEED_INTERVAL_REG = 70001;
    SECURITY_ENGINE->CONFIG_REG = (ALG_RNG | DST_MEMORY);
    SECURITY_ENGINE->CRYPTO_REG = (keyslot << 24) | 0x108;
    SECURITY_ENGINE->RNG_CONFIG_REG = 5;
    SECURITY_ENGINE->BLOCK_COUNT_REG = 0;
    trigger_se_blocking_op(1, output_buf, 0x10, NULL, 0);
}

void se_generate_random(unsigned int keyslot, void *dst, size_t size) {
    if (keyslot >= KEYSLOT_AES_MAX) {
        panic();
    }
    
    uint32_t num_blocks = size >> 4;
    size_t aligned_size = num_blocks << 4;
    SECURITY_ENGINE->CONFIG_REG = (ALG_RNG | DST_MEMORY);
    SECURITY_ENGINE->CRYPTO_REG = (keyslot << 24) | 0x108;
    SECURITY_ENGINE->RNG_CONFIG_REG = 4;
    
    if (num_blocks >= 1) {
        SECURITY_ENGINE->BLOCK_COUNT_REG = num_blocks - 1;
        trigger_se_blocking_op(1, dst, aligned_size, NULL, 0);
    }
    if (size > aligned_size) {
        se_perform_aes_block_operation(dst + aligned_size, size - aligned_size, NULL, 0);
    }

}