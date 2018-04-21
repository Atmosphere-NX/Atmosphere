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

void panic(uint32_t code);
void generic_panic(void);
void panic_predefined(uint32_t which);
bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be);

#endif
