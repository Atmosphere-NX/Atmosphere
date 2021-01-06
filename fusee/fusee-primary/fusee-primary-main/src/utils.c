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
#include "di.h"
#include "se.h"
#include "fuse.h"
#include "pmc.h"
#include "timers.h"
#include "i2c.h"
#include "panic.h"
#include "car.h"
#include "btn.h"
#include "max77620.h"
#include "../../../fusee/common/log.h"
#include "../../../fusee/common/vsprintf.h"
#include "../../../fusee/common/display/video_fb.h"

#include <inttypes.h>

#define u8 uint8_t
#define u32 uint32_t
#include "rebootstub_bin.h"
#undef u8
#undef u32

static bool is_soc_mariko(void) {
    return fuse_get_soc_type() == 1;
}

__attribute__((noreturn)) static void shutdown_system(bool reboot) {
    /* Ensure that i2c5 is in a coherent state. */
    i2c_config(I2C_5);
    clkrst_reboot(CARDEVICE_I2C5);
    i2c_init(I2C_5);

    /* Get value, set or clear software reset mask. */
    uint8_t on_off_2_val = 0;
    i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_ONOFFCNFG2, &on_off_2_val, 1);
    if (reboot) {
        on_off_2_val |= MAX77620_ONOFFCNFG2_SFT_RST_WK;
    } else {
        on_off_2_val &= ~(MAX77620_ONOFFCNFG2_SFT_RST_WK);
    }
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_ONOFFCNFG2, &on_off_2_val, 1);

    /* Set software reset mask. */
    uint8_t on_off_1_val = 0;
    i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, &on_off_1_val, 1);
    on_off_1_val |= MAX77620_ONOFFCNFG1_SFT_RST;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, &on_off_1_val, 1);

    while (true) {
        /* Wait for reboot. */
    }
}

extern uint8_t __reboot_start__[], __reboot_end__[];

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

__attribute__((noreturn)) void reboot_to_self(void) {
    if (is_soc_mariko()) {
        /* If mariko, we can't reboot to self/payload, so just reboot. */
        shutdown_system(true);
    } else {
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

        /* Copy our low part into safe IRAM. */
        for (size_t i = 0; i < 0x8000; i += sizeof(uint32_t)) {
            write32le((void *)0x40030000, i, read32le((void *)0x40008000, i));
        }

        /* Copy our start page into fatal IRAM. */
        for (size_t i = 0; i < 0x1000; i += sizeof(uint32_t)) {
            write32le((void *)0x4003D000, i, read32le((void *)0x40010000, i));
        }

        /* Copy our reboot handler to the rebootstub target. */
        for (size_t i = 0; i < (__reboot_end__ - __reboot_start__); i += sizeof(uint32_t)) {
            write32le((void *)0x40010000, i, read32le(__reboot_start__, i));
        }

        /* Trigger warm reboot. */
        APBDEV_PMC_SCRATCH0_0 = (1 << 0);

        /* Reset the processor. */
        APBDEV_PMC_CONTROL = BIT(4);

        while (true) {
            /* Wait for reboot. */
        }
    }
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
    /* Forcefully initialize the screen if logging is disabled. */
    if (log_get_log_level() == SCREEN_LOG_LEVEL_NONE) {
        /* Zero-fill the framebuffer and register it as printk provider. */
        video_init((void *)0xC0000000);

        /* Initialize the display. */
        display_init();

        /* Set the framebuffer. */
        display_init_framebuffer((void *)0xC0000000);

        /* Turn on the backlight after initializing the lfb */
        /* to avoid flickering. */
        display_backlight(true);
    }

    /* Override the global logging level. */
    log_set_log_level(SCREEN_LOG_LEVEL_ERROR);

    /* Display fatal error. */
    va_list args;
    print(SCREEN_LOG_LEVEL_ERROR, "Fatal error: ");
    va_start(args, fmt);
    vprint(SCREEN_LOG_LEVEL_ERROR, fmt, args);
    va_end(args);
    print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX,"\nPress POWER to reboot\n");

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
