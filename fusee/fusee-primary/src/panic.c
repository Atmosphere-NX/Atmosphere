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
 
#include <stdio.h>
#include "panic.h"
#include "di.h"
#include "pmc.h"
#include "fuse.h"
#include "utils.h"
#include "fs_utils.h"
#include "lib/log.h"

static uint32_t g_panic_code = 0;

static const char *get_error_desc_str(uint32_t error_desc) {
    switch (error_desc) {
        case 0x100:
            return "Instruction Abort";
        case 0x101:
            return "Data Abort";
        case 0x102:
            return "PC Misalignment";
        case 0x103:
            return "SP Misalignment";
        case 0x104:
            return "Trap";
        case 0x106:
            return "SError";
        case 0x301:
            return "Bad SVC";
        case 0xFFE:
            return "std::abort() called";
        default:
            return "Unknown";
    }
}

static void _check_and_display_atmosphere_fatal_error(void) {
    /* Check for valid magic. */
    if (ATMOSPHERE_FATAL_ERROR_CONTEXT->magic != ATMOSPHERE_REBOOT_TO_FATAL_MAGIC &&
        ATMOSPHERE_FATAL_ERROR_CONTEXT->magic != ATMOSPHERE_REBOOT_TO_FATAL_MAGIC_0) {
        return;
    }

    {
        /* Copy fatal error context to the stack. */
        atmosphere_fatal_error_ctx ctx = *(ATMOSPHERE_FATAL_ERROR_CONTEXT);

        /* Change magic to invalid, to prevent double-display of error/bootlooping. */
        ATMOSPHERE_FATAL_ERROR_CONTEXT->magic = 0xCCCCCCCC;

        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "A fatal error occurred when running Atmosph\xe8re.\n");
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "Title ID:   %016llx\n", ctx.title_id);
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "Error Desc: %s (0x%x)\n", get_error_desc_str(ctx.error_desc), ctx.error_desc);

        /* Save context to the SD card. */
        {
            char filepath[0x40];
            snprintf(filepath, sizeof(filepath) - 1, "/atmosphere/fatal_errors/report_%016llx.bin", ctx.report_identifier);
            filepath[sizeof(filepath)-1] = 0;
            write_to_file(&ctx, sizeof(ctx), filepath);
            print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX,"Report saved to %s\n", filepath);
        }

        /* Display error. */
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX,"\nPress POWER to reboot\n");
    }

    wait_for_button_and_reboot();
}

void check_and_display_panic(void) {
    /* Handle a panic sent via a stratosphere module. */
    _check_and_display_atmosphere_fatal_error();
    
    /* We also handle our own panics. */
    /* In the case of our own panics, we assume that the display has already been initialized. */
    bool has_panic = APBDEV_PMC_RST_STATUS_0 != 0 || g_panic_code != 0;
    uint32_t code = g_panic_code == 0 ? APBDEV_PMC_SCRATCH200_0 : g_panic_code;

    has_panic = has_panic && !(APBDEV_PMC_RST_STATUS_0 != 1 && code == PANIC_CODE_SAFEMODE);

    if (has_panic) {
        uint32_t color;

        /* Check for predefined codes: */
        switch (code & MASK(20)) {
            case 0x01: /* Package2 signature verification failed. */
            case 0x02: /* Package2 meta verification failed. */
            case 0x03: /* Package2 version check failed. */
            case 0x04: /* Package2 payload verification failed. */
                color = PANIC_COLOR_KERNEL;
                break;
            case 0x05: /* Unknown SMC. */
            case 0x06: /* Unknown Abort. */
                color = PANIC_COLOR_SECMON_GENERIC;
                break;
            case 0x07: /* Invalid CPU context. */
            case 0x08: /* Invalid SE state. */
            case 0x09: /* CPU is already awake (2.0.0+). */
                color = PANIC_COLOR_SECMON_DEEPSLEEP;
                break;
            case 0x10: /* Unknown exception. */
                color = PANIC_COLOR_SECMON_EXCEPTION;
                break;
            case 0x30: /* General bootloader error. */
            case 0x31: /* Invalid DRAM ID. */
            case 0x32: /* Invalid size. */
            case 0x33: /* Invalid arguement. */
            case 0x34: /* Bad GPT. */
            case 0x35: /* Failed to boot SafeMode. */
            case 0x36: /* Activity monitor fired (4.0.0+). */
                color = PANIC_COLOR_BOOTLOADER_GENERIC;
                break;
            case 0x40: /* Kernel panic. */
                color = PANIC_COLOR_KERNEL;
                break;
            default:
                color = code >> 20;
                color |= color << 4;
                break;
        }

        if (g_panic_code == 0) {
            display_init();
        }

        display_color_screen(color);
        wait_for_button_and_reboot();
    } else {
        g_panic_code = 0;
        APBDEV_PMC_SCRATCH200_0 = 0;
    }
}

__attribute__ ((noreturn)) void panic(uint32_t code) {
    /* Set panic code. */
    if (g_panic_code == 0) {
        g_panic_code = code;
        APBDEV_PMC_SCRATCH200_0 = code;
    }

    fuse_disable_programming();
    APBDEV_PMC_CRYPTO_OP_0 = 1; /* Disable all SE operations. */

    check_and_display_panic();
    while(true);
}
