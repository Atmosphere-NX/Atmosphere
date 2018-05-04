#include "utils.h"
#include "hwinit.h"
#include "loader.h"
#include "stage2.h"
#include "nxboot.h"
#include "sd_utils.h"
#include "lib/printk.h"
#include "display/video_fb.h"

/* TODO: Add a #define for this size, somewhere. Also, primary can only actually load 0x7000. */
char g_bct0[0x8000];


/* Allow for main(int argc, void **argv) signature. */
#pragma GCC diagnostic ignored "-Wmain"

void __init_heap(void) {
    extern char* fake_heap_start;
    extern char* fake_heap_end;

    fake_heap_start = (char*)0xF0000000;
    fake_heap_end   = (char*)0xFFF00000;
}

int main(int argc, void **argv) {
    stage2_args_t args = {0};
    loader_ctx_t *loader_ctx = get_loader_ctx();

    if (argc != STAGE2_ARGC || ((args = *((stage2_args_t *)argv[STAGE2_ARGV_ARGUMENT_STRUCT])).version != 0)) {
        generic_panic();
    }

    resume_sd_state((struct mmc *)args.sd_mmc, (FATFS *)args.sd_fs);

    /* Copy the BCT0 from unsafe primary memory into our memory. */
    strncpy(g_bct0, args.bct0, sizeof(g_bct0));

    /* TODO: What other hardware init should we do here? */

    /* Setup LFB. */
    video_resume(args.lfb, args.console_row, args.console_col);

    printk("Welcome to Atmosph\xe8re Fus\xe9" "e Stage 2!\n");
    printk("Stage 2 executing from: %s\n", (const char *)argv[STAGE2_ARGV_PROGRAM_PATH]);

    /* This will load all remaining binaries off of the SD. */
    load_payload(g_bct0);

    printk("Loaded payloads!\n");

    if (loader_ctx->chainload_entrypoint != NULL) {
        /* TODO: What do we want to do in terms of argc/argv? */
        loader_ctx->chainload_entrypoint(0, NULL);
    } else {
        /* If we don't have a chainload entrypoint set, we're booting Horizon. */
        nxboot_main();
    }
    return 0;
}

