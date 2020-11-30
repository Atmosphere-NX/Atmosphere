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

#include <stdio.h>
#include "panic.h"
#include "di.h"
#include "pmc.h"
#include "fuse.h"
#include "utils.h"
#include "fs_utils.h"
#include "../../../fusee/common/log.h"
#include "../../../fusee/common/display/video_fb.h"

#define PROGRAM_ID_AMS_MITM 0x010041544D530000ull
#define PROGRAM_ID_BOOT     0x0100000000000005ull

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
        case 0xF00:
	        return "Kernel Panic";
        case 0xFFD:
	        return "Stack overflow";
        case 0xFFE:
            return "std::abort() called";
        default:
            return "Unknown";
    }
}

static void _try_suggest_fix(const atmosphere_fatal_error_ctx *ctx) {
    /* Try to recognize certain errors automatically, and suggest fixes for them. */
    const char *suggestion = NULL;

    if (ctx->error_desc == 0xFFE) {
        if (ctx->program_id == PROGRAM_ID_AMS_MITM) {
            /* When a user has archive bits set improperly, attempting to create an automatic backup will fail */
            /* to create the file path with error 0x202 (fs::ResultPathNotFound()) */
            if (ctx->gprs[0] == 0x202) {
                /* When the archive bit error is occurring, it manifests as failure to create automatic backup. */
                /* Thus, we can search the stack for the automatic backups path. */
                const char * const automatic_backups_prefix = "automatic_backups/X" /* ..... */;
                const int prefix_len = strlen(automatic_backups_prefix);

                for (size_t i = 0; i + prefix_len < ctx->stack_dump_size; ++i) {
                    if (memcmp(&ctx->stack_dump[i], automatic_backups_prefix, prefix_len) == 0) {
                        suggestion = "The atmosphere directory may improperly have archive\n"
                                     "bits set. Please try running an archive bit fixer tool\n"
                                     "(for example, the one in Hekate).\n";
                        break;
                    }
                }
            } else if (ctx->gprs[0] == 0x249A02) { /* fs::ResultResultExFatUnavailable() */
                /* When a user installs non-exFAT firm but has an exFAT formatted SD card, this error will */
                /* be returned on attempt to access the SD card. */
                suggestion = "Your console has non-exFAT firmware installed, but your SD card\n"
                             "is formatted as exFAT. Format your SD card as FAT32, or manually\n"
                             "flash exFAT firmware to package2.\n";
            }
        } else if (ctx->program_id == PROGRAM_ID_BOOT) {
            /* 9.x -> 10.x updated the API for SvcQueryIoMapping. */
            /* This can cause the kernel to reject incorrect-ABI calls by boot when a partial update is applied */
            /* (older kernel in package2, for some reason). */
            for (size_t i = 0; i < 8; ++i) {
                if (ctx->gprs[i] == 0xF201) {
                    suggestion = "A partial update may have been improperly performed.\n"
                                 "To fix, try manually flashing latest package2 to MMC.\n"
                                 "\n"
                                 "For help doing this, seek support in the ReSwitched or\n"
                                 "Nintendo Homebrew discord servers.\n";
                    break;
                }
            }
        }
    } else if (ctx->error_desc == 0xF00) { /* Kernel Panic */
        suggestion = "Please contact SciresM#0524 on Discord, or create an issue\n"
                     "on the Atmosphere GitHub issue tracker. Thank you very much\n"
                     "for helping to test mesosphere.\n";
    }

    if (suggestion != NULL) {
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "\n%s", suggestion);
    }
}

static void _check_and_display_atmosphere_fatal_error(void) {
    /* Check for valid magic. */
    if (ATMOSPHERE_FATAL_ERROR_CONTEXT->magic != ATMOSPHERE_REBOOT_TO_FATAL_MAGIC   &&
        ATMOSPHERE_FATAL_ERROR_CONTEXT->magic != ATMOSPHERE_REBOOT_TO_FATAL_MAGIC_1 &&
        ATMOSPHERE_FATAL_ERROR_CONTEXT->magic != ATMOSPHERE_REBOOT_TO_FATAL_MAGIC_0)
    {
        return;
    }

    {
        /* Zero-fill the framebuffer and register it as printk provider. */
        video_init((void *)0xC0000000);

        /* Initialize the display. */
        display_init();

        /* Set the framebuffer. */
        display_init_framebuffer((void *)0xC0000000);

        /* Turn on the backlight after initializing the lfb */
        /* to avoid flickering. */
        display_backlight(true);

        /* Override the global logging level. */
        log_set_log_level(SCREEN_LOG_LEVEL_ERROR);

        /* Copy fatal error context to the stack. */
        atmosphere_fatal_error_ctx ctx = *(ATMOSPHERE_FATAL_ERROR_CONTEXT);

        /* Change magic to invalid, to prevent double-display of error/bootlooping. */
        ATMOSPHERE_FATAL_ERROR_CONTEXT->magic = 0xCCCCCCCC;

        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "A fatal error occurred when running Atmosph\xe8re.\n");
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "Program ID: %016llx\n", ctx.program_id);
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "Error Desc: %s (0x%x)\n", get_error_desc_str(ctx.error_desc), ctx.error_desc);

        /* Save context to the SD card. */
        {
            char filepath[0x40];
            snprintf(filepath, sizeof(filepath) - 1, "/atmosphere/fatal_errors/report_%016llx.bin", ctx.report_identifier);
            filepath[sizeof(filepath)-1] = 0;
            if (write_to_file(&ctx, sizeof(ctx), filepath) != sizeof(ctx)) {
                print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "Failed to save report to the SD card!\n");
            } else {
                print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "Report saved to %s\n", filepath);
            }
        }

        /* Try to print a fix suggestion via automatic error detection. */
        _try_suggest_fix(&ctx);

        /* Display error. */
        print(SCREEN_LOG_LEVEL_ERROR | SCREEN_LOG_LEVEL_NO_PREFIX, "\nPress POWER to reboot\n");
    }

    /* Wait for button and reboot. */
    wait_for_button_and_reboot();
}

void check_and_display_panic(void) {
    /* Handle a panic sent via a stratosphere module. */
    _check_and_display_atmosphere_fatal_error();

    /* We also handle our own panics. */
    bool has_panic = ((APBDEV_PMC_RST_STATUS_0 != 0) || (g_panic_code != 0));
    uint32_t code = (g_panic_code == 0) ? APBDEV_PMC_SCRATCH200_0 : g_panic_code;
    has_panic = has_panic && !((APBDEV_PMC_RST_STATUS_0 != 1) && (code == PANIC_CODE_SAFEMODE));

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

        /* Initialize the display. */
        display_init();

        /* Fill the screen. */
        display_color_screen(color);

        /* Wait for button and reboot. */
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
