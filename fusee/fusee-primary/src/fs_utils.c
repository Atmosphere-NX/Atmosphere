#include "fs_utils.h"
#include "hwinit.h"
#include "sdmmc.h"
#include "lib/printk.h"
#include "lib/fatfs/ff.h"

FATFS sd_fs;
static int mounted_sd = 0;

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

size_t read_from_file(void *dst, size_t dst_size, const char *filename) {
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
