#ifndef EXOSPHERE_BPMP_H
#define EXOSPHERE_BPMP_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere register definitions for the Tegra X1 BPMP vectors. */

static inline uintptr_t get_bpmp_vector_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_EXCEPTION_VECTORS);
}

#define BPMP_VECTOR_BASE (get_bpmp_vector_base())

#define EVP_CPU_RESET_VECTOR_0     (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x100)))

#define BPMP_VECTOR_RESET          (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x200)))
#define BPMP_VECTOR_UNDEF          (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x204)))
#define BPMP_VECTOR_SWI            (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x208)))
#define BPMP_VECTOR_PREFETCH_ABORT (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x20C)))
#define BPMP_VECTOR_DATA_ABORT     (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x210)))
#define BPMP_VECTOR_UNK            (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x214)))
#define BPMP_VECTOR_IRQ            (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x218)))
#define BPMP_VECTOR_FIQ            (*((volatile uint32_t *)(BPMP_VECTOR_BASE + 0x21C)))

#endif
