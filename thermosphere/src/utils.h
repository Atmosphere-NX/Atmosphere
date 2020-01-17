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
#define LIKELY(expr)    __builtin_expect((expr), 1)
#define UNLIKELY(expr)  __builtin_expect((expr), 0)

#define ALINLINE        __attribute__((always_inline))

#define TEMPORARY       __attribute__((section(".tempbss")))

#define PANIC(...)      do { DEBUG(__VA_ARGS__); panic(); } while (false)

#define FOREACH_BIT(tmpmsk, var, word) for (u64 tmpmsk = (word), var = __builtin_ctzll(word); tmpmsk != 0; tmpmsk &= ~BITL(var), var = __builtin_ctzll(tmpmsk))

#define _DECLARE_ASM_ARITHMETIC_UNARY_HELPER(sz, regalloc, op)\
static inline u##sz __##op##sz(u##sz n)\
{\
    u##sz res;\
    __asm__ __volatile__ (STRINGIZE(op) " %" STRINGIZE(regalloc) "[res], %" STRINGIZE(regalloc) "[n]" : [res] "=r" (res) : [n] "r" (n));\
    return res;\
}

#define _DECLARE_ASM_ARITHMETIC_UNARY_HELPER64(op)  _DECLARE_ASM_ARITHMETIC_UNARY_HELPER(64, x, op)
#define _DECLARE_ASM_ARITHMETIC_UNARY_HELPER32(op)  _DECLARE_ASM_ARITHMETIC_UNARY_HELPER(32, w, op)

_DECLARE_ASM_ARITHMETIC_UNARY_HELPER64(rbit)
_DECLARE_ASM_ARITHMETIC_UNARY_HELPER32(rbit)

typedef enum ReadWriteDirection {
    DIRECTION_READ      = BIT(0),
    DIRECTION_WRITE     = BIT(1),
    DIRECTION_READWRITE = DIRECTION_READ | DIRECTION_WRITE,
} ReadWriteDirection;

static inline void __wfi(void)
{
    __asm__ __volatile__ ("wfi" ::: "memory");
}

static inline void __wfe(void)
{
    __asm__ __volatile__ ("wfe" ::: "memory");
}

static inline void __sev(void)
{
    __asm__ __volatile__ ("sev" ::: "memory");
}

static inline void __sevl(void)
{
    __asm__ __volatile__ ("sevl" ::: "memory");
}

/*
    Domains:
        - Inner shareable: typically cores within a cluster (maybe more) with L1+L2 caches
        - Outer shareable: all the cores in all clusters that can be coherent
        - System: everything else
    Since we only support 1 single cluster, we basically only need to consider the inner
    shareable domain, except before doing DMA...
*/
static inline void __dmb(void)
{
    __asm__ __volatile__ ("dmb ish" ::: "memory");
}

static inline void __dsb(void)
{
    __asm__ __volatile__ ("dsb ish" ::: "memory");
}

static inline void __dmb_sy(void)
{
    __asm__ __volatile__ ("dmb sy" ::: "memory");
}

static inline void __dsb_sy(void)
{
    __asm__ __volatile__ ("dsb sy" ::: "memory");
}

static inline void __isb(void)
{
    __asm__ __volatile__ ("isb" ::: "memory");
}

static inline void __tlb_invalidate_el2(void)
{
    __asm__ __volatile__ ("tlbi alle2is" ::: "memory");
}

static inline void __tlb_invalidate_el2_local(void)
{
    __asm__ __volatile__ ("tlbi alle2" ::: "memory");
}

static inline void __tlb_invalidate_el1_stage12_local(void)
{
    __asm__ __volatile__ ("tlbi alle1" ::: "memory");
}

bool overlaps(u64 as, u64 ae, u64 bs, u64 be);

// Assumes addr is valid, must be called with interrupts masked
static inline uintptr_t va2pa(const void *el2_vaddr) {
    // NOTE: interrupt must be disabled when calling this func
    // For debug purposes only
    uintptr_t PAR;
    uintptr_t va = (uintptr_t)el2_vaddr;
    __asm__ __volatile__ ("at s1e2r, %0" :: "r"(va));
    __asm__ __volatile__ ("mrs %0, par_el1" : "=r"(PAR));
    return (PAR & MASK2L(47, 12)) | (va & MASKL(12));
}

static inline uintptr_t get_physical_address_el1_stage12(bool *valid, uintptr_t el1_vaddr) {
    // NOTE: interrupt must be disabled when calling this func
    uintptr_t PAR;
    __asm__ __volatile__ ("at s12e1r, %0" :: "r"(el1_vaddr)); // note: we don't care whether it's writable in EL1&0 translation regime
    __asm__ __volatile__ ("mrs %0, par_el1" : "=r"(PAR));
    *valid = (PAR & 1) == 0ull;
    return (PAR & 1) ? 0ull : (PAR & MASK2L(47, 12)) | ((uintptr_t)el1_vaddr & MASKL(12));
}

bool readEl1Memory(void *dst, uintptr_t addr, size_t size);
bool writeEl1Memory(uintptr_t addr, const void *src, size_t size);

static inline void panic(void) {
#ifndef PLATFORM_QEMU
    __builtin_trap();
#endif
    for (;;);
}
