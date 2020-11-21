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
#include "lib/fatfs/ff.h"
#include "lib/log.h"

FATFS sd_fs;
static int g_sd_device_rc = 0;
static int g_sd_fs_rc = 0;
sdmmc_t g_sd_sdmmc;
sdmmc_device_t g_sd_device;

bool acquire_sd_device(void)
{
	/* Already initialized. */
	if (g_sd_device_rc++ > 0)
		return true;

    /* Enable AHB redirection if necessary. */
    mc_acquire_ahb_redirect();
	
	if (sdmmc_device_sd_init(&g_sd_device, &g_sd_sdmmc, SDMMC_BUS_WIDTH_4BIT, SDMMC_SPEED_UHS_SDR104))
		return true;

	mc_release_ahb_redirect();
	
	g_sd_device_rc--;

	return false;
}

void release_sd_device(void)
{
	/* No need to finalize. */
	if (--g_sd_device_rc > 0)
		return;

	sdmmc_device_finish(&g_sd_device);

	/* Disable AHB redirection if necessary. */
	mc_release_ahb_redirect();
}

bool mount_sd(void)
{
    /* Already mounted. */
    if (g_sd_fs_rc++ > 0)
        return true;

    /* Make sure SD device is initialized. */
    if (!acquire_sd_device()) {
	    g_sd_fs_rc--;
	    return false;
    }
    
    /* Mount SD filesystem. */
    if (f_mount(&sd_fs, "", 1) == FR_OK) {
	    print(SCREEN_LOG_LEVEL_INFO, "Mounted SD card!\n");
	    return true;
    }

    release_sd_device();

    g_sd_fs_rc--;

    return false;
}

void unmount_sd(void)
{
	if (--g_sd_fs_rc > 0)
		return;
	
	f_mount(NULL, "", 1);

	release_sd_device();
}

void temporary_unmount_sd(bool *was_mounted)
{
	if (g_sd_fs_rc > 0) {
		f_mount(NULL, "", 1);
		*was_mounted = true;
	} else {
		*was_mounted = false;
	}
}

void temporary_remount_sd(void)
{
    if (f_mount(&sd_fs, "", 1) != FR_OK) {
	    fatal_error("Failed to remount SD card after temporary operation!\n");
    }
}

uint32_t get_file_size(const char *filename)
{
    /* SD card hasn't been mounted yet. */
    if (g_sd_fs_rc == 0)
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
    if (g_sd_fs_rc == 0)
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
    if (g_sd_fs_rc == 0)
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
