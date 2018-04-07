#ifndef FUSEE_SE_H
#define FUSEE_SE_H

#include "utils.h"
#include <assert.h>

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

typedef struct security_engine {
  unsigned int _0x0;
  unsigned int _0x4;
  unsigned int OPERATION_REG;
  unsigned int INT_ENABLE_REG;
  unsigned int INT_STATUS_REG;
  unsigned int CONFIG_REG;
  unsigned int IN_LL_ADDR_REG;
  unsigned int _0x1C;
  unsigned int _0x20;
  unsigned int OUT_LL_ADDR_REG;
  unsigned int _0x28;
  unsigned int _0x2C;
  unsigned char HASH_RESULT_REG[0x20];
  unsigned char _0x50[0x20];
  unsigned int CONTEXT_SAVE_CONFIG_REG;
  unsigned char _0x74[0x18C];
  unsigned int SHA_CONFIG_REG;
  unsigned int SHA_MSG_LENGTH_REG;
  unsigned int _0x20C;
  unsigned int _0x210;
  unsigned int SHA_MSG_LEFT_REG;
  unsigned int _0x218;
  unsigned int _0x21C;
  unsigned int _0x220;
  unsigned int _0x224;
  unsigned char _0x228[0x5C];
  unsigned int AES_KEY_READ_DISABLE_REG;
  unsigned int AES_KEYSLOT_FLAGS[0x10];
  unsigned char _0x2C4[0x3C];
  unsigned int _0x300;
  unsigned int CRYPTO_REG;
  unsigned int CRYPTO_CTR_REG[4];
  unsigned int BLOCK_COUNT_REG;
  unsigned int AES_KEYTABLE_ADDR;
  unsigned int AES_KEYTABLE_DATA;
  unsigned int _0x324;
  unsigned int _0x328;
  unsigned int _0x32C;
  unsigned int CRYPTO_KEYTABLE_DST_REG;
  unsigned char _0x334[0xC];
  unsigned int RNG_CONFIG_REG;
  unsigned int RNG_SRC_CONFIG_REG;
  unsigned int RNG_RESEED_INTERVAL_REG;
  unsigned char _0x34C[0xB4];
  unsigned int RSA_CONFIG;
  unsigned int RSA_KEY_SIZE_REG;
  unsigned int RSA_EXP_SIZE_REG;
  unsigned int RSA_KEY_READ_DISABLE_REG;
  unsigned int RSA_KEYSLOT_FLAGS[2];
  unsigned int _0x418;
  unsigned int _0x41C;
  unsigned int RSA_KEYTABLE_ADDR;
  unsigned int RSA_KEYTABLE_DATA;
  unsigned char RSA_OUTPUT[0x100];
  unsigned char _0x528[0x2D8];
  unsigned int FLAGS_REG;
  unsigned int ERR_STATUS_REG;
  unsigned int _0x808;
  unsigned int _0x80C;
  unsigned int _0x810;
  unsigned int _0x814;
  unsigned int _0x818;
  unsigned int _0x81C;
  unsigned char _0x820[0x17E0];
} security_engine_t;

static_assert(sizeof(security_engine_t) == 0x2000, "Mis-defined Security Engine Registers!");

typedef struct {
    uint32_t address;
    uint32_t size;
} se_addr_info_t;

typedef struct {
    uint32_t num_entries; /* Set to total entries - 1 */
    se_addr_info_t addr_info; /* This should really be an array...but for our use case it works. */
} se_ll_t;


/* WIP, API subject to change. */

static inline volatile security_engine_t *get_security_engine(void) {
    return (volatile security_engine_t *)0x70012000;
}

#define SECURITY_ENGINE (get_security_engine())

/* This function MUST be registered to fire on the appropriate interrupt. */

void se_check_error_status_reg(void);
void se_check_for_error(void);
void se_trigger_interrupt(void);

void se_validate_stored_vector(void);
void se_generate_stored_vector(void);

void se_verify_flags_cleared(void);

void set_aes_keyslot_flags(unsigned int keyslot, unsigned int flags);
void set_rsa_keyslot_flags(unsigned int keyslot, unsigned int flags);
void clear_aes_keyslot(unsigned int keyslot);
void clear_rsa_keyslot(unsigned int keyslot);

void set_aes_keyslot(unsigned int keyslot, const void *key, size_t key_size);
void decrypt_data_into_keyslot(unsigned int keyslot_dst, unsigned int keyslot_src, const void *wrapped_key, size_t wrapped_key_size);
void set_rsa_keyslot(unsigned int keyslot, const void *modulus, size_t modulus_size, const void *exponent, size_t exp_size);
void set_aes_keyslot_iv(unsigned int keyslot, const void *iv, size_t iv_size);
void set_se_ctr(const void *ctr);

/* Secure AES API */
void se_aes_128_xts_nintendo_decrypt(unsigned int keyslot_1, unsigned int keyslot_2, unsigned int base_sector, void *dst, const void *src, size_t size, unsigned int sector_size);
void se_aes_128_xts_nintendo_encrypt(unsigned int keyslot_1, unsigned int keyslot_2, unsigned int base_sector, void *dst, const void *src, size_t size, unsigned int sector_size);
void se_compute_aes_128_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size);
void se_compute_aes_256_cmac(unsigned int keyslot, void *cmac, size_t cmac_size, const void *data, size_t data_size);
void se_aes_128_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);
void se_aes_256_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);
void se_aes_ctr_crypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size);
void se_aes_ecb_decrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);
void se_aes_256_cbc_encrypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *iv);

/* Hash API */
void se_calculate_sha256(void *dst, const void *src, size_t src_size);

/* RSA API */
void se_get_exp_mod_output(void *buf, size_t size);
void se_synchronous_exp_mod(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);
bool se_rsa2048_pss_verify(const void *signature, size_t signature_size, const void *modulus, size_t modulus_size, const void *data, size_t data_size);

/* RNG API */
void se_initialize_rng(unsigned int keyslot);
void se_generate_random(unsigned int keyslot, void *dst, size_t size);

#endif /* EXOSPHERE_SE_H */
