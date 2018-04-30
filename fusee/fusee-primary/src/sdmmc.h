#ifndef __FUSEE_SDMMC_H__
#define __FUSEE_SDMMC_H__

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"


/* Opaque pointer to the Tegra SDMMC registers */
struct tegra_sdmmc;

/**
 * Represents the different types of devices an MMC object
 * can represent.
 */
enum mmc_card_type {
    MMC_CARD_EMMC,
    MMC_CARD_MMC,
    MMC_CARD_SD,
    MMC_CARD_CART,
};

/**
 * Primary data structure describing a Fusée MMC driver.
 */
struct mmc {
    /* Controller properties */
    char *name;
    unsigned int timeout;
    enum mmc_card_type card_type;
    bool use_dma;

    /* Card properties */
    uint8_t cid[15];
    uint32_t relative_address;

    uint8_t partition_support;
    uint8_t partition_config;
    uint8_t partition_setting;
    uint8_t partition_attribute;
    uint32_t partition_switch_time;

    uint8_t read_block_order;
    bool uses_block_addressing;

    /* Pointers to hardware structures */
    volatile struct tegra_sdmmc *regs;
};


/**
 * SDMMC controllers
 */
enum sdmmc_controller {
    SWITCH_MICROSD = 0,
    SWITCH_EMMC = 3
};


/**
 * Parititions supported by the Switch eMMC.
 */
enum sdmmc_partition {
    MMC_PARTITION_USER  = 0,
    MMC_PARTITION_BOOT1 = 1,
    MMC_PARITTION_BOOT2 = 2,
};


/**
 * Initiailzes an SDMMC controller for use with an eMMC or SD card device.
 *
 * @param mmc An (uninitialized) structure for the MMC device.
 * @param controller The controller number to be initialized. Either SWITCH_MICROSD or SWITCH_EMMC.
 */
int sdmmc_init(struct mmc *mmc, enum sdmmc_controller controller);


/**
 * Selects the active MMC partition. Can be used to select
 * boot partitions for access. Affects all operations going forward.
 *
 * @param mmc The MMC controller whose card is to be used.
 * @param partition The partition number to be selected.
 *
 * @return 0 on success, or an error code on failure.
 */
int sdmmc_select_partition(struct mmc *mmc, enum sdmmc_partition partition);


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
