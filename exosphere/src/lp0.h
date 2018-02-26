#ifndef EXOSPHERE_LP0_H
#define EXOSPHERE_LP0_H

#include <stdint.h>

/* Exosphere Deep Sleep Entry implementation. */

uint32_t cpu_suspend(uint64_t power_state, uint64_t entrypoint, uint64_t argument);

#endif