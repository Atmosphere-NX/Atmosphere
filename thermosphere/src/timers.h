/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#pragma once

#include "utils.h"

#define TIMERS_BASE 0x60005000ull
#define MAKE_TIMERS_REG(n) MAKE_REG32(TIMERS_BASE + n)

#define TIMERUS_CNTR_1US_0 MAKE_TIMERS_REG(0x10)
#define SHARED_INTR_STATUS_0 MAKE_TIMERS_REG(0x1A0)
#define SHARED_TIMER_SECURE_CFG_0 MAKE_TIMERS_REG(0x1A4)

typedef struct {
    u32 CONFIG;
    u32 STATUS;
    u32 COMMAND;
    u32 PATTERN;
} watchdog_timers_t;

#define GET_WDT(n) ((volatile watchdog_timers_t *)(TIMERS_BASE + 0x100 + 0x20 * n))
#define WDT_REBOOT_PATTERN 0xC45A
#define GET_WDT_REBOOT_CFG_REG(n) MAKE_REG32(TIMERS_BASE + 0x60 + 0x8 * n)

void wait(u32 microseconds);

static inline u32 get_time(void) {
    return TIMERUS_CNTR_1US_0;
}
