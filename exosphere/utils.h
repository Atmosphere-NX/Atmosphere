#ifndef EXOSPHERE_UTILS_H
#define EXOSPHERE_UTILS_H

#include <stdint.h>
#include <stddef.h>

void panic(void);

static inline uint32_t read32le(const void *dword, size_t offset) {
    return *(uint32_t *)((uintptr_t)dword + offset);
}

static inline uint32_t read32be(const unsigned char *dword, size_t offset) {
    return __builtin_bswap32(read32le(dword, offset));
}

static inline unsigned int get_core_id(void) {
    unsigned int core_id;
    __asm__ __volatile__ ("mrs %0, MPIDR_EL1" : "=r"(core_id));
    return core_id & 3;
}

#endif
