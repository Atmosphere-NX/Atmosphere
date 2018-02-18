#ifndef EXOSPHERE_TIMERS_H
#define EXOSPHERE_TIMERS_H

#include <stdint.h>

/* Exosphere driver for the Tegra X1 Timers. */

void set_timer_address(void *timer_base);
void *get_timer_address(void); /* This is inlined in timers.c */

#define TIMERUS_CNTR_1US_0 (*((volatile uint32_t *)(get_timer_address() + 0x10)))

void wait(uint32_t microseconds);

#endif