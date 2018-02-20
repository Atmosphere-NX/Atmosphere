#include <stdint.h>

#include "utils.h"
#include "sealedkeys.h"
#include "se.h"

const uint8_t g_titlekey_seal_key_source[0x10] = {
    0xCB, 0xB7, 0x6E, 0x38, 0xA1, 0xCB, 0x77, 0x0F, 0xB2, 0xA5, 0xB2, 0x9D, 0xD8, 0x56, 0x9F, 0x76
};


const uint8_t g_sealed_key_sources[CRYPTOUSECASE_MAX][0x10] = {
    {0x4D, 0x87, 0x09, 0x86, 0xC4, 0x5D, 0x20, 0x72, 0x2F, 0xBA, 0x10, 0x53, 0xDA, 0x92, 0xE8, 0xA9},
    {0x25, 0x03, 0x31, 0xFB, 0x25, 0x26, 0x0B, 0x79, 0x8C, 0x80, 0xD2, 0x69, 0x98, 0xE2, 0x22, 0x77},
    {0x76, 0x14, 0x1D, 0x34, 0x93, 0x2D, 0xE1, 0x84, 0x24, 0x7B, 0x66, 0x65, 0x55, 0x04, 0x65, 0x81},
    {0xAF, 0x3D, 0xB7, 0xF3, 0x08, 0xA2, 0xD8, 0xA2, 0x08, 0xCA, 0x18, 0xA8, 0x69, 0x46, 0xC9, 0x0B}
};

void seal_key_internal(void *dst, const void *src, const uint8_t *seal_key_source) {
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, KEYSLOT_SWITCH_SESSIONKEY, seal_key_source, 0x10);
    se_aes_ecb_encrypt_block(KEYSLOT_SWITCH_TEMPKEY, dst, 0x10, src, 0x10);
}

void unseal_key_internal(unsigned int keyslot, const void *src, const uint8_t *seal_key_source) {
    decrypt_data_into_keyslot(KEYSLOT_SWITCH_TEMPKEY, KEYSLOT_SWITCH_SESSIONKEY, seal_key_source, 0x10);
    decrypt_data_into_keyslot(keyslot, KEYSLOT_SWITCH_TEMPKEY, src, 0x10);
}


void seal_titlekey(void *dst, size_t dst_size, const void *src, size_t src_size) {
    if (usecase >= CRYPTOUSECASE_MAX || dst_size != 0x10 || src_size != 0x10) {
        panic();
    }
    
    seal_key_internal(dst, src, g_titlekey_seal_key_source);

}

void unseal_titlekey(unsigned int keyslot, const void *src, size_t src_size) {
    if (src_size != 0x10) {
        panic();
    }
    
    unseal_key_internal(keyslot, src, g_titlekey_seal_key_source);
}


void seal_key(void *dst, size_t dst_size, const void *src, size_t src_size, unsigned int usecase) {
    if (usecase >= CRYPTOUSECASE_MAX || dst_size != 0x10 || src_size != 0x10) {
        panic();
    }
    
    
    seal_key_internal(dst, src, g_sealed_key_sources[usecase]);
}

void unseal_key(unsigned int keyslot, const void *src, size_t src_size, unsigned int usecase) {
    if (usecase >= CRYPTOUSECASE_MAX || src_size != 0x10) {
        panic();
    }
    
    seal_key_internal(dst, src, g_sealed_key_sources[usecase]);
}