#ifndef EXOSPHERE_UTILS_H
#define EXOSPHERE_UTILS_H

#include <stdint.h>

void panic(void);

unsigned int read32le(const unsigned char *dword, unsigned int offset);
unsigned int read32be(const unsigned char *dword, unsigned int offset);

static inline uint32_t get_core_id(void) {
    uint32_t core_id;
    asm volatile("mrs %0, MPIDR_EL1" : "=r"(core_id));
    return core_id;
}

#endif