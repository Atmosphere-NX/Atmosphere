/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
 
#ifndef FUSEE_UTILS_H
#define FUSEE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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
#define NOINLINE        __attribute__((noinline))

#define SET_SYSREG(reg, val) do { temp_reg = (val); __asm__ __volatile__ ("msr " #reg ", %0" :: "r"(temp_reg) : "memory"); } while(false)

static inline uintptr_t get_physical_address(const void *addr) {
    return (uintptr_t)addr;
}

static inline uint32_t read32le(const volatile void *dword, size_t offset) {
    uintptr_t addr = (uintptr_t)dword + offset;
    volatile uint32_t *target = (uint32_t *)addr;
    return *target;
}

static inline uint32_t read32be(const volatile void *dword, size_t offset) {
    return __builtin_bswap32(read32le(dword, offset));
}

static inline uint64_t read64le(const volatile void *qword, size_t offset) {
    uintptr_t addr = (uintptr_t)qword + offset;
    volatile uint64_t *target = (uint64_t *)addr;
    return *target;
}

static inline uint64_t read64be(const volatile void *qword, size_t offset) {
    return __builtin_bswap64(read64le(qword, offset));
}

static inline void write32le(volatile void *dword, size_t offset, uint32_t value) {
    uintptr_t addr = (uintptr_t)dword + offset;
    volatile uint32_t *target = (uint32_t *)addr;
    *target = value;
}

static inline void write32be(volatile void *dword, size_t offset, uint32_t value) {
    write32le(dword, offset, __builtin_bswap32(value));
}

static inline void write64le(volatile void *qword, size_t offset, uint64_t value) {
    uintptr_t addr = (uintptr_t)qword + offset;
    volatile uint64_t *target = (uint64_t *)addr;
    *target = value;
}

static inline void write64be(volatile void *qword, size_t offset, uint64_t value) {
    write64le(qword, offset, __builtin_bswap64(value));
}

static inline bool check_32bit_additive_overflow(uint32_t a, uint32_t b) {
    return __builtin_add_overflow_p(a, b, (uint32_t)0);
}

static inline bool check_32bit_address_loadable(uintptr_t addr) {
    /* FWIW the bootROM forbids loading anything between 0x40000000 and 0x40010000, using it for itself... */
    return (addr >= 0x40010000u && addr < 0x40040000u) || addr >= 0x80000000u;
}

static inline bool check_32bit_address_range_loadable(uintptr_t addr, size_t size) {
    return
        !__builtin_add_overflow_p(addr, size, (uintptr_t)0) && /* the range doesn't overflow */
        check_32bit_address_loadable(addr) && check_32bit_address_loadable(addr + size) && /* bounds are valid */
        !(addr >= 0x40010000u && addr < 0x40040000u && addr + size >= 0x40040000u) /* the range doesn't cross MMIO */
    ;
}

bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be);
static inline bool overlaps_a(const void *as, const void *ae, const void *bs, const void *be) {
    return overlaps((uint64_t)(uintptr_t)as, (uint64_t)(uintptr_t)ae, (uint64_t)(uintptr_t)bs, (uint64_t)(uintptr_t)be);
}

static inline bool check_32bit_address_range_in_program(uintptr_t addr, size_t size) {
    extern uint8_t __chainloader_start__[], __chainloader_end__[];
    extern uint8_t __stack_bottom__[], __stack_top__[];
    extern uint8_t __start__[], __end__[];
    uint8_t *start = (uint8_t *)addr, *end = start + size;

    return overlaps_a(start, end, __chainloader_start__, __chainloader_end__) ||
    overlaps_a(start, end, __stack_bottom__, __stack_top__) ||
    overlaps_a(start, end, (void *)0xC0000000, (void *)0xC03C0000) || /* framebuffer */
    overlaps_a(start, end, __start__, __end__);
}

__attribute__((noreturn)) void watchdog_reboot(void);
__attribute__((noreturn)) void pmc_reboot(uint32_t scratch0);
void prepare_for_reboot_to_self(void);
__attribute__((noreturn)) void reboot_to_self(void);
__attribute__((noreturn)) void wait_for_button_and_reboot(void);

__attribute__((noreturn)) void generic_panic(void);
__attribute__((noreturn)) void fatal_error(const char *fmt, ...);

#endif
