/**
 * Fusée SD/MMC driver for the Switch
 *  ~ktemkin
 */

#ifndef __FUSEE_SDMMC_H__
#define __FUSEE_SDMMC_H__

#include <stdbool.h>
#include <stdint.h>
#include "utils.h"
#include "gpio.h"

/* Opaque pointer to the Tegra SDMMC registers */
struct tegra_sdmmc;


/**
 * Bus widths supported by the SD/MMC cards.
 */
enum sdmmc_bus_width {
    MMC_BUS_WIDTH_1BIT = 0,
    MMC_BUS_WIDTH_4BIT = 1,
    MMC_BUS_WIDTH_8BIT = 2,

    SD_BUS_WIDTH_1BIT = 0,
    SD_BUS_WIDTH_4BIT = 2
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
    SD_VERSION_1_0 = 0,
    SD_VERSION_1_1 = 1,
    SD_VERSION_2_0 = 2,

};


/**
 * SDMMC controllers
 */
enum sdmmc_controller {
    SWITCH_MICROSD = 0,
    SWITCH_EMMC = 3
};

/**
 * Write permission modes for SD cards.
 */
enum sdmmc_write_permission {
    SDMMC_WRITE_DISABLED,
    SDMMC_WRITE_ENABLED,

    /* use this with the utmost caution so you don't wind up with a brick  */
    SDMMC_WRITE_ENABLED_INCLUDING_EMMC,
};


/**
 * Methods by which we can touch registers accessed via.
 * CMD_SWITCH_MODE.
 */
enum sdmmc_switch_access_mode {

    /* Normal commands */
    MMC_SWITCH_MODE_CMD_SET    = 0,
    MMC_SWITCH_MODE_SET_BITS   = 1,
    MMC_SWITCH_MODE_CLEAR_BITS = 2,
    MMC_SWITCH_MODE_WRITE_BYTE = 3,

    /* EXTCSD access */
    MMC_SWITCH_EXTCSD_NORMAL   = 1,
};


/**
 * Offsets into the SWITCH_MODE argument.
 */
enum sdmmc_switch_argument_offsets {
    MMC_SWITCH_VALUE_SHIFT = 8,
    MMC_SWITCH_FIELD_SHIFT = 16,
    MMC_SWITCH_ACCESS_MODE_SHIFT = 24,
};


/**
 * Fields that can be modified by CMD_SWITCH_MODE.
 */
enum sdmmc_switch_field {
    /* Fields */
    MMC_GROUP_ERASE_DEF = 175,
    MMC_PARTITION_CONFIG = 179,
    MMC_BUS_WIDTH = 183,
};



/**
 * Primary data structure describing a Fusée MMC driver.
 */
struct mmc {
    enum sdmmc_controller controller;

    /* Controller properties */
    const char *name;
    bool use_dma;
    unsigned int timeout;
    enum tegra_named_gpio card_detect_gpio;
    enum sdmmc_card_type card_type;
    enum sdmmc_write_permission write_enable;

    /* Per-controller operations. */
    int (*set_up_clock_and_io)(struct mmc *mmc);
    int (*enable_supplies)(struct mmc *mmc);
    int (*switch_to_low_voltage)(struct mmc *mmc);
    bool (*card_present)(struct mmc *mmc);

    /* Per-card-type operations */
    int (*card_init)(struct mmc *mmc);
    int (*establish_relative_address)(struct mmc *mmc);
    int (*switch_mode)(struct mmc *mmc, enum sdmmc_switch_access_mode mode,
            enum sdmmc_switch_field field, uint16_t value, uint32_t timeout);
    int (*switch_bus_width)(struct mmc *mmc, enum sdmmc_bus_width width);

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
    uint8_t write_block_order;
    bool uses_block_addressing;

    /* Current operation status flags */
    uint32_t active_data_buffer;

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
    SDMMC_PARTITION_USER  = 0,
    SDMMC_PARTITION_BOOT0 = 1,
    SDMMC_PARTITION_BOOT1 = 2,
};


/**
 * Sets the current SDMMC debugging loglevel.
 *
 * @param loglevel Current log level. A higher value prints more logs.
 */
void sdmmc_set_loglevel(int loglevel);


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
 * Releases the SDMMC write lockout, enabling access to the card.
 * Note that by default, setting this to WRITE_ENABLED will not allow access to eMMC.
 * Check the source for a third constant that can be used to enable eMMC writes.
 *
 * @param perms The permissions to apply-- typically WRITE_DISABLED or WRITE_ENABLED.
 */
void sdmmc_set_write_enable(struct mmc *mmc, enum sdmmc_write_permission perms);


/**
 * Writes a sector or sectors to a given SD/MMC card.
 *
 * @param mmc The MMC device to work with.
 * @param buffer The input buffer to write.
 * @param block The sector number to write from.
 * @param count The number of sectors to write.
 *
 * @return 0 on success, or an error code on failure.
 */
int sdmmc_write(struct mmc *mmc, const void *buffer, uint32_t block, unsigned int count);


/**
 * Checks to see whether an SD card is present.
 *
 * @mmc mmc The controller with which to check for card presence.
 * @return true iff a card is present
 */
bool sdmmc_card_present(struct mmc *mmc);

#endif
