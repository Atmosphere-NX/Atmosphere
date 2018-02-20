#ifndef EXOSPHERE_TITLEKEY_H
#define EXOSPHERE_TITLEKEY_H

#include <stdint.h>

void tkey_set_expected_db_prefix(uint64_t *db_prefix);
void tkey_set_master_key_rev(unsigned int master_key_rev);

size_t tkey_rsa_unwrap(void *dst, size_t dst_size, void *src, size_t src_size);

void tkey_aes_unwrap(void *dst, size_t dst_size, const void *src, size_t src_size);

#endif