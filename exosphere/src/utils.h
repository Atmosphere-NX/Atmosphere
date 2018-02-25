#ifndef EXOSPHERE_UTILS_H
#define EXOSPHERE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BIT(x)  (1u   << (x))
#define BITL(x) (1ull << (x))

void panic(uint32_t code);
void generic_panic(void);
bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be);

static inline uintptr_t get_physical_address(const void *vaddr) {
    uintptr_t PAR;
    __asm__ __volatile__ ("at s1e3r, %0" :: "r"(vaddr));
    __asm__ __volatile__ ("mrs %0, par_el1" : "=r"(PAR));
    return (PAR & 1) ? 0ULL : (PAR & 0x00000FFFFFFFF000ULL) | ((uintptr_t)vaddr & 0xFFF);
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

static inline unsigned int get_core_id(void) {
    uint64_t core_id;
    __asm__ __volatile__ ("mrs %0, mpidr_el1" : "=r"(core_id));
    return (unsigned int)core_id & 3;
}

static inline bool check_32bit_additive_overflow(uint32_t a, uint32_t b) {
    return __builtin_add_overflow_p(a, b, (uint32_t)0);
}

#endif
