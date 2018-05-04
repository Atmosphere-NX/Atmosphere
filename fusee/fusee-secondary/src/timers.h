#ifndef FUSEE_TIMERS_H
#define FUSEE_TIMERS_H

#include "utils.h"

#define TIMERS_BASE 0x60005000

#define MAKE_TIMERS_REG(n) MAKE_REG32(TIMERS_BASE + n)

#define TIMERUS_CNTR_1US_0 MAKE_REG32(TIMERS_BASE + 0x10)

typedef struct {
    uint32_t CONFIG;
    uint32_t STATUS;
    uint32_t COMMAND;
    uint32_t PATTERN;
} watchdog_timers_t;

#define GET_WDT(n) ((volatile watchdog_timers_t *)(TIMERS_BASE + 0x100 + 0x20 * n))
#define WDT_REBOOT_PATTERN 0xC45A
#define GET_WDT_REBOOT_CFG_REG(n) MAKE_REG32(TIMERS_BASE + 0x60 + 0x8*n)

void wait(uint32_t microseconds);

static inline uint32_t get_time(void) {
    return TIMERUS_CNTR_1US_0;
}

/**
 * Returns the number of microseconds that have passed since a given get_time().
 */
static inline uint32_t get_time_since(uint32_t base) {
    return get_time() - base;
}

/**
 * Delays for a given number of microseconds.
 */
static inline void udelay(unsigned usecs)
{
  uint32_t start = get_time();
  while (get_time() - start < usecs) ;
}

__attribute__ ((noreturn)) void watchdog_reboot(void);

#endif
