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

#include <string.h>

#include "utils.h"
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
    if (se_get_regs()->SE_ERR_STATUS) {
        generic_panic();
    }
}

void se_check_for_error(void) {
    volatile tegra_se_t *se = se_get_regs();
    if (se->SE_INT_STATUS & 0x10000 || se->SE_STATUS & 3 || se->SE_ERR_STATUS) {
        generic_panic();
    }
}

void se_verify_flags_cleared(void) {
    if (se_get_regs()->SE_STATUS & 3) {
        generic_panic();
    }
}

/* Set the flags for an AES keyslot. */
void set_aes_keyslot_flags(unsigned int keyslot, unsigned int flags) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    /* Misc flags. */
    if (flags & ~0x80) {
        se->SE_CRYPTO_KEYTABLE_ACCESS[keyslot] = ~flags;
    }

    /* Disable keyslot reads. */
    if (flags & 0x80) {
        se->SE_CRYPTO_SECURITY_PERKEY &= ~(1 << keyslot);
    }
}

/* Set the flags for an RSA keyslot. */
void set_rsa_keyslot_flags(unsigned int keyslot, unsigned int flags) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_RSA_MAX) {
        generic_panic();
    }

    /* Misc flags. */
    if (flags & ~0x80) {
        /* TODO: Why are flags assigned this way? */
        se->SE_RSA_KEYTABLE_ACCESS[keyslot] = (((flags >> 4) & 4) | (flags & 3)) ^ 7;
    }

    /* Disable keyslot reads. */
    if (flags & 0x80) {
        se->SE_RSA_SECURITY_PERKEY &= ~(1 << keyslot);
    }
}

void clear_aes_keyslot(unsigned int keyslot) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    /* Zero out the whole keyslot and IV. */
    for (unsigned int i = 0; i < 0x10; i++) {
        se->SE_CRYPTO_KEYTABLE_ADDR = (keyslot << 4) | i;
        se->SE_CRYPTO_KEYTABLE_DATA = 0;
    }
}

void clear_rsa_keyslot(unsigned int keyslot) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_RSA_MAX) {
        generic_panic();
    }

    /* Zero out the whole keyslot. */
    for (unsigned int i = 0; i < 0x40; i++) {
        /* Select Keyslot Modulus[i] */
        se->SE_RSA_KEYTABLE_ADDR = (keyslot << 7) | i | 0x40;
        se->SE_RSA_KEYTABLE_DATA = 0;
    }
    for (unsigned int i = 0; i < 0x40; i++) {
        /* Select Keyslot Expontent[i] */
        se->SE_RSA_KEYTABLE_ADDR = (keyslot << 7) | i;
        se->SE_RSA_KEYTABLE_DATA = 0;
    }
}

void set_aes_keyslot(unsigned int keyslot, const void *key, size_t key_size) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX || key_size > KEYSIZE_AES_MAX) {
        generic_panic();
    }

    for (size_t i = 0; i < (key_size >> 2); i++) {
        se->SE_CRYPTO_KEYTABLE_ADDR = (keyslot << 4) | i;
        se->SE_CRYPTO_KEYTABLE_DATA = read32le(key, 4 * i);
    }
}

void set_rsa_keyslot(unsigned int keyslot, const void  *modulus, size_t modulus_size, const void *exponent, size_t exp_size) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_RSA_MAX || modulus_size > KEYSIZE_RSA_MAX || exp_size > KEYSIZE_RSA_MAX) {
        generic_panic();
    }

    for (size_t i = 0; i < (modulus_size >> 2); i++) {
        se->SE_RSA_KEYTABLE_ADDR = (keyslot << 7) | 0x40 | i;
        se->SE_RSA_KEYTABLE_DATA = read32be(modulus, (4 * (modulus_size >> 2)) - (4 * i) - 4);
    }

    for (size_t i = 0; i < (exp_size >> 2); i++) {
        se->SE_RSA_KEYTABLE_ADDR = (keyslot << 7) | i;
        se->SE_RSA_KEYTABLE_DATA = read32be(exponent, (4 * (exp_size >> 2)) - (4 * i) - 4);
    }

    g_se_modulus_sizes[keyslot] = modulus_size;
    g_se_exp_sizes[keyslot] = exp_size;
}

void set_aes_keyslot_iv(unsigned int keyslot, const void *iv, size_t iv_size) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX || iv_size > 0x10) {
        generic_panic();
    }

    for (size_t i = 0; i < (iv_size >> 2); i++) {
        se->SE_CRYPTO_KEYTABLE_ADDR = (keyslot << 4) | 8 | i;
        se->SE_CRYPTO_KEYTABLE_DATA = read32le(iv, 4 * i);
    }
}

void clear_aes_keyslot_iv(unsigned int keyslot) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    for (size_t i = 0; i < (0x10 >> 2); i++) {
        se->SE_CRYPTO_KEYTABLE_ADDR = (keyslot << 4) | 8 | i;
        se->SE_CRYPTO_KEYTABLE_DATA = 0;
    }
}

void set_se_ctr(const void *ctr) {
    for (unsigned int i = 0; i < 4; i++) {
        se_get_regs()->SE_CRYPTO_LINEAR_CTR[i] = read32le(ctr, i * 4);
    }
}

void decrypt_data_into_keyslot(unsigned int keyslot_dst, unsigned int keyslot_src, const void *wrapped_key, size_t wrapped_key_size) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot_dst >= KEYSLOT_AES_MAX || keyslot_src >= KEYSLOT_AES_MAX || wrapped_key_size > KEYSIZE_AES_MAX) {
        generic_panic();
    }

    /* Write config, validate. */
    se->SE_CONFIG = (ALG_AES_DEC | DST_KEYTAB);
    if (se->SE_CONFIG != (ALG_AES_DEC | DST_KEYTAB)) {
        generic_panic();
    }
    se->SE_CRYPTO_CONFIG = keyslot_src << 24;
    if (se->SE_CRYPTO_CONFIG != (keyslot_src << 24)) {
        generic_panic();
    }
    se->SE_CRYPTO_LAST_BLOCK = 0;
    if (se->SE_CRYPTO_LAST_BLOCK != 0) {
        generic_panic();
    }
    se->SE_CRYPTO_KEYTABLE_DST = keyslot_dst << 8;
    if (se->SE_CRYPTO_KEYTABLE_DST != (keyslot_dst << 8)) {
        generic_panic();
    }

    /* Clear address context. */
    se->SE_IN_LL_ADDR = 0;
    se->SE_OUT_LL_ADDR = 0;
    if (se->SE_IN_LL_ADDR != 0 || se->SE_OUT_LL_ADDR != 0) {
        generic_panic();
    }

    trigger_se_blocking_op(OP_START, NULL, 0, wrapped_key, wrapped_key_size);

    /* Validate address context. */
    if (se->SE_IN_LL_ADDR == 0 || se->SE_OUT_LL_ADDR == 0) {
        generic_panic();
    }
}

void se_synchronous_exp_mod(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    volatile tegra_se_t *se = se_get_regs();
    uint8_t ALIGN(16) stack_buf[KEYSIZE_RSA_MAX];

    if (keyslot >= KEYSLOT_RSA_MAX || src_size > KEYSIZE_RSA_MAX || dst_size > KEYSIZE_RSA_MAX) {
        generic_panic();
    }

    /* Endian swap the input. */
    for (size_t i = 0; i < src_size; i++) {
        stack_buf[i] = *((uint8_t *)src + src_size - i - 1);
    }

    se->SE_CONFIG = (ALG_RSA | DST_RSAREG);
    se->SE_RSA_CONFIG = keyslot << 24;
    se->SE_RSA_KEY_SIZE = (g_se_modulus_sizes[keyslot] >> 6) - 1;
    se->SE_RSA_EXP_SIZE = g_se_exp_sizes[keyslot] >> 2;

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
        *p_out = read32be(se_get_regs()->SE_RSA_OUTPUT, offset);
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
    volatile tegra_se_t *se = se_get_regs();
    se_ll_t in_ll;
    se_ll_t out_ll;

    ll_init(&in_ll, (void *)src, src_size);
    ll_init(&out_ll, dst, dst_size);

    /* Set the LLs. */
    se->SE_IN_LL_ADDR = (uint32_t) get_physical_address(&in_ll);
    se->SE_OUT_LL_ADDR = (uint32_t) get_physical_address(&out_ll);

    /* Set registers for operation. */
    se->SE_ERR_STATUS = se->SE_ERR_STATUS;
    se->SE_INT_STATUS = se->SE_INT_STATUS;

    if (se->SE_IN_LL_ADDR != (uint32_t) get_physical_address(&in_ll) || se->SE_OUT_LL_ADDR != (uint32_t) get_physical_address(&out_ll) || (se->SE_INT_STATUS & 0x10) || (se->SE_STATUS & 0x3)) {
        generic_panic();
    }

    se->SE_OPERATION = op;

    while (!(se->SE_INT_STATUS & 0x10)) { /* Wait a while */ }
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
    se_get_regs()->SE_CRYPTO_LAST_BLOCK = 0;
    trigger_se_blocking_op(OP_START, block, sizeof(block), block, sizeof(block));

    /* Copy output data into dst. */
    if (dst_size != 0) {
        memcpy(dst, block, dst_size);
    }
}

void se_aes_ctr_crypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX || ctr_size != 0x10) {
        generic_panic();
    }
    unsigned int num_blocks = src_size >> 4;

    /* Unknown what this write does, but official code writes it for CTR mode. */
    se->SE_SPARE = 1;
    se->SE_CONFIG = (ALG_AES_ENC | DST_MEMORY);
    se->SE_CRYPTO_CONFIG = (keyslot << 24) | 0x91E;
    set_se_ctr(ctr);

    /* Handle any aligned blocks. */
    size_t aligned_size = (size_t)num_blocks << 4;
    if (aligned_size) {
        se->SE_CRYPTO_LAST_BLOCK = num_blocks - 1;
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
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX || dst_size != 0x10 || src_size != 0x10) {
        generic_panic();
    }

    /* Set configuration high (256-bit vs 128-bit) based on parameter. */
    se->SE_CONFIG = (ALG_AES_ENC | DST_MEMORY) | (config_high << 16);
    se->SE_CRYPTO_CONFIG = keyslot << 24 | 0x100;
    se_perform_aes_block_operation(dst, 0x10, src, 0x10);
}

void se_aes_128_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    se_aes_ecb_encrypt_block(keyslot, dst, dst_size, src, src_size, 0);
}

void se_aes_256_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    se_aes_ecb_encrypt_block(keyslot, dst, dst_size, src, src_size, 0x202);
}

void se_aes_ecb_decrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX || dst_size != 0x10 || src_size != 0x10) {
        generic_panic();
    }

    se->SE_CONFIG = (ALG_AES_DEC | DST_MEMORY);
    se->SE_CRYPTO_CONFIG = keyslot << 24;
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

void se_compute_aes_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size, unsigned int config_high) {
    volatile tegra_se_t *se = se_get_regs();

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

    se->SE_CONFIG = (ALG_AES_ENC | DST_HASHREG) | (config_high << 16);
    se->SE_CRYPTO_CONFIG = (keyslot << 24) | (0x145);
    clear_aes_keyslot_iv(keyslot);

    unsigned int num_blocks = (data_size + 0xF) >> 4;
    /* Handle aligned blocks. */
    if (num_blocks > 1) {
        se->SE_CRYPTO_LAST_BLOCK = num_blocks - 2;
        trigger_se_blocking_op(OP_START, NULL, 0, data, data_size);
        se->SE_CRYPTO_CONFIG |= 0x80;
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
    se->SE_CRYPTO_LAST_BLOCK = 0;
    trigger_se_blocking_op(OP_START, NULL, 0, last_block, sizeof(last_block));

    /* Copy output CMAC. */
    for (unsigned int i = 0; i < (cmac_size >> 2); i++) {
        ((uint32_t *)cmac)[i] = read32le(se->SE_HASH_RESULT, i << 2);
    }
}

void se_compute_aes_128_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size) {
    se_compute_aes_cmac(keyslot, cmac, cmac_size, data, data_size, 0);
}
void se_compute_aes_256_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size) {
    se_compute_aes_cmac(keyslot, cmac, cmac_size, data, data_size, 0x202);
}

void se_aes_256_cbc_encrypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *iv) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX || src_size < 0x10) {
        generic_panic();
    }

    se->SE_CONFIG = (ALG_AES_ENC | DST_MEMORY) | (0x202 << 16);
    se->SE_CRYPTO_CONFIG = (keyslot << 24) | 0x144;
    set_aes_keyslot_iv(keyslot, iv, 0x10);
    se->SE_CRYPTO_LAST_BLOCK = (src_size >> 4) - 1;
    trigger_se_blocking_op(OP_START, dst, dst_size, src, src_size);
}

void se_aes_128_cbc_decrypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX || src_size < 0x10) {
        generic_panic();
    }

    se->SE_CONFIG = (ALG_AES_DEC | DST_MEMORY) | (0x000 << 16);
    se->SE_CRYPTO_CONFIG = (keyslot << 24) | 0x66;
    clear_aes_keyslot_iv(keyslot);
    se->SE_CRYPTO_LAST_BLOCK = (src_size >> 4) - 1;
    trigger_se_blocking_op(OP_START, dst, dst_size, src, src_size);
}

/* SHA256 Implementation. */
void se_calculate_sha256(void *dst, const void *src, size_t src_size) {
    volatile tegra_se_t *se = se_get_regs();

    /* Setup config for SHA256, size = BITS(src_size) */
    se->SE_CONFIG = (ENCMODE_SHA256 | ALG_SHA | DST_HASHREG);
    se->SE_SHA_CONFIG = 1;
    se->SE_SHA_MSG_LENGTH[0] = (uint32_t)(src_size << 3);
    se->SE_SHA_MSG_LENGTH[1] = 0;
    se->SE_SHA_MSG_LENGTH[2] = 0;
    se->SE_SHA_MSG_LENGTH[3] = 0;
    se->SE_SHA_MSG_LEFT[0] = (uint32_t)(src_size << 3);
    se->SE_SHA_MSG_LEFT[1] = 0;
    se->SE_SHA_MSG_LEFT[2] = 0;
    se->SE_SHA_MSG_LEFT[3] = 0;

    /* Trigger the operation. */
    trigger_se_blocking_op(OP_START, NULL, 0, src, src_size);

    /* Copy output hash. */
    for (unsigned int i = 0; i < (0x20 >> 2); i++) {
        ((uint32_t *)dst)[i] = read32be(se->SE_HASH_RESULT, i << 2);
    }
}

/* RNG API */
void se_initialize_rng(unsigned int keyslot) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    /* To initialize the RNG, we'll perform an RNG operation into an output buffer. */
    /* This will be discarded, when done. */
    uint8_t ALIGN(16) output_buf[0x10];

    se->SE_RNG_SRC_CONFIG = 3; /* Entropy enable + Entropy lock enable */
    se->SE_RNG_RESEED_INTERVAL = 70001;
    se->SE_CONFIG = (ALG_RNG | DST_MEMORY);
    se->SE_CRYPTO_CONFIG = (keyslot << 24) | 0x108;
    se->SE_RNG_CONFIG = 5;
    se->SE_CRYPTO_LAST_BLOCK = 0;
    trigger_se_blocking_op(OP_START, output_buf, 0x10, NULL, 0);
}

void se_generate_random(unsigned int keyslot, void *dst, size_t size) {
    volatile tegra_se_t *se = se_get_regs();

    if (keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    uint32_t num_blocks = size >> 4;
    size_t aligned_size = num_blocks << 4;
    se->SE_CONFIG = (ALG_RNG | DST_MEMORY);
    se->SE_CRYPTO_CONFIG = (keyslot << 24) | 0x108;
    se->SE_RNG_CONFIG = 4;

    if (num_blocks >= 1) {
        se->SE_CRYPTO_LAST_BLOCK = num_blocks - 1;
        trigger_se_blocking_op(OP_START, dst, aligned_size, NULL, 0);
    }
    if (size > aligned_size) {
        se_perform_aes_block_operation(dst + aligned_size, size - aligned_size, NULL, 0);
    }
}

void se_generate_random_key(unsigned int dst_keyslot, unsigned int rng_keyslot) {
    volatile tegra_se_t *se = se_get_regs();

    if (dst_keyslot >= KEYSLOT_AES_MAX || rng_keyslot >= KEYSLOT_AES_MAX) {
        generic_panic();
    }

    /* Setup Config. */
    se->SE_CONFIG = (ALG_RNG | DST_KEYTAB);
    se->SE_CRYPTO_CONFIG = (rng_keyslot << 24) | 0x108;
    se->SE_RNG_CONFIG = 4;
    se->SE_CRYPTO_LAST_BLOCK = 0;

    /* Generate low part of key. */
    se->SE_CRYPTO_KEYTABLE_DST = (dst_keyslot << 8);
    trigger_se_blocking_op(OP_START, NULL, 0, NULL, 0);
    /* Generate high part of key. */
    se->SE_CRYPTO_KEYTABLE_DST = (dst_keyslot << 8) | 1;
    trigger_se_blocking_op(OP_START, NULL, 0, NULL, 0);
}

/* SE context save API. */
void se_set_in_context_save_mode(bool is_context_save_mode) {
    volatile tegra_se_t *se = se_get_regs();

    uint32_t val = se->SE_SE_SECURITY;
    if (is_context_save_mode) {
        val |= 0x10000;
    } else {
        val &= 0xFFFEFFFF;
    }
    se->SE_SE_SECURITY = val;
    /* Perform a useless read from flags reg. */
    (void)(se->SE_STATUS);
}

void se_generate_srk(unsigned int srkgen_keyslot) {
    volatile tegra_se_t *se = se_get_regs();

    se->SE_CONFIG = (ALG_RNG | DST_SRK);
    se->SE_CRYPTO_CONFIG = (srkgen_keyslot << 24) | 0x108;
    se->SE_RNG_CONFIG = 6;
    se->SE_CRYPTO_LAST_BLOCK = 0;
    trigger_se_blocking_op(OP_START, NULL, 0, NULL, 0);
}

void se_encrypt_with_srk(void *dst, size_t dst_size, const void *src, size_t src_size) {
    uint8_t output[0x80];
    uint8_t *aligned_out = (uint8_t *)(((uintptr_t)output + 0x7F) & ~0x3F);
    if (dst_size > 0x10) {
        generic_panic();
    }

    if (dst_size) {
        trigger_se_blocking_op(OP_CTX_SAVE, aligned_out, dst_size, src, src_size);
        memcpy(dst, aligned_out, dst_size);
    } else {
        trigger_se_blocking_op(OP_CTX_SAVE, aligned_out, 0, src, src_size);
    }
}

void se_save_context(unsigned int srkgen_keyslot, unsigned int rng_keyslot, void *dst) {
    volatile tegra_se_t *se = se_get_regs();
    uint8_t _work_buf[0x80];
    uint8_t *work_buf = (uint8_t *)(((uintptr_t)_work_buf + 0x7F) & ~0x3F);

    /* Generate the SRK (context save encryption key). */
    se_generate_random_key(srkgen_keyslot, rng_keyslot);
    se_generate_srk(srkgen_keyslot);

    se_generate_random(rng_keyslot, work_buf, 0x10);

    /* Save random initial block. */
    se->SE_CONFIG = (ALG_AES_ENC | DST_MEMORY);
    se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_MEM);
    se->SE_CRYPTO_LAST_BLOCK = 0;
    se_encrypt_with_srk(dst, 0x10, work_buf, 0x10);

    /* Save Sticky Bits. */
    for (unsigned int i = 0; i < 0x2; i++) {
        se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_STICKY_BITS) | (i << CTX_SAVE_STICKY_BIT_INDEX_SHIFT);
        se->SE_CRYPTO_LAST_BLOCK = 0;
        se_encrypt_with_srk(dst + 0x10 + (i * 0x10), 0x10, NULL, 0);
    }

    /* Save AES Key Table. */
    for (unsigned int i = 0; i < KEYSLOT_AES_MAX; i++) {
        se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_KEYTABLE_AES) | (i << CTX_SAVE_KEY_INDEX_SHIFT) | (CTX_SAVE_KEY_LOW_BITS);
        se->SE_CRYPTO_LAST_BLOCK = 0;
        se_encrypt_with_srk(dst + 0x30 + (i * 0x20), 0x10, NULL, 0);
        se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_KEYTABLE_AES) | (i << CTX_SAVE_KEY_INDEX_SHIFT) | (CTX_SAVE_KEY_HIGH_BITS);
        se->SE_CRYPTO_LAST_BLOCK = 0;
        se_encrypt_with_srk(dst + 0x40 + (i * 0x20), 0x10, NULL, 0);
    }

    /* Save AES Original IVs. */
    for (unsigned int i = 0; i < KEYSLOT_AES_MAX; i++) {
        se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_KEYTABLE_AES) | (i << CTX_SAVE_KEY_INDEX_SHIFT) | (CTX_SAVE_KEY_ORIGINAL_IV);
        se->SE_CRYPTO_LAST_BLOCK = 0;
        se_encrypt_with_srk(dst + 0x230 + (i * 0x10), 0x10, NULL, 0);
    }

    /* Save AES Updated IVs */
    for (unsigned int i = 0; i < KEYSLOT_AES_MAX; i++) {
        se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_KEYTABLE_AES) | (i << CTX_SAVE_KEY_INDEX_SHIFT) | (CTX_SAVE_KEY_UPDATED_IV);
        se->SE_CRYPTO_LAST_BLOCK = 0;
        se_encrypt_with_srk(dst + 0x330 + (i * 0x10), 0x10, NULL, 0);
    }

    /* Save RSA Keytable. */
    uint8_t *rsa_ctx_out = (uint8_t *)dst + 0x430;
    for (unsigned int rsa_key = 0; rsa_key < KEYSLOT_RSA_MAX; rsa_key++) {
        for (unsigned int mod_exp = 0; mod_exp < 2; mod_exp++) {
            for (unsigned int sub_block = 0; sub_block < 0x10; sub_block++) {
                se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_KEYTABLE_RSA) | ((2 * rsa_key + (1 - mod_exp)) << CTX_SAVE_RSA_KEY_INDEX_SHIFT) | (sub_block << CTX_SAVE_RSA_KEY_BLOCK_INDEX_SHIFT);
                se->SE_CRYPTO_LAST_BLOCK = 0;
                se_encrypt_with_srk(rsa_ctx_out, 0x10, NULL, 0);
                rsa_ctx_out += 0x10;
            }
        }
    }

    /* Save "Known Pattern. " */
    static const uint8_t context_save_known_pattern[0x10] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
    se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_MEM);
    se->SE_CRYPTO_LAST_BLOCK = 0;
    se_encrypt_with_srk(dst + 0x830, 0x10, context_save_known_pattern, 0x10);

    /* Save SRK into PMC registers. */
    se->SE_CTX_SAVE_CONFIG = (CTX_SAVE_SRC_SRK);
    se->SE_CRYPTO_LAST_BLOCK = 0;
    se_encrypt_with_srk(work_buf, 0, NULL, 0);
    se->SE_CONFIG = 0;
    se_encrypt_with_srk(work_buf, 0, NULL, 0);
}
