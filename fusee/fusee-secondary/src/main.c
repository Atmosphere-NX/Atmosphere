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
    
    if (argc != STAGE2_ARGC || *((u32 *)argv[STAGE2_ARGV_VERSION]) != 0) {
        generic_panic();
    }
    
    /* TODO: What other hardware init should we do here? */
    
    /* Setup LFB. */
    /* TODO: How can we keep the console line/offset to resume printing? */
    video_init((u32 *)argv[STAGE2_ARGV_LFB]);
    
    printk("Welcome to Atmosph\xe8re Fus\xe9" "e Stage 2!\n");
    
    /* This will load all remaining binaries off of the SD. */
    entrypoint = load_payload((const char *)argv[STAGE2_ARGV_CONFIG]);
    
    /* TODO: What do we want to do in terms of argc/argv? */
    entrypoint(0, NULL);
    return 0;
}

