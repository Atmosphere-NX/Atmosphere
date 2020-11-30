/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fs_utils.h"
#include "mc.h"
#include "../../../fusee/common/fatfs/ff.h"
#include "../../../fusee/common/log.h"

FATFS sd_fs;
static bool g_sd_mounted = false;
static bool g_sd_initialized = false;
static bool g_ahb_redirect_enabled = false;
sdmmc_t g_sd_sdmmc;
sdmmc_device_t g_sd_device;


bool mount_sd(void)
{
    /* Already mounted. */
    if (g_sd_mounted)
        return true;

    /* Enable AHB redirection if necessary. */
    if (!g_ahb_redirect_enabled) {
        mc_enable_ahb_redirect();
        g_ahb_redirect_enabled = true;
    }

    if (!g_sd_initialized) {
        /* Initialize SD. */
        if (sdmmc_device_sd_init(&g_sd_device, &g_sd_sdmmc, SDMMC_BUS_WIDTH_4BIT, SDMMC_SPEED_SD_SDR104))
        {
            g_sd_initialized = true;

            /* Mount SD. */
            if (f_mount(&sd_fs, "", 1) == FR_OK) {
                print(SCREEN_LOG_LEVEL_INFO, "Mounted SD card!\n");
                g_sd_mounted = true;
            }
        }
        else
            fatal_error("Failed to initialize the SD card!.\n");
    }

    return g_sd_mounted;
}

void unmount_sd(void)
{
    if (g_sd_mounted)
    {
        f_mount(NULL, "", 1);
        sdmmc_device_finish(&g_sd_device);
        g_sd_mounted = false;
    }

    /* Disable AHB redirection if necessary. */
    if (g_ahb_redirect_enabled) {
        mc_disable_ahb_redirect();
        g_ahb_redirect_enabled = false;
    }
}

uint32_t get_file_size(const char *filename)
{
    /* SD card hasn't been mounted yet. */
    if (!g_sd_mounted)
        return 0;

    /* Open the file for reading. */
    FIL f;
    if (f_open(&f, filename, FA_READ) != FR_OK)
        return 0;

    /* Get the file size. */
    uint32_t file_size = f_size(&f);

    /* Close the file. */
    f_close(&f);

    return file_size;
}

int read_from_file(void *dst, uint32_t dst_size, const char *filename)
{
    /* SD card hasn't been mounted yet. */
    if (!g_sd_mounted)
        return 0;

    /* Open the file for reading. */
    FIL f;
    if (f_open(&f, filename, FA_READ) != FR_OK)
        return 0;

    /* Sync. */
    f_sync(&f);

    /* Read from file. */
    UINT br = 0;
    int res = f_read(&f, dst, dst_size, &br);
    f_close(&f);

    return (res == FR_OK) ? (int)br : 0;
}

int write_to_file(void *src, uint32_t src_size, const char *filename)
{
    /* SD card hasn't been mounted yet. */
    if (!g_sd_mounted)
        return 0;

    /* Open the file for writing. */
    FIL f;
    if (f_open(&f, filename, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
        return 0;

    /* Sync. */
    f_sync(&f);

    /* Write to file. */
    UINT bw = 0;
    int res = f_write(&f, src, src_size, &bw);
    f_close(&f);

    return (res == FR_OK) ? (int)bw : 0;
}