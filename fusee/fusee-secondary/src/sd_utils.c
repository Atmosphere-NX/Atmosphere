#include <stdio.h>
#include "sd_utils.h"
#include "hwinit.h"
#include "sdmmc.h"

/* This is used by diskio.h. */
struct mmc sd_mmc;
static int initialized_sd = 0;

int initialize_sd(void) {
    if (initialized_sd) {
        return 1;
    }
    mc_enable_ahb_redirect();
    if (sdmmc_init(&sd_mmc, SWITCH_MICROSD) == 0) {
        printf("Initialized SD card!\n");
        initialized_sd = 1;
    } else {
        printf("Failed to initialize the SD card!\n");
        return 0;
    }
    return initialized_sd;
}

size_t read_sd_file(void *dst, size_t dst_size, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return 0;
    } else {
        size_t sz = fread(dst, 1, dst_size, file);
        fclose(file);
        return sz;
    }
}
