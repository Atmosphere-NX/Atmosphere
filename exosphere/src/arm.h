#ifndef EXOSPHERE_ARM_H
#define EXOSPHERE_ARM_H

#include <stdint.h>

void tlb_invalidate_all(void);
void tlb_invalidate_all_inner_shareable(void);

void tlb_invalidate_page(const volatile void *page);
void tlb_invalidate_page_inner_shareable(const void *page);

void flush_dcache_all(void);
void invalidate_dcache_all(void);

void flush_dcache_range(const void *start, const void *end);
void invalidate_dcache_range(const void *start, const void *end);

void invalidate_icache_inner_shareable(void);


void call_with_stack_pointer(uintptr_t stack_pointer, void (*function)(void));

#endif
