#ifndef EXOSPHERE_LP0_H
#define EXOSPHERE_LP0_H

#include <stdint.h>

/* Exosphere Deep Sleep Entry implementation. */

#define LP0_TZRAM_SAVE_SIZE 0xE000

uint32_t cpu_suspend(uint64_t power_state, uint64_t entrypoint, uint64_t argument);

#endif