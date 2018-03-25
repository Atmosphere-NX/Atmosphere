#ifndef EXOSPHERE_GCM_H
#define EXOSPHERE_GCM_H

#include <stdbool.h>
#include <stdint.h>

size_t gcm_decrypt_key(void *dst, size_t dst_size,
                       const void *src, size_t src_size,
                       const void *sealed_kek, size_t kek_size,
                       const void *wrapped_key, size_t key_size,
                       unsigned int usecase, bool is_personalized, 
                       uint8_t *out_deviceid_high);


void gcm_encrypt_key(void *dst, size_t dst_size,
                       const void *src, size_t src_size,
                       const void *sealed_kek, size_t kek_size,
                       const void *wrapped_key, size_t key_size,
                       unsigned int usecase, uint64_t deviceid_high);

#endif
