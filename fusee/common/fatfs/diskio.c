/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdbool.h>
#include <string.h>
#include "ff.h"         /* Obtains integer types */
#include "diskio.h"     /* Declarations of disk functions */
#include "ffconf.h"

#if defined(FUSEE_STAGE1_SRC)
#include "../../../fusee/fusee-primary/fusee-primary-main/src/fs_utils.h"
#elif defined(FUSEE_STAGE2_SRC)
#include "../../../fusee/fusee-secondary/src/device_partition.h"
#elif defined(SEPT_STAGE2_SRC)
#include "../../../sept/sept-secondary/src/fs_utils.h"
#endif

#ifdef FUSEE_STAGE2_SRC
/* fs_dev.c */
extern device_partition_t *g_volume_to_devparts[FF_VOLUMES];
#endif


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
    BYTE pdrv       /* Physical drive nmuber to identify the drive */
)
{
    #ifdef FUSEE_STAGE2_SRC
    device_partition_t *devpart = g_volume_to_devparts[pdrv];
    if (devpart)
        return devpart->initialized ? RES_OK : STA_NOINIT;
    else
        return STA_NODISK;
    #else
    return RES_OK;
    #endif
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    #ifdef FUSEE_STAGE2_SRC
    /* We aren't using FF_MULTI_PARTITION, so pdrv = volume id. */
    device_partition_t *devpart = g_volume_to_devparts[pdrv];
    if (!devpart)
        return STA_NODISK;
    else if (devpart->initializer)
        return devpart->initializer(devpart) ? STA_NOINIT : RES_OK;
    else
        return RES_OK;
    #else
    return RES_OK;
    #endif
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE pdrv,      /* Physical drive nmuber to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT count      /* Number of sectors to read */
)
{
    #ifdef FUSEE_STAGE2_SRC
    /* We aren't using FF_MULTI_PARTITION, so pdrv = volume id. */
    device_partition_t *devpart = g_volume_to_devparts[pdrv];
    if (!devpart)
        return RES_PARERR;
    else if (devpart->reader)
        return device_partition_read_data(devpart, buff, sector, count) ? RES_ERROR : RES_OK;
    else
        return RES_ERROR;
    #else
    switch (pdrv) {
        case 0:
            return sdmmc_device_read(&g_sd_device, sector, count, (void *)buff) ? RES_OK : RES_ERROR;
        default:
            return RES_PARERR;
    }
    #endif
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    #ifdef FUSEE_STAGE2_SRC
    /* We aren't using FF_MULTI_PARTITION, so pdrv = volume id. */
    device_partition_t *devpart = g_volume_to_devparts[pdrv];
    if (!devpart)
        return RES_PARERR;
    else if (devpart->writer)
        return device_partition_write_data(devpart, buff, sector, count) ? RES_ERROR : RES_OK;
    else
        return RES_ERROR;
    #else
    switch (pdrv) {
        case 0:
            return sdmmc_device_write(&g_sd_device, sector, count, (void *)buff) ? RES_OK : RES_ERROR;
        default:
            return RES_PARERR;
    }
    #endif
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    #ifdef FUSEE_STAGE2_SRC
    /* We aren't using FF_MULTI_PARTITION, so pdrv = volume id. */
    device_partition_t *devpart = g_volume_to_devparts[pdrv];
    switch (cmd) {
        case GET_SECTOR_SIZE:
            *(WORD *)buff = devpart ? (WORD)devpart->sector_size : 512;
            return RES_OK;
        default:
            return RES_OK;
    }
    #else
    return RES_OK;
    #endif
}

