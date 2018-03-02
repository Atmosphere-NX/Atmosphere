#ifndef EXOSPHERE_SYSCRT0_H
#define EXOSPHERE_SYSCRT0_H

#include <stdint.h>

#include "memory_map.h"

/* Exosphere driver for the Tegra X1 SYSCRT0 Registers. */

#define SYSCRT0_BASE  (MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_SYSCTR0))


#define MAKE_SYSCRT0_REG(n) (*((volatile uint32_t *)(SYSCRT0_BASE + n)))



#endif