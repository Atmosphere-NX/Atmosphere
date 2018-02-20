#ifndef EXOSPHERE_SE_H
#define EXOSPHERE_SE_H

#include <stdint.h>
#include <stddef.h>

/* Exosphere driver for the Tegra X1 security engine. */

#define KEYSLOT_SWITCH_TEMPKEY 0x9
#define KEYSLOT_SWITCH_SESSIONKEY 0xA
#define KEYSLOT_SWITCH_RNGKEY 0xB
#define KEYSLOT_SWITCH_MASTERKEY 0xC
#define KEYSLOT_SWITCH_DEVICEKEY 0xD

/* This keyslot was added in 4.0.0. */
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

#define HASH_DISABLE (0x0)
#define HASH_ENABLE (0x1)

#define OP_ABORT 0
#define OP_START 1
#define OP_RESTART 2
#define OP_CTX_SAVE 3
#define OP_RESTART_IN 4


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
  unsigned char cmacOutput[0x10];
  unsigned char reserved0x40[0x240];
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
  unsigned char _0x334[0xCC];
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

typedef struct {
    uint32_t address;
    uint32_t size;
} se_addr_info_t;

typedef struct {
    uint32_t num_entries; /* Set to total entries - 1 */
    se_addr_info_t addr_info; /* This should really be an array...but for our use case it works. */
} se_ll_t;

/* TODO: Define constants for the C driver. */


/* WIP, API subject to change. */


/* This function MUST be registered to fire on the appropriate interrupt. */
void se_operation_completed(void);

void set_security_engine_address(security_engine_t *security_engine);
security_engine_t *get_security_engine_address(void);

void se_check_for_error(void);
void se_trigger_interrupt(void);

void set_aes_keyslot_flags(unsigned int keyslot, unsigned int flags);
void set_rsa_keyslot_flags(unsigned int keyslot, unsigned int flags);
void clear_aes_keyslot(unsigned int keyslot);
void clear_rsa_keyslot(unsigned int keyslot);

void set_aes_keyslot(unsigned int keyslot, const void *key, size_t key_size);
void decrypt_data_into_keyslot(unsigned int keyslot_dst, unsigned int keyslot_src, const void *wrapped_key, size_t wrapped_key_size);
void set_rsa_keyslot(unsigned int keyslot, const void *modulus, size_t modulus_size, const void *exponent, size_t exp_size);
void set_aes_keyslot_iv(unsigned int keyslot, const void *iv, size_t iv_size);
void set_se_ctr(const void *ctr);


/* Insecure AES API */
void se_aes_ctr_crypt_insecure(unsigned int keyslot, uint32_t out_ll_paddr, uint32_t in_ll_paddr, size_t size, const void *ctr, unsigned int (*callback)(void));
void se_aes_cbc_encrypt_insecure(unsigned int keyslot, uint32_t out_ll_paddr, uint32_t in_ll_paddr, size_t size, const void *iv, unsigned int (*callback)(void));
void se_aes_cbc_decrypt_insecure(unsigned int keyslot, uint32_t out_ll_paddr, uint32_t in_ll_paddr, size_t size, const void *iv, unsigned int (*callback)(void));

/* Secure AES API */
void se_compute_aes_128_cmac(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);
void se_compute_aes_256_cmac(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);
void se_aes_ctr_crypt(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *ctr, size_t ctr_size);
void se_aes_ecb_encrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);
void se_aes_ecb_decrypt_block(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size);

/* Hash API */
void se_calculate_sha256(void *dst, const void *src, size_t src_size);

/* RSA API */
void se_exp_mod(unsigned int keyslot, void *buf, size_t size, unsigned int (*callback)(void));
void se_get_exp_mod_output(void *buf, size_t size);

void se_generate_random(unsigned int keyslot, void *dst, size_t size);

/* TODO: SE context save API, consider extending AES API for secure world vs non-secure world operations. */
/* In particular, smc_crypt_aes takes in raw DMA lists, and we need to support that. */

#endif /* EXOSPHERE_SE_H */
