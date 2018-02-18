#include <stdint.h>

#include "utils.h"
#include "se.h"

void trigger_se_rsa_op(void *buf, unsigned int size);
void trigger_se_aes_op(unsigned int op, char *dst, unsigned int dst_size, const unsigned char *src, unsigned int src_size);

/* Globals for driver. */
volatile security_engine_t *g_security_engine;

unsigned int (*g_se_callback)(void);

unsigned int g_se_modulus_sizes[KEYSLOT_RSA_MAX];
unsigned int g_se_exp_sizes[KEYSLOT_RSA_MAX];

/* Set the global security engine pointer. */
void set_security_engine_address(security_engine_t *security_engine) {
    g_security_engine = security_engine;
}

/* Get the global security engine pointer. */
security_engine_t *get_security_engine_address(void) {
    return g_security_engine;
}

void set_security_engine_callback(unsigned int (*callback)(void)) {
    if (callback == NULL || g_se_callback != NULL) {
        panic();
    }
    
    g_se_callback = callback;
}

/* Set the flags for an AES keyslot. */
void set_aes_keyslot_flags(unsigned int keyslot, unsigned int flags) {
    if (g_security_engine == NULL || keyslot >= KEYSLOT_AES_MAX) {
        panic();
    }
    
    /* Misc flags. */
    if (flags & ~0x80) {
        g_security_engine->AES_KEYSLOT_FLAGS[keyslot] = ~flags;
    }
    
    /* Disable keyslot reads. */
    if (flags & 0x80) {
        g_security_engine->AES_KEY_READ_DISABLE_REG &= ~(1 << keyslot);
    }
}

/* Set the flags for an RSA keyslot. */
void set_rsa_keyslot_flags(unsigned int keyslot, unsigned int flags) {
    if (g_security_engine == NULL || keyslot >= KEYSLOT_RSA_MAX) {
        panic();
    }
    
    /* Misc flags. */
    if (flags & ~0x80) {
        /* TODO: Why are flags assigned this way? */
        g_security_engine->RSA_KEYSLOT_FLAGS[keyslot] = (((flags >> 4) & 4) | (flags & 3)) ^ 7;
    }
    
    /* Disable keyslot reads. */
    if (flags & 0x80) {
        g_security_engine->RSA_KEY_READ_DISABLE_REG &= ~(1 << keyslot);
    }
}

void clear_aes_keyslot(unsigned int keyslot) {
    if (g_security_engine == NULL || keyslot >= KEYSLOT_AES_MAX) {
        panic();
    }
    
    /* Zero out the whole keyslot and IV. */
    for (unsigned int i = 0; i < 0x10; i++) {
        g_security_engine->AES_KEYTABLE_ADDR = (keyslot << 4) | i;
        g_security_engine->AES_KEYTABLE_DATA = 0;
    }
}

void clear_rsa_keyslot(unsigned int keyslot) {
    if (g_security_engine == NULL || keyslot >= KEYSLOT_RSA_MAX) {
        panic();
    }
    
    /* Zero out the whole keyslot. */
    for (unsigned int i = 0; i < 0x40; i++) {
        /* Select Keyslot Modulus[i] */
        g_security_engine->RSA_KEYTABLE_ADDR = (keyslot << 7) | i | 0x40;
        g_security_engine->RSA_KEYTABLE_DATA = 0;
    }
    for (unsigned int i = 0; i < 0x40; i++) {
        /* Select Keyslot Expontent[i] */
        g_security_engine->RSA_KEYTABLE_ADDR = (keyslot << 7) | i;
        g_security_engine->RSA_KEYTABLE_DATA = 0;
    }
}

void set_aes_keyslot(unsigned int keyslot, const unsigned char *key, unsigned int key_size) {
    if (g_security_engine == NULL || keyslot >= KEYSLOT_AES_MAX || key_size > KEYSIZE_AES_MAX) {
        panic();
    }
    
    for (unsigned int i = 0; i < (key_size >> 2); i++) {
        g_security_engine->AES_KEYTABLE_ADDR = (keyslot << 4) | i;
        g_security_engine->AES_KEYTABLE_DATA = read32le(key, 4 * i);
    }
}

void set_rsa_keyslot(unsigned int keyslot, const unsigned char *modulus, unsigned int modulus_size, const unsigned char *exp, unsigned int exp_size) {
    if (g_security_engine == NULL || keyslot >= KEYSLOT_RSA_MAX || modulus_size > KEYSIZE_RSA_MAX || exp_size > KEYSIZE_RSA_MAX) {
        panic();
    }
    
    for (unsigned int i = 0; i < (modulus_size >> 2); i++) {
        g_security_engine->RSA_KEYTABLE_ADDR = (keyslot << 7) | 0x40 | i;
        g_security_engine->RSA_KEYTABLE_DATA = read32be(modulus, 4 * i);
    }
    
    for (unsigned int i = 0; i < (exp_size >> 2); i++) {
        g_security_engine->RSA_KEYTABLE_ADDR = (keyslot << 7) | i;
        g_security_engine->RSA_KEYTABLE_DATA = read32be(exp, 4 * i);
    }
    
    g_se_modulus_sizes[keyslot] = modulus_size;
    g_se_exp_sizes[keyslot] = exp_size;
}


void set_aes_keyslot_iv(unsigned int keyslot, const unsigned char *iv, unsigned int iv_size) {
    if (g_security_engine == NULL || keyslot >= KEYSLOT_AES_MAX || iv_size > 0x10) {
        panic();
    }
    
    for (unsigned int i = 0; i < (iv_size >> 2); i++) {
        g_security_engine->AES_KEYTABLE_ADDR = (keyslot << 4) | 8 | i;
        g_security_engine->AES_KEYTABLE_DATA = read32le(iv, 4 * i);
    }
}

void set_se_ctr(const char *ctr) {
    if (g_security_engine == NULL) {
        panic();
    }
    
    for (unsigned int i = 0; i < 4; i++) {
        g_security_engine->CRYPTO_CTR_REG[i] = read32le(ctr, i * 4);
    }
}

void decrypt_data_into_keyslot(unsigned int keyslot_dst, unsigned int keyslot_src, const unsigned char *wrapped_key, unsigned int wrapped_key_size) {
    if (g_security_engine == NULL || keyslot_dst >= KEYSLOT_AES_MAX || keyslot_src >= KEYSIZE_AES_MAX || wrapped_key_size > KEYSIZE_AES_MAX) {
        panic();
    }
    
    g_security_engine->CONFIG_REG = (ALG_AES_DEC | DST_KEYTAB);
    g_security_engine->CRYPTO_REG = keyslot_src << 24;
    g_security_engine->BLOCK_COUNT_REG = 0;
    g_se_callback->CRYPTO_KEYTABLE_DST_REG = keyslot_dst << 8;
    
    /* TODO: Cache flush the wrapped key. */
    
    trigger_se_aes_op(OP_START, NULL, 0, wrapped_key, wrapped_key_size);
}


void se_crypt_aes(unsigned int keyslot, unsigned char *dst, unsigned int dst_size, const unsigned char *src, unsigned int src_size, unsigned int config, unsigned int mode, unsigned int (*callback)(void));

void se_exp_mod(unsigned int keyslot, unsigned char *buf, unsigned int size, unsigned int (*callback)(void)) {
    unsigned char stack_buf[KEYSIZE_RSA_MAX];
    
    if (g_security_engine == NULL || keyslot >= KEYSLOT_RSA_MAX || size > KEYSIZE_RSA_MAX) {
        panic();
    }
    
    /* Endian swap the input. */
    for (unsigned int i = size; i > 0; i--) {
        stack_buf[i] = buf[size - i];
    }
    
    /* TODO: Flush cache for stack copy. */
    
    g_security_engine->CONFIG_REG = (ALG_RSA | DST_RSAREG);
    g_security_engine->RSA_CONFIG = keyslot << 24;
    g_security_engine->RSA_KEY_SIZE_REG = (g_se_modulus_sizes[keyslot] >> 6) - 1;
    g_security_engine->RSA_EXP_SIZE_REG = g_se_exp_sizes[keyslot] >> 2;
    
    set_security_engine_callback(callback);
    
    trigger_se_rsa_op(stack_buf, size);
    
    while (!(g_security_engine->INT_STATUS_REG & 2)) { /* Wait a while */ }
}