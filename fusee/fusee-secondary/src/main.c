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

static stage2_args_t *g_stage2_args;

/* Allow for main(int argc, void **argv) signature. */
#pragma GCC diagnostic ignored "-Wmain"

int main(int argc, void **argv) {
    loader_ctx_t *loader_ctx = get_loader_ctx();
    void *framebuffer = memalign(0x1000, CONFIG_VIDEO_VISIBLE_ROWS * CONFIG_VIDEO_COLS * CONFIG_VIDEO_PIXEL_SIZE);

    /* Initialize the display. */
    display_init();

    if (framebuffer == NULL) {
        generic_panic();
    }

    /* Initalize the framebuffer and console/stdout */
    display_init_framebuffer(framebuffer);
    console_init(framebuffer);

    /* Turn on the backlight after initializing the lfb */
    /* to avoid flickering. */
    display_enable_backlight(true);

    if (argc != STAGE2_ARGC) {
        generic_panic();
    }
    g_stage2_args = (stage2_args_t *)argv[STAGE2_ARGV_ARGUMENT_STRUCT];

    if(g_stage2_args->version != 0) {
        generic_panic();
    }

    initialize_sd();
    if(fsdev_mount_all() == -1) {
        perror("Failed to mount at least one FAT parition");
    }
    fsdev_set_default_device("sdmc");

    /* TODO: What other hardware init should we do here? */

    printf(u8"Welcome to Atmosphère Fusée Stage 2!\n");
    printf("Stage 2 executing from: %s\n", (const char *)argv[STAGE2_ARGV_PROGRAM_PATH]);

    /* This will load all remaining binaries off of the SD. */
    load_payload(g_stage2_args->bct0);

    printf("Loaded payloads!\n");

    /* Unmount everything (this causes all open files to be flushed and closed) */
    fsdev_unmount_all();

    /* Deinitialize the framebuffer and display */
    display_enable_backlight(false);
    display_end();
    free(framebuffer);

    if (loader_ctx->chainload_entrypoint != 0) {
        /* TODO: What else do we want to do in terms of argc/argv? */
        const char *path = get_loader_ctx()->file_paths[get_loader_ctx()->file_id_of_entrypoint];
        strcpy(g_chainloader_arg_data, path);
        relocate_and_chainload(1);
    } else {
        /* If we don't have a chainload entrypoint set, we're booting Horizon. */
        nxboot_main();
    }
    return 0;
}
