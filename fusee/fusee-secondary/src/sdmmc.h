/**
 * Fusée SD/MMC driver for the Switch
 *  ~ktemkin
 */

#ifndef __FUSEE_SDMMC_H__
#define __FUSEE_SDMMC_H__

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"

/* Opaque pointer to the Tegra SDMMC registers */
struct tegra_sdmmc;


/**
 * Bus widths supported by the SD/MMC cards.
 */
enum sdmmc_bus_width {
    MMC_BUS_WIDTH_1BIT = 0,
    MMC_BUS_WIDTH_4BIT = 1,
    MMC_BUS_WIDTH_8BIT = 2,
};


/**
 * Describes the voltages that the host will use to drive the SD card.
 * CAUTION: getting these wrong can damage (especially embedded) cards!
 */
enum sdmmc_bus_voltage {
   MMC_VOLTAGE_3V3 = 0b111,
   MMC_VOLTAGE_3V0 = 0b110,
   MMC_VOLTAGE_1V8 = 0b101,
};


/**
 * Represents the different types of devices an MMC object
 * can represent.
 */
enum sdmmc_card_type {
    MMC_CARD_EMMC,
    MMC_CARD_MMC,
    MMC_CARD_SD,
    MMC_CARD_CART,
};



/**
 * Specification versions for SD/MMC cards.
 */
enum sdmmc_spec_version  {

    /* MMC card versions */
    MMC_VERSION_4 = 0,

    /* SD card versions */
    SD_VERSION_1 = 1,
    SD_VERSION_2 = 2,

};


/**
 * SDMMC controllers
 */
enum sdmmc_controller {
    SWITCH_MICROSD = 0,
    SWITCH_EMMC = 3
};


/**
 * Primary data structure describing a Fusée MMC driver.
 */
struct mmc {
    enum sdmmc_controller controller;

    /* Controller properties */
    const char *name;
    unsigned int timeout;
    enum sdmmc_card_type card_type;
    bool use_dma;

    /* Card properties */
    uint8_t cid[15];
    uint32_t relative_address;
    uint8_t partitioned;
    enum sdmmc_spec_version spec_version;
    enum sdmmc_bus_width max_bus_width;
    enum sdmmc_bus_voltage operating_voltage;

    uint8_t partition_support;
    uint8_t partition_config;
    uint8_t partition_attribute;
    uint32_t partition_switch_time;

    uint8_t read_block_order;
    bool uses_block_addressing;

    /* Pointers to hardware structures */
    volatile struct tegra_sdmmc *regs;
};




/**
 * Primary data structure describing a Fusée MMC driver.
 */
struct mmc;


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


/**
 * Checks to see whether an SD card is present.
 *
 * @mmc mmc The controller with which to check for card presence.
 * @return true iff a card is present
 */
bool sdmmc_card_present(struct mmc *mmc);

#endif
