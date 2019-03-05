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
 
#ifndef EXOSPHERE_WARMBOOT_BIN_SE_H
#define EXOSPHERE_WARMBOOT_BIN_SE_H

#define SE_BASE 0x70012000
#define MAKE_SE_REG(n) MAKE_REG32(SE_BASE + n)

#define KEYSLOT_SWITCH_LP0TZRAMKEY 0x2
#define KEYSLOT_SWITCH_SRKGENKEY 0x8
#define KEYSLOT_SWITCH_PACKAGE2KEY 0x8
#define KEYSLOT_SWITCH_TEMPKEY 0x9
#define KEYSLOT_SWITCH_SESSIONKEY 0xA
#define KEYSLOT_SWITCH_RNGKEY 0xB
#define KEYSLOT_SWITCH_MASTERKEY 0xC
#define KEYSLOT_SWITCH_DEVICEKEY 0xD

/* This keyslot was added in 4.0.0. */
#define KEYSLOT_SWITCH_4XNEWDEVICEKEYGENKEY 0xD
#define KEYSLOT_SWITCH_4XNEWCONSOLEKEYGENKEY 0xE
#define KEYSLOT_SWITCH_4XOLDDEVICEKEY 0xF

/* This keyslot was added in 5.0.0. */
#define KEYSLOT_SWITCH_5XNEWDEVICEKEYGENKEY 0xA

#define KEYSLOT_AES_MAX 0x10
#define KEYSLOT_RSA_MAX 0x2

#define KEYSIZE_AES_MAX 0x20
#define KEYSIZE_RSA_MAX 0x100

#define ALG_SHIFT (12)
#define ALG_DEC_SHIFT (8)
#define ALG_NOP (0 << ALG_SHIFT)
#define ALG_AES_ENC (1 << ALG_SHIFT)
#define ALG_AES_DEC ((1 << ALG_DEC_SHIFT) | ALG_NOP)
#define ALG_RNG (2 << ALG_SHIFT)
#define ALG_SHA (3 << ALG_SHIFT)
#define ALG_RSA (4 << ALG_SHIFT)

#define DST_SHIFT (2)
#define DST_MEMORY (0 << DST_SHIFT)
#define DST_HASHREG (1 << DST_SHIFT)
#define DST_KEYTAB (2 << DST_SHIFT)
#define DST_SRK (3 << DST_SHIFT)
#define DST_RSAREG (4 << DST_SHIFT)

#define ENCMODE_SHIFT (24)
#define DECMODE_SHIFT (16)
#define ENCMODE_SHA256 (5 << ENCMODE_SHIFT)

#define HASH_DISABLE (0x0)
#define HASH_ENABLE (0x1)

#define OP_ABORT 0
#define OP_START 1
#define OP_RESTART 2
#define OP_CTX_SAVE 3
#define OP_RESTART_IN 4

#define CTX_SAVE_SRC_SHIFT 29
#define CTX_SAVE_SRC_STICKY_BITS (0 << CTX_SAVE_SRC_SHIFT)
#define CTX_SAVE_SRC_KEYTABLE_AES (2 << CTX_SAVE_SRC_SHIFT)
#define CTX_SAVE_SRC_KEYTABLE_RSA (1 << CTX_SAVE_SRC_SHIFT)
#define CTX_SAVE_SRC_MEM (4 << CTX_SAVE_SRC_SHIFT)
#define CTX_SAVE_SRC_SRK (6 << CTX_SAVE_SRC_SHIFT)

#define CTX_SAVE_KEY_LOW_BITS 0
#define CTX_SAVE_KEY_HIGH_BITS 1
#define CTX_SAVE_KEY_ORIGINAL_IV 2
#define CTX_SAVE_KEY_UPDATED_IV 3

#define CTX_SAVE_STICKY_BIT_INDEX_SHIFT 24
#define CTX_SAVE_KEY_INDEX_SHIFT 8
#define CTX_SAVE_RSA_KEY_INDEX_SHIFT 16
#define CTX_SAVE_RSA_KEY_BLOCK_INDEX_SHIFT 12

#define RSA_2048_BYTES 0x100

typedef struct {
    uint32_t _0x0;
    uint32_t _0x4;
    uint32_t OPERATION_REG;
    uint32_t INT_ENABLE_REG;
    uint32_t INT_STATUS_REG;
    uint32_t CONFIG_REG;
    uint32_t IN_LL_ADDR_REG;
    uint32_t _0x1C;
    uint32_t _0x20;
    uint32_t OUT_LL_ADDR_REG;
    uint32_t _0x28;
    uint32_t _0x2C;
    uint8_t HASH_RESULT_REG[0x20];
    uint8_t _0x50[0x20];
    uint32_t CONTEXT_SAVE_CONFIG_REG;
    uint8_t _0x74[0x18C];
    uint32_t SHA_CONFIG_REG;
    uint32_t SHA_MSG_LENGTH_REG;
    uint32_t _0x208;
    uint32_t _0x20C;
    uint32_t _0x210;
    uint32_t SHA_MSG_LEFT_REG;
    uint32_t _0x218;
    uint32_t _0x21C;
    uint32_t _0x220;
    uint32_t _0x224;
    uint8_t _0x228[0x58];
    uint32_t AES_KEY_READ_DISABLE_REG;
    uint32_t AES_KEYSLOT_FLAGS[0x10];
    uint8_t _0x2C4[0x3C];
    uint32_t _0x300;
    uint32_t CRYPTO_REG;
    uint32_t CRYPTO_CTR_REG[4];
    uint32_t BLOCK_COUNT_REG;
    uint32_t AES_KEYTABLE_ADDR;
    uint32_t AES_KEYTABLE_DATA;
    uint32_t _0x324;
    uint32_t _0x328;
    uint32_t _0x32C;
    uint32_t CRYPTO_KEYTABLE_DST_REG;
    uint8_t _0x334[0xC];
    uint32_t RNG_CONFIG_REG;
    uint32_t RNG_SRC_CONFIG_REG;
    uint32_t RNG_RESEED_INTERVAL_REG;
    uint8_t _0x34C[0xB4];
    uint32_t RSA_CONFIG;
    uint32_t RSA_KEY_SIZE_REG;
    uint32_t RSA_EXP_SIZE_REG;
    uint32_t RSA_KEY_READ_DISABLE_REG;
    uint32_t RSA_KEYSLOT_FLAGS[2];
    uint32_t _0x418;
    uint32_t _0x41C;
    uint32_t RSA_KEYTABLE_ADDR;
    uint32_t RSA_KEYTABLE_DATA;
    uint8_t RSA_OUTPUT[0x100];
    uint8_t _0x528[0x2D8];
    uint32_t FLAGS_REG;
    uint32_t ERR_STATUS_REG;
    uint32_t _0x808;
    uint32_t SPARE_0;
    uint32_t _0x810;
    uint32_t _0x814;
    uint32_t _0x818;
    uint32_t _0x81C;
    uint8_t _0x820[0x17E0];
} tegra_se_t;

typedef struct {
    uint32_t address;
    uint32_t size;
} se_addr_info_t;

typedef struct {
    uint32_t num_entries; /* Set to total entries - 1 */
    se_addr_info_t addr_info; /* This should really be an array...but for our use case it works. */
} se_ll_t;

static inline volatile tegra_se_t *se_get_regs(void) {
    return (volatile tegra_se_t *)SE_BASE;
}

void se_check_error_status_reg(void);
void se_check_for_error(void);

void se_verify_flags_cleared(void);

void clear_aes_keyslot(unsigned int keyslot);
void clear_rsa_keyslot(unsigned int keyslot);
void clear_aes_keyslot_iv(unsigned int keyslot);

void se_compute_aes_256_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size);
void se_aes_256_cbc_decrypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);

#endif
