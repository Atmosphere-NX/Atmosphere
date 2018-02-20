#ifndef EXOSPHERE_RANDOM_CACHE_H
#define EXOSPHERE_RANDOM_CACHE_H

#include <stdint.h>

/* This method must be called on startup. */
void randomcache_init(void);
void randomcache_refill(void);

void randomcache_getbytes(void *dst, size_t num_bytes);


#endif