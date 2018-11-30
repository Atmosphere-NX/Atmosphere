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
 
#include <stdbool.h>
#include <stdarg.h>
#include "utils.h"
#include "se.h"
#include "fuse.h"
#include "pmc.h"
#include "timers.h"
#include "panic.h"
#include "car.h"
#include "btn.h"

#include "lib/log.h"

#include <inttypes.h>

void wait(uint32_t microseconds) {
    uint32_t old_time = TIMERUS_CNTR_1US_0;
    while (TIMERUS_CNTR_1US_0 - old_time <= microseconds) {
        /* Spin-lock. */
    }
}

__attribute__((noreturn)) void watchdog_reboot(void) {
    volatile watchdog_timers_t *wdt = GET_WDT(4);
    wdt->PATTERN = WDT_REBOOT_PATTERN;
    wdt->COMMAND = 2; /* Disable Counter. */
    GET_WDT_REBOOT_CFG_REG(4) = 0xC0000000;
    wdt->CONFIG = 0x8019; /* Full System Reset after Fourth Counter expires, using TIMER(9). */
    wdt->COMMAND = 1; /* Enable Counter. */
    while (true) {
        /* Wait for reboot. */
    }
}

__attribute__((noreturn)) void pmc_reboot(uint32_t scratch0) {
    APBDEV_PMC_SCRATCH0_0 = scratch0;

    /* Reset the processor. */
    APBDEV_PMC_CONTROL = BIT(4);

    while (true) {
        /* Wait for reboot. */
    }
}

__attribute__((noreturn)) void wait_for_button_and_reboot(void) {
    uint32_t button;
    while (true) {
        button = btn_read();
        if (button & BTN_POWER) {
            pmc_reboot(1 << 1);
        }
    }
}

__attribute__ ((noreturn)) void generic_panic(void) {
    panic(0xFF000006);
}

__attribute__((noreturn)) void fatal_error(const char *fmt, ...) {
    va_list args;
    print(SCREEN_LOG_LEVEL_ERROR, "Fatal error: ");
    va_start(args, fmt);
    vprint(SCREEN_LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX,"\nPress POWER to reboot\n");
    wait_for_button_and_reboot();
}

__attribute__((noinline)) bool overlaps(uint64_t as, uint64_t ae, uint64_t bs, uint64_t be)
{
    if(as <= bs && bs <= ae)
        return true;
    if(bs <= as && as <= be)
        return true;
    return false;
}

/* Adapted from https://gist.github.com/ccbrown/9722406 */
void hexdump(const void* data, size_t size, uintptr_t addrbase) {
    const uint8_t *d = (const uint8_t *)data;
    char ascii[17];
    ascii[16] = '\0';

    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) {
            print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "%0*" PRIXPTR ": | ", 2 * sizeof(addrbase), addrbase + i);
        }
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "%02X ", d[i]);
        if (d[i] >= ' ' && d[i] <= '~') {
            ascii[i % 16] = d[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, " ");
            if ((i+1) % 16 == 0) {
                print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, " ");
                }
                for (size_t j = (i+1) % 16; j < 16; j++) {
                    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "   ");
                }
                print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "|  %s \n", ascii);
            }
        }
    }
}
