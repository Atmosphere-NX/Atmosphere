/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdbool.h>
#include <string.h>
#include "diskio.h"		/* FatFs lower layer API */
#include "../../sdmmc.h"
#include "../../hwinit.h"

static bool g_ahb_redirect_enabled = false;

/* Global sd struct. */
static struct mmc g_sd_mmc = {0};
static bool g_sd_initialized = false;

int initialize_sd_mmc(void) {
	if (!g_ahb_redirect_enabled) {
		mc_enable_ahb_redirect();
		g_ahb_redirect_enabled = true;
	}

	if (!g_sd_initialized) {
		int rc = sdmmc_init(&g_sd_mmc, SWITCH_MICROSD);
		if (rc == 0) {
			g_sd_initialized = true;
			return 0;
		} else {
			return rc;
		}
	} else {
		return 0;
	}
}

/*
Uncomment if needed:
static struct mmc nand_mmc = {0};
static bool g_nand_initialized = false;

int initialize_nand_mmc(void) {
	if (!g_ahb_redirect_enabled) {
		mc_enable_ahb_redirect();
		g_ahb_redirect_enabled = true;
	}

	if (!g_nand_initialized) {
		int rc = sdmmc_init(&g_sd_mmc, SWITCH_EMMC);
		if (rc == 0) {
			g_nand_initialized = true;
			return 0;
		} else {
			return rc;
		}
	} else {
		return 0;
	}
}

*/

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	switch (pdrv) {
		case 0:
			return initialize_sd_mmc() == 0 ? 0 : STA_NOINIT;
		default:
			return STA_NODISK;
	}
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	switch (pdrv) {
		case 0:
			return sdmmc_read(&g_sd_mmc, buff, sector, count) == 0 ? RES_OK : RES_ERROR;
		default:
			return RES_PARERR;
	}
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	switch (pdrv) {
		case 0:
			return sdmmc_write(&g_sd_mmc, buff, sector, count) == 0 ? RES_OK : RES_ERROR;
		default:
			return RES_PARERR;
	}
}



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	return 0;
}

