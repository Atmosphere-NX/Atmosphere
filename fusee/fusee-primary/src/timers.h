/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef FUSEE_TIMERS_H
#define FUSEE_TIMERS_H

#include "utils.h"

#define TIMERS_BASE 0x60005000
#define MAKE_TIMERS_REG(n) MAKE_REG32(TIMERS_BASE + n)

#define TIMERUS_CNTR_1US_0 MAKE_TIMERS_REG(0x10)
#define TIMERUS_USEC_CFG_0 MAKE_TIMERS_REG(0x14)
#define SHARED_INTR_STATUS_0 MAKE_TIMERS_REG(0x1A0)
#define SHARED_TIMER_SECURE_CFG_0 MAKE_TIMERS_REG(0x1A4)

#define RTC_BASE 0x7000E000
#define MAKE_RTC_REG(n) MAKE_REG32(RTC_BASE + n)

#define RTC_SECONDS MAKE_RTC_REG(0x08)
#define RTC_SHADOW_SECONDS MAKE_RTC_REG(0x0C)
#define RTC_MILLI_SECONDS MAKE_RTC_REG(0x10)

typedef struct {
    uint32_t CONFIG;
    uint32_t STATUS;
    uint32_t COMMAND;
    uint32_t PATTERN;
} watchdog_timers_t;

#define GET_WDT(n) ((volatile watchdog_timers_t *)(TIMERS_BASE + 0x100 + 0x20 * n))
#define WDT_REBOOT_PATTERN 0xC45A
#define GET_WDT_REBOOT_CFG_REG(n) MAKE_REG32(TIMERS_BASE + 0x60 + 0x8 * n)

void wait(uint32_t microseconds);

static inline uint32_t get_time_s(void) {
    return RTC_SECONDS;
}

static inline uint32_t get_time_ms(void) {
    return (RTC_MILLI_SECONDS | (RTC_SHADOW_SECONDS << 10));
}

static inline uint32_t get_time_us(void) {
    return TIMERUS_CNTR_1US_0;
}

/**
 * Returns the time in microseconds.
 */
static inline uint32_t get_time(void) {
    return get_time_us();
}

/**
 * Returns the number of microseconds that have passed since a given get_time().
 */
static inline uint32_t get_time_since(uint32_t base) {
    return get_time_us() - base;
}

/**
 * Delays for a given number of microseconds.
 */
static inline void udelay(uint32_t usecs) {
    uint32_t start = get_time_us();
    while (get_time_us() - start < usecs);
}

/**
 * Delays for a given number of milliseconds.
 */
static inline void mdelay(uint32_t msecs) {
    uint32_t start = get_time_ms();
    while (get_time_ms() - start < msecs);
}

__attribute__ ((noreturn)) void watchdog_reboot(void);

#endif
