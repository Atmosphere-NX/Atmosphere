#ifndef EXOSPHERE_TIMERS_H
#define EXOSPHERE_TIMERS_H

#include <stdint.h>
#include "mmu.h"

/* Exosphere driver for the Tegra X1 Timers. */

#define TIMERS_BASE (mmio_get_device_address(MMIO_DEVID_TMRs_WDTs))

#define TIMERUS_CNTR_1US_0 (*((volatile uint32_t *)(TIMERS_BASE + 0x10)))

void wait(uint32_t microseconds);

#endif