#ifndef EXOSPHERE_PMC_H
#define EXOSPHERE_PMC_H

#include <stdint.h>
#include "mmu.h"

/* Exosphere register definitions for the Tegra X1 PMC. */

#define PMC_BASE (mmio_get_device_address(MMIO_DEVID_RTC_PMC) + 0x400ULL)

#define APBDEV_PMC_PWRGATE_TOGGLE_0 (*((volatile uint32_t *)(PMC_BASE + 0x30)))
#define APBDEV_PMC_PWRGATE_STATUS_0 (*((volatile uint32_t *)(PMC_BASE + 0x38)))

#endif