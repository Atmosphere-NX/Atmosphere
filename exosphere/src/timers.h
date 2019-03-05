/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#ifndef EXOSPHERE_TIMERS_H
#define EXOSPHERE_TIMERS_H

#include <stdint.h>
#include "memory_map.h"

/* Exosphere driver for the Tegra X1 Timers. */

static inline uintptr_t get_timers_base(void) {
    return MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_TMRs_WDTs);
}

#define TIMERS_BASE (get_timers_base())
#define MAKE_TIMERS_REG(n) MAKE_REG32(TIMERS_BASE + n)

#define TIMERUS_CNTR_1US_0 MAKE_TIMERS_REG(0x10)
#define SHARED_INTR_STATUS_0 MAKE_TIMERS_REG(0x1A0)
#define SHARED_TIMER_SECURE_CFG_0 MAKE_TIMERS_REG(0x1A4)

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

static inline uint32_t get_time(void) {
    return TIMERUS_CNTR_1US_0;
}

__attribute__ ((noreturn)) void watchdog_reboot(void);

#endif
