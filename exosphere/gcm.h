#ifndef EXOSPHERE_GCM_H
#define EXOSPHERE_GCM_H

#include <stdint.h>

size_t gcm_decrypt_key(unsigned int keyslot, void *dst, size_t dst_size, const void *src, size_t src_size, const void *sealed_kek, size_t kek_size, const void *wrapped_key, size_t key_size, unsigned int usecase, int is_personalized);

#endif
