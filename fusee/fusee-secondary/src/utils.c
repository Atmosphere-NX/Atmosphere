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
#include "car.h"
#include "timers.h"
#include "btn.h"
#include "i2c.h"
#include "panic.h"
#include "max77620.h"
#include "../../../fusee/common/log.h"

#include <stdio.h>
#include <inttypes.h>

#define u8 uint8_t
#define u32 uint32_t
#include "fusee_primary_bin.h"
#include "sept_primary_bin.h"
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

__attribute__((noreturn)) static void reboot_to_payload(void) {
    if (is_soc_mariko()) {
        /* Reboot to payload isn't possible on mariko, so just do normal reboot. */
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

        /* Trigger warm reboot. */
        pmc_reboot(1 << 0);
        while (true) { }
    }
}

__attribute__((noreturn)) void reboot_to_fusee_primary(void) {
    /* Copy fusee-primary into IRAM low. */
    for (size_t i = 0; i < fusee_primary_bin_size; i += sizeof(uint32_t)) {
        write32le((void *)0x40010000, i, read32le(fusee_primary_bin, i));
    }

    reboot_to_payload();
}

__attribute__((noreturn)) void reboot_to_sept(const void *tsec_fw, size_t tsec_fw_length, const void *stage2, size_t stage2_size) {
    if (is_soc_mariko()) {
        /* Reboot to sept isn't possible on mariko, so just do normal reboot. */
        shutdown_system(true);
    } else {
        /* Copy tsec firmware. */
        for (size_t i = 0; i < tsec_fw_length; i += sizeof(uint32_t)) {
            write32le((void *)0x40010F00, i, read32le(tsec_fw, i));
        }
        MAKE_REG32(0x40010EFC) = tsec_fw_length;

        /* Copy stage 2. */
        for (size_t i = 0; i < stage2_size; i += sizeof(uint32_t)) {
            write32le((void *)0x40016FE0, i, read32le(stage2, i));
        }

        /* Copy sept into IRAM low. */
        for (size_t i = 0; i < sept_primary_bin_size; i += sizeof(uint32_t)) {
            write32le((void *)0x4003F000, i, read32le(sept_primary_bin, i));
        }

        /* Patch SDRAM init to perform an SVC immediately after second write */
        APBDEV_PMC_SCRATCH45_0 = 0x2E38DFFF;
        APBDEV_PMC_SCRATCH46_0 = 0x6001DC28;
        /* Set SVC handler to jump to reboot stub in IRAM. */
        APBDEV_PMC_SCRATCH33_0 = 0x4003F000;
        APBDEV_PMC_SCRATCH40_0 = 0x6000F208;

        /* Trigger warm reboot. */
        pmc_reboot(1 << 0);
        while (true) { }
    }
}

__attribute__((noreturn)) void reboot_to_iram_payload(void *payload, size_t payload_size) {
    /* Copy sept into IRAM low. */
    for (size_t i = 0; i < payload_size; i += sizeof(uint32_t)) {
        write32le((void *)0x40010000, i, read32le(payload, i));
    }

    reboot_to_payload();
}

__attribute__((noreturn)) void wait_for_button_and_reboot(void) {
    uint32_t button;
    while (true) {
        button = btn_read();
        if (button & BTN_POWER) {
            reboot_to_fusee_primary();
        }
    }
}

void wait_for_button(void) {
    uint32_t button;
    while (true) {
        button = btn_read();
        if (button) {
            return;
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
    if(as <= bs && bs < ae)
        return true;
    if(bs <= as && as < be)
        return true;
    return false;
}
