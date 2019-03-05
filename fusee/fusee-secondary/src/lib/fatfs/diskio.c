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
#include "ffconf.h"
#include "../../device_partition.h"

/* fs_dev.c */
extern device_partition_t *g_volume_to_devparts[FF_VOLUMES];

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	device_partition_t *devpart = g_volume_to_devparts[pdrv];
    if (devpart)
        return devpart->initialized ? RES_OK : STA_NOINIT;
    else
        return STA_NODISK;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	/* We aren't using FF_MULTI_PARTITION, so pdrv = volume id. */
	device_partition_t *devpart = g_volume_to_devparts[pdrv];
	if (!devpart)
		return STA_NODISK;
	else if (devpart->initializer)
		return devpart->initializer(devpart) ? STA_NOINIT : RES_OK;
	else 
		return RES_OK;
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
	/* We aren't using FF_MULTI_PARTITION, so pdrv = volume id. */
	device_partition_t *devpart = g_volume_to_devparts[pdrv];
	if (!devpart)
		return RES_PARERR;
	else if (devpart->reader)
		return device_partition_read_data(devpart, buff, sector, count) ? RES_ERROR : RES_OK;
	else
		return RES_ERROR;
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
	/* We aren't using FF_MULTI_PARTITION, so pdrv = volume id. */
	device_partition_t *devpart = g_volume_to_devparts[pdrv];
	if (!devpart)
		return RES_PARERR;
	else if (devpart->writer)
		return device_partition_write_data(devpart, buff, sector, count) ? RES_ERROR : RES_OK;
	else
		return RES_ERROR;
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
	device_partition_t *devpart = g_volume_to_devparts[pdrv];
	switch (cmd) {
        case GET_SECTOR_SIZE:
            *(WORD *)buff = devpart ? (WORD)devpart->sector_size : 512;
            return RES_OK;
        default:
            return RES_OK;
	}
}

