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
#include "nxboot.h"
#include "console.h"
#include "fs_utils.h"
#include "nxfs.h"
#include "gpt.h"
#include "../../../fusee/common/display/video_fb.h"
#include "../../../fusee/common/sdmmc/sdmmc.h"
#include "../../../fusee/common/log.h"

extern void (*__program_exit_callback)(int rc);

static __attribute__((__aligned__(0x200))) stage2_args_t g_stage2_args_store;
static stage2_args_t *g_stage2_args;
static bool g_do_nxboot;

static void setup_env(void) {
    /* Initialize the display and console. */
    if (console_init() < 0) {
        generic_panic();
    }

    /* Set up exception handlers. */
    setup_exception_handlers();

    /* Initialize the file system by mounting the SD card. */
    if (nxfs_init() < 0) {
        fatal_error("Failed to initialize the file system: %s\n", strerror(errno));
    }
}

static void cleanup_env(void) {
    /* Terminate the file system. */
    nxfs_end();
}

static void exit_callback(int rc) {
    (void)rc;
    if (g_do_nxboot) {
        /* TODO: halt */
    } else {
        relocate_and_chainload();
    }
}

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

/* Allow for main(int argc, void **argv) signature. */
#pragma GCC diagnostic ignored "-Wmain"

int main(int argc, void **argv) {
    loader_ctx_t *loader_ctx = get_loader_ctx();

    /* Check argc. */
    if (argc != STAGE2_ARGC) {
        generic_panic();
    }

    g_stage2_args = &g_stage2_args_store;
    memcpy(g_stage2_args, (stage2_args_t *)argv[STAGE2_ARGV_ARGUMENT_STRUCT], sizeof(*g_stage2_args));

    /* Check stage2 version field. */
    if (g_stage2_args->version != 0) {
        generic_panic();
    }

    /* Override the global logging level. */
    log_set_log_level(g_stage2_args->log_level);

    /* Initialize the boot environment. */
    setup_env();

    print(SCREEN_LOG_LEVEL_DEBUG | SCREEN_LOG_LEVEL_NO_PREFIX, u8"Welcome to Atmosphère Fusée Stage 2!\n");
    print(SCREEN_LOG_LEVEL_DEBUG, "Stage 2 executing from: %s\n", (const char *)argv[STAGE2_ARGV_PROGRAM_PATH]);

    /* Load BCT0 from SD if needed. */
    if (strcmp(g_stage2_args->bct0, "") == 0) {
        uint32_t bct_tmp_buf[sizeof(g_stage2_args->bct0) / sizeof(uint32_t)] = {0};
        if (!read_from_file(bct_tmp_buf, sizeof(bct_tmp_buf) - 1, "atmosphere/config/BCT.ini")) {
            const char * const default_bct0 = get_default_bct0();
            memcpy(bct_tmp_buf, default_bct0, strlen(default_bct0));
        }
        memcpy(g_stage2_args->bct0, bct_tmp_buf, sizeof(bct_tmp_buf));
    }

    /* This will load all remaining binaries off of the SD. */
    load_payload(g_stage2_args->bct0);
    print(SCREEN_LOG_LEVEL_INFO, "Loaded payloads!\n");

    g_do_nxboot = (loader_ctx->chainload_entrypoint == 0);
    if (g_do_nxboot) {
        print(SCREEN_LOG_LEVEL_INFO, "Now performing nxboot.\n");

        /* Start boot. */
        uint32_t boot_memaddr = nxboot_main();

        /* Terminate the boot environment. */
        cleanup_env();

        /* Finish boot. */
        nxboot_finish(boot_memaddr);
    } else {
        /* TODO: What else do we want to do in terms of argc/argv? */
        const char *path = get_loader_ctx()->file_paths_to_load[get_loader_ctx()->file_id_of_entrypoint];
        print(SCREEN_LOG_LEVEL_INFO, "Now chainloading.\n");
        g_chainloader_argc = 1;
        strcpy(g_chainloader_arg_data, path);

        /* Terminate the boot environment. */
        cleanup_env();
    }

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, chainload or halt ourselves. */
    __program_exit_callback = exit_callback;

    return 0;
}
