#include <stdio.h>
#include "sd_utils.h"
#include "hwinit.h"
#include "sdmmc.h"

/* This is used by diskio.h. */
struct mmc sd_mmc;
static int initialized_sd = 0;

void save_sd_state(void **mmc) {
    *mmc = &sd_mmc;
}
void resume_sd_state(void *mmc) {
    sd_mmc = *(struct mmc *)mmc;
    initialized_sd = 1;
}

int initialize_sd(void) {
    if (initialized_sd) {
        return 1;
    }
    mc_enable_ahb_redirect();
    if (sdmmc_init(&sd_mmc, SWITCH_MICROSD) == 0) {
        printf("Initialized SD card!\n");
        initialized_sd = 1;
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
