#ifndef EXOSPHERE_CACHE_H
#define EXOSPHERE_CACHE_H

#include <stdint.h>

void tlb_invalidate_all(void);
void tlb_invalidate_all_inner_shareable(void);

void tlb_invalidate_page(const void *page);
void tlb_invalidate_page_inner_shareable(const void *page);

void flush_dcache_all(void);
void invalidate_dcache_all(void);

void flush_dcache_range(const void *start, const void *end);
void invalidate_dcache_range(const void *start, const void *end);

void invalidate_icache_inner_shareable(void);

#endif
