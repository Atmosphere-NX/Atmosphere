/*
 * Copyright (c) 2018 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
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

void invalidate_icache_all_inner_shareable(void);
void invalidate_icache_all(void);

void finalize_powerdown(void);
void call_with_stack_pointer(uintptr_t stack_pointer, void (*function)(void));

#endif
