#ifndef EXOSPHERE_PMC_H
#define EXOSPHERE_PMC_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere register definitions for the Tegra X1 PMC. */

static inline uintptr_t get_pmc_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_RTC_PMC) + 0x400ull;
}

#define PMC_BASE (get_pmc_base())

#define APBDEV_PMC_DPD_ENABLE_0 (*((volatile uint32_t *)(PMC_BASE + 0x24)))

#define APBDEV_PMC_PWRGATE_TOGGLE_0 (*((volatile uint32_t *)(PMC_BASE + 0x30)))
#define APBDEV_PMC_PWRGATE_STATUS_0 (*((volatile uint32_t *)(PMC_BASE + 0x38)))

#define APBDEV_PMC_SCRATCH0_0 (*((volatile uint32_t *)(PMC_BASE + 0x50)))

#define APBDEV_PM_0 (*((volatile uint32_t *)(PMC_BASE + 0x14)))
#define APBDEV_PMC_WAKE2_STATUS_0 (*((volatile uint32_t *)(PMC_BASE + 0x168)))
#define APBDEV_PMC_CNTRL2_0 (*((volatile uint32_t *)(PMC_BASE + 0x440)))

#endif
