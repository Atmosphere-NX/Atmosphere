#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include "utils.h"
#include "hwinit.h"
#include "loader.h"
#include "chainloader.h"
#include "stage2.h"
#include "nxboot.h"
#include "console.h"
#include "sd_utils.h"
#include "fs_dev.h"
#include "display/video_fb.h"

extern void (*__program_exit_callback)(int rc);

static stage2_args_t *g_stage2_args;
static void *g_framebuffer;
static bool g_do_nxboot;

static void setup_env(void) {
    g_framebuffer = memalign(0x1000, CONFIG_VIDEO_VISIBLE_ROWS * CONFIG_VIDEO_COLS * CONFIG_VIDEO_PIXEL_SIZE);

    /* Note: the framebuffer needs to be >= 0xC0000000, this is fine because of where our heap is. */
    if (g_framebuffer == NULL) {
        generic_panic();
    }

    /* Zero-fill the framebuffer and set the console up. */
    console_init(g_framebuffer);

    /* Initialize the display. */
    display_init();

    /* Set the framebuffer. */
    display_init_framebuffer(g_framebuffer);

    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_enable_backlight(true);

    initialize_sd();
    if(fsdev_mount_all() == -1) {
        perror("Failed to mount at least one FAT parition");
        generic_panic();
    }
    fsdev_set_default_device("sdmc");

    /* TODO: What other hardware init should we do here? */
}

static void cleanup_env(void) {
    /* Unmount everything (this causes all open files to be flushed and closed) */
    fsdev_unmount_all();

    /* Deinitialize the framebuffer and display */
    /*display_enable_backlight(false);
    display_end();
    free(g_framebuffer);
    g_framebuffer = NULL;*/
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

    /* Initialize the display, console, FS, etc. */
    setup_env();

    if (argc != STAGE2_ARGC) {
        printf("Error: Invalid argc (expected %d, got %d)!\n", STAGE2_ARGC, argc);
        generic_panic();
    }
    g_stage2_args = (stage2_args_t *)argv[STAGE2_ARGV_ARGUMENT_STRUCT];

    if(g_stage2_args->version != 0) {
        printf("Error: Incorrect Stage2 args version (expected %lu, got %lu)!\n", 0ul, g_stage2_args->version);
        generic_panic();
    }

    printf(u8"Welcome to Atmosphère Fusée Stage 2!\n");
    printf("Stage 2 executing from: %s\n", (const char *)argv[STAGE2_ARGV_PROGRAM_PATH]);

    /* This will load all remaining binaries off of the SD. */
    load_payload(g_stage2_args->bct0);

    printf("Loaded payloads!\n");

    g_do_nxboot = loader_ctx->chainload_entrypoint == 0;
    if (g_do_nxboot) {
        nxboot_main();
    } else {
        /* TODO: What else do we want to do in terms of argc/argv? */
        const char *path = get_loader_ctx()->file_paths[get_loader_ctx()->file_id_of_entrypoint];
        g_chainloader_argc = 1;
        strcpy(g_chainloader_arg_data, path);
    }

    /* Deinitialize the display, console, FS, etc. */
    cleanup_env();

    /* Finally, after the cleanup routines (__libc_fini_array, etc.) are called, chainload or halt ourselves. */
    __program_exit_callback = exit_callback;

    return 0;
}
