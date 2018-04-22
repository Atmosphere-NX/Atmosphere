#ifndef __FUSEE_SDMMC_H__
#define __FUSEE_SDMMC_H__

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"


/* Opaque pointer to the Tegra SDMMC registers */
struct tegra_sdmmc;

/**
 * Primary data structure describing a Fusée MMC driver.
 */
struct mmc {
    char *name;

    volatile struct tegra_sdmmc *regs;

    unsigned int timeout;
};


/**
 * SDMMC controllers
 */
enum sdmmc_controller {
    SWITCH_MICROSD = 0,
    SWITCH_EMMC = 3
};



/**
 * Initiailzes an SDMMC controller for use with an eMMC or SD card device.
 *
 * @param mmc An (uninitialized) structure for the MMC device.
 * @param controller The controller number to be initialized. Either SWITCH_MICROSD or SWITCH_EMMC.
 */
int sdmmc_init(struct mmc *mmc, enum sdmmc_controller controller);


/**
 * Reads a sector or sectors from a given SD card.
 *
 * @param mmc The MMC device to work with.
 * @param buffer The output buffer to target.
 * @param sector The sector number to read.
 * @param count The number of sectors to read.
 */
int sdmmc_read(struct mmc *mmc, void *buffer, uint32_t sector, unsigned int count);


#endif
