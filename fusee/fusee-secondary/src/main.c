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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>

#include "utils.h"
#include "panic.h"
#include "exception_handlers.h"
#include "loader.h"
#include "chainloader.h"
#include "stage2.h"
#include "mtc.h"
#include "nxboot.h"
#include "console.h"
#include "fs_utils.h"
#include "nxfs.h"
#include "gpt.h"
#include "splash_screen.h"
#include "display/video_fb.h"
#include "sdmmc/sdmmc.h"
#include "lib/log.h"

extern void (*__program_exit_callback)(int rc);

static stage2_args_t *g_stage2_args;
static bool g_do_nxboot;

static void setup_env(void) {
    /* Set the console up. */
    if (console_init(g_stage2_args->display_initialized) == -1) {
        generic_panic();
    }

    /* Set up exception handlers. */
    setup_exception_handlers();

    if (nxfs_mount_all() < 0) {
        fatal_error("Failed to mount at least one parition: %s\n", strerror(errno));
    }
    
    /* Train DRAM. */
    train_dram();
}


static void cleanup_env(void) {
    /* Unmount everything (this causes all open files to be flushed and closed) */
    nxfs_unmount_all();
}

static void exit_callback(int rc) {
    (void)rc;
    if (g_do_nxboot) {
        /* TODO: halt */
    } else {
        relocate_and_chainload();
    }
}

/* Allow for main(int argc, void **argv) signature. */
#pragma GCC diagnostic ignored "-Wmain"

int main(int argc, void **argv) {
    loader_ctx_t *loader_ctx = get_loader_ctx();

    if (argc != STAGE2_ARGC) {
        generic_panic();
    }
    
    g_stage2_args = (stage2_args_t *)argv[STAGE2_ARGV_ARGUMENT_STRUCT];

    if (g_stage2_args->version != 0) {
        generic_panic();
    }
    
    /* Override the global logging level. */
    log_set_log_level(g_stage2_args->log_level);
    
    /* Initialize the display, console, FS, etc. */
    setup_env();

    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, u8"Welcome to Atmosphère Fusée Stage 2!\n");
    print(SCREEN_LOG_LEVEL_DEBUG, "Stage 2 executing from: %s\n", (const char *)argv[STAGE2_ARGV_PROGRAM_PATH]);
    
    /* Load BCT0 from SD if needed. */
    if (strcmp(g_stage2_args->bct0, "") == 0) {
        read_from_file(g_stage2_args->bct0, sizeof(g_stage2_args->bct0) - 1, "atmosphere/BCT.ini");
        if (!read_from_file(g_stage2_args->bct0, sizeof(g_stage2_args->bct0) - 1, "atmosphere/BCT.ini")) {
            fatal_error("Failed to read BCT0 from SD!\n");
        }
    }

    /* This will load all remaining binaries off of the SD. */
    load_payload(g_stage2_args->bct0);

    print(SCREEN_LOG_LEVEL_INFO, "Loaded payloads!\n");

    g_do_nxboot = loader_ctx->chainload_entrypoint == 0;
    if (g_do_nxboot) {
        print(SCREEN_LOG_LEVEL_INFO, "Now performing nxboot.\n");
        uint32_t boot_memaddr = nxboot_main();
        /* Wait for the splash screen to have been displayed as long as it should be. */
        splash_screen_wait_delay();
        /* Cleanup environment. */
        cleanup_env();
        /* Finish boot. */
        nxboot_finish(boot_memaddr);
    } else {
        /* TODO: What else do we want to do in terms of argc/argv? */
        const char *path = get_loader_ctx()->file_paths_to_load[get_loader_ctx()->file_id_of_entrypoint];
        print(SCREEN_LOG_LEVEL_MANDATORY, "Now chainloading.\n");
        g_chainloader_argc = 1;
        strcpy(g_chainloader_arg_data, path);
    }

    /* Deinitialize the display, console, FS, etc. */
    cleanup_env();

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, chainload or halt ourselves. */
    __program_exit_callback = exit_callback;

    return 0;
}
