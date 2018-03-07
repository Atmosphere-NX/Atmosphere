#ifndef EXOSPHERE_SYSCTR0_H
#define EXOSPHERE_SYSCTR0_H

#include <stdint.h>

#include "memory_map.h"

/* Exosphere driver for the Tegra X1 SYSCTR0 Registers. */

#define SYSCTR0_BASE  (MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_SYSCTR0))


#define MAKE_SYSCTR0_REG(n) MAKE_REG32(SYSCTR0_BASE + n)



#endif
