#ifndef EXOSPHERE_BOOTUP_H
#define EXOSPHERE_BOOTUP_H

#include <stdint.h>

void bootup_misc_mmio(void);

void setup_4x_mmio(void);

void setup_current_core_state(void);

#endif