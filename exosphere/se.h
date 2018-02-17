#ifndef EXOSPHERE_SE_H
#define EXOSPHERE_SE_H

/* Exosphere driver for the Tegra X1 security engine. */

/* TODO: Define constants for the C driver. */

/* WIP, API subject to change. */

#define KEYSLOT_AES_MAX 0x10
#define KEYSLOT_RSA_MAX 0x2

void set_security_engine_address(void *security_engine);

void set_aes_keyslot_flags(unsigned int keyslot, unsigned int flags);
void set_rsa_keyslot_flags(unsigned int keyslot, unsigned int flags);
void clear_aes_keyslot(unsigned int keyslot);
void clear_rsa_keyslot(unsigned int keyslot);

void set_aes_keyslot(unsigned int keyslot, const char *key, unsigned int key_size);
void crypt_data_into_keyslot(unsigned int keyslot_dst, unsigned int keyslot_src, const char *wrapped_key, unsigned int wrapped_key_size);
void set_rsa_keyslot(unsigned int keyslot, const char *modulus, unsigned int modulus_size, const char *exp, unsigned int exp_size);
void set_aes_keyslot_iv(unsigned int keyslot, const char *iv, unsigned int iv_size);
void set_se_ctr(const unsigned int *ctr);

void se_crypt_aes(unsigned int keyslot, char *dst, unsigned int dst_size, const char *src, unsigned int src_size, unsigned int config, unsigned int mode, unsigned int (*callback)(void));
void se_exp_mod(unsigned int keyslot, char *buf, unsigned int size, unsigned int (*callback)(void));

/* TODO: SE context save API, consider extending AES API for secure world vs non-secure world operations. */
/* In particular, smc_crypt_aes takes in raw DMA lists, and we need to support that. */

#endif /* EXOSPHERE_SE_H */