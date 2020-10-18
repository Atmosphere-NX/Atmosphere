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
#include "hwinit.h"
#include "timers.h"
#include "fs_utils.h"
#include "stage2.h"
#include "chainloader.h"
#include "sdmmc/sdmmc.h"
#include "lib/fatfs/ff.h"
#include "lib/log.h"
#include "lib/vsprintf.h"
#include "display/video_fb.h"
#include "fastboot/fastboot.h"

extern void (*__program_exit_callback)(int rc);

static char g_bct0_buffer[BCTO_MAX_SIZE] __attribute__((section(".dram"))) = {0};

#define DEFAULT_BCT0 \
"BCT0\n"\
"[stage1]\n"\
"stage2_path = atmosphere/fusee-secondary.bin\n"\
"stage2_mtc_path = atmosphere/fusee-mtc.bin\n"\
"stage2_addr = 0xF0000000\n"\
"stage2_entrypoint = 0xF0000000\n"\
"[exosphere]\n"\
"debugmode = 1\n"\
"debugmode_user = 0\n"\
"disable_user_exception_handlers = 0\n"\
"[stratosphere]\n" \
"[fastboot]\n" \
"force_enable = 0\n" \
"button_timeout_ms = 0\n"

static const char *load_config(void) {
    if (!read_from_file(g_bct0_buffer, BCTO_MAX_SIZE, "atmosphere/config/BCT.ini")) {
        print(SCREEN_LOG_LEVEL_DEBUG, "Failed to read BCT0 from SD!\n");
        print(SCREEN_LOG_LEVEL_DEBUG, "Using default BCT0!\n");
        memcpy(g_bct0_buffer, DEFAULT_BCT0, sizeof(DEFAULT_BCT0));
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

static void setup_env(void) {
    /* Initialize hardware. */
    nx_hwinit();

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
    const char *bct0_string;
    stage2_args_t *stage2_args;
    uint32_t stage2_version = 0;
    bct0_t bct0;

    /* Initialize the boot environment. */
    setup_env();

    /* Check for panics. */
    check_and_display_panic();

    /* Load the BCT0 configuration ini off of the SD. */
    bct0_string = load_config();

    /* Parse the BCT0 configuration ini. */
    if (bct0_parse(bct0_string, &bct0) < 0) {
        fatal_error("Failed to parse BCT.ini!\n");
    }

    /* Override the global logging level. */
    log_set_log_level(bct0.log_level);

    if (bct0.log_level != SCREEN_LOG_LEVEL_NONE) {
        /* Initialize the display for debugging. */
        log_setup_display();
    }

    /* Say hello. */
    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, "Welcome to Atmosph\xe8re Fus\xe9" "e!\n");
    print(SCREEN_LOG_LEVEL_DEBUG, "Using color linear framebuffer at 0x%p!\n", log_get_display_framebuffer());

    /* Run the MTC binary. */
    if (!stage2_run_mtc(&bct0)) {
        print(SCREEN_LOG_LEVEL_WARNING, "DRAM training failed! Continuing with untrained DRAM.\n");
    }

    /* Assert that our configuration is sane. */
    stage2_validate_config(&bct0);

    /* Try to enter fastboot, if we are configured to. */
    switch(fastboot_enter(&bct0)) {
    case FASTBOOT_INVALID:
    case FASTBOOT_SKIPPED:
    case FASTBOOT_LOAD_STAGE2:
        /* Load the loader payload into DRAM. */
        stage2_load(&bct0);
        break;
    case FASTBOOT_CHAINLOAD:
        print(SCREEN_LOG_LEVEL_DEBUG, "fastboot: chainloading\n");
        break;
    }

    /* Setup argument data. */
    strcpy(g_chainloader_arg_data, bct0.stage2_path);
    stage2_args = (stage2_args_t *)(g_chainloader_arg_data + strlen(bct0.stage2_path) + 1); /* May be unaligned. */
    memcpy(&stage2_args->version, &stage2_version, 4);
    memcpy(&stage2_args->log_level, &bct0.log_level, sizeof(bct0.log_level));
    strcpy(stage2_args->bct0, bct0_string);
    g_chainloader_argc = 2;

    /* Terminate the boot environment. */
    cleanup_env();

    if (bct0.log_level != SCREEN_LOG_LEVEL_NONE) {
        /* Wait a while for debugging. */
        mdelay(1000);

        /* Terminate the display for debugging. */
        log_cleanup_display();
    }

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, jump to Stage2. */
    __program_exit_callback = exit_callback;
    return 0;
}
