/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#pragma once

#include "types.h"
#include "preprocessor.h"
#include "debug_log.h"

#define BIT(n)      (1u   << (n))
#define BITL(n)     (1ull << (n))
#define MASK(n)     (BIT(n) - 1)
#define MASKL(n)    (BITL(n) - 1)
#define MASK2(a,b)  (MASK(a) & ~MASK(b))
#define MASK2L(a,b) (MASKL(a) & ~MASKL(b))

#define MAKE_REG32(a)   (*(volatile u32 *)(uintptr_t)(a))

#define ALIGN(m)        __attribute__((aligned(m)))
#define PACKED          __attribute__((packed))

#define ALINLINE        __attribute__((always_inline))

#define TEMPORARY       __attribute__((section(".tempbss")))

#define FOREACH_BIT(tmpmsk, var, word) for (u64 tmpmsk = (word), var = __builtin_ctzll(word); tmpmsk != 0; tmpmsk &= ~BITL(var), var = __builtin_ctzll(tmpmsk))

#define PANIC(...)      do { DEBUG(__VA_ARGS__); panic(); } while (false)

static inline void __dsb_sy(void)
{
    __asm__ __volatile__ ("dsb sy" ::: "memory");
}

static inline void __isb(void)
{
    __asm__ __volatile__ ("isb" ::: "memory");
}

static inline void __tlb_invalidate_el1_stage12(void)
{
    __asm__ __volatile__ ("tlbi alle1" ::: "memory");
}

bool overlaps(u64 as, u64 ae, u64 bs, u64 be);

static inline uintptr_t get_physical_address_el1_stage12(bool *valid, const uintptr_t el1_vaddr) {
    // NOTE: interrupt must be disabled when calling this func
    uintptr_t PAR;
    __asm__ __volatile__ ("at s12e1r, %0" :: "r"(el1_vaddr)); // note: we don't care whether it's writable in EL1&0 translation regime
    __asm__ __volatile__ ("mrs %0, par_el1" : "=r"(PAR));
    *valid = (PAR & 1) == 0ull;
    return (PAR & 1) ? 0ull : (PAR & MASK2L(40, 12)) | ((uintptr_t)el1_vaddr & MASKL(12));
}

bool readEl1Memory(void *dst, uintptr_t addr, size_t size);
bool writeEl1Memory(uintptr_t addr, const void *src, size_t size);

static inline void panic(void) {
#ifndef PLATFORM_QEMU
    __builtin_trap();
#endif
    for (;;);
}
