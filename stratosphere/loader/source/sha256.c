/* Based on linux source code */
/*
 * sha256_base.h - core logic for SHA-256 implementations
 *
 * Copyright (C) 2015 Linaro Ltd <ard.biesheuvel@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "sha256.h"

#define unlikely(x)    __builtin_expect(!!(x), 0)

void sha256_block_data_order (uint32_t *ctx, const void *in, size_t num);
  
int sha256_init(struct sha256_state *sctx)
{
	sctx->state[0] = SHA256_H0;
	sctx->state[1] = SHA256_H1;
	sctx->state[2] = SHA256_H2;
	sctx->state[3] = SHA256_H3;
	sctx->state[4] = SHA256_H4;
	sctx->state[5] = SHA256_H5;
	sctx->state[6] = SHA256_H6;
	sctx->state[7] = SHA256_H7;
	sctx->count = 0;

	return 0;
}

int sha256_update(struct sha256_state *sctx,
					const void *data,
					size_t len)
{
    const u8 *data8 = (const u8 *)data;
    unsigned int len32 = (unsigned int)len;
	unsigned int partial = sctx->count % SHA256_BLOCK_SIZE;

	sctx->count += len32;

	if (unlikely((partial + len32) >= SHA256_BLOCK_SIZE)) {
		int blocks;

		if (partial) {
			int p = SHA256_BLOCK_SIZE - partial;

			memcpy(sctx->buf + partial, data8, p);
			data8 += p;
			len32 -= p;

			sha256_block_data_order(sctx->state, sctx->buf, 1);
		}

		blocks = len32 / SHA256_BLOCK_SIZE;
		len32 %= SHA256_BLOCK_SIZE;

		if (blocks) {
			sha256_block_data_order(sctx->state, data8, blocks);
			data8 += blocks * SHA256_BLOCK_SIZE;
		}
		partial = 0;
	}
	if (len32)
		memcpy(sctx->buf + partial, data8, len32);

	return 0;
}

int sha256_finalize(struct sha256_state *sctx)
{
	const int bit_offset = SHA256_BLOCK_SIZE - sizeof(u64);
	u64 *bits = (u64 *)(sctx->buf + bit_offset);
	unsigned int partial = sctx->count % SHA256_BLOCK_SIZE;

	sctx->buf[partial++] = 0x80;
	if (partial > bit_offset) {
		memset(sctx->buf + partial, 0x0, SHA256_BLOCK_SIZE - partial);
		partial = 0;

		sha256_block_data_order(sctx->state, sctx->buf, 1);
	}

	memset(sctx->buf + partial, 0x0, bit_offset - partial);
	*bits = __builtin_bswap64(sctx->count << 3);
	sha256_block_data_order(sctx->state, sctx->buf, 1);

	return 0;
}

int sha256_finish(struct sha256_state *sctx, void *out)
{
    unsigned int digest_size = 32;
	u32 *digest = (u32 *)out;
	int i;
	
	// Switch: misalignment shouldn't be a problem here...
	for (i = 0; digest_size > 0; i++, digest_size -= sizeof(u32))
		*digest++ = __builtin_bswap32(sctx->state[i]);

	*sctx = (struct sha256_state){};
	return 0;
}

#ifdef __cplusplus
}
#endif
