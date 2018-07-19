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
#include "display/video_fb.h"
#include "sdmmc/sdmmc.h"

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

    /* TODO: What other hardware init should we do here? */
}

static void cleanup_env(void) {
    /* Unmount everything (this causes all open files to be flushed and closed) */
    nxfs_unmount_all();
    //console_end();
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
    
    /* Set the SDMMC's driver logging level. */
    sdmmc_set_log_level(SDMMC_LOG_INFO);
    
    /* Initialize the display, console, FS, etc. */
    setup_env();

    printf(u8"Welcome to Atmosphère Fusée Stage 2!\n");
    printf("Stage 2 executing from: %s\n", (const char *)argv[STAGE2_ARGV_PROGRAM_PATH]);

    /* This will load all remaining binaries off of the SD. */
    load_payload(g_stage2_args->bct0);

    printf("Loaded payloads!\n");

    g_do_nxboot = loader_ctx->chainload_entrypoint == 0;
    if (g_do_nxboot) {
        printf("Now performing nxboot.\n");
        nxboot_main();
    } else {
        /* TODO: What else do we want to do in terms of argc/argv? */
        const char *path = get_loader_ctx()->file_paths_to_load[get_loader_ctx()->file_id_of_entrypoint];
        printf("Now chainloading.\n");
        g_chainloader_argc = 1;
        strcpy(g_chainloader_arg_data, path);
    }

    /* Deinitialize the display, console, FS, etc. */
    cleanup_env();

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, chainload or halt ourselves. */
    __program_exit_callback = exit_callback;

    return 0;
}
