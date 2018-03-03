#ifndef EXOSPHERE_BOOTUP_H
#define EXOSPHERE_BOOTUP_H

#include <stdint.h>

void bootup_misc_mmio(void);

void setup_4x_mmio(void);

void setup_current_core_state(void);

void identity_unmap_iram_cd_tzram(void);

void secure_additional_devices(void);

void set_extabt_serror_taken_to_el3(bool taken_to_el3);

#endif
