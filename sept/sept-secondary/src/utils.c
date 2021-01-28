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
#include "../../../fusee/common/log.h"

#include <inttypes.h>

#define u8 uint8_t
#define u32 uint32_t
#include "rebootstub_bin.h"
#undef u8
#undef u32

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

void prepare_for_reboot_to_self(void) {
    /* Write warmboot to scratch0. */
    APBDEV_PMC_SCRATCH0_0  = 0x00000001;

    /* Patch SDRAM init to perform an SVC immediately after second write */
    APBDEV_PMC_SCRATCH45_0 = 0x2E38DFFF;
    APBDEV_PMC_SCRATCH46_0 = 0x6001DC28;
    /* Set SVC handler to jump to reboot stub in IRAM. */
    APBDEV_PMC_SCRATCH33_0 = 0x4003F000;
    APBDEV_PMC_SCRATCH40_0 = 0x6000F208;

    /* Copy reboot stub into IRAM high. */
    for (size_t i = 0; i < rebootstub_bin_size; i += sizeof(uint32_t)) {
        write32le((void *)0x4003F000, i, read32le(rebootstub_bin, i));
    }
}

__attribute__((noreturn)) void reboot_to_self(void) {
    /* Prep IRAM for reboot. */
    prepare_for_reboot_to_self();

    /* Trigger warm reboot. */
    pmc_reboot(1 << 0);
}

__attribute__((noreturn)) void wait_for_button_and_reboot(void) {
    uint32_t button;
    while (true) {
        button = btn_read();
        if (button & BTN_POWER) {
            reboot_to_self();
        }
    }
}

__attribute__ ((noreturn)) void generic_panic(void) {
    panic(0xFF000006);
}

__attribute__((noreturn)) void fatal_error(const char *fmt, ...) {
    /* Override the global logging level. */
    log_set_log_level(SCREEN_LOG_LEVEL_ERROR);
        
    /* Display fatal error. */
    va_list args;
    print(SCREEN_LOG_LEVEL_ERROR, "Fatal error: ");
    va_start(args, fmt);
    vprint(SCREEN_LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "\n Press POWER to reboot.\n");
    
    /* Wait for button and reboot. */
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
