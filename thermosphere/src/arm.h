#pragma once

#include "types.h"

void flush_dcache_all(void);
void invalidate_dcache_all(void);

void flush_dcache_range(const void *start, const void *end);
void invalidate_dcache_range(const void *start, const void *end);

void invalidate_icache_all_inner_shareable(void);
void invalidate_icache_all(void);

void set_memory_registers_enable_mmu(uintptr_t ttbr0, u64 tcr, u64 mair);
