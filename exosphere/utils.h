#ifndef EXOSPHERE_UTILS_H
#define EXOSPHERE_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BIT(x)  (1u   << (x))
#define BITL(x) (1ull << (x))

void panic(uint32_t code);
void generic_panic(void);

uint32_t get_physical_address(void *vaddr);

static inline uint32_t read32le(const void *dword, size_t offset) {
    return *(uint32_t *)((uintptr_t)dword + offset);
}

static inline uint32_t read32be(const unsigned char *dword, size_t offset) {
    return __builtin_bswap32(read32le(dword, offset));
}

static inline uint64_t read64le(const void *qword, size_t offset) {
    return *(uint64_t *)((uintptr_t)dword + offset);
}

static __attribute__((noinline)) bool check_32bit_additive_overflow(uint32_t a, uint32_t b) {
    uint64_t x = (uint64_t)a + (uint64_t)b;
    return x > (uint64_t)(UINT32_MAX);
}

static __attribute__((noinline)) bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be)
{
    if(as <= bs && bs <= ae)
        return true;
    if(bs <= as && as <= be)
        return true;
    return false;
}


static inline unsigned int get_core_id(void) {
    uint64_t core_id;
    __asm__ __volatile__ ("mrs %0, MPIDR_EL1" : "=r"(core_id));
    return (unsigned int)core_id & 3;
}

#endif
