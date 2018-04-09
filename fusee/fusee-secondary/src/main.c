#include "utils.h"
#include "hwinit.h"
#include "loader.h"
#include "stage2.h"
#include "lib/printk.h"
#include "display/video_fb.h"


/* Allow for main(int argc, void **argv) signature. */
#pragma GCC diagnostic ignored "-Wmain"

int main(int argc, void **argv) {
    entrypoint_t entrypoint;
    stage2_args_t *args;
    
    if (argc != STAGE2_ARGC || ((args = (stage2_args_t *)argv[STAGE2_ARGV_ARGUMENT_STRUCT])->version != 0)) {
        generic_panic();
    }
    
    /* TODO: What other hardware init should we do here? */
    
    /* Setup LFB. */
    video_resume(args->lfb, args->console_row, args->console_col);
    
    printk("Welcome to Atmosph\xe8re Fus\xe9" "e Stage 2!\n");
    printk("Stage 2 executing from: %s\n", (const char *)argv[STAGE2_ARGV_PROGRAM_PATH]);
    
    /* This will load all remaining binaries off of the SD. */
    entrypoint = load_payload(args->bct0);
    
    /* TODO: What do we want to do in terms of argc/argv? */
    entrypoint(0, NULL);
    return 0;
}

