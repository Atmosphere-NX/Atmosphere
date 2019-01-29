#pragma once

/* Based on linux source code */

#ifdef __cplusplus
extern "C" {
#endif

#include <switch/types.h>

#define SHA256_DIGEST_SIZE      32
#define SHA256_BLOCK_SIZE       64

#define SHA256_H0   0x6a09e667UL
#define SHA256_H1   0xbb67ae85UL
#define SHA256_H2	0x3c6ef372UL
#define SHA256_H3	0xa54ff53aUL
#define SHA256_H4	0x510e527fUL
#define SHA256_H5	0x9b05688cUL
#define SHA256_H6	0x1f83d9abUL
#define SHA256_H7	0x5be0cd19UL

struct sha256_state {
	u32 state[SHA256_DIGEST_SIZE / 4];
	u64 count;
	u8 buf[SHA256_BLOCK_SIZE];
};

int sha256_init(struct sha256_state *sctx);
int sha256_update(struct sha256_state *sctx, const void *data, size_t len);
int sha256_finalize(struct sha256_state *sctx);
int sha256_finish(struct sha256_state *sctx, void *out);

#ifdef __cplusplus
}
#endif
