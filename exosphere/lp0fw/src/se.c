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
#include "lp0.h"
#include "se.h"

static void trigger_se_blocking_op(unsigned int op, void *dst, size_t dst_size, const void *src, size_t src_size);

/* Initialize a SE linked list. */
static void __attribute__((__noinline__)) ll_init(volatile se_ll_t *ll, void *buffer, size_t size) {
    ll->num_entries = 0; /* 1 Entry. */

    if (buffer != NULL) {
        ll->addr_info.address = (uint32_t) buffer;
        ll->addr_info.size = (uint32_t) size;
    } else {
        ll->addr_info.address = 0;
        ll->addr_info.size = 0;
    }
}

void se_check_error_status_reg(void) {
    if (se_get_regs()->SE_ERR_STATUS) {
        reboot();
    }
}

void se_check_for_error(void) {
    volatile tegra_se_t *se = se_get_regs();
    if (se->SE_INT_STATUS & 0x10000 || se->SE_STATUS & 3 || se->SE_ERR_STATUS) {
        reboot();
    }
}

void se_verify_flags_cleared(void) {
    if (se_get_regs()->SE_STATUS & 3) {
        reboot();
    }
}

void clear_aes_keyslot(unsigned int keyslot) {
    volatile tegra_se_t *se = se_get_regs();
    
    if (keyslot >= KEYSLOT_AES_MAX) {
        reboot();
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
        reboot();
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

void clear_aes_keyslot_iv(unsigned int keyslot) {
    volatile tegra_se_t *se = se_get_regs();
    
    if (keyslot >= KEYSLOT_AES_MAX) {
        reboot();
    }

    for (size_t i = 0; i < (0x10 >> 2); i++) {
        se->SE_CRYPTO_KEYTABLE_ADDR = (keyslot << 4) | 8 | i;
        se->SE_CRYPTO_KEYTABLE_DATA = 0;
    }
}


void trigger_se_blocking_op(unsigned int op, void *dst, size_t dst_size, const void *src, size_t src_size) {
    volatile tegra_se_t *se = se_get_regs();
    se_ll_t in_ll;
    se_ll_t out_ll;

    ll_init(&in_ll, (void *)src, src_size);
    ll_init(&out_ll, dst, dst_size);

    /* Set the LLs. */
    se->SE_IN_LL_ADDR = (uint32_t)(&in_ll);
    se->SE_OUT_LL_ADDR = (uint32_t) (&out_ll);

    /* Set registers for operation. */
    se->SE_ERR_STATUS = se->SE_ERR_STATUS;
    se->SE_INT_STATUS = se->SE_INT_STATUS;
    se->SE_OPERATION = op;

    while (!(se->SE_INT_STATUS & 0x10)) { /* Wait a while */ }
    se_check_for_error();
}

/* Secure AES Functionality. */
void se_perform_aes_block_operation(void *dst, size_t dst_size, const void *src, size_t src_size) {
    uint8_t block[0x10] = {0};

    if (src_size > sizeof(block) || dst_size > sizeof(block)) {
        reboot();
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

void se_aes_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, unsigned int config_high) {
    volatile tegra_se_t *se = se_get_regs();
    
    if (keyslot >= KEYSLOT_AES_MAX || dst_size != 0x10 || src_size != 0x10) {
        reboot();
    }

    /* Set configuration high (256-bit vs 128-bit) based on parameter. */
    se->SE_CONFIG = (ALG_AES_ENC | DST_MEMORY) | (config_high << 16);
    se->SE_CRYPTO_CONFIG = keyslot << 24 | 0x100;
    se_perform_aes_block_operation(dst, 0x10, src, 0x10);
}

void se_aes_ecb_decrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    volatile tegra_se_t *se = se_get_regs();
    
    if (keyslot >= KEYSLOT_AES_MAX || dst_size != 0x10 || src_size != 0x10) {
        reboot();
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

void se_compute_aes_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size, unsigned int config_high) {
    volatile tegra_se_t *se = se_get_regs();
    
    if (keyslot >= KEYSLOT_AES_MAX) {
        reboot();
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
        ((uint32_t *)cmac)[i] = ((volatile uint32_t *)se->SE_HASH_RESULT)[i];
    }
}

void se_compute_aes_256_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size) {
    se_compute_aes_cmac(keyslot, cmac, cmac_size, data, data_size, 0x202);
}

void se_aes_256_cbc_decrypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size) {
    volatile tegra_se_t *se = se_get_regs();
    
    if (keyslot >= KEYSLOT_AES_MAX || src_size < 0x10) {
        reboot();
    }

    se->SE_CONFIG = (ALG_AES_DEC | DST_MEMORY) | (0x202 << 16);
    se->SE_CRYPTO_CONFIG = (keyslot << 24) | 0x66;
    clear_aes_keyslot_iv(keyslot);
    se->SE_CRYPTO_LAST_BLOCK = (src_size >> 4) - 1;
    trigger_se_blocking_op(OP_START, dst, dst_size, src, src_size);
}