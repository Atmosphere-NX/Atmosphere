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

#include "utils.h"
#include "exception_handlers.h"
#include "panic.h"
#include "fuse.h"
#include "hwinit.h"
#include "di.h"
#include "timers.h"
#include "fs_utils.h"
#include "stage2.h"
#include "chainloader.h"
#include "../../../fusee/common/sdmmc/sdmmc.h"
#include "../../../fusee/common/fatfs/ff.h"
#include "../../../fusee/common/log.h"
#include "../../../fusee/common/vsprintf.h"
#include "../../../fusee/common/ini.h"
#include "../../../fusee/common/display/video_fb.h"

extern void (*__program_exit_callback)(int rc);

static void *g_framebuffer;
static char g_bct0_buffer[BCTO_MAX_SIZE];

#define CONFIG_LOG_LEVEL_KEY "log_level"

static const char *get_default_bct0(void) {
    return "BCT0\n"
           "[stage1]\n"
           "stage2_path = atmosphere/fusee-secondary.bin\n"
           "stage2_mtc_path = atmosphere/fusee-mtc.bin\n"
           "stage2_addr = 0xF0000000\n"
           "stage2_entrypoint = 0xF0000000\n"
           "\n"
           "[stratosphere]\n"
           "\n";
}

static const char *load_config(void) {
    if (!read_from_file(g_bct0_buffer, BCTO_MAX_SIZE, "atmosphere/config/BCT.ini")) {
        print(SCREEN_LOG_LEVEL_DEBUG, "Failed to read BCT0 from SD!\n");
        print(SCREEN_LOG_LEVEL_DEBUG, "Using default BCT0!\n");

        const char * const default_bct0 = get_default_bct0();
        memcpy(g_bct0_buffer, default_bct0, strlen(default_bct0));
    }

    if (memcmp(g_bct0_buffer, "BCT0", 4) != 0) {
        fatal_error("Unexpected magic in BCT.ini!\n");
    }

    /* Return pointer to first line of the ini. */
    const char *bct0 = g_bct0_buffer;
    while (*bct0 && *bct0 != '\n') {
        bct0++;
    }

    if (!bct0) {
        fatal_error("BCT.ini has no newline!\n");
    }

    return bct0;
}

static int config_ini_handler(void *user, const char *section, const char *name, const char *value) {
    if (strcmp(section, "config") == 0) {
        if (strcmp(name, CONFIG_LOG_LEVEL_KEY) == 0) {
            ScreenLogLevel *config_log_level = (ScreenLogLevel *)user;
            int log_level = 0;
            sscanf(value, "%d", &log_level);
            *config_log_level = (ScreenLogLevel)log_level;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

static void setup_display(void) {
    g_framebuffer = (void *)0xC0000000;

    /* Zero-fill the framebuffer and register it as printk provider. */
    video_init(g_framebuffer);

    /* Initialize the display. */
    display_init();

    /* Set the framebuffer. */
    display_init_framebuffer(g_framebuffer);

    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_backlight(true);
}

static void cleanup_display(void) {
    /* Turn off the backlight. */
    display_backlight(false);

    /* Terminate the display. */
    display_end();
}

static void setup_env(void) {
    /* Initialize hardware. */
    nx_hwinit(false);

    /* Set up the exception handlers. */
    setup_exception_handlers();

    /* Mount the SD card. */
    mount_sd();
}

static void cleanup_env(void) {
    /* Unmount the SD card. */
    unmount_sd();
}

static void exit_callback(int rc) {
    (void)rc;
    relocate_and_chainload();
}

int main(void) {
    const char *bct0;
    const char *stage2_path;
    stage2_args_t *stage2_args;
    uint32_t stage2_version = 0;
    ScreenLogLevel log_level = SCREEN_LOG_LEVEL_NONE;

    /* Initialize the boot environment. */
    setup_env();

    /* Check for panics. */
    check_and_display_panic();

    /* Load the BCT0 configuration ini off of the SD. */
    bct0 = load_config();

    /* Extract the logging level from the BCT.ini file. */
    if (ini_parse_string(bct0, config_ini_handler, &log_level) < 0) {
        fatal_error("Failed to parse BCT.ini!\n");
    }

    /* Override the global logging level. */
    log_set_log_level(log_level);

    if (log_level != SCREEN_LOG_LEVEL_NONE) {
        /* Initialize the display for debugging. */
        setup_display();
    }

    /* Say hello. */
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "Welcome to Atmosph\xe8re Fus\xe9" "e!\n");
    print(SCREEN_LOG_LEVEL_DEBUG, "Using color linear framebuffer at 0x%p!\n", g_framebuffer);

    /* Load the loader payload into DRAM. */
    load_stage2(bct0);

    /* Setup argument data. */
    stage2_path = stage2_get_program_path();
    strcpy(g_chainloader_arg_data, stage2_path);
    stage2_args = (stage2_args_t *)(g_chainloader_arg_data + strlen(stage2_path) + 1); /* May be unaligned. */
    memcpy(&stage2_args->version, &stage2_version, 4);
    memcpy(&stage2_args->log_level, &log_level, sizeof(log_level));
    strcpy(stage2_args->bct0, bct0);
    g_chainloader_argc = 2;

    /* Terminate the boot environment. */
    cleanup_env();

    if (log_level != SCREEN_LOG_LEVEL_NONE) {
        /* Wait a while for debugging. */
        mdelay(1000);

        /* Terminate the display for debugging. */
        cleanup_display();
    }

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, jump to Stage2. */
    __program_exit_callback = exit_callback;
    return 0;
}