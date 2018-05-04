#include "sd_utils.h"
#include "hwinit.h"
#include "sdmmc.h"
#include "lib/printk.h"
#include "lib/fatfs/ff.h"

/* This is used by diskio.h. */
struct mmc sd_mmc;
FATFS sd_fs;
static int initialized_sd = 0;
static int mounted_sd = 0;

void save_sd_state(void **mmc, void **ff) {
    *mmc = &sd_mmc;
    *ff = &sd_fs;
}
void resume_sd_state(void *mmc, void *ff) {
    sd_mmc = *(struct mmc *)mmc;
    sd_fs = *(FATFS *)ff;
    initialized_sd = 1;
    mounted_sd = 1;
}

int initialize_sd(void) {
    if (initialized_sd) {
        return 1;
    }
    mc_enable_ahb_redirect();
    if (sdmmc_init(&sd_mmc, SWITCH_MICROSD) == 0) {
        printk("Initialized SD card!\n");
        initialized_sd = 1;
    }
    return initialized_sd;
}

int mount_sd(void) {
    if (mounted_sd) {
        return 1;
    }
    if (f_mount(&sd_fs, "", 1) == FR_OK) {
        printk("Mounted SD card!\n");
        mounted_sd = 1;
    }
    return mounted_sd;
}

size_t read_sd_file(void *dst, size_t dst_size, const char *filename) {
    if (!initialized_sd && initialize_sd() == 0) {
        return 0;
    }
    if (!mounted_sd && mount_sd() == 0) {
        return 0;
    }

    FIL f;
    if (f_open(&f, filename, FA_READ) != FR_OK) {
        return 0;
    }

    UINT br;
    int res = f_read(&f, dst, dst_size, &br);
    f_close(&f);

    return res == FR_OK ? (int)br : 0;
}
