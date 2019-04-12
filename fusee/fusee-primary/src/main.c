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
 
#include "utils.h"
#include "exception_handlers.h"
#include "panic.h"
#include "hwinit.h"
#include "di.h"
#include "timers.h"
#include "fs_utils.h"
#include "stage2.h"
#include "chainloader.h"
#include "sdmmc/sdmmc.h"
#include "lib/fatfs/ff.h"
#include "lib/log.h"
#include "lib/vsprintf.h"
#include "lib/ini.h"
#include "display/video_fb.h"

extern void (*__program_exit_callback)(int rc);

static void *g_framebuffer;
static char g_bct0_buffer[BCTO_MAX_SIZE];

#define CONFIG_LOG_LEVEL_KEY "log_level"

#define DEFAULT_BCT0_FOR_DEBUG \
"BCT0\n"\
"[stage1]\n"\
"stage2_path = atmosphere/fusee-secondary.bin\n"\
"stage2_addr = 0xF0000000\n"\
"stage2_entrypoint = 0xF0000000\n"

static const char *load_config(void) {
    if (!read_from_file(g_bct0_buffer, BCTO_MAX_SIZE, "atmosphere/BCT.ini")) {
        print(SCREEN_LOG_LEVEL_DEBUG, "Failed to read BCT0 from SD!\n");
        print(SCREEN_LOG_LEVEL_DEBUG, "Using default BCT0!\n");
        memcpy(g_bct0_buffer, DEFAULT_BCT0_FOR_DEBUG, sizeof(DEFAULT_BCT0_FOR_DEBUG));
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

static void setup_env(void) {
    g_framebuffer = (void *)0xC0000000;

    /* Initialize hardware. */
    nx_hwinit();

    /* Zero-fill the framebuffer and register it as printk provider. */
    video_init(g_framebuffer);

    /* Initialize the display. */
    display_init();

    /* Set the framebuffer. */
    display_init_framebuffer(g_framebuffer);

    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_backlight(true);

    /* Set up the exception handlers. */
    setup_exception_handlers();
        
    /* Mount the SD card. */
    mount_sd();
}

static void cleanup_env(void) {
    /* Unmount the SD card. */
    unmount_sd();

    display_backlight(false);
    display_end();
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
    ScreenLogLevel log_level = SCREEN_LOG_LEVEL_MANDATORY;
    
    /* Override the global logging level. */
    log_set_log_level(log_level);
    
    /* Initialize the display, console, etc. */
    setup_env();

    /* Check for panics. */
    check_and_display_panic();
    
    /* Load the BCT0 configuration ini off of the SD. */
    bct0 = load_config();

    /* Extract the logging level from the BCT.ini file. */
    if (ini_parse_string(bct0, config_ini_handler, &log_level) < 0) {
        fatal_error("Failed to parse BCT.ini!\n");
    }
    
    /* Say hello. */
    print(SCREEN_LOG_LEVEL_MANDATORY, "Welcome to Atmosph\xe8re Fus\xe9" "e!\n");
    print(SCREEN_LOG_LEVEL_DEBUG, "Using color linear framebuffer at 0x%p!\n", g_framebuffer);
    
    /* Load the loader payload into DRAM. */
    load_stage2(bct0);

    /* Setup argument data. */
    stage2_path = stage2_get_program_path();
    strcpy(g_chainloader_arg_data, stage2_path);
    stage2_args = (stage2_args_t *)(g_chainloader_arg_data + strlen(stage2_path) + 1); /* May be unaligned. */
    memcpy(&stage2_args->version, &stage2_version, 4);
    memcpy(&stage2_args->log_level, &log_level, sizeof(log_level));
    stage2_args->display_initialized = false;
    strcpy(stage2_args->bct0, bct0);
    g_chainloader_argc = 2;
    
    /* Wait a while. */
    mdelay(1000);
    
    /* Deinitialize the display, console, etc. */
    cleanup_env();

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, jump to Stage2. */
    __program_exit_callback = exit_callback;
    return 0;
}
