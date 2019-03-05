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
 
#ifndef EXOSPHERE_UTILS_H
#define EXOSPHERE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "panic_color.h"

#define BIT(n)      (1u   << (n))
#define BITL(n)     (1ull << (n))
#define MASK(n)     (BIT(n) - 1)
#define MASKL(n)    (BITL(n) - 1)
#define MASK2(a,b)  (MASK(a) & ~MASK(b))
#define MASK2L(a,b) (MASKL(a) & ~MASKL(b))

#define MAKE_REG32(a)   (*(volatile uint32_t *)(a))

#define ALIGN(m)        __attribute__((aligned(m)))
#define PACKED          __attribute__((packed))

#define ALINLINE        __attribute__((always_inline))

#define SET_SYSREG(reg, val) do { temp_reg = (val); __asm__ __volatile__ ("msr " #reg ", %0" :: "r"(temp_reg) : "memory"); } while(false)

/* Custom stuff below */

/* For coldboot */
typedef struct {
    uint8_t     *vma;
    uint8_t     *end_vma;
    uintptr_t   lma;
} coldboot_crt0_reloc_t;

typedef struct {
    uintptr_t               reloc_base;
    size_t                  loaded_bin_size;
    size_t                  nb_relocs_pre_mmu_init;     /* first is always warmboot_crt0 */
    size_t                  nb_relocs_post_mmu_init;    /* first is always main segment excl. .bss */
    coldboot_crt0_reloc_t   relocs[];
} coldboot_crt0_reloc_list_t;

__attribute__ ((noreturn)) void panic(uint32_t code);
__attribute__ ((noreturn)) void generic_panic(void);
__attribute__ ((noreturn)) void panic_predefined(uint32_t which);
bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be);

uintptr_t get_iram_address_for_debug(void);

static inline uintptr_t get_physical_address(const void *vaddr) {
    uintptr_t PAR;
    __asm__ __volatile__ ("at s1e3r, %0" :: "r"(vaddr));
    __asm__ __volatile__ ("mrs %0, par_el1" : "=r"(PAR));
    return (PAR & 1) ? 0ull : (PAR & MASK2L(40, 12)) | ((uintptr_t)vaddr & MASKL(12));
}

static inline uint32_t read32le(const volatile void *dword, size_t offset) {
    return *(uint32_t *)((uintptr_t)dword + offset);
}

static inline uint32_t read32be(const volatile void *dword, size_t offset) {
    return __builtin_bswap32(read32le(dword, offset));
}

static inline uint64_t read64le(const volatile void *qword, size_t offset) {
    return *(uint64_t *)((uintptr_t)qword + offset);
}

static inline uint64_t read64be(const volatile void *qword, size_t offset) {
    return __builtin_bswap64(read64le(qword, offset));
}

static inline void write32le(volatile void *dword, size_t offset, uint32_t value) {
    *(uint32_t *)((uintptr_t)dword + offset) = value;
}

static inline void write32be(volatile void *dword, size_t offset, uint32_t value) {
    write32le(dword, offset, __builtin_bswap32(value));
}

static inline void write64le(volatile void *qword, size_t offset, uint64_t value) {
    *(uint64_t *)((uintptr_t)qword + offset) = value;
}

static inline void write64be(volatile void *qword, size_t offset, uint64_t value) {
    write64le(qword, offset, __builtin_bswap64(value));
}

static inline unsigned int get_core_id(void) {
    uint64_t core_id;
    __asm__ __volatile__ ("mrs %0, mpidr_el1" : "=r"(core_id));
    return (unsigned int)core_id & 3;
}

static inline uint64_t get_debug_authentication_status(void) {
    uint64_t debug_auth;
    __asm__ __volatile__ ("mrs  %0, dbgauthstatus_el1" : "=r"(debug_auth));
    return debug_auth;
}

static inline uint32_t get_spsr(void) {
    uint32_t spsr;
    __asm__ __volatile__ ("mrs  %0, spsr_el3" : "=r"(spsr));
    return spsr;
}

static inline bool check_32bit_additive_overflow(uint32_t a, uint32_t b) {
    return __builtin_add_overflow_p(a, b, (uint32_t)0);
}

#endif
