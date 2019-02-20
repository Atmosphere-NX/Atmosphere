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

static void setup_env(void) {
    g_framebuffer = (void *)0xC0000000;

    /* Initialize hardware. */
    nx_hwinit();

    /* Check for panics. */
    check_and_display_panic();

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
    ScreenLogLevel log_level = SCREEN_LOG_LEVEL_MANDATORY;
    
    /* Override the global logging level. */
    log_set_log_level(log_level);
    
    /* Initialize the display, console, etc. */
    setup_env();
    
    /* Say hello. */
    print(SCREEN_LOG_LEVEL_MANDATORY, "Welcome to Atmosph\xe8re sept-secondary!\n");
    while (true) { }
    
    /* TODO: Derive keys. */
    
    /* TODO: Chainload to payload. */
    
    /* Wait a while. */
    mdelay(1000);
    
    /* Deinitialize the display, console, etc. */
    cleanup_env();

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, jump to Stage2. */
    __program_exit_callback = exit_callback;
    return 0;
}
