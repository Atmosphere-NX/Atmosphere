/**
 * Fusée SD/MMC driver for the Switch
 *  ~ktemkin
 */

#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#include "lib/driver_utils.h"
#include "sdmmc.h"
#include "car.h"
#include "pinmux.h"
#include "timers.h"
#include "apb_misc.h"
#include "gpio.h"
#include "supplies.h"
#include "pmc.h"
#include "pad_control.h"
#include "apb_misc.h"
#define TEGRA_SDMMC_BASE (0x700B0000)
#define TEGRA_SDMMC_SIZE (0x200)


/**
 * Map of tegra SDMMC registers
 */
struct tegra_sdmmc {

    /* SDHCI standard registers */
    uint32_t dma_address;
    uint16_t block_size;
    uint16_t block_count;
    uint32_t argument;
    uint16_t transfer_mode;
    uint16_t command;
    uint32_t response[0x4];
    uint32_t buffer;
    uint32_t present_state;
    uint8_t host_control;
    uint8_t power_control;
    uint8_t block_gap_control;
    uint8_t wake_up_control;
    uint16_t clock_control;
    uint8_t timeout_control;
    uint8_t software_reset;
    uint32_t int_status;
    uint32_t int_enable;
    uint32_t signal_enable;
    uint16_t acmd12_err;
    uint16_t host_control2;
    uint32_t capabilities;
    uint32_t capabilities_1;
    uint32_t max_current;
    uint32_t _0x4c;
    uint16_t set_acmd12_error;
    uint16_t set_int_error;
    uint16_t adma_error;
    uint8_t _0x56[0x2];
    uint32_t adma_address;
    uint32_t upper_adma_address;
    uint16_t preset_for_init;
    uint16_t preset_for_default;
    uint16_t preset_for_high;
    uint16_t preset_for_sdr12;
    uint16_t preset_for_sdr25;
    uint16_t preset_for_sdr50;
    uint16_t preset_for_sdr104;
    uint16_t preset_for_ddr50;
    uint8_t _0x70[0x4];
    uint32_t _0x74[0x22];
    uint16_t slot_int_status;
    uint16_t host_version;

    /* vendor specific registers */
    uint32_t vendor_clock_cntrl;
    uint32_t vendor_sys_sw_cntrl;
    uint32_t vendor_err_intr_status;
    uint32_t vendor_cap_overrides;
    uint32_t vendor_boot_cntrl;
    uint32_t vendor_boot_ack_timeout;
    uint32_t vendor_boot_dat_timeout;
    uint32_t vendor_debounce_count;
    uint32_t vendor_misc_cntrl;
    uint32_t max_current_override;
    uint32_t max_current_override_hi;
    uint32_t _0x12c[0x20];
    uint32_t vendor_io_trim_cntrl;

    /* start of sdmmc2/sdmmc4 only */
    uint32_t vendor_dllcal_cfg;
    uint32_t vendor_dll_ctrl0;
    uint32_t vendor_dll_ctrl1;
    uint32_t vendor_dllcal_cfg_sta;
    /* end of sdmmc2/sdmmc4 only */

    uint32_t vendor_tuning_cntrl0;
    uint32_t vendor_tuning_cntrl1;
    uint32_t vendor_tuning_status0;
    uint32_t vendor_tuning_status1;
    uint32_t vendor_clk_gate_hysteresis_count;
    uint32_t vendor_preset_val0;
    uint32_t vendor_preset_val1;
    uint32_t vendor_preset_val2;
    uint32_t sdmemcomppadctrl;
    uint32_t auto_cal_config;
    uint32_t auto_cal_interval;
    uint32_t auto_cal_status;
    uint32_t io_spare;
    uint32_t sdmmca_mccif_fifoctrl;
    uint32_t timeout_wcoal_sdmmca;
    uint32_t _0x1fc;
};

/**
 * SDMMC response lengths
 */
enum sdmmc_response_type {
    MMC_RESPONSE_NONE           = 0,
    MMC_RESPONSE_LEN136         = 1,
    MMC_RESPONSE_LEN48          = 2,
    MMC_RESPONSE_LEN48_CHK_BUSY = 3,

};

/**
 * Lengths of SD command responses
 */
enum sdmmc_constants {
    /* Bytes in a LEN136 response */
    MMC_RESPONSE_SIZE_LEN136    = 15,
};

/**
 * SDMMC clock divider constants
 */
enum sdmmc_clock_dividers {

    /* Clock dividers: SD */
    MMC_CLOCK_DIVIDER_SDR12  = 31, // 16.5, from the TRM table
    MMC_CLOCK_DIVIDER_SDR25  = 15, // 8.5, from the table
    MMC_CLOCK_DIVIDER_SDR50  = 7,  // 4.5, from the table
    MMC_CLOCK_DIVIDER_SDR104 = 4,  // 2, from the datasheet

    /* Clock dividers: MMC */
    MMC_CLOCK_DIVIDER_HS26   = 30, // 16, from the TRM table
    MMC_CLOCK_DIVIDER_HS52   = 14, // 8, from the table

    MMC_CLOCK_DIVIDER_HS200  = 2,  // 1 -- NOTE THIS IS WITH RESPECT TO PLLC4_OUT2_LJ
    MMC_CLOCK_DIVIDER_HS400  = 2,  // 1 -- NOTE THIS IS WITH RESPECT TO PLLC4_OUT2_LJ
};


/**
 * SDMMC clock divider constants
 */
enum sdmmc_clock_sources {

    /* Clock dividers: SD */
    MMC_CLOCK_SOURCE_SDR12  = 0, // PLLP
    MMC_CLOCK_SOURCE_SDR25  = 0,
    MMC_CLOCK_SOURCE_SDR50  = 0,
    MMC_CLOCK_SOURCE_SDR104 = 0,

    /* Clock dividers: MMC */
    MMC_CLOCK_SOURCE_HS26   = 0, // PLLP
    MMC_CLOCK_SOURCE_HS52   = 0,
    MMC_CLOCK_SOURCE_HS200  = 1, // PLLC4_OUT2_LJ
    MMC_CLOCK_SOURCE_HS400  = 1,

};

/**
 * SDMMC response sanity checks
 * see the standard for when these should be used
 */
enum sdmmc_response_checks {
    MMC_CHECKS_NONE   = 0,
    MMC_CHECKS_CRC    = (1 << 3),
    MMC_CHECKS_INDEX  = (1 << 4),
    MMC_CHECKS_ALL    = (1 << 4) | (1 << 3),
};

/**
 * General masks for SDMMC registers.
 */
enum sdmmc_register_bits {

    /* Present state register */
    MMC_COMMAND_INHIBIT      = (1 << 0),
    MMC_DATA_INHIBIT         = (1 << 1),
    MMC_BUFFER_WRITE_ENABLE  = (1 << 10),
    MMC_BUFFER_READ_ENABLE   = (1 << 11),
    MMC_DAT0_LINE_STATE      = (1 << 20),
    MMC_READ_ACTIVE          = (1 << 9),
    MMC_WRITE_ACTIVE         = (1 << 8),

    /* Block size register */
    MMC_DMA_BOUNDARY_MAXIMUM = (0x7 << 12),
    MMC_DMA_BOUNDARY_512K    = (0x7 << 12),
    MMC_DMA_BOUNDARY_64K     = (0x4 << 12),
    MMC_DMA_BOUNDARY_32K     = (0x3 << 12),
    MMC_DMA_BOUNDARY_16K     = (0x2 << 12),
    MMC_DMA_BOUNDARY_8K      = (0x1 << 12),
    MMC_DMA_BOUNDARY_4K      = (0x0 << 12),
    MMC_TRANSFER_BLOCK_512B  = (0x200 << 0),

    /* Command register */
    MMC_COMMAND_NUMBER_SHIFT = 8,
    MMC_COMMAND_RESPONSE_TYPE_SHIFT = 0,
    MMC_COMMAND_HAS_DATA     = 1 << 5,
    MMC_COMMAND_TYPE_ABORT   = 3 << 6,
    MMC_COMMAND_CHECK_NUMBER = 1 << 4,

    /* Transfer mode arguments */
    MMC_TRANSFER_DMA_ENABLE = (1 << 0),
    MMC_TRANSFER_LIMIT_BLOCK_COUNT = (1 << 1),
    MMC_TRANSFER_MULTIPLE_BLOCKS = (1 << 5),
    MMC_TRANSFER_AUTO_CMD_MASK = (0x3 << 2),
    MMC_TRANSFER_AUTO_CMD12 = (0x1 << 2),
    MMC_TRANSFER_AUTO_CMD23 = (0x2 << 2),
    MMC_TRANSFER_AUTO_CMD = (0x3 << 2),
    MMC_TRANSFER_CARD_TO_HOST = (1 << 4),

    /* Interrupt status */
    MMC_STATUS_COMMAND_COMPLETE = (1 << 0),
    MMC_STATUS_TRANSFER_COMPLETE = (1 << 1),
    MMC_STATUS_DMA_INTERRUPT = (1 << 3),
    MMC_STATUS_BUFFER_READ_READY = (1 << 5),
    MMC_STATUS_COMMAND_TIMEOUT = (1 << 16),
    MMC_STATUS_COMMAND_CRC_ERROR = (1 << 17),
    MMC_STATUS_COMMAND_END_BIT_ERROR = (1 << 18),
    MMC_STATUS_COMMAND_INDEX_ERROR = (1 << 19),

    MMC_STATUS_ERROR_MASK =  (0xF << 16),

    /* Clock control */
    MMC_CLOCK_CONTROL_CARD_CLOCK_ENABLE      = (1 << 2),
    MMC_CLOCK_CONTROL_FREQUENCY_MASK         = (0x3FF << 6),
    MMC_CLOCK_CONTROL_FREQUENCY_SHIFT        = 8,
    MMC_CLOCK_CONTROL_FREQUENCY_INIT         = 0x18, // generates 400kHz from the TRM dividers
    MMC_CLOCK_CONTROL_FREQUENCY_PASSTHROUGH  = 0x00, // passes through the CAR clock unmodified

    /* Host control */
    MMC_DMA_SELECT_MASK = (0x3 << 3),
    MMC_DMA_SELECT_SDMA = (0x0 << 3),
    MMC_HOST_BUS_WIDTH_MASK = (1 << 1) | (1 << 5),
    MMC_HOST_BUS_WIDTH_4BIT = (1 << 1),
    MMC_HOST_BUS_WIDTH_8BIT = (1 << 5),
    MMC_HOST_ENABLE_HIGH_SPEED = (1 << 2),

    /* Host control 2 */
    MMC_HOST2_DRIVE_STRENGTH_MASK = (0x3 << 4),
    MMC_HOST2_DRIVE_STRENGTH_B = (0x0 << 4),
    MMC_HOST2_DRIVE_STRENGTH_A = (0x1 << 4),
    MMC_HOST2_DRIVE_STRENGTH_C = (0x2 << 4),
    MMC_HOST2_DRIVE_STRENGTH_D = (0x3 << 4),
    MMC_HOST2_USE_1V8_SIGNALING = (1 << 3),
    MMC_HOST2_EXECUTE_TUNING = (1 << 6),
    MMC_HOST2_SAMPLING_CLOCK_ENABLED = (1 << 7),
    MMC_HOST2_UHS_MODE_MASK = (0x7 << 3),

    /* Software reset */
    MMC_SOFT_RESET_FULL = (1 << 0),
    MMC_SOFT_RESET_CMD = (1 << 1),
    MMC_SOFT_RESET_DAT = (1 << 2),

    /* Vendor clock control */
    MMC_CLOCK_TAP_MASK    = (0xFF << 16),
    MMC_CLOCK_TAP_SDMMC1  = (0x04 << 16),
    MMC_CLOCK_TAP_SDMMC4  = (0x00 << 16),

    MMC_CLOCK_TRIM_MASK   = (0xFF << 24),
    MMC_CLOCK_TRIM_SDMMC1 = (0x02 << 24),
    MMC_CLOCK_TRIM_SDMMC4 = (0x08 << 24),

    MMC_CLOCK_PADPIPE_CLKEN_OVERRIDE = (1 << 3),

    /* Autocal configuration */
    MMC_AUTOCAL_PDPU_CONFIG_MASK = 0x7f7f,
    MMC_AUTOCAL_PDPU_SDMMC1_1V8 = 0x7b7b,
    MMC_AUTOCAL_PDPU_SDMMC1_3V3 = 0x7d00,
    MMC_AUTOCAL_PDPU_SDMMC4_1V8 = 0x0505,
    MMC_AUTOCAL_START = (1 << 31),
    MMC_AUTOCAL_ENABLE = (1 << 29),

    /* Autocal status */
    MMC_AUTOCAL_ACTIVE = (1 << 31),

    /* Power control */
    MMC_POWER_CONTROL_VOLTAGE_MASK = (0x3 << 1),
    MMC_POWER_CONTROL_VOLTAGE_SHIFT = 1,
    MMC_POWER_CONTROL_POWER_ENABLE = (1 << 0),

    /* Capabilities register high */
    MMC_SDR50_REQUIRES_TUNING = (1 << 13),

    /* Vendor tuning control 0*/
    MMC_VENDOR_TUNING_TRIES_MASK = (0x7 << 13),
    MMC_VENDOR_TUNING_TRIES_SHIFT = 13,

    MMC_VENDOR_TUNING_MULTIPLIER_MASK = (0x7F << 6),
    MMC_VENDOR_TUNING_MULTIPLIER_UNITY = (1 << 6),

    MMC_VENDOR_TUNING_DIVIDER_MASK = (0x7 << 3),

    MMC_VENDOR_TUNING_SET_BY_HW = (1 << 17),

    /* Vendor tuning control 1*/
    MMC_VENDOR_TUNING_STEP_SIZE_SDR50_DEFAULT  = (0 << 0),
    MMC_VENDOR_TUNING_STEP_SIZE_SDR104_DEFAULT = (0 << 4),

    /* Vendor capability overrides */
    MMC_VENDOR_CAPABILITY_DQS_TRIM_MASK  = (0x3f << 8),
    MMC_VENDOR_CAPABILITY_DQS_TRIM_HS400 = (0x11 << 8),
};


/**
 * Represents the possible tuning modes for the X1 SDMMC controller.
 */
enum sdmmc_tuning_attempts {
    MMC_VENDOR_TUNING_TRIES_40   = 0,
    MMC_VENDOR_TUNING_TRIES_64   = 1,
    MMC_VENDOR_TUNING_TRIES_128  = 2,
    MMC_VENDOR_TUNING_TRIES_192  = 3,
    MMC_VENDOR_TUNING_TRIES_256  = 4,

    /* Helpful aliases; values are from the TRM */
    MMC_VENDOR_TUNING_TRIES_SDR50   = 4,
    MMC_VENDOR_TUNING_TRIES_SDR104  = 2,
    MMC_VENDOR_TUNING_TRIES_HS200   = 2,
    MMC_VENDOR_TUNING_TRIES_HS400   = 2,
};


/* Constant map that converts from a MMC_VENDOR_TUNING_TRIES_* value to the number of tries. */
static const int sdmmc_tuning_iterations[] = {40, 64, 128, 192, 256};

/**
 * SDMMC commands
 */
enum sdmmc_command {
    CMD_GO_IDLE_OR_INIT = 0,
    CMD_SEND_OPERATING_CONDITIONS = 1,
    CMD_ALL_SEND_CID = 2,
    CMD_SET_RELATIVE_ADDR = 3,
    CMD_GET_RELATIVE_ADDR = 3,
    CMD_SET_DSR = 4,
    CMD_TOGGLE_SLEEP_AWAKE = 5,
    CMD_SWITCH_MODE = 6,
    CMD_APP_SWITCH_WIDTH = 6,
    CMD_TOGGLE_CARD_SELECT = 7,
    CMD_SEND_EXT_CSD = 8,
    CMD_SEND_IF_COND = 8,
    CMD_SEND_CSD = 9,
    CMD_SEND_CID = 10,
    CMD_SWITCH_TO_LOW_VOLTAGE = 11,
    CMD_STOP_TRANSMISSION = 12,
    CMD_READ_STATUS = 13,
    CMD_BUS_TEST = 14,
    CMD_GO_INACTIVE = 15,
    CMD_SET_BLKLEN = 16,
    CMD_READ_SINGLE_BLOCK = 17,
    CMD_READ_MULTIPLE_BLOCK = 18,
    CMD_SD_SEND_TUNING_BLOCK = 19,
    CMD_MMC_SEND_TUNING_BLOCK = 21,
    CMD_WRITE_SINGLE_BLOCK = 24,
    CMD_WRITE_MULTIPLE_BLOCK = 25,

    CMD_APP_SEND_OP_COND = 41,
    CMD_APP_SET_CARD_DETECT = 42,
    CMD_APP_SEND_SCR = 51,
    CMD_APP_COMMAND = 55,
};


/**
 * Fields that can be modified by CMD_SWITCH_MODE.
 */
enum sdmmc_switch_field {
    /* Fields */
    MMC_PARTITION_CONFIG = 179,
    MMC_BUS_WIDTH = 183,
    MMC_HS_TIMING = 185,
};



/**
 * String descriptions of each command.
 */
static const char *sdmmc_command_string[] = {
    "CMD_GO_IDLE_OR_INIT",
    "CMD_SEND_OPERATING_CONDITIONS",
    "CMD_ALL_SEND_CID",
    "CMD_SET_RELATIVE_ADDR",
    "CMD_SET_DSR",
    "CMD_TOGGLE_SLEEP_AWAKE",
    "CMD_SWITCH_MODE",
    "CMD_TOGGLE_CARD_SELECT",
    "CMD_SEND_EXT_CSD/CMD_SEND_IF_COND",
    "CMD_SEND_CSD",
    "CMD_SEND_CID ",
    "CMD_SWITCH_TO_LOW_VOLTAGE",
    "CMD_STOP_TRANSMISSION",
    "CMD_READ_STATUS",
    "CMD_BUS_TEST",
    "CMD_GO_INACTIVE",
    "CMD_SET_BLKLEN",
    "CMD_READ_SINGLE_BLOCK",
    "CMD_READ_MULTIPLE_BLOCK",
    "CMD_SD_SEND_TUNING_BLOCK",
    "<invalid>",
    "CMD_MMC_SEND_TUNING_BLOCK",
    "<invalid>",
    "<invalid>",
    "CMD_WRITE_SINGLE_BLOCK",
    "CMD_WRITE_WRITE_BLOCK",
};


/**
 * SDMMC command argument numbers
 */
enum sdmmc_command_magic {
    MMC_ENABLE_BOOT_INIT_MAGIC = 0xf0f0f0f0,
    MMC_ENABLE_BOOT_ENABLE_MAGIC = 0xfffffffa,

    MMC_EMMC_OPERATING_COND_CAPACITY_MAGIC = 0x00ff8080,

    MMC_EMMC_OPERATING_COND_CAPACITY_MASK  = 0x0fffffff,
    MMC_EMMC_OPERATING_COND_BUSY           = (0x04 << 28),
    MMC_EMMC_OPERATING_COND_READY          = (0x0c << 28),
    MMC_EMMC_OPERATING_READINESS_MASK      = (0x0f << 28),

    MMC_SD_OPERATING_COND_READY            = (1 << 31),
    MMC_SD_OPERATING_COND_HIGH_CAPACITY    = (1 << 30),
    MMC_SD_OPERATING_COND_ACCEPTS_1V8      = (1 << 24),
    MMC_SD_OPERATING_COND_ACCEPTS_3V3      = (1 << 20),

    /* READ_STATUS responses */
    MMC_STATUS_MASK           = (0xf << 9),
    MMC_STATUS_PROGRAMMING    = (0x7 << 9),
    MMC_STATUS_READY_FOR_DATA = (0x1 << 8),
    MMC_STATUS_CHECK_ERROR    = (~0x0206BF7F),

    /* IF_COND components */
    MMC_IF_VOLTAGE_3V3 = (1 << 8),
    MMC_IF_CHECK_PATTERN = 0xAA,

    /* Switch mode constants */
    SDMMC_SWITCH_MODE_MODE_SHIFT = 31,
    SDMMC_SWITCH_MODE_ALL_FUNCTIONS_UNUSED = 0xFFFFFF,
    SDMMC_SWITCH_MODE_FUNCTION_MASK = 0xF,
    SDMMC_SWITCH_MODE_GROUP_SIZE_BITS = 4,

    SDMMC_SWITCH_MODE_MODE_QUERY = 0,
    SDMMC_SWITCH_MODE_MODE_APPLY = 1,
    SDMMC_SWITCH_MODE_ACCESS_MODE = 0,
    SDMMC_SWITCH_MODE_NO_GROUP = -1,

    /* Misc constants */
    MMC_DEFAULT_BLOCK_ORDER = 9,
    MMC_VOLTAGE_SWITCH_TIME = 5000, // 5ms
    MMC_POST_CLOCK_DELAY = 1000, // 1ms
    MMC_SPEED_MMC_OFFSET = 10,

    MMC_AUTOCAL_TIMEOUT = 10 * 1000, // 10ms
    MMC_TUNING_TIMEOUT = 150 * 1000, // 150ms
    MMC_TUNING_BLOCK_ORDER_4BIT = 6,
    MMC_TUNING_BLOCK_ORDER_8BIT = 7,
};


/**
 * Version magic numbers for different CSD versions.
 */
enum sdmmc_csd_versions {
    MMC_CSD_VERSION1 = 0,
    MMC_CSD_VERSION2 = 1,
};


/**
 * Positions of different fields in various CSDs.
 * May eventually be replaced with a bitfield struct, if we use enough of the CSDs.
 */
enum sdmmc_csd_extents {

    /* csd structure version */
    MMC_CSD_STRUCTURE_START = 126,
    MMC_CSD_STRUCTURE_WIDTH = 2,

    /* read block length */
    MMC_CSD_V1_READ_BL_LENGTH_START = 80,
    MMC_CSD_V1_READ_BL_LENGTH_WIDTH = 4,

};


/**
 * Positions of the different fields in the Extended CSD.
 */
enum sdmmc_ext_csd_extents {
    MMC_EXT_CSD_SIZE = 512,

    /* Hardware partition registers */
    MMC_EXT_CSD_PARTITION_SETTING_COMPLETE = 155,
    MMC_EXT_CSD_PARTITION_SETTING_COMPLETED = (1 << 0),

    MMC_EXT_CSD_PARTITION_ATTRIBUTE = 156,
    MMC_EXT_CSD_PARTITION_ENHANCED_ATTRIBUTE = 0x1f,

    MMC_EXT_CSD_PARTITION_SUPPORT = 160,
    MMC_SUPPORTS_HARDWARE_PARTS   = (1 << 0),

    MMC_EXT_CSD_ERASE_GROUP_DEF = 175,
    MMC_EXT_CSD_ERASE_GROUP_DEF_BIT = (1 << 0),

    MMC_EXT_CSD_PARTITION_CONFIG  = 179,
    MMC_EXT_CSD_PARTITION_SELECT_MASK = 0x7,

    MMC_EXT_CSD_PARTITION_SWITCH_TIME = 199,
    MMC_EXT_CSD_PARTITION_SWITCH_SCALE_US = 10000,

    /* Card type register; we skip entries for
     * non-1V8 modes, as we're fixed to 1V8 */
    MMC_EXT_CSD_CARD_TYPE = 196,
    MMC_EXT_CSD_CARD_TYPE_HS26      = (1 << 0),
    MMC_EXT_CSD_CARD_TYPE_HS52      = (1 << 1),
    MMC_EXT_CSD_CARD_TYPE_HS200_1V8 = (1 << 4),
    MMC_EXT_CSD_CARD_TYPE_HS400_1V8 = (1 << 6),

    /* Current HS mode register */
    MMC_EXT_CSD_HS_TIMING = 185,
};


/**
 * Bitfield struct representing an SD SCR.
 */
struct PACKED sdmmc_scr {
    uint32_t reserved1;
    uint16_t reserved0;
    uint8_t supports_width_1bit : 1;
    uint8_t supports_width_reserved0 : 1;
    uint8_t supports_width_4bit : 1;
    uint8_t supports_width_reserved1 : 1;
    uint8_t security_support : 3;
    uint8_t data_after_erase : 1;
    uint8_t spec_version : 4;
    uint8_t scr_version : 4;
};


/**
 * Bitfield struct represneting an SD card's function status.
 */
struct PACKED sdmmc_function_status {

    /* NOTE: all of the below are reversed from CPU endianness! */
    uint16_t current_consumption;
    uint16_t group6_support;
    uint16_t group5_support;
    uint16_t group4_support;
    uint16_t group3_support;
    uint16_t group2_support;

    /* support for various speed modes */
    uint16_t group1_support_reserved1  : 8;
    uint16_t sdr12_support   :  1;
    uint16_t sdr25_support   :  1;
    uint16_t sdr50_support   :  1;
    uint16_t sdr104_support  :  1;
    uint16_t ddr50_support   :  1;
    uint16_t group1_support_reserved2  : 3;


    uint8_t group5_selection :  4;
    uint8_t group6_selection :  4;
    uint8_t group3_selection :  4;
    uint8_t group4_selection :  4;
    uint8_t active_access_mode :  4;
    uint8_t group2_selection :  4;

    uint8_t structure_version;
    uint16_t group6_busy_status;
    uint16_t group5_busy_status;
    uint16_t group4_busy_status;
    uint16_t group3_busy_status;
    uint16_t group2_busy_status;
    uint16_t group1_busy_status;
    uint8_t reserved[34];
};

/* Callback function typedefs */
typedef int (*fault_handler_t)(struct mmc *mmc);

/* Forward declarations */
static int sdmmc_send_simple_command(struct mmc *mmc, enum sdmmc_command command,
        enum sdmmc_response_type response_type, uint32_t argument, void *response_buffer);
static int sdmmc_wait_for_event(struct mmc *mmc,
        uint32_t target_irq, uint32_t state_conditions,
        uint32_t fault_conditions, fault_handler_t fault_handler);
static int sdmmc_send_command(struct mmc *mmc, enum sdmmc_command command,
        enum sdmmc_response_type response_type, enum sdmmc_response_checks checks,
        uint32_t argument, void *response_buffer, uint16_t blocks_to_transfer,
        bool is_write, bool auto_terminate, void *data_buffer);
static int sdmmc_use_block_size(struct mmc *mmc, int block_order);
static uint8_t sdmmc_get_block_order(struct mmc *mmc, bool is_write);
static void sdmmc_prepare_command_data(struct mmc *mmc, uint16_t blocks,
        bool is_write, bool auto_terminate, bool use_dma, int argument);
static void sdmmc_prepare_command_registers(struct mmc *mmc, int blocks_to_xfer,
        enum sdmmc_command command, enum sdmmc_response_type response_type,
        enum sdmmc_response_checks checks);
static int sdmmc_wait_for_command_readiness(struct mmc *mmc);
static int sdmmc_wait_for_data_readiness(struct mmc *mmc);

/* SDMMC debug enable */
static int sdmmc_loglevel = 0;

/**
 * Page-aligned bounce buffer to target with SDMMC DMA.
 * If the size of this buffer is changed, the block_size
 */
static uint8_t ALIGN(4096) sdmmc_bounce_buffer[1024 * 8];
static const uint16_t sdmmc_bounce_dma_boundary = MMC_DMA_BOUNDARY_8K;


/**
 * Sets the current SDMMC debugging loglevel.
 *
 * @param loglevel Current log level. A higher value prints more logs.
 * @return The loglevel prior to when this was applied, for easy restoration.
 */
int sdmmc_set_loglevel(int loglevel)
{
    int original_loglevel = sdmmc_loglevel;
    sdmmc_loglevel = loglevel;

    return original_loglevel;
}


/**
 * Internal utility function for generating debug prints at various log levels.
 */
static void mmc_vprint(struct mmc *mmc, char *fmt, int required_loglevel, va_list list)
{
    // Allow debug prints to be silenced by a negative loglevel.
    //TODO: respect the log level, most likely there are still some timing problems
    //which make it not working when the logging is supressed
    //if (sdmmc_loglevel < required_loglevel)
    //    return;

    printk("%s: ", mmc->name);
    vprintk(fmt, list);
    printk("\n");
}


/**
 * Normal SDMMC print for SDMMC information.
 */
static void mmc_print(struct mmc *mmc, char *fmt, ...)
{
    va_list list;

    va_start(list, fmt);
    mmc_vprint(mmc, fmt, 0, list);
    va_end(list);
}


/**
 * Normal SDMMC print for SDMMC information.
 */
static void mmc_debug(struct mmc *mmc, char *fmt, ...)
{
    va_list list;

    va_start(list, fmt);
    mmc_vprint(mmc, fmt, 1, list);
    va_end(list);
}


/**
 * @return a statically allocated string that describes the given command
 */
static const char *sdmmc_get_speed_string(enum sdmmc_bus_speed speed)
{
    switch (speed) {
        case SDMMC_SPEED_INIT:   return "400kHz (init)";

        // SD card speeds
        case SDMMC_SPEED_SDR12:  return "12.5MB/s";
        case SDMMC_SPEED_SDR25:  return "25MB/s";
        case SDMMC_SPEED_SDR50:  return "50MB/s";
        case SDMMC_SPEED_SDR104: return "104MB/s";
        case SDMMC_SPEED_DDR50:  return "104MB/s (DDR)";
        case SDMMC_SPEED_OTHER:  return "<invalid speed>";

        // eMMC card speeds
        case SDMMC_SPEED_HS26:  return "26 MHz";
        case SDMMC_SPEED_HS52:  return "52 MHz";
        case SDMMC_SPEED_HS200: return "200MHz";
        case SDMMC_SPEED_HS400: return "200MHz (DDR)";
    }

    return "";
}


/**
 * @return a statically allocated string that describes the given command
 */
static const char *sdmmc_get_command_string(enum sdmmc_command command)
{
    switch (command) {

        // Commands that aren't in the lower block.
        case CMD_APP_COMMAND:
            return "CMD_APP_COMMAND";
        case CMD_APP_SEND_OP_COND:
            return "CMD_APP_SEND_OP_COND";
        case CMD_APP_SET_CARD_DETECT:
            return "CMD_APP_SET_CARD_DETECT";
        case CMD_APP_SEND_SCR:
            return "CMD_APP_SEND_SCR";

        // For commands with low numbers, read them string from the relevant array.
        default:
            return sdmmc_command_string[command];
    }
}


/**
 * Debug: print out any errors that occurred during a command timeout
 */
void mmc_print_command_errors(struct mmc *mmc, int command_errno)
{
    if (command_errno & MMC_STATUS_COMMAND_TIMEOUT)
        mmc_print(mmc, "ERROR: command timed out!");

    if (command_errno & MMC_STATUS_COMMAND_CRC_ERROR)
        mmc_print(mmc, "ERROR: command response had invalid CRC");

    if (command_errno & MMC_STATUS_COMMAND_END_BIT_ERROR)
        mmc_print(mmc, "ERROR: command response had invalid end bit");

    if (command_errno & MMC_STATUS_COMMAND_INDEX_ERROR)
        mmc_print(mmc, "ERROR: response appears not to be for the last issued command");

}


/**
 * Retreives the SDMMC register range for the given controller.
 */
static struct tegra_sdmmc *sdmmc_get_regs(enum sdmmc_controller controller)
{
    // Start with the base addresss of the SDMMC_BLOCK
    uintptr_t addr = TEGRA_SDMMC_BASE;

    // Offset our address by the controller number.
    addr += (controller * TEGRA_SDMMC_SIZE);

    // Return the controller.
    return (struct tegra_sdmmc *)addr;
}

/**
 * Performs a soft-reset of the SDMMC controller.
 *
 * @param mmc The MMC controller to be reset.
 * @return 0 if the device successfully came out of reset; or an error code otherwise
 */
static int sdmmc_hardware_reset(struct mmc *mmc, uint32_t reset_flags)
{
    uint32_t timebase = get_time();

    // Reset the MMC controller...
    mmc->regs->software_reset |= reset_flags;

    // Wait for the SDMMC controller to come back up...
    while (mmc->regs->software_reset & reset_flags) {
        if (get_time_since(timebase) > mmc->timeout) {
            mmc_print(mmc, "failed to bring up SDMMC controller");
            return ETIMEDOUT;
        }
    }

    return 0;
}

/**
 * Performs low-level initialization for SDMMC4, used for the eMMC.
 */
static int sdmmc4_set_up_clock_and_io(struct mmc *mmc)
{
    volatile struct tegra_car *car = car_get_regs();
    volatile struct tegra_padctl *padctl = padctl_get_regs();
    (void)mmc;

    // Put SDMMC4 in reset
    car->rst_dev_l_set |= 0x8000;

    // Configure the clock to place the device into the initial mode.
    car->clk_src[CLK_SOURCE_SDMMC4] = CLK_SOURCE_FIRST | MMC_CLOCK_DIVIDER_SDR12;

    // Set the legacy divier used for detecting timeouts.
    car->clk_src_y[CLK_SOURCE_SDMMC_LEGACY] = CLK_SOURCE_FIRST | MMC_CLOCK_DIVIDER_SDR12;

    // Set SDMMC4 clock enable
    car->clk_enb_l_set |= 0x8000;
    car->clk_enb_y_set |= CAR_CONTROL_SDMMC_LEGACY;

    // host_clk_delay(0x64, clk_freq) -> Delay 100 host clock cycles
    udelay(5000);

    // Take SDMMC4 out of reset
    car->rst_dev_l_clr |= 0x8000;

    // Enable input paths for all pins.
    padctl->sdmmc4_control |=
        PADCTL_SDMMC4_ENABLE_DATA_IN | PADCTL_SDMMC4_ENABLE_CLK_IN | PADCTL_SDMMC4_DEEP_LOOPBACK;

    return 0;
}

/**
 * Sets the voltage that the given SDMMC is currently working with.
 *
 * @param mmc The controller to affect.
 * @param voltage The voltage to apply.
 */
static void sdmmc_set_working_voltage(struct mmc *mmc, enum sdmmc_bus_voltage voltage)
{
    // Apply the voltage...
    mmc->operating_voltage = voltage;

    // Set up the SD card's voltage.
    mmc->regs->power_control &= ~MMC_POWER_CONTROL_VOLTAGE_MASK;
    mmc->regs->power_control |= voltage << MMC_POWER_CONTROL_VOLTAGE_SHIFT;

    // Switch to 1V8 signaling.
    mmc->regs->host_control2 |= MMC_HOST2_USE_1V8_SIGNALING;

    // Mark the power as on.
    mmc->regs->power_control |= MMC_POWER_CONTROL_POWER_ENABLE;
}


/**
 * Enables power supplies for SDMMC4, used for eMMC.
 */
static int sdmmc4_enable_supplies(struct mmc *mmc)
{
    // As a booot device, SDMMC4's power supply is always on.
    // Modify the controller to know the voltage being applied to it,
    // and return success.
    sdmmc_set_working_voltage(mmc, MMC_VOLTAGE_1V8);
    return 0;
}


/**
 * Enables power supplies for SDMMC1, used for the SD card slot.
 */
static int sdmmc1_enable_supplies(struct mmc *mmc)
{
    volatile struct tegra_pmc *pmc = pmc_get_regs();
    volatile struct tegra_pinmux *pinmux = pinmux_get_regs();

    // Set PAD_E_INPUT_OR_E_PWRD (relevant for eMMC only)
    mmc->regs->sdmemcomppadctrl |= 0x80000000;

    // Ensure the PMC is prepared for the SDMMC card to recieve power.
    pmc->no_iopower  &= ~PMC_CONTROL_SDMMC1;
    pmc->pwr_det_val |= PMC_CONTROL_SDMMC1;

    // Set up SD card voltages.
    udelay(1000);
    supply_enable(SUPPLY_MICROSD, false);
    udelay(1000);

    // Modify the controller to know the voltage being applied to it.
    sdmmc_set_working_voltage(mmc, MMC_VOLTAGE_3V3);

    // Configure the enable line for the SD card power.
    pinmux->dmic3_clk  =  PINMUX_SELECT_FUNCTION0;
    gpio_configure_mode(GPIO_MICROSD_SUPPLY_ENABLE, GPIO_MODE_GPIO);
    gpio_configure_direction(GPIO_MICROSD_SUPPLY_ENABLE, GPIO_DIRECTION_OUTPUT);

    // Bring up the SD card fixed regulator.
    gpio_write(GPIO_MICROSD_SUPPLY_ENABLE, GPIO_LEVEL_HIGH);
    return 0;
}


/**
 * Configures clocking parameters for a given controller.
 *
 * @param mmc The MMC controller to set up.
 * @param operating_voltage The operating voltage for the bus, currently.
 */
static int sdmmc_set_up_clocking_parameters(struct mmc *mmc, enum sdmmc_bus_voltage operating_voltage)
{
    // Clear the I/O conditioning constants.
    mmc->regs->vendor_clock_cntrl &= ~(MMC_CLOCK_TRIM_MASK | MMC_CLOCK_TAP_MASK);

    // Per the TRM, set the PADPIPE clock enable.
    mmc->regs->vendor_clock_cntrl |= MMC_CLOCK_PADPIPE_CLKEN_OVERRIDE;

    // Set up the I/O conditioning constants used to ensure we have a reliable clock.
    // Constants above and procedure below from the TRM.
    switch (mmc->controller) {
        case SWITCH_EMMC:
            if (operating_voltage != MMC_VOLTAGE_1V8) {
                mmc_print(mmc, "ERROR: eMMC can only run at 1V8, but mmc struct claims voltage %d", operating_voltage);
                return EINVAL;
            }

            mmc->regs->auto_cal_config &= ~MMC_AUTOCAL_PDPU_CONFIG_MASK;
            mmc->regs->auto_cal_config |= MMC_AUTOCAL_PDPU_SDMMC4_1V8;
            mmc->regs->vendor_clock_cntrl |= (MMC_CLOCK_TRIM_SDMMC4 | MMC_CLOCK_TAP_SDMMC4);
            break;

        case SWITCH_MICROSD:
            switch (operating_voltage) {
                case MMC_VOLTAGE_1V8:
                    mmc->regs->auto_cal_config &= ~MMC_AUTOCAL_PDPU_CONFIG_MASK;
                    mmc->regs->auto_cal_config |= MMC_AUTOCAL_PDPU_SDMMC1_1V8;
                    break;
                case MMC_VOLTAGE_3V3:
                    mmc->regs->auto_cal_config &= ~MMC_AUTOCAL_PDPU_CONFIG_MASK;
                    mmc->regs->auto_cal_config |= MMC_AUTOCAL_PDPU_SDMMC1_3V3;
                    break;
                default:
                    mmc_print(mmc, "ERROR: microsd does not support voltage %d", operating_voltage);
                    return EINVAL;
            }
            mmc->regs->vendor_clock_cntrl |= (MMC_CLOCK_TRIM_SDMMC1 | MMC_CLOCK_TAP_SDMMC1);
            break;

        default:
            printk("ERROR: initialization not yet writen for SDMMC%d", mmc->controller);
            return ENODEV;
    }

    return 0;
}


/**
 * Enables or disables delivering a clock to the downstream SD/MMC card.
 *
 * @param mmc The controller to be affected.
 * @param enabled True if the clock should be enabled; false to disable.
 */
void sdmmc_clock_enable(struct mmc *mmc, bool enabled)
{
    // Set or clear the card clock enable bit according to the
    // controller paramter.
    if (enabled)
        mmc->regs->clock_control |= MMC_CLOCK_CONTROL_CARD_CLOCK_ENABLE;
    else
        mmc->regs->clock_control &= ~MMC_CLOCK_CONTROL_CARD_CLOCK_ENABLE;
}


/**
 * Runs SDMMC automatic calibration-- this tunes the parameters used for SDMMC
 * signal intergrity.
 *
 * @param mmc The controller whose card is to be tuned.
 * @param restart_sd_clock True iff the SD card should be started after calibration.
 *
 * @return 0 on success, or an error code on failure
 */
static int sdmmc_run_autocal(struct mmc *mmc, bool restart_sd_clock)
{
    uint32_t timebase;
    int ret = 0;

    // Stop the SD card's clock, so our autocal sequence doesn't
    // confuse the target card.
    sdmmc_clock_enable(mmc, false);

    // Start automatic calibration...
    mmc->regs->auto_cal_config |= (MMC_AUTOCAL_START | MMC_AUTOCAL_ENABLE);
    udelay(1);

    // ... and wait until the autocal is complete
    timebase = get_time();
    while ((mmc->regs->auto_cal_status & MMC_AUTOCAL_ACTIVE)) {

        // Ensure we haven't timed out...
        if (get_time_since(timebase) > MMC_AUTOCAL_TIMEOUT) {
            mmc_print(mmc, "ERROR: autocal timed out!");
            if (mmc->controller == SWITCH_MICROSD) {
                // Fallback driver strengths from Tegra X1 TRM
                uint32_t drvup = (mmc->operating_voltage == MMC_VOLTAGE_3V3) ? 0x12 : 0x11;
                uint32_t drvdn = (mmc->operating_voltage == MMC_VOLTAGE_3V3) ? 0x12 : 0x15;
                uint32_t value = APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL_0;
                value &= ~(SDMMC1_PAD_CAL_DRVUP_MASK | SDMMC1_PAD_CAL_DRVDN_MASK);
                value |= (drvup << SDMMC1_PAD_CAL_DRVUP_SHIFT);
                value |= (drvdn << SDMMC1_PAD_CAL_DRVDN_SHIFT);
                APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL_0 = value;
            } else if (mmc->controller == SWITCH_EMMC) {
                uint32_t value = APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0;
                value &= ~(CFG2TMC_EMMC4_PAD_DRVUP_COMP_MASK | CFG2TMC_EMMC4_PAD_DRVDN_COMP_MASK);
                value |= (0x10 << CFG2TMC_EMMC4_PAD_DRVUP_COMP_SHIFT);
                value |= (0x10 << CFG2TMC_EMMC4_PAD_DRVDN_COMP_SHIFT);
                APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 = value;
            }
            mmc->regs->auto_cal_config &= ~MMC_AUTOCAL_ENABLE;
            ret = ETIMEDOUT;
            break;
        }
    }

    // If requested, enable the SD clock.
    if (restart_sd_clock)
        sdmmc_clock_enable(mmc, true);

    return ret;
}


/**
 * Switches the Switch's microSD card into low-voltage mode.
 *
 * @param mmc The MMC controller via which to communicate.
 * @return 0 on success, or an error code on failure.
 */
static int sdmmc1_switch_to_low_voltage(struct mmc *mmc)
{
    volatile struct tegra_pmc *pmc = pmc_get_regs();
    int rc;

    // Let the SD card know we're about to switch into low-voltage mode.
    // Set up the card's relative address.
    rc = sdmmc_send_simple_command(mmc, CMD_SWITCH_TO_LOW_VOLTAGE, MMC_RESPONSE_LEN48, 0, NULL);
    if (rc) {
        mmc_print(mmc, "card was not willling to switch to low voltage! (%d)", rc);
        return rc;
    }

    // Switch the MicroSD card supply into its low-voltage mode.
    supply_enable(SUPPLY_MICROSD, true);
    pmc->pwr_det_val &= ~PMC_CONTROL_SDMMC1;

    // Apply our clocking parameters for low-voltage mode.
    rc = sdmmc_set_up_clocking_parameters(mmc, MMC_VOLTAGE_1V8);
    if (rc) {
        mmc_print(mmc, "WARNING: could not optimize card clocking parameters. (%d)", rc);
    }

    // Rerun the main clock calibration...
    rc = sdmmc_run_autocal(mmc, false);
    if (rc)
        mmc_print(mmc, "WARNING: failed to re-calibrate after voltage switch!");

    // ... and ensure the host is set up to apply the relevant change.
    sdmmc_set_working_voltage(mmc, MMC_VOLTAGE_1V8);
    udelay(MMC_VOLTAGE_SWITCH_TIME);

    // Enable the SD clock.
    sdmmc_clock_enable(mmc, true);
    udelay(MMC_POST_CLOCK_DELAY);

    mmc_debug(mmc, "now running from 1V8");
    return 0;
}


/**
 * Low-voltage switching method for controllers that don't
 * support a low-voltage switch. Always fails.
 *
 * @param mmc The MMC controller via which to communicate.
 * @return ENOSYS, indicating failure, always
 */
static int sdmmc_always_fail(struct mmc *mmc)
{
    // This card is always in low voltage and should never have to switch.
    return ENOSYS;
}


/**
 * Sets up the clock for the given SDMMC controller.
 * Assumes the controller's clock has been stopped with sdmmc_clock_enable before use.
 *
 * @param mmc The controller to work with.
 * @param source The clock source, as defined by the CAR.
 * @param car_divisor. The divisor for that source in the CAR. Usually one of the MMC_CLOCK_DIVIDER macros;
 *      a divider of N results in a clock that's (N/2) + 1 slower.
 * @param sdmmc_divisor An additional divisor applied in the SDMMC controller.
 */
static void sdmmc4_configure_clock(struct mmc *mmc, int source, int car_divisor, int sdmmc_divisor)
{
    volatile struct tegra_car *car = car_get_regs();

    // Set up the CAR aspect of the clock, and wait 2uS per change per the TRM.
    car->clk_enb_l_clr = CAR_CONTROL_SDMMC4;
    car->clk_src[CLK_SOURCE_SDMMC4] = source | car_divisor;
    udelay(2);
    car->clk_enb_l_set = CAR_CONTROL_SDMMC4;

    // ... and the SDMMC section of it.
    mmc->regs->clock_control &= ~MMC_CLOCK_CONTROL_FREQUENCY_MASK;
    mmc->regs->clock_control |= sdmmc_divisor << MMC_CLOCK_CONTROL_FREQUENCY_SHIFT;
}


/**
 * Sets up the clock for the given SDMMC controller.
 * Assumes the controller's clock has been stopped with sdmmc_clock_enable before use.
 *
 * @param mmc The controller to work with.
 * @param source The clock source, as defined by the CAR.
 * @param car_divisor. The divisor for that source in the CAR. Usually one of the MMC_CLOCK_DIVIDER macros;
 *      a divider of N results in a clock that's (N/2) + 1 slower.
 * @param sdmmc_divisor An additional divisor applied in the SDMMC controller.
 */
static void sdmmc1_configure_clock(struct mmc *mmc, int source, int car_divisor, int sdmmc_divisor)
{
    volatile struct tegra_car *car = car_get_regs();

    // Set up the CAR aspect of the clock, and wait 2uS per change per the TRM.
    car->clk_enb_l_clr = CAR_CONTROL_SDMMC1;
    car->clk_src[CLK_SOURCE_SDMMC1] = source | car_divisor;
    udelay(2);
    car->clk_enb_l_set = CAR_CONTROL_SDMMC1;

    // ... and the SDMMC section of it.
    mmc->regs->clock_control &= ~MMC_CLOCK_CONTROL_FREQUENCY_MASK;
    mmc->regs->clock_control |= sdmmc_divisor << MMC_CLOCK_CONTROL_FREQUENCY_SHIFT;
}


/**
 * Runs a single iteration of an active SDMMC clock tune.
 * You probably want sdmmc_tune_clock instead.
 */
static int sdmmc_run_tuning_iteration(struct mmc *mmc, enum sdmmc_command tuning_command)
{
    int rc;
    uint32_t saved_int_enable = mmc->regs->int_enable;

    // Enable the BUFFER_READ_READY interrupt for this run, and make sure it's not set.
    mmc->regs->int_enable |= MMC_STATUS_BUFFER_READ_READY;
    mmc->regs->int_status = mmc->regs->int_status;

    // Wait until we can issue commands to the device.
    rc = sdmmc_wait_for_command_readiness(mmc);
    if (rc) {
        mmc_print(mmc, "card not willing to accept commands (%d / %08x)", rc, mmc->regs->present_state);
        return EBUSY;
    }

    rc = sdmmc_wait_for_data_readiness(mmc);
    if (rc) {
        mmc_print(mmc, "card not willing to accept data-commands (%d / %08x)", rc, mmc->regs->present_state);
        return EBUSY;
    }

    // Disable the SD card clock. [TRM 32.7.6.2 Step 3]
    // NVIDIA notes that issuing the re-tune command triggers a re-selection of clock tap, but also
    // due to a hardware issue, causes a one-microsecond window in which a clock glitch can occur.
    // We'll disable the SD card clock temporarily so the card itself isn't affected by the glitch.
    sdmmc_clock_enable(mmc, false);

    // Issue our tuning command.  [TRM 32.7.6.2 Step 4]
    sdmmc_prepare_command_data(mmc, 1, false, false, false, 0);
    sdmmc_prepare_command_registers(mmc, 1, tuning_command, MMC_RESPONSE_LEN48, MMC_CHECKS_ALL);

    // Wait for 1us  [TRM 32.7.6.2 Step 5]
    // As part of the workaround above, we'll wait one microsecond for the glitch window to pass.
    udelay(1);

    // Issue a software reset for the data and command lines. [TRM 32.7.6.2 Step 6/7]
    // This completes the workaround by ensuring the glitch didn't leave the sampling hardware
    // for these lines in an uncertain state. This function blocks until the glitch window has
    // complete, so it handles both TRM steps 7 and 8.
    sdmmc_hardware_reset(mmc, MMC_SOFT_RESET_CMD | MMC_SOFT_RESET_DAT);

    // Restore the SDMMC clock, now that the workaround is complete.  [TRM 32.7.6.2 Step 8]
    // This enables the actual command to be issued.
    sdmmc_clock_enable(mmc, true);

    // Wait for the command to be completed. [TRM 32.7.6.2 Step 9]
    rc = sdmmc_wait_for_event(mmc, MMC_STATUS_BUFFER_READ_READY, 0, 0, NULL);

    // Always restore the prior interrupt settings.
    mmc->regs->int_enable = saved_int_enable;

    // If we had an error waiting for the interrupt, something went wrong.
    // TODO: decide if this should be a retry condition?
    if (rc) {
        mmc_print(mmc, "buffer ready ready didn't go high in time?");
        mmc_print(mmc, "error message: %s", strerror(rc));
        mmc_print(mmc, "interrupt reg: %08x", mmc->regs->int_status);
        mmc_print(mmc, "interrupt en: %08x", mmc->regs->int_enable);
        return rc;
    }

    // Check the status of the "execute tuning", which indicates the success of
    // this tuning step. [TRM 32.7.6.2 Step 10]
    if (mmc->regs->host_control2 & MMC_HOST2_EXECUTE_TUNING)
        return EAGAIN;

    return 0;
}


/**
 * Performs an SDMMC clock tuning -- should be issued when switching up to a UHS-I
 * mode, and periodically thereafter per the spec.
 *
 * @param mmc The controller to tune.
 * @param iterations The total number of iterations to perform.
 * @param tuning_command The command to be used for tuning; usually CMD19/21.
 */
static int sdmmc_tune_clock(struct mmc *mmc, enum sdmmc_tuning_attempts iterations,
        enum sdmmc_command tuning_command)
{
    int rc;

    // We follow the NVIDIA-suggested tuning procedure (TRM section 32.7.6),
    // including sections where it deviates from the SDMMC specifications.
    //
    // This seems to produce the most reliable results, and includes workarounds
    // for bugs in the X1 hardware.

    // Read the current block order, so we can restore it.
    int original_block_order = sdmmc_get_block_order(mmc, false);

    // Stores the current timeout; we'll restore it in a bit.
    int original_timeout = mmc->timeout;

    // The SDMMC host spec suggests tuning should occur over 40 iterations, so we'll stick to that.
    // Vendors seem to deviate from this, so this is a possible area to look into if something doesn't
    // wind up working correctly.
    int attempts_remaining = sdmmc_tuning_iterations[iterations];
    mmc_debug(mmc, "executing tuning (%d iterations)", attempts_remaining);

    // Allow the tuning hardware to control e.g. our clock taps.
    mmc->regs->vendor_tuning_cntrl0 |= MMC_VENDOR_TUNING_SET_BY_HW;

    // Apply our number of tries.
    mmc->regs->vendor_tuning_cntrl0 &= ~MMC_VENDOR_TUNING_TRIES_MASK;
    mmc->regs->vendor_tuning_cntrl0 |= (iterations << MMC_VENDOR_TUNING_TRIES_SHIFT);

    // Don't use a multiplier or a divider.
    mmc->regs->vendor_tuning_cntrl0 &= ~(MMC_VENDOR_TUNING_MULTIPLIER_MASK | MMC_VENDOR_TUNING_DIVIDER_MASK);
    mmc->regs->vendor_tuning_cntrl0 |= MMC_VENDOR_TUNING_MULTIPLIER_UNITY;

    // Use zero step sizes; per TRM 32.7.6.1.
    mmc->regs->vendor_tuning_cntrl1 = MMC_VENDOR_TUNING_STEP_SIZE_SDR50_DEFAULT | MMC_VENDOR_TUNING_STEP_SIZE_SDR104_DEFAULT;

    // Start the tuning process. [TRM 32.7.6.2 Step 2]
    mmc->regs->host_control2 |= MMC_HOST2_EXECUTE_TUNING;

    // Momentarily step down to a smaller block size, so we don't
    // have to allocate a huge buffer for this command.
    mmc->read_block_order = mmc->tuning_block_order;

    // Step down to the timeout recommended in the specification.
    mmc->timeout = MMC_TUNING_TIMEOUT;

    // Iterate an iteration of the tuning process.
    while (attempts_remaining--) {

        // Run an iteration of our tuning process.
        rc = sdmmc_run_tuning_iteration(mmc, tuning_command);

        // If we have an error other than "retry, break.
        if (rc != EAGAIN)
            break;
    }

    // Restore the original parameters.
    mmc->read_block_order = original_block_order;
    mmc->timeout = original_timeout;

    // If we exceeded our attempts, set this as a timeout.
    if (rc == EAGAIN) {
        mmc_print(mmc, "tuning attempts exceeded!");
        rc = ETIMEDOUT;
    }

    // If the tuning failed, for any reason, print and return the error.
    if (rc) {
        mmc_print(mmc, "ERROR: failed to tune the SDMMC clock! (%d)", rc);
        mmc_print(mmc, "error message %s", strerror(rc));
        return rc;
    }

    // If we've made it here, this iteration completed tuning.
    // Check for a tuning failure (SAMPLE CLOCK = 0). [TRM 32.7.6.2 Step 11]
    if (!(mmc->regs->host_control2 & MMC_HOST2_SAMPLING_CLOCK_ENABLED)) {
        mmc_print(mmc, "ERROR: tuning failed after complete iteration!");
        mmc_print(mmc, "host_control2: %08x", mmc->regs->host_control2);
        return EIO;
    }

    return 0;
}


/**
 * Configures the host controller to work at a given UHS-I mode.
 *
 * @param mmc The controller to work with
 * @param speed The UHS or pre-UHS speed to work at.
 */
static void sdmmc_set_uhs_mode(struct mmc *mmc, enum sdmmc_bus_speed speed)
{
    // Set the UHS mode register.
    mmc->regs->host_control2 &= MMC_HOST2_UHS_MODE_MASK;
    mmc->regs->host_control2 |= speed;
}


/**
 * Applies the appropriate clock dividers to the CAR and SD controller to enable use of the
 * provided speed. Does not handle any requisite communications with the card.
 *
 * @param mmc The controller to affect.
 * @param speed The speed to apply.
 * @param enable_after If set, the SDMMC clock will be enabled after the change. If not, it will be left disabled.
 */
static int sdmmc_apply_clock_speed(struct mmc *mmc, enum sdmmc_bus_speed speed, bool enable_after)
{
    int rc;

    // By default, don't execute tuning after applying the clock.
    bool execute_tuning = false;
    enum sdmmc_tuning_attempts tuning_attempts = MMC_VENDOR_TUNING_TRIES_40;
    enum sdmmc_command tuning_command = CMD_SD_SEND_TUNING_BLOCK;

    // Ensure the clocks are not currently running to avoid glitches.
    sdmmc_clock_enable(mmc, false);

    // Clear the registers of any existing values, so we can apply new ones.
    mmc->regs->host_control &= ~MMC_HOST_ENABLE_HIGH_SPEED;

    // Apply the dividers according to the speed provided.
    switch (speed) {

        // 400kHz initialization mode.
        case SDMMC_SPEED_INIT:
            mmc->configure_clock(mmc, MMC_CLOCK_SOURCE_SDR12, MMC_CLOCK_DIVIDER_SDR12, MMC_CLOCK_CONTROL_FREQUENCY_INIT);
            sdmmc_set_uhs_mode(mmc, SDMMC_SPEED_SDR12);
            break;

        // 25MHz default speed (SD)
        case SDMMC_SPEED_SDR12:
            mmc->configure_clock(mmc, MMC_CLOCK_SOURCE_SDR12, MMC_CLOCK_DIVIDER_SDR12, MMC_CLOCK_CONTROL_FREQUENCY_PASSTHROUGH);
            sdmmc_set_uhs_mode(mmc, SDMMC_SPEED_SDR12);
            break;

        // 26MHz default speed (MMC)
        case SDMMC_SPEED_HS26:
            mmc->configure_clock(mmc, MMC_CLOCK_SOURCE_HS26, MMC_CLOCK_DIVIDER_HS26, MMC_CLOCK_CONTROL_FREQUENCY_PASSTHROUGH);
            break;

        // 50MHz high speed (SD)
        case SDMMC_SPEED_SDR25:
            // Configure the host to use high-speed timing.
            mmc->regs->host_control |= MMC_HOST_ENABLE_HIGH_SPEED;
            mmc->configure_clock(mmc, MMC_CLOCK_SOURCE_SDR25, MMC_CLOCK_DIVIDER_SDR25, MMC_CLOCK_CONTROL_FREQUENCY_PASSTHROUGH);
            sdmmc_set_uhs_mode(mmc, SDMMC_SPEED_SDR25);
            break;

        // 52MHz high speed (MMC)
        case SDMMC_SPEED_HS52:
            mmc->configure_clock(mmc, MMC_CLOCK_SOURCE_HS52, MMC_CLOCK_DIVIDER_HS52, MMC_CLOCK_CONTROL_FREQUENCY_PASSTHROUGH);
            break;

        // 100MHz UHS-I (SD)
        case SDMMC_SPEED_SDR50:
            mmc->regs->host_control |= MMC_HOST_ENABLE_HIGH_SPEED;
            mmc->configure_clock(mmc, MMC_CLOCK_SOURCE_SDR50, MMC_CLOCK_DIVIDER_SDR50, MMC_CLOCK_CONTROL_FREQUENCY_PASSTHROUGH);
            // Tegra X1 Series Processors Silicon Errata MMC-2 mentions setting SDR104 mode as workaround.
            sdmmc_set_uhs_mode(mmc, SDMMC_SPEED_SDR104);

            execute_tuning = true;
            tuning_attempts = MMC_VENDOR_TUNING_TRIES_SDR50;
            break;

        // 200MHz UHS-I (SD)
        case SDMMC_SPEED_SDR104:
            mmc->regs->host_control |= MMC_HOST_ENABLE_HIGH_SPEED;
            mmc->configure_clock(mmc, MMC_CLOCK_SOURCE_SDR104, MMC_CLOCK_DIVIDER_SDR104, MMC_CLOCK_CONTROL_FREQUENCY_PASSTHROUGH);
            sdmmc_set_uhs_mode(mmc, SDMMC_SPEED_SDR104);

            execute_tuning = true;
            tuning_attempts = MMC_VENDOR_TUNING_TRIES_SDR104;
            break;

        // 200MHz single-data rate (MMC)
        case SDMMC_SPEED_HS200:
        case SDMMC_SPEED_HS400:
            mmc->regs->host_control |= MMC_HOST_ENABLE_HIGH_SPEED;
            mmc->configure_clock(mmc, MMC_CLOCK_SOURCE_HS200, MMC_CLOCK_DIVIDER_HS200, MMC_CLOCK_CONTROL_FREQUENCY_PASSTHROUGH);
            sdmmc_set_uhs_mode(mmc, SDMMC_SPEED_SDR104); // Per datasheet; we set the controller in SDR104 mode.

            // Execute tuning.
            execute_tuning = true;
            tuning_attempts = MMC_VENDOR_TUNING_TRIES_HS200;
            tuning_command = CMD_MMC_SEND_TUNING_BLOCK;
            break;

        default:
            mmc_print(mmc, "ERROR: switching to unsupported speed!\n");
            return ENOSYS;
    }

    // If we need to execute tuning for this clock mode, do so.
    if (execute_tuning) {
        rc = sdmmc_tune_clock(mmc, tuning_attempts, tuning_command);
        if (rc) {
            mmc_print(mmc, "WARNING: clock tuning failed! speed mode can't be used. (%d)", rc);
            sdmmc_clock_enable(mmc, true);
            return rc;
        }
    }

    // Special case: HS400 mode should be applied _after_ HS200 is applied, so we apply that
    // first above, and then switch up and re-tune.
    if (speed == SDMMC_SPEED_HS400) {
        sdmmc_set_uhs_mode(mmc, SDMMC_SPEED_OTHER); // Special value, per datasheet

        // Per the TRM, we should also use this opportunity to set up the DQS path trimmer.
        // TODO: should this be used only in HS400?
        mmc->regs->vendor_cap_overrides &= ~MMC_VENDOR_CAPABILITY_DQS_TRIM_MASK;
        mmc->regs->vendor_cap_overrides |= MMC_VENDOR_CAPABILITY_DQS_TRIM_HS400;

        // TODO: run the DLLCAL here!

        mmc_print(mmc, "TODO: double the data rate here!");
    }


    // Re-enable the clock, if necessary.
    if (enable_after) {
        sdmmc_clock_enable(mmc, true);
        udelay(MMC_POST_CLOCK_DELAY);
    }

    // Finally store the operating speed.
    mmc->operating_speed = speed;
    return 0;
}


/**
 * Performs low-level initialization for SDMMC1, used for the SD card slot.
 */
static int sdmmc1_set_up_clock_and_io(struct mmc *mmc)
{
    volatile struct tegra_car *car = car_get_regs();
    volatile struct tegra_pinmux *pinmux = pinmux_get_regs();
    volatile struct tegra_padctl *padctl = padctl_get_regs();
    (void)mmc;

    // Set up each of the relevant pins to be connected to output drivers,
    // and selected for SDMMC use.
    pinmux->sdmmc1_clk  = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT;
    pinmux->sdmmc1_cmd  = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;
    pinmux->sdmmc1_dat3 = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;
    pinmux->sdmmc1_dat2 = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;
    pinmux->sdmmc1_dat1 = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;
    pinmux->sdmmc1_dat0 = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;

    // Set up the card detect pin as a GPIO input.
    pinmux->pz1 = PINMUX_TRISTATE | PINMUX_SELECT_FUNCTION1 | PINMUX_PULL_UP | PINMUX_INPUT;
    gpio_configure_mode(GPIO_MICROSD_CARD_DETECT, GPIO_MODE_GPIO);
    gpio_configure_direction(GPIO_MICROSD_CARD_DETECT, GPIO_DIRECTION_INPUT);
    udelay(100);

    // Ensure we're using GPIO and not GPIO for the SD card's card detect.
    padctl->vgpio_gpio_mux_sel &= ~PADCTL_SDMMC1_CD_SOURCE;

    // Put SDMMC1 in reset
    car->rst_dev_l_set = CAR_CONTROL_SDMMC1;

    // Configure the clock to place the device into the initial mode.
    car->clk_src[CLK_SOURCE_SDMMC1] = CLK_SOURCE_FIRST | MMC_CLOCK_DIVIDER_SDR12;

    // Set the legacy divier used for detecting timeouts.
    car->clk_src_y[CLK_SOURCE_SDMMC_LEGACY] = CLK_SOURCE_FIRST | MMC_CLOCK_DIVIDER_SDR12;

    // Set SDMMC1 clock enable
    car->clk_enb_l_set = CAR_CONTROL_SDMMC1;
    car->clk_enb_y_set = CAR_CONTROL_SDMMC_LEGACY;

    // host_clk_delay(0x64, clk_freq) -> Delay 100 host clock cycles
    udelay(5000);

    // Take SDMMC4 out of reset
    car->rst_dev_l_clr |= CAR_CONTROL_SDMMC1;

    // Enable clock loopback.
    padctl->sdmmc1_control |= PADCTL_SDMMC1_DEEP_LOOPBACK;

    return 0;
}


/**
 * Initialize the low-level SDMMC hardware.
 * Thanks to hexkyz for this init code.
 *
 * FIXME: clean up the magic numbers, split into sections.
 */
static int sdmmc_hardware_init(struct mmc *mmc)
{
    volatile struct tegra_sdmmc *regs = mmc->regs;

    uint32_t timebase;
    bool is_timeout;

    int rc;

    // Initialize the Tegra resources necessary to use the given piece of hardware.
    rc = mmc->set_up_clock_and_io(mmc);
    if (rc) {
        mmc_print(mmc, "ERROR: could not set up controller for use!");
        return rc;
    }

    // Software reset the SDMMC device.
    rc = sdmmc_hardware_reset(mmc, MMC_SOFT_RESET_FULL);
    if (rc) {
        mmc_print(mmc, "failed to reset!");
        return rc;
    }

    // Turn on the card's power supplies...
    rc = mmc->enable_supplies(mmc);
    if (rc) {
        mmc_print(mmc, "ERROR: could power on the card!");
        return rc;
    }


    // Set IO_SPARE[19] (one cycle delay)
    regs->io_spare |= 0x80000;

    // Clear SEL_VREG
    regs->vendor_io_trim_cntrl &= ~(0x04);

    // Set SDMMC2TMC_CFG_SDMEMCOMP_VREF_SEL to 0x07
    regs->sdmemcomppadctrl &= ~(0x0F);
    regs->sdmemcomppadctrl |= 0x07;

    // Set ourselves up to have a stable.
    rc = sdmmc_set_up_clocking_parameters(mmc, mmc->operating_voltage);
    if (rc) {
        mmc_print(mmc, "WARNING: could not optimize card clocking parameters. (%d)", rc);
    }

    // Set PAD_E_INPUT_OR_E_PWRD
    regs->sdmemcomppadctrl |= 0x80000000;

    // Wait one milisecond
    udelay(1000);

    // Run automatic calibration.
    rc = sdmmc_run_autocal(mmc, false);
    if (rc) {
        mmc_print(mmc, "autocal failed! (%d)", rc);
        return rc;
    }

    // Clear PAD_E_INPUT_OR_E_PWRD (relevant for eMMC only)
    regs->sdmemcomppadctrl &= ~(0x80000000);

    // Set SDHCI_CLOCK_INT_EN
    regs->clock_control |= 0x01;

    // Program a timeout of 2000ms
    timebase = get_time();
    is_timeout = false;

    // Wait for SDHCI_CLOCK_INT_STABLE to be set
    while (!(regs->clock_control & 0x02) && !is_timeout) {
        // Keep checking if timeout expired
        is_timeout = get_time_since(timebase) > 2000000;
    }

    // Clock failed to stabilize
    if (is_timeout) {
        mmc_print(mmc, "clock never stabalized!");
        return -1;
    }

    // FIXME: replace this to support better clocks
    regs->host_control2 = 0;

    // Clear SDHCI_PROG_CLOCK_MODE
    regs->clock_control &= ~(0x20);

    // Ensure we're using Single-operation DMA (SDMA) mode for DMA.
    regs->host_control &= ~MMC_DMA_SELECT_MASK;

    // Set the timeout to be the maximum value
    regs->timeout_control &= ~(0x0F);
    regs->timeout_control |= 0x0E;

    // Ensure we start off using a single-bit bus.
    mmc->regs->host_control &= ~MMC_HOST_BUS_WIDTH_MASK;

    // TODO: move me into enable voltages, if applicable?

    // Clear TAP_VAL_UPDATED_BY_HW
    regs->vendor_tuning_cntrl0 &= ~(0x20000);

    // Start off in non-high-speed mode.
    regs->host_control &= ~MMC_HOST_ENABLE_HIGH_SPEED;

    // Clear SDHCI_CTRL_VDD_180
    regs->host_control2 &= ~(0x08);

    // Set up the card's initialization.
    rc = sdmmc_apply_clock_speed(mmc, SDMMC_SPEED_INIT, true);
    if (rc) {
        mmc_print(mmc, "ERROR: could not set SDMMC card speed!");
        return rc;
    }

    return 0;
}


/*
 * Blocks until the card has reached a given physical state,
 * as indicated by the present state register.
 *
 * @param mmc The MMC controller whose state we should wait on
 * @param present_state_mask A mask that indicates when we should return.
 *      Returns when the mask bits are no longer set in present_state if invert is true,
 *      or true when the mask bits are _set_ in the present state if invert is false.
 *
 * @return 0 on success, or an error on failure
 */
static int sdmmc_wait_for_physical_state(struct mmc *mmc, uint32_t present_state_mask, bool invert)
{
    uint32_t timebase = get_time();
    uint32_t condition;

    // Retry until the event or an error happens
    while (true) {

        // Handle timeout.
        if (get_time_since(timebase) > mmc->timeout) {
            mmc_print(mmc, "timed out waiting for command readiness!");
            return ETIMEDOUT;
        }

        // Read the status, and invert the condition, if necessary.
        condition = mmc->regs->present_state & present_state_mask;
        if (invert) {
            condition = !condition;
        }

        // Return once our condition is met.
        if (condition)
            return 0;
    }
}


/**
 * Blocks until the SD driver is ready for a command,
 * or the MMC controller's timeout interval is met.
 *
 * @param mmc The MMC controller
 */
static int sdmmc_wait_for_command_readiness(struct mmc *mmc)
{
    return sdmmc_wait_for_physical_state(mmc, MMC_COMMAND_INHIBIT, true);
}


/**
 * Blocks until the SD driver is ready to transmit data,
 * or the MMC controller's timeout interval is met.
 *
 * @param mmc The MMC controller whose data line we should wait for.
 */
static int sdmmc_wait_for_data_readiness(struct mmc *mmc)
{
    return sdmmc_wait_for_physical_state(mmc, MMC_DATA_INHIBIT, true);
}


/**
 * Blocks until the SD driver's data lines are clear,
 * indicating the card is no longer busy.
 *
 * @param mmc The MMC controller whose data line we should wait for.
 */
static int sdmmc_wait_until_no_longer_busy(struct mmc *mmc)
{
    return sdmmc_wait_for_physical_state(mmc, MMC_DAT0_LINE_STATE, false);
}

/**
 * Handles an event in which the given SDMMC controller's DMA buffers have
 * become full, and must be emptied again before they can be used.
 *
 * @param mmc The MMC controller that has suffered a full buffer.
 */
static int sdmmc_flush_bounce_buffer(struct mmc *mmc)
{
    // Determine the total amount copied by subtracting the current pointer from
    // its starting address-- effectively by figuring out how far we got in the bounce buffer.
    uint32_t total_copied = mmc->regs->dma_address - (uint32_t)sdmmc_bounce_buffer;

    // If we have a DMA buffer we're copying to, empty it out.
    if (mmc->active_data_buffer) {

        // Copy the data to the user buffer, and advance in the user buffer
        // by the amount coppied.
        memcpy((void *)mmc->active_data_buffer, sdmmc_bounce_buffer, total_copied);
        mmc->active_data_buffer += total_copied;
    }

    // Reset the DMA to point at the beginning of our bounce buffer for another interation.
    mmc->regs->dma_address = (uint32_t)sdmmc_bounce_buffer;
    return 0;
}

/**
 * Generic SDMMC waiting function.
 *
 * @param mmc The MMC controller on which to wait.
 * @param target_irq A bitmask that specifies the interrupt bits that
 *      will make this function return success.
 * @param state_condition A bitmask that specifies a collection of bits
 *      that indicate business in present_state. If zero, all of the relevant
 *      conditions becoming false will cause a sucessful return.
 * @param fault_conditions A bitmask that specifies the bits that
 *      will make this function trigger its fault handler.
 *  @param fault_handler A function that's called to handle DMA faults.
 *      If it returns nonzero, this method will abort immediately; if it
 *      returns zero, it'll clear the error and continue.
 *
 * @return 0 on sucess, EFAULT if a fault condition occurs,
 *      or an error code if a transfer failure occurs
 */
static int sdmmc_wait_for_event(struct mmc *mmc,
        uint32_t target_irq, uint32_t state_conditions,
        uint32_t fault_conditions, fault_handler_t fault_handler)
{
    uint32_t timebase = get_time();
    uint32_t intstatus;
    int rc;

    // Wait until we either wind up ready, or until we've timed out.
    while (true) {
        if (get_time_since(timebase) > mmc->timeout)
            return ETIMEDOUT;

        // Read intstatus into temporary variable to make sure that the
        // priorities are: fault conditions, target irq, errors
        // This makes sure that if fault conditions and target irq
        // comes nearly at the same time that the fault handler will
        // always be called
        intstatus = mmc->regs->int_status;
        if (intstatus & fault_conditions) {

            // If we don't have a handler, fault.
            if (!fault_handler) {
                mmc_print(mmc, "ERROR: unhandled DMA fault!");
                return EFAULT;
            }

            // Call the DMA fault handler.
            rc = fault_handler(mmc);
            if (rc) {
                mmc_print(mmc, "ERROR: unhandled DMA fault! (%d)", rc);
                return rc;
            }

            // Finally, EOI the relevant interrupt.
            mmc->regs->int_status = fault_conditions;
            intstatus &= ~(fault_conditions);

            // Reset the timebase, so it applies to the next
            // DMA interval.
            timebase = get_time();
        }

        if (intstatus & target_irq)
            return 0;

        if (state_conditions && !(mmc->regs->present_state & state_conditions))
            return 0;

        // If an error occurs, return it.
        if (intstatus & MMC_STATUS_ERROR_MASK)
            return (intstatus & MMC_STATUS_ERROR_MASK);
    }
}

/**
 * Blocks until the SD driver has completed issuing a command.
 *
 * @param mmc The MMC controller
 */
static int sdmmc_wait_for_command_completion(struct mmc *mmc)
{
    return sdmmc_wait_for_event(mmc, MMC_STATUS_COMMAND_COMPLETE, 0, 0, NULL);
}


/**
 * Blocks until the SD driver has completed issuing a command.
 *
 * @param mmc The MMC controller
 */
static int sdmmc_wait_for_transfer_completion(struct mmc *mmc)
{
    int rc = sdmmc_wait_for_event(mmc, MMC_STATUS_TRANSFER_COMPLETE,
            MMC_WRITE_ACTIVE | MMC_READ_ACTIVE, MMC_STATUS_DMA_INTERRUPT, sdmmc_flush_bounce_buffer);


    return rc;
}


/**
 *  Returns the block order for a given operation on the MMC controller.
 *
 *  @param mmc The MMC controller for which we're quierying block size.
 *  @param is_write True iff the given operation is a write.
 */
static uint8_t sdmmc_get_block_order(struct mmc *mmc, bool is_write)
{
    if (is_write)
        return mmc->write_block_order;
    else
        return mmc->read_block_order;
}


/**
 *  Returns the block size for a given operation on the MMC controller.
 *
 *  @param mmc The MMC controller for which we're quierying block size.
 *  @param is_write True iff the given operation is a write.
 */
static uint32_t sdmmc_get_block_size(struct mmc *mmc, bool is_write)
{
    return (1 << sdmmc_get_block_order(mmc, is_write));
}


/**
 * Handles execution of a DATA stage using the CPU, rather than by using DMA.
 *
 * @param mmc The MMc controller to work with.
 * @param blocks The number of blocks to work with.
 * @param is_write True iff the data is being set _to_ the CARD.
 * @param data_buffer The data buffer to be transmitted or populated.
 *
 * @return 0 on success, or an error code on failure.
 */
static int sdmmc_handle_cpu_transfer(struct mmc *mmc, uint16_t blocks, bool is_write, void *data_buffer)
{
    uint16_t blocks_remaining = blocks;
    uint16_t bytes_remaining_in_block;

    uint32_t timebase = get_time();

    // Get a window that lets us work with the data buffer in 32-bit chunks.
    uint32_t *buffer = data_buffer;

    // Figure out the mask to check based on whether this is a read or a write.
    uint32_t mask = is_write ? MMC_BUFFER_WRITE_ENABLE : MMC_BUFFER_READ_ENABLE;

    // While we have blocks left to read...
    while (blocks_remaining) {

        // Get the number of bytes per block read.
        bytes_remaining_in_block = sdmmc_get_block_size(mmc, false);

        // Wait for a block read to complete.
        while (!(mmc->regs->present_state & mask)) {

            // If an error occurs, return it.
            if (mmc->regs->int_status & MMC_STATUS_ERROR_MASK) {
                return (mmc->regs->int_status & MMC_STATUS_ERROR_MASK);
            }

            // Check for timeout.
            if (get_time_since(timebase) > mmc->timeout)
                return ETIMEDOUT;
        }

        // While we've still bytes left to read.
        while (bytes_remaining_in_block) {

            // Check for timeout.
            if (get_time_since(timebase) > mmc->timeout)
                return ETIMEDOUT;

            // Transfer the data to the relevant
            if (is_write) {
                if ((uintptr_t)buffer & 3) {
                    // Handle unaligned buffers
                    uint32_t w;
                    uint8_t *data = (uint8_t *)buffer;
                    w = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
                    mmc->regs->buffer = w;
                } else {
                    mmc->regs->buffer = *buffer;
                }
            } else {
                if ((uintptr_t)buffer & 3) {
                    // Handle unaligned buffers
                    uint32_t w = mmc->regs->buffer;
                    uint8_t *data = (uint8_t *)buffer;
                    data[0] = w & 0xFF;
                    data[1] = (w >> 8) & 0xFF;
                    data[2] = (w >> 16) & 0xFF;
                    data[3] = (w >> 24) & 0xFF;
                } else {
                    *buffer = mmc->regs->buffer;
                }
            }

            // Advance by a register size...
            bytes_remaining_in_block -= sizeof(mmc->regs->buffer);
            ++buffer;
        }

        // Advice by a block...
        --blocks_remaining;
    }

    return 0;
}

/**
 * Prepare the data-related registers for command transmission.
 *
 * @param mmc The device to be used to transmit.
 * @param blocks The total number of blocks to be transferred.
 * @param is_write True iff we're sending data _to_ the card.
 * @param auto_termiante True iff we should instruct the system
 *      to reclaim the data lines after a transaction.
 */
static void sdmmc_prepare_command_data(struct mmc *mmc, uint16_t blocks,
        bool is_write, bool auto_terminate, bool use_dma, int argument)
{
    if (blocks) {
        uint16_t block_size = sdmmc_get_block_size(mmc, is_write);

        // If we're using DMA, target our bounce buffer.
        if (use_dma)
            mmc->regs->dma_address = (uint32_t)sdmmc_bounce_buffer;

        // Set up the DMA block size and count.
        // This is synchronized with the size of our bounce buffer.
        mmc->regs->block_size = sdmmc_bounce_dma_boundary | block_size;
        mmc->regs->block_count = blocks;
    }

    // Populate the command argument.
    mmc->regs->argument = argument;

    if (blocks) {
        uint32_t to_write = MMC_TRANSFER_LIMIT_BLOCK_COUNT;

        // If this controller should use DMA, set that up.
        if (use_dma)
            to_write |= MMC_TRANSFER_DMA_ENABLE;

        // If this is a multi-block datagram, indicate so.
        if (blocks > 1)
            to_write |= MMC_TRANSFER_MULTIPLE_BLOCKS;

        // If this command should automatically terminate, set the host to
        // terminate it after the block span is complete.
        if (auto_terminate) {
            // If we're in SDR104, use AUTO_CMD23 intead of AUTO_CMD12,
            // per the host controller specification.
            if (mmc->operating_speed == SDMMC_SPEED_SDR104)
                to_write |= MMC_TRANSFER_AUTO_CMD23;
            else
                to_write |= MMC_TRANSFER_AUTO_CMD12;
        }

        // If this is a read, set the READ mode.
        if (!is_write)
            to_write |= MMC_TRANSFER_CARD_TO_HOST;

        mmc->regs->transfer_mode = to_write;
    }

}


/**
 * Prepare the command-related registers for command transmission.
 *
 * @param mmc The device to be used to transmit.
 * @param blocks_to_xfer The total number of blocks to be transferred.
 * @param command The command number to issue.
 * @param response_type The type of response we'll expect.
 */
static void sdmmc_prepare_command_registers(struct mmc *mmc, int blocks_to_xfer,
        enum sdmmc_command command, enum sdmmc_response_type response_type, enum sdmmc_response_checks checks)
{
    // Populate the command number
    uint16_t to_write = (command << MMC_COMMAND_NUMBER_SHIFT) | (response_type << MMC_COMMAND_RESPONSE_TYPE_SHIFT) | checks;

    // If this is a "stop transmitting" command, set the abort flag.
    if (command == CMD_STOP_TRANSMISSION)
        to_write |= MMC_COMMAND_TYPE_ABORT;

    // If this command has a data stage, include it.
    // Note that tuning commands are atypical, but are considered to have a data stage
    // consiting of the tuning pattern.
    if (blocks_to_xfer)
        to_write |= MMC_COMMAND_HAS_DATA;

    // Write our command to the given register.
    // This must be all done at once, as writes to this register have semantic meaning.
    mmc->regs->command = to_write;
}


/**
 * Enables or disables the SDMMC interrupts.
 * We leave these masked, but checkt their status in their status register.
 *
 * @param mmc The eMMC device to work with.
 * @param enabled True if interrupts should enabled, or false to disable them.
 */
static void sdmmc_enable_interrupts(struct mmc *mmc, bool enabled)
{
    // Get an mask that represents all interrupts.
    uint32_t all_interrupts =
        MMC_STATUS_COMMAND_COMPLETE | MMC_STATUS_TRANSFER_COMPLETE |
        MMC_STATUS_DMA_INTERRUPT | MMC_STATUS_ERROR_MASK;

    // Clear any pending interrupts.
    mmc->regs->int_status = all_interrupts;

    // And enable or disable the pseudo-interrupts.
    if (enabled) {
        mmc->regs->int_enable |= all_interrupts;
    } else {
        mmc->regs->int_enable &= ~all_interrupts;
    }
}

/**
 * Handle the response to an SDMMC command, copying the data
 * from the SDMMC response holding area to the user-provided response buffer.
 */
static int sdmmc_handle_command_response(struct mmc *mmc,
        enum sdmmc_response_type type, void *response_buffer)
{
    uint32_t *buffer = (uint32_t *)response_buffer;
    int rc;

    // If we don't have a place to put the response,
    // skip copying it out.
    if (!response_buffer)
        return 0;


    switch (type) {

        // Easy case: we don't have a response. We don't need to do anything.
        case MMC_RESPONSE_NONE:
            break;

        // If we have a response we have to wait on busy-completion for,
        // wait for the DAT0 line to clear.
        case MMC_RESPONSE_LEN48_CHK_BUSY:
            mmc_print(mmc, "waiting for card to stop being busy...");
            rc = sdmmc_wait_until_no_longer_busy(mmc);
            if (rc) {
                mmc_print(mmc, "failure waiting for card to finish being busy (%d)", rc);
                return rc;
            }
            // (fall-through)

        // If we have a 48-bit response, then we have 32 bits of response and 16 bits of CRC/command.
        // The naming is a little odd, but that's thanks to the SDMMC standard.
        case MMC_RESPONSE_LEN48:
            *buffer = mmc->regs->response[0];
            break;

        // If we have a 136-bit response, we have 128 of response and 8 bits of CRC.
        // TODO: validate that this is the right format/endianness/everything
        case MMC_RESPONSE_LEN136:

            // Copy the response to the buffer manually.
            // We avoid memcpy here, because this is volatile.
            for(int i = 0; i < 4; ++i)
                buffer[i] = mmc->regs->response[i];
            break;

        default:
            mmc_print(mmc, "invalid response type; not handling response");
    }

    return 0;
}


/**
 * Sends a command to the SD card, and awaits a response.
 *
 * @param mmc The SDMMC device to be used to transmit the command.
 * @param response_type The type of response to expect-- mostly specifies the length.
 * @param checks Determines which sanity checks the host controller should run.
 * @param argument The argument to the SDMMC command.
 * @param response_buffer A buffer to store the response. Should be at uint32_t for a LEN48 command,
 *      or 16 bytes for a LEN136 command. If this arguemnt is NULL, no response will be returned.
 * @param blocks_to_transfer The number of SDMMC blocks to be transferred with the given command,
 *      or 0 to indicate that this command should not expect response data.
 * @param is_write True iff the given command issues data _to_ the card, instead of vice versa.
 * @param auto_terminate True iff the gven command needs to be terminated with e.g. CMD12
 * @param data_buffer A byte buffer that either contains the data to be sent, or which should
 *      receive data, depending on the is_write argument.
 *
 * @returns 0 on success, an error number on failure
 */
static int sdmmc_send_command(struct mmc *mmc, enum sdmmc_command command,
        enum sdmmc_response_type response_type, enum sdmmc_response_checks checks,
        uint32_t argument, void *response_buffer, uint16_t blocks_to_transfer,
        bool is_write, bool auto_terminate, void *data_buffer)
{
    uint32_t total_data_to_xfer = sdmmc_get_block_size(mmc, is_write) * blocks_to_transfer;
    int rc;

    // Store user data buffer for use by future DMA operations.
    mmc->active_data_buffer = (uint32_t)data_buffer;

    // Sanity check: if this is a data transfer, make sure we have a data buffer...
    if (blocks_to_transfer && !data_buffer) {
        mmc_print(mmc, "WARNING: no data buffer provided, but this is a data transfer!");
        mmc_print(mmc, "this does nothing; but is supported for debug");
    }

    // Wait until we can issue commands to the device.
    rc = sdmmc_wait_for_command_readiness(mmc);
    if (rc) {
        mmc_print(mmc, "card not willing to accept commands (%d / %08x)", rc, mmc->regs->present_state);
        return -EBUSY;
    }

    // If this is a data command, or a command that uses the data lines for busy-detection.
    if (blocks_to_transfer || (response_type == MMC_RESPONSE_LEN48_CHK_BUSY)) {
        rc = sdmmc_wait_for_data_readiness(mmc);
        if (rc) {
            mmc_print(mmc, "card not willing to accept data-commands (%d / %08x)", rc, mmc->regs->present_state);
            return -EBUSY;
        }
    }

    // Periodically recalibrate the SD controller
    if (mmc->controller == SWITCH_MICROSD) {
        sdmmc_run_autocal(mmc, true);
    }

    // If we have data to send, prepare it.
    sdmmc_prepare_command_data(mmc, blocks_to_transfer, is_write, auto_terminate, mmc->use_dma, argument);

    // If this is a write and we have data, we'll need to populate the bounce buffer before
    // issuing the command.
    if (blocks_to_transfer && is_write && mmc->use_dma && data_buffer)
        memcpy(sdmmc_bounce_buffer, (void *)mmc->active_data_buffer, total_data_to_xfer);

    // Ensure we get the status response we want.
    sdmmc_enable_interrupts(mmc, true);

    // Configure the controller to send the command.
    sdmmc_prepare_command_registers(mmc, blocks_to_transfer, command, response_type, checks);

    // Wait for the command to be completed.
    rc = sdmmc_wait_for_command_completion(mmc);
    if (rc) {
        mmc_print(mmc, "failed to issue %s (arg=%x, rc=%d)", sdmmc_get_command_string(command), argument, rc);
        mmc_print_command_errors(mmc, rc);

        sdmmc_enable_interrupts(mmc, false);
        return rc;
    }

    // Copy the response received to the output buffer, if applicable.
    rc = sdmmc_handle_command_response(mmc, response_type, response_buffer);
    if (rc) {
        mmc_print(mmc, "failed to handle %s response! (%d)", sdmmc_get_command_string(command), rc);
        return rc;
    }

    // If we had a data stage, handle it.
    if (blocks_to_transfer) {

        // If this is a DMA transfer, wait for its completion.
        if (mmc->use_dma) {

            // Wait for the transfer to be complete...
            rc = sdmmc_wait_for_transfer_completion(mmc);
            if (rc) {
                mmc_print(mmc, "failed to complete %s data stage via DMA (%d)", sdmmc_get_command_string(command), command, rc);
                sdmmc_enable_interrupts(mmc, false);
                return rc;
            }

            // If this is a read, and we've just finished a transfer, copy the data from
            // our bounce buffer to the target data buffer.
            if (!is_write && data_buffer)
                sdmmc_flush_bounce_buffer(mmc);
        }
        // Otherwise, perform the transfer using the CPU.
        else {
            rc = sdmmc_handle_cpu_transfer(mmc, blocks_to_transfer, is_write, data_buffer);
            if (rc) {
                mmc_print(mmc, "failed to complete CMD%d data stage via CPU (%d)", command, rc);
                sdmmc_enable_interrupts(mmc, false);
                return rc;
            }
        }
    }

    // Disable resporting psuedo-interrupts.
    // (This is mostly for when the GIC is brought up)
    sdmmc_enable_interrupts(mmc, false);

    mmc_debug(mmc, "completed %s.", sdmmc_get_command_string(command));
    return 0;
}


/**
 * Convenience function that sends a simple SDMMC command
 * and awaits response.  Wrapper around sdmmc_send_command.
 *
 * @param mmc The SDMMC device to be used to transmit the command.
 * @param response_type The type of response to expect-- mostly specifies the length.
 * @param argument The argument to the SDMMC command.
 * @param response_buffer A buffer to store the response. Should be at uint32_t for a LEN48 command,
 *      or 16 bytes for a LEN136 command.
 *
 * @returns 0 on success, an error number on failure
 */
static int sdmmc_send_simple_command(struct mmc *mmc, enum sdmmc_command command,
        enum sdmmc_response_type response_type, uint32_t argument, void *response_buffer)
{
    // If we don't expect a response, don't check; otherwise check everything.
    enum sdmmc_response_checks checks = (response_type == MMC_RESPONSE_NONE) ? MMC_CHECKS_NONE : MMC_CHECKS_ALL;

    // Deletegate the full checks function.
    return sdmmc_send_command(mmc, command, response_type, checks, argument, response_buffer, 0, false, false, NULL);
}


/**
 * Sends an SDMMC application command.
 *
 * @param mmc The SDMMC device to be used to transmit the command.
 * @param response_type The type of response to expect-- mostly specifies the length.
 * @param checks Determines which sanity checks the host controller should run.
 * @param argument The argument to the SDMMC command.
 * @param response_buffer A buffer to store the response. Should be at uint32_t for a LEN48 command,
 *      or 16 bytes for a LEN136 command.
 *
 * @returns 0 on success, an error number on failure
 */
static int sdmmc_send_app_command(struct mmc *mmc, enum sdmmc_command command,
        enum sdmmc_response_type response_type, enum sdmmc_response_checks checks,
        uint32_t argument, void *response_buffer, uint16_t blocks_to_transfer,
        bool auto_terminate, void *data_buffer)
{
    int rc;

    // First, send the application command.
    rc = sdmmc_send_simple_command(mmc, CMD_APP_COMMAND, MMC_RESPONSE_LEN48, mmc->relative_address << 16, NULL);
    if (rc) {
        mmc_print(mmc, "failed to prepare application command %s! (%d)", sdmmc_get_command_string(command), rc);
        return rc;
    }

    // And issue the body of the command.
    return sdmmc_send_command(mmc, command, response_type, checks, argument, response_buffer,
            blocks_to_transfer, false, auto_terminate, data_buffer);
}


/**
 * Sends an SDMMC application command.
 *
 * @param mmc The SDMMC device to be used to transmit the command.
 * @param response_type The type of response to expect-- mostly specifies the length.
 * @param checks Determines which sanity checks the host controller should run.
 * @param argument The argument to the SDMMC command.
 * @param response_buffer A buffer to store the response. Should be at uint32_t for a LEN48 command,
 *      or 16 bytes for a LEN136 command.
 *
 * @returns 0 on success, an error number on failure
 */
static int sdmmc_send_simple_app_command(struct mmc *mmc, enum sdmmc_command command,
        enum sdmmc_response_type response_type, enum sdmmc_response_checks checks,
        uint32_t argument, void *response_buffer)
{
    // Deletegate to the full app command function.
    return sdmmc_send_app_command(mmc, command, response_type, checks, argument, response_buffer, 0, false, NULL);
}


/**
 * Reads a collection of bits from the CSD register.
 *
 * @param csd An array of four uint32_ts containing the CSD.
 * @param start The bit number to start at.
 * @param width. The width of the relveant read.
 *
 * @returns the extracted bits
 */
static uint32_t sdmmc_extract_csd_bits(uint32_t *csd, int start, int width)
{
    uint32_t relevant_dword, result;
    int offset_into_dword, bits_into_next_dword;

    // Sanity check our span.
    if ((start + width) > 128) {
        printk("MMC ERROR: invalid CSD slice!\n");
        return 0xFFFFFFFF;
    }

    // Figure out where the relevant range is in our CSD.
    relevant_dword = csd[start / 32];
    offset_into_dword = start % 32;

    // Grab all the bits we can from the relevant DWORD.
    result = relevant_dword >> offset_into_dword;

    // Special case: if we spanned a word boundary, we'll
    // need to read one word.
    //
    // FIXME: I'm writing this at 5AM, and this requires basic arithemtic,
    // my greatest weakness. This is going to be stupid wrong.
    if (offset_into_dword + width > 32) {
        bits_into_next_dword = (offset_into_dword + width) - 32;

        // Grab the next dword in the CSD...
        relevant_dword = csd[(start / 32) + 1];

        // ... mask away the bits higher than the bits we want...
        relevant_dword &= (1 << (bits_into_next_dword)) - 1;

        // .. and shift the relevant bits up to their position.
        relevant_dword <<= (width - bits_into_next_dword);

        // Finally, combine in the new word.
        result |= relevant_dword;
    }

    return result;
}


/**
 * Parses a fetched CSD per the Version 1 standard.
 *
 * @param mmc The MMC structure to be populated.
 * @param csd A four-dword array containing the read CSD.
 *
 * @returns int 0 on success, or an error code if the CSD appears invalid
 */
static int sdmmc_parse_csd_version1(struct mmc *mmc, uint32_t *csd)
{
    // Get the maximum allowed read-block size.
    mmc->read_block_order = sdmmc_extract_csd_bits(csd, MMC_CSD_V1_READ_BL_LENGTH_START, MMC_CSD_V1_READ_BL_LENGTH_WIDTH);

    // TODO: handle other attributes
    return 0;
}


/**
 * Decides on a block transfer sized based on the information observed,
 * and applies it to the card.
 *
 * @param mmc The controller to use to set the order
 * @param block_order The order (log-base-2) of the block size to be used.
 */
static int sdmmc_use_block_size(struct mmc *mmc, int block_order)
{
    int rc;

    // Inform the card of the block size we'll want to use.
    rc = sdmmc_send_simple_command(mmc, CMD_SET_BLKLEN, MMC_RESPONSE_LEN48, 1 << block_order, NULL);
    if (rc) {
        mmc_print(mmc, "could not fetch the CID");
        return ENODEV;
    }

    // On success, store the relevant block size.
    mmc->read_block_order = block_order;
    mmc->write_block_order = block_order;
    return 0;
}


/**
 * Reads the active SD card's SD Configuration Register, and updates the object's properties.
 *
 * @param mmc The controller with which to query and to update.
 * @returns 0 on success, or an errno on failure
 */
static int sdmmc_read_and_parse_scr(struct mmc *mmc)
{
    int rc;
    struct sdmmc_scr scr;

    // Read the current block order, so we can restore it.
    int original_block_order = sdmmc_get_block_order(mmc, false);

    // Always request a single 8-byte block.
    const int block_order = 3;
    const int num_blocks = 1;

    // Momentarily step down to a smaller block size, so we don't
    // have to allocate a huge buffer for this command.
    mmc->read_block_order = block_order;

    // Request the CSD from the device.
    rc = sdmmc_send_app_command(mmc, CMD_APP_SEND_SCR, MMC_RESPONSE_LEN48, MMC_CHECKS_ALL, 0, NULL, num_blocks, false, &scr);
    if (rc) {
        mmc_print(mmc, "could not get the card's SCR!");
        mmc->read_block_order = original_block_order;
        return rc;
    }

    // Store the SCR data.
    mmc->spec_version = scr.spec_version;

    // Restore the original block order.
    mmc->read_block_order = original_block_order;

    return 0;
}


/**
 * Reads the active MMC card's Card Specific Data, and updates the MMC object's properties.
 *
 * @param mmc The MMC to be queired and updated.
 * @returns 0 on success, or an errno on failure
 */
static int sdmmc_read_and_parse_csd(struct mmc *mmc)
{
    int rc;
    uint32_t csd[4];
    uint16_t csd_version;

    // Request the CSD from the device.
    rc = sdmmc_send_simple_command(mmc, CMD_SEND_CSD, MMC_RESPONSE_LEN136, mmc->relative_address << 16, csd);
    if (rc) {
        mmc_print(mmc, "could not get the card's CSD!");
        return ENODEV;
    }

    // Figure out the CSD version.
    csd_version = sdmmc_extract_csd_bits(csd, MMC_CSD_STRUCTURE_START, MMC_CSD_STRUCTURE_WIDTH);

    // Handle each CSD version.
    switch (csd_version) {

        // Handle version 1 CSDs.
        // (The Switch eMMC appears to always use ver1 CSDs.)
        case MMC_CSD_VERSION1:
            return sdmmc_parse_csd_version1(mmc, csd);

        // For now, don't support any others.
        default:
            mmc_print(mmc, "ERROR: we don't currently support cards with v%d CSDs!", csd_version);
            return ENOTTY;
    }
}


/**
 * Reads the active MMC card's Card Specific Data, and updates the MMC object's properties.
 *
 * @param mmc The MMC to be queired and updated.
 * @returns 0 on success, or an errno on failure
 */
static int sdmmc_read_and_parse_ext_csd(struct mmc *mmc)
{
    int rc;
    uint8_t ext_csd[MMC_EXT_CSD_SIZE];

    // Read the single EXT CSD block.
    rc = sdmmc_send_command(mmc, CMD_SEND_EXT_CSD, MMC_RESPONSE_LEN48,
            MMC_CHECKS_ALL, 0, NULL, 1, false, false, ext_csd);
    if (rc) {
        mmc_print(mmc, "ERROR: failed to read the extended CSD!");
        return rc;
    }

    /**
     * Parse the extended CSD:
     */

    // Hardware partition support.
    mmc->partition_support     = ext_csd[MMC_EXT_CSD_PARTITION_SUPPORT];
    mmc->partition_config      = ext_csd[MMC_EXT_CSD_PARTITION_CONFIG] & ~MMC_EXT_CSD_PARTITION_SELECT_MASK;
    mmc->partition_switch_time = ext_csd[MMC_EXT_CSD_PARTITION_SWITCH_TIME] * MMC_EXT_CSD_PARTITION_SWITCH_SCALE_US;
    mmc->partitioned           = ext_csd[MMC_EXT_CSD_PARTITION_SETTING_COMPLETE] & MMC_EXT_CSD_PARTITION_SETTING_COMPLETED;
    mmc->partition_attribute   = ext_csd[MMC_EXT_CSD_PARTITION_ATTRIBUTE];

    // Speed support.
    mmc->mmc_card_type         = ext_csd[MMC_EXT_CSD_CARD_TYPE];

    return 0;
}


/**
 * Switches the SDMMC card and controller to the fullest bus width possible.
 *
 * @param mmc The MMC controller to switch up to a full bus width.
 */
static int sdmmc_mmc_switch_bus_width(struct mmc *mmc, enum sdmmc_bus_width width)
{
    // Ask the card to adjust to the wider bus width.
    int rc = mmc->switch_mode(mmc, MMC_SWITCH_EXTCSD_NORMAL,
            MMC_BUS_WIDTH, width, mmc->timeout, NULL);
    if (rc) {
        mmc_print(mmc, "could not switch mode on the card side!");
        return rc;
    }

    // Apply the same changes on the host side.
    mmc->regs->host_control &= ~MMC_HOST_BUS_WIDTH_MASK;
    switch (width) {
        case MMC_BUS_WIDTH_4BIT:
            mmc->regs->host_control |= MMC_HOST_BUS_WIDTH_4BIT;
            break;
        case MMC_BUS_WIDTH_8BIT:
            mmc->regs->host_control |= MMC_HOST_BUS_WIDTH_8BIT;
            break;
        default:
            break;
    }
    return 0;
}


/**
 * Switches the SDMMC card and controller to the fullest bus width possible.
 *
 * @param mmc The MMC controller to switch up to a full bus width.
 */
static int sdmmc_sd_switch_bus_width(struct mmc *mmc, enum sdmmc_bus_width width)
{
    // By default, SD DAT3 is used for card detect. We'll need to
    // release it from this function by dropping its pull-up resistor
    // before we can use the line for data. Do so.
    int rc = sdmmc_send_simple_app_command(mmc, CMD_APP_SET_CARD_DETECT,
            MMC_RESPONSE_LEN48, MMC_CHECKS_ALL, 0, NULL);
    if (rc) {
        mmc_print(mmc, "could not stop using DAT3 as a card detect!");
        return rc;
    }

    // Ask the card to adjust to the wider bus width.
    rc = sdmmc_send_simple_app_command(mmc, CMD_APP_SWITCH_WIDTH,
            MMC_RESPONSE_LEN48, MMC_CHECKS_ALL, width, NULL);
    if (rc) {
        mmc_print(mmc, "could not switch mode on the card side!");
        return rc;
    }

    // Apply the same changes on the host side.
    mmc->regs->host_control &= ~MMC_HOST_BUS_WIDTH_MASK;
    if (mmc->max_bus_width == SD_BUS_WIDTH_4BIT) {
        mmc->regs->host_control |= MMC_HOST_BUS_WIDTH_4BIT;
    }

    return 0;
}


/**
 * Optimize our SDMMC transfer mode to fully utilize the bus.
 */
static int sdmmc_optimize_transfer_mode(struct mmc *mmc)
{
    int rc;

    // Switch the device to its maximum bus width.
    rc = mmc->switch_bus_width(mmc, mmc->max_bus_width);
    if (rc) {
        mmc_print(mmc, "could not switch the controller's bus width!");
        return rc;
    }

    // Automatically optimize speed as much as is possible. How this works depends on
    // the type of card.
    rc = mmc->optimize_speed(mmc);
    if (rc) {
        mmc_print(mmc, "could not optimize the controller's speed!");
        return rc;
    }

    return 0;
}


/**
 * Switches the active card and host controller to the given speed mode.
 *
 * @param mmc The MMC controller to affect.
 * @param speed The speed to switch to.
 * @param status_out The status result of the relevant switch operation.
 */
static int sdmmc_switch_bus_speed(struct mmc *mmc, enum sdmmc_bus_speed speed)
{
    int rc;

    // Ask the card to switch to the new speed.
    rc = mmc->card_switch_bus_speed(mmc, speed);
    if (rc) {
        mmc_print(mmc, "WARNING: could not switch card to %s", sdmmc_get_speed_string(speed));
        return rc;
    }

    // Switch the host controller so it talks the relevant speed.
    rc = sdmmc_apply_clock_speed(mmc, speed, true);
    if (rc) {
        mmc_print(mmc, "WARNING: could not switch the host to %s", sdmmc_get_speed_string(speed));

        // Fall back to the original speed before returning..
        sdmmc_apply_clock_speed(mmc, mmc->operating_speed, true);
        return rc;
    }

    mmc_debug(mmc, "now operating at %s!", sdmmc_get_speed_string(speed));
    return 0;
}


/**
 * Switches the active SD card and host controller to the given speed mode.
 *
 * @param mmc The MMC controller to affect.
 * @param speed The speed to switch to.
 * @param status_out The status result of the relevant switch operation.
 */
static int sdmmc_sd_switch_bus_speed(struct mmc *mmc, enum sdmmc_bus_speed speed)
{
    struct sdmmc_function_status status_out;
    int rc;

    // Read the supported functions from the card.
    rc = mmc->switch_mode(mmc, SDMMC_SWITCH_MODE_MODE_APPLY, SDMMC_SWITCH_MODE_ACCESS_MODE, speed, 0, &status_out);
    if (rc) {
        mmc_print(mmc, "WARNING: could not issue the bus speed switch");
        return rc;
    }

    // Check that the active operating mode has switched to the new mode.
    if (status_out.active_access_mode != speed) {
        mmc_print(mmc, "WARNING: device did not accept mode %s!", sdmmc_get_speed_string(speed));
        mmc_print(mmc, "         reported mode is %s / %d", sdmmc_get_speed_string(status_out.active_access_mode), status_out.active_access_mode);
        return EINVAL;
    }

    return 0;
}


/**
 * Switches the active MMC card and host controller to the given speed mode.
 *
 * @param mmc The MMC controller to affect.
 * @param speed The speed to switch to.
 * @param status_out The status result of the relevant switch operation.
 */
static int sdmmc_mmc_switch_bus_speed(struct mmc *mmc, enum sdmmc_bus_speed speed)
{
    int rc;
    uint8_t ext_csd[MMC_EXT_CSD_SIZE];

    // To disambiguate constants, we add ten to every MMC speed constant.
    // we have to undo this before sending the constants out to the card.
    uint32_t argument = speed - MMC_SPEED_MMC_OFFSET;

    // Read the supported functions from the card.
    rc = mmc->switch_mode(mmc, MMC_SWITCH_MODE_WRITE_BYTE, MMC_HS_TIMING, argument, 0, NULL);
    if (rc) {
        mmc_print(mmc, "WARNING: could not issue the mode switch");
        return rc;
    }

    // Read the single EXT-CSD block, which contains the current speed.
    rc = sdmmc_send_command(mmc, CMD_SEND_EXT_CSD, MMC_RESPONSE_LEN48,
            MMC_CHECKS_ALL, 0, NULL, 1, false, false, ext_csd);
    if (rc) {
        mmc_print(mmc, "ERROR: failed to read the extended CSD after mode-switch!");
        return rc;
    }

    // Check the ext-csd to make sure the change took.
    if (ext_csd[MMC_EXT_CSD_HS_TIMING] != argument) {
        mmc_print(mmc, "WARNING: device did not accept mode %s!", sdmmc_get_speed_string(speed));
        mmc_print(mmc, "         reported mode is %s / %d", sdmmc_get_speed_string(ext_csd[MMC_EXT_CSD_HS_TIMING]), ext_csd[MMC_EXT_CSD_HS_TIMING]);
        return EINVAL;
    }

    return 0;
}


/**
 * Optimize our SDMMC transfer speed by maxing out the bus as much as is possible.
 */
static int sdmmc_sd_optimize_speed(struct mmc *mmc)
{
    int rc;
    struct sdmmc_function_status function_status;

    // We started off with the controller opearting with a divided clock,
    // which is meant to keep us within the pre-init operating frequency of 400kHz.
    // We're now set up, so we can drop the divider. From this point forward, the
    // clock is now driven by the CAR frequency.
    rc = sdmmc_apply_clock_speed(mmc, SDMMC_SPEED_SDR12, true);
    if (rc) {
        mmc_print(mmc, "WARNING: could not step up to normal operating speeds!");
        mmc_print(mmc, "         ... expect delays.");
        return rc;
    }
    mmc_debug(mmc, "now operating at 12.5MB/s!");

    // If we're not at a full bus width, we can't switch to a higher speed.
    // We're already optimal, vacuously succeed.
    if (mmc->max_bus_width != SD_BUS_WIDTH_4BIT)
        return 0;

    // Read the supported functions from the card.
    rc = mmc->switch_mode(mmc, SDMMC_SWITCH_MODE_MODE_QUERY, SDMMC_SWITCH_MODE_NO_GROUP, 0, 0, &function_status);
    if (rc) {
        mmc_print(mmc, "WARNING: could not query card mode status; speed will be limited");
        return rc;
    }

    // Debug information for very vebose modes.
    mmc_debug(mmc, "this card supports the following speed modes:");
    mmc_debug(mmc, "SDR12: %d SDR25: %d SDR50: %d SDR104: %d DDR50: %d\n",
            function_status.sdr12_support, function_status.sdr25_support,
            function_status.sdr50_support, function_status.sdr104_support,
            function_status.ddr50_support);

    // If we're operating in a UHS-compatbile low-voltage mode, check for UHS-modes.
    if (mmc->operating_voltage == MMC_VOLTAGE_1V8) {

        // Try each of the UHS-I modes, we support.
        if (function_status.sdr104_support && !sdmmc_switch_bus_speed(mmc, SDMMC_SPEED_SDR104))
            return 0;
        if (function_status.sdr50_support && !sdmmc_switch_bus_speed(mmc, SDMMC_SPEED_SDR50))
            return 0;
    }

    // If we support High Speed but not a UHS-I mode, use it.
    if (function_status.sdr25_support)
        return sdmmc_switch_bus_speed(mmc, SDMMC_SPEED_SDR25);

    mmc_debug(mmc, "couldn't improve speed above the speed already set.");
    return 0;
}


/**
 * Optimize our MMC transfer speed by maxing out the bus as much as is possible.
 */
static int sdmmc_mmc_optimize_speed(struct mmc *mmc)
{
    int rc;
    bool hs52_support, hs200_support, hs400_support;

    // XXX
    sdmmc_set_loglevel(3);

    // We started off with the controller opearting with a divided clock,
    // which is meant to keep us within the pre-init operating frequency of 400kHz.
    // We're now set up, so we can drop the divider. From this point forward, the
    // clock is now driven by the CAR frequency.
    rc = sdmmc_apply_clock_speed(mmc, SDMMC_SPEED_SDR12, true);
    if (rc) {
        mmc_print(mmc, "WARNING: could not step up to normal operating speeds!");
        mmc_print(mmc, "         ... expect delays.");
        return rc;
    }
    mmc_debug(mmc, "now operating at 25MB/s!");

    // Determine which high-speed modes are supported, for easy reference below.
    hs200_support = mmc->mmc_card_type & MMC_EXT_CSD_CARD_TYPE_HS400_1V8;
    hs400_support = mmc->mmc_card_type & MMC_EXT_CSD_CARD_TYPE_HS400_1V8;
    hs52_support  = mmc->mmc_card_type & MMC_EXT_CSD_CARD_TYPE_HS52;

    // First, try modes that are only supported at 1V8 and below,
    // if we can.
    if (mmc->operating_voltage == MMC_VOLTAGE_1V8) {
        //if (hs400_support && !sdmmc_switch_bus_speed(mmc, SDMMC_SPEED_HS400))
        //    return 0;
        if (hs200_support && !sdmmc_switch_bus_speed(mmc, SDMMC_SPEED_HS200))
            return 0;

        (void)hs400_support;
    }

    // Next, try the legacy "high speed" mode.
    if (hs52_support)
        return sdmmc_switch_bus_speed(mmc, SDMMC_SPEED_HS52);

    mmc_debug(mmc, "couldn't improve speed above the default");
    return 0;
}


/**
 * Requests that an MMC target use the card's current relative address.
 *
 * @param mmc The SDMMC controller to work with.
 * @return 0 on success, or an error code on failure.
 */
static int sdmmc_set_relative_address(struct mmc *mmc)
{
    int rc;

    // Set up the card's relative address.
    rc = sdmmc_send_simple_command(mmc, CMD_SET_RELATIVE_ADDR, MMC_RESPONSE_LEN48, mmc->relative_address << 16, NULL);
    if (rc) {
        mmc_print(mmc, "could not set the card's relative address! (%d)", rc);
        return rc;
    }

    return 0;
}


/**
 * Requests that an SD target report a relative address for us to use
 * to communicate with it.
 *
 * @param mmc The SDMMC controller to work with.
 * @return 0 on success, or an error code on failure.
 */
static int sdmmc_get_relative_address(struct mmc *mmc)
{
    int rc;
    uint32_t response;

    // Set up the card's relative address.
    rc = sdmmc_send_simple_command(mmc, CMD_GET_RELATIVE_ADDR, MMC_RESPONSE_LEN48, 0, &response);
    if (rc) {
        mmc_print(mmc, "could not get the card's relative address! (%d)", rc);
        return rc;
    }

    // Apply the fetched relative address.
    mmc->relative_address = response >> 16;
    return 0;
}


/**
 * Shared card initialization for SD and MMC cards.
 * Used to bring the card fully online and gather information about the card.
 *
 * @param mmc The MMC controller that will perform the initilaization.
 */
static int sdmmc_card_init(struct mmc *mmc)
{
    int rc;
    uint32_t response[4];

    // Retreive the card ID.
    rc = sdmmc_send_simple_command(mmc, CMD_ALL_SEND_CID, MMC_RESPONSE_LEN136, 0, response);
    if (rc) {
        mmc_print(mmc, "could not fetch the CID");
        return rc;
    }

    // Store the card ID for later.
    memcpy(mmc->cid, response, sizeof(mmc->cid));

    // Establish a relative address to communicate with
    rc = mmc->establish_relative_address(mmc);
    if (rc) {
        mmc_print(mmc, "could not establish a relative address! (%d)", rc);
        return rc;
    }

    // Read and handle card's Card Specific Data (CSD).
    rc = sdmmc_read_and_parse_csd(mmc);
    if (rc) {
        mmc_print(mmc, "could not populate CSD attributes! (%d)", rc);
        return rc;
    }

    // Select our eMMC card, so it knows we're talking to it.
    rc = sdmmc_send_simple_command(mmc, CMD_TOGGLE_CARD_SELECT, MMC_RESPONSE_LEN48, mmc->relative_address << 16, response);
    if (rc) {
        mmc_print(mmc, "could not select the active card for use! (%d)", rc);
        return rc;
    }

    // Set up a block transfer size of 512B blocks.
    // 1) every card supports this, and 2) we use SDMA, which only supports up to 512B
    rc = sdmmc_use_block_size(mmc, MMC_DEFAULT_BLOCK_ORDER);
    if (rc) {
        mmc_print(mmc, "could not set up block transfer sizes! (%d)", rc);
        return rc;
    }

    return 0;
}


/**
 * Blocks until the eMMC card is fully initialized.
 *
 * @param mmc The MMC device that should do the waiting.
 */
static int sdmmc_mmc_wait_for_card_readiness(struct mmc *mmc)
{
    int rc;
    uint32_t response[4];


    while (true) {

        uint32_t response_masked;

        // Ask the SD card to identify its state. It will respond with readiness and a capacity magic.
        int original_loglevel = sdmmc_set_loglevel(0);
        rc = sdmmc_send_command(mmc, CMD_SEND_OPERATING_CONDITIONS, MMC_RESPONSE_LEN48,
                MMC_CHECKS_NONE, 0x40000080, response, 0, false, false, NULL);
        sdmmc_set_loglevel(original_loglevel);

        if (rc) {
            mmc_print(mmc, "ERROR: could not read the card's operating conditions!");
            return rc;
        }

        // Validate that this is a valid Switch eMMC.
        // Per the spec, any card greater than 2GiB should respond with this magic number.
        response_masked = response[0] & MMC_EMMC_OPERATING_COND_CAPACITY_MASK;
        if (response_masked != MMC_EMMC_OPERATING_COND_CAPACITY_MAGIC) {
            mmc_print(mmc, "ERROR: this doesn't appear to be a valid Switch eMMC!");
            return ENOTTY;
        }

        // If the device has just become ready, we're done!
        response_masked = response[0] & MMC_EMMC_OPERATING_READINESS_MASK;
        if (response_masked == MMC_EMMC_OPERATING_COND_READY) {
            return 0;
        }
    }
}


/**
 * Blocks until the SD card is fully initialized.
 *
 * @param mmc The MMC device that should do the waiting.
 * @aparam response Out argument that recieves the final, ready command response.
 *      Should have roon for uint32_t.
 */
static int sdmmc_sd_wait_for_card_readiness(struct mmc *mmc, uint32_t *response)
{
    int rc;
    uint32_t argument = MMC_SD_OPERATING_COND_ACCEPTS_3V3;

    // If this is a SDv2 or higher card, check for an SDHC card,
    // and for low-voltage support.
    if (mmc->spec_version >= SD_VERSION_2_0) {
        argument |= MMC_SD_OPERATING_COND_HIGH_CAPACITY;
        argument |= MMC_SD_OPERATING_COND_ACCEPTS_1V8;
    }

    while (true) {

        // Ask the SD card to identify its state.
        int original_loglevel = sdmmc_set_loglevel(0);
        rc = sdmmc_send_simple_app_command(mmc, CMD_APP_SEND_OP_COND,
                MMC_RESPONSE_LEN48, MMC_CHECKS_NONE, argument, response);
        sdmmc_set_loglevel(original_loglevel);
        if (rc) {
            mmc_print(mmc, "ERROR: could not read the card's operating conditions!");
            return rc;
        }

        // If the device has just become ready, we're done!
        if (response[0] & MMC_SD_OPERATING_COND_READY)
            return 0;

        // Wait a delay so we're not spamming the card incessantly.
        udelay(1000);
    }
}


/**
 * Handles MMC-specific card initialization.
 */
static int sdmmc_mmc_card_init(struct mmc *mmc)
{
    int rc;

    mmc_debug(mmc, "setting up card as MMC");

    // Bring the bus out of its idle state.
    rc = sdmmc_send_simple_command(mmc, CMD_GO_IDLE_OR_INIT, MMC_RESPONSE_NONE, 0, NULL);
    if (rc) {
        mmc_print(mmc, "could not bring bus to idle!");
        return rc;
    }

    // Wait for the card to finish being busy.
    rc = sdmmc_mmc_wait_for_card_readiness(mmc);
    if (rc) {
        mmc_print(mmc, "card failed to come up! (%d)", rc);
        return rc;
    }

    // Run the common core card initialization.
    rc = sdmmc_card_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set up card (%d)!", rc);
        return rc;
    }

    // Read and handle card's Extended Card Specific Data (ext-CSD).
    rc = sdmmc_read_and_parse_ext_csd(mmc);
    if (rc) {
        mmc_print(mmc, "could not populate extended-CSD attributes! (%d)", rc);
        return EPIPE;
    }

    return 0;
}


/**
 * Evalutes a check pattern response (used with interface commands)
 * and validates that it contains our common check pattern.
 *
 * @param response The response recieved after a given command.
 * @return True iff the given response has a valid check pattern.
 */
static bool sdmmc_check_pattern_present(uint32_t response)
{
    uint32_t pattern_byte = response & 0xFF;
    return pattern_byte == MMC_IF_CHECK_PATTERN;
}


/**
 * Handles SD-specific card initialization.
 */
static int sdmmc_sd_card_init(struct mmc *mmc)
{
    int rc;
    uint32_t ocr, response;

    mmc_debug(mmc, "setting up card as SD");

    // Bring the bus out of its idle state.
    rc = sdmmc_send_simple_command(mmc, CMD_GO_IDLE_OR_INIT, MMC_RESPONSE_NONE, 0, NULL);
    if (rc) {
        mmc_print(mmc, "could not bring bus to idle!");
        return rc;
    }

    // Validate that the card can handle working with the voltages we can provide.
    rc = sdmmc_send_simple_command(mmc, CMD_SEND_IF_COND, MMC_RESPONSE_LEN48, MMC_IF_VOLTAGE_3V3 | MMC_IF_CHECK_PATTERN, &response);
    if (rc || !sdmmc_check_pattern_present(response)) {

        // TODO: This is either a broken, SDv1 or MMC card.
        // Handle the latter two cases as best we can.

        mmc_print(mmc, "ERROR: this card isn't an SDHC card!");
        mmc_print(mmc, "       we don't yet support low-capacity cards. :(");
        return rc;
    }
    // If this responded, indicate that this is a v2 card.
    else {
        // store that this is a v2 card
        mmc->spec_version = SD_VERSION_2_0;
    }

    // Wait for the card to finish being busy.
    rc = sdmmc_sd_wait_for_card_readiness(mmc, &ocr);
    if (rc) {
        mmc_print(mmc, "card failed to come up! (%d)", rc);
        return rc;
    }

    // If the response indicated this was a high capacity card,
    // always use block addressing.
    mmc->uses_block_addressing = !!(ocr & MMC_SD_OPERATING_COND_HIGH_CAPACITY);

    // If the card supports using 1V8, drop down using lower voltages.
    if (mmc->allow_voltage_switching && ocr & MMC_SD_OPERATING_COND_ACCEPTS_1V8) {
        if (mmc->operating_voltage != MMC_VOLTAGE_1V8) {
            rc = mmc->switch_to_low_voltage(mmc);
            if (rc)
                mmc_print(mmc, "WARNING: could not switch to low-voltage mode! (%d)", rc);
        }
    }

    // Run the common core card initialization.
    rc = sdmmc_card_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set up card (%d)!", rc);
        return rc;
    }

    // Read the card's SCR.
    rc = sdmmc_read_and_parse_scr(mmc);
    if (rc) {
        mmc_print(mmc, "failed to read SCR! (%d)!", rc);
        return rc;
    }

    return 0;
}


/**
 * @returns true iff the given READ_STATUS response indicates readiness
 */
static bool sdmmc_status_indicates_readiness(uint32_t status)
{
    // If the card is currently programming, it's not ready.
    if ((status & MMC_STATUS_MASK) == MMC_STATUS_PROGRAMMING)
        return false;

    // Return true iff the card is ready for data.
    return status & MMC_STATUS_READY_FOR_DATA;
}


/**
 * Waits for card readiness; should be issued after e.g. enabling partitioning.
 *
 * @param mmc The MMC to wait on.
 * @param 0 if the wait completed with the card being ready; or an error code otherwise
 */
static int sdmmc_wait_for_card_ready(struct mmc *mmc, uint32_t timeout)
{
    int rc;
    uint32_t status;

    uint32_t timebase = get_time();

    while (true) {
        // Read the card's status.
        rc = sdmmc_send_simple_command(mmc, CMD_READ_STATUS, MMC_RESPONSE_LEN48, mmc->relative_address << 16, &status);

        // Ensure we haven't timed out.
        if (get_time_since(timebase) > timeout)
            return ETIMEDOUT;

        // If we couldn't read, try again.
        if (rc)
            continue;

        // Check to see if we hit a fatal error.
        if (status & MMC_STATUS_CHECK_ERROR)
            return EPIPE;

        // Check for ready status.
        if (sdmmc_status_indicates_readiness(status))
            return 0;
    }
}


/**
 * Issues a SWITCH_MODE command, which can be used to write registers on the MMC card's controller,
 * and thus to e.g. switch partitions.
 *
 * @param mmc The MMC device to use for comms.
 * @param mode The access mode with which to access the controller.
 * @param field The field to access.
 * @param value The argument to the access mode.
 * @param timeout The timeout, which is often longer than the normal MMC timeout.
 *
 * @return 0 on success, or an error code on failure
 */
static int sdmmc_mmc_switch_mode(struct mmc *mmc, int mode, int field, int value, uint32_t timeout, void *unused)
{
    // Collapse our various parameters into a single argument.
    uint32_t argument =
        (mode  << MMC_SWITCH_ACCESS_MODE_SHIFT) |
        (field << MMC_SWITCH_FIELD_SHIFT)       |
        (value << MMC_SWITCH_VALUE_SHIFT);

    // Issue the switch mode command.
    int rc = sdmmc_send_simple_command(mmc, CMD_SWITCH_MODE, MMC_RESPONSE_LEN48_CHK_BUSY, argument, NULL);
    if (rc){
        mmc_print(mmc, "failed to issue SWITCH_MODE command! (%d / %d / %d; rc=%d)", mode, field, value, rc);
        return rc;
    }

    // Wait until we have a sense of the card status to return.
    if (timeout != 0) {
        rc = sdmmc_wait_for_card_ready(mmc, timeout);
        if (rc){
            mmc_print(mmc, "failed to talk to the card after SWITCH_MODE (%d)", rc);
            return rc;
        }
    }

    return 0;
}


/**
 * @return True iff the given MMC card supports hardare partitions.
 */
static bool sdmmc_supports_hardware_partitions(struct mmc *mmc)
{
    return mmc->partition_support & MMC_SUPPORTS_HARDWARE_PARTS;
}


/**
 * card detect method for built-in cards.
 */
bool sdmmc_builtin_card_present(struct mmc *mmc)
{
    return true;
}

/**
 * card detect method for GPIO-based card detects
 */
bool sdmmc_external_card_present(struct mmc *mmc)
{
    return !gpio_read(mmc->card_detect_gpio);
}

/**
 * Issues an SD card mode-switch command.
 *
 * @param mmc The controller to use.
 * @param mode The mode flag -- one to set function data, zero to query.
 * @param group The SD card function group-- see the SD card Physical Layer spec. Set this negative to not apply arguments.
 * @param response 64-byte buffer to store the response.
 */
static int sdmmc_sd_switch_mode(struct mmc *mmc, int mode, int group, int value, uint32_t timeout, void *response)
{
    int rc;

    // Read the current block order, so we can restore it.
    int original_block_order = sdmmc_get_block_order(mmc, false);

    // Always request a single 64-byte block.
    const int block_order = 6;
    const int num_blocks = 1;

    // Build the argument we're going to issue.
    uint32_t argument = (mode << SDMMC_SWITCH_MODE_MODE_SHIFT) | SDMMC_SWITCH_MODE_ALL_FUNCTIONS_UNUSED;

    // If we have a group argument, apply the group and value.
    if(group >= 0) {
        const int group_shift = group * SDMMC_SWITCH_MODE_GROUP_SIZE_BITS;
        argument &= ~(SDMMC_SWITCH_MODE_FUNCTION_MASK << group_shift);
        argument |= value << group_shift;
    }

    // Momentarily step down to a smaller block size, so we don't
    // have to allocate a huge buffer for this command.
    mmc->read_block_order = block_order;

    // Issue the command itself.
    rc = sdmmc_send_command(mmc, CMD_SWITCH_MODE, MMC_RESPONSE_LEN48, MMC_CHECKS_ALL, argument, NULL, num_blocks, false, false, response);
    if (rc) {
        mmc_print(mmc, "could not issue switch command!");
        mmc->read_block_order = original_block_order;
        return rc;
    }

    // Restore the original block order.
    mmc->read_block_order = original_block_order;

    return 0;
}


/**
 * Switches a given SDMMC Controller where
 */
static void sdmmc_apply_card_type(struct mmc *mmc, enum sdmmc_card_type type)
{
    // Store the card type for our own reference...
    mmc->card_type = type;

    // Set up our per-protocol function pointers.
    switch (type) {

        // MMC-protoco cards
        case MMC_CARD_EMMC:
        case MMC_CARD_MMC:
            mmc->card_init = sdmmc_mmc_card_init;
            mmc->establish_relative_address = sdmmc_set_relative_address;
            mmc->switch_mode = sdmmc_mmc_switch_mode;
            mmc->switch_bus_width = sdmmc_mmc_switch_bus_width;
            mmc->card_switch_bus_speed = sdmmc_mmc_switch_bus_speed;
            mmc->optimize_speed = sdmmc_mmc_optimize_speed;

            break;

        // SD-protocol cards
        case MMC_CARD_SD:
            mmc->card_init = sdmmc_sd_card_init;
            mmc->establish_relative_address = sdmmc_get_relative_address;
            mmc->switch_mode = sdmmc_sd_switch_mode;
            mmc->switch_bus_width = sdmmc_sd_switch_bus_width;
            mmc->optimize_speed = sdmmc_sd_optimize_speed;
            mmc->card_switch_bus_speed = sdmmc_sd_switch_bus_speed;
            break;

        // Switch-cart protocol cards
        case MMC_CARD_CART:
            printk("BUG: trying to use an impossible code path, hanging!\n");
            while(true);
    }
}


/**
 * Populates the given MMC object with defaults for its controller.
 *
 * @param mmc The mmc object to populate.
 */
static int sdmmc_initialize_defaults(struct mmc *mmc)
{
    // Set up based on the controller
    switch (mmc->controller) {
        case SWITCH_EMMC:
            mmc->name = "eMMC";
            mmc->max_bus_width = MMC_BUS_WIDTH_8BIT;
            mmc->tuning_block_order = MMC_TUNING_BLOCK_ORDER_8BIT;
            mmc->operating_voltage = MMC_VOLTAGE_1V8;

            // Set up function pointers for each of our per-instance functions.
            mmc->set_up_clock_and_io = sdmmc4_set_up_clock_and_io;
            mmc->enable_supplies = sdmmc4_enable_supplies;
            mmc->switch_to_low_voltage = sdmmc_always_fail;
            mmc->card_present = sdmmc_builtin_card_present;
            mmc->configure_clock = sdmmc4_configure_clock;

            // The EMMC controller always uses an EMMC card.
            sdmmc_apply_card_type(mmc, MMC_CARD_EMMC);

            // The Switch's eMMC always uses block addressing.
            mmc->uses_block_addressing = true;
            break;

        case SWITCH_MICROSD:
            mmc->name = "uSD";
            mmc->card_type = MMC_CARD_SD;
            mmc->max_bus_width = SD_BUS_WIDTH_4BIT;
            mmc->tuning_block_order = MMC_TUNING_BLOCK_ORDER_4BIT;
            mmc->operating_voltage = MMC_VOLTAGE_3V3;
            mmc->card_detect_gpio = GPIO_MICROSD_CARD_DETECT;

            // Per-instance functions.
            mmc->set_up_clock_and_io = sdmmc1_set_up_clock_and_io;
            mmc->enable_supplies = sdmmc1_enable_supplies;
            mmc->switch_to_low_voltage = sdmmc1_switch_to_low_voltage;
            mmc->card_present = sdmmc_external_card_present;
            mmc->configure_clock = sdmmc1_configure_clock;

            // For the microSD card slot, assume we have an SD-type card.
            // Negotiation has a chance to change this, later.
            sdmmc_apply_card_type(mmc, MMC_CARD_SD);

            // Start off assuming byte addressing; we'll detect and correct this
            // later, if necessary.
            mmc->uses_block_addressing = false;
            break;

        default:
            printk("ERROR: initialization not yet writen for SDMMC%d", mmc->controller + 1);
            return ENOSYS;
    }

    return 0;
}


/**
 * Set up a new SDMMC driver.
 *
 * @param mmc The SDMMC structure to be initiailized with the device state.
 * @param controler The controller description to be used; usually SWITCH_EMMC
 *      or SWITCH_MICROSD.
 * @param allow_voltage_switching True if we should allow voltage switching,
 *      which may not make sense if we're about to chainload to another component without
 *      preseving the overall structure.
 */
int sdmmc_init(struct mmc *mmc, enum sdmmc_controller controller, bool allow_voltage_switching)
{
    int rc;

    // Get a reference to the registers for the relevant SDMMC controller.
    mmc->controller = controller;
    mmc->regs = sdmmc_get_regs(controller);
    mmc->allow_voltage_switching = allow_voltage_switching;

    // Set the defaults for the card, including the default function pointers
    // for the assumed card type, and the per-controller options.
    rc = sdmmc_initialize_defaults(mmc);
    if (rc) {
        printk("ERROR: controller SDMMC%d not currently supported!\n", controller + 1);
        return rc;
    }

    // Default to a timeout of 1S.
    mmc->timeout = 1000000;
    mmc->partition_switch_time = 1000;

    // Use DMA, by default.
    mmc->use_dma = true;

    // Don't allow writing unless the caller explicitly enables it.
    mmc->write_enable = SDMMC_WRITE_DISABLED;

    // Default to relative address of zero.
    mmc->relative_address = 0;

    // Initialize the raw SDMMC controller.
    rc = sdmmc_hardware_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set up controller! (%d)", rc);
        return rc;
    }

    // ... and verify that the card is there.
    if (!mmc->card_present(mmc)) {
        mmc_print(mmc, "ERROR: no card detected!");
        return ENODEV;
    }

    // Handle the initialization that's specific to the card type.
    rc = mmc->card_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set run card-specific initialization (%d)!", rc);
        return rc;
    }

    // Switch to a transfer mode that can more efficiently utilize the bus.
    rc = sdmmc_optimize_transfer_mode(mmc);
    if (rc) {
        mmc_print(mmc, "WARNING: could not optimize bus utlization! (%d)", rc);
    }

    return 0;
}


/**
 * Imports a SDMMC driver struct from another program. This mainly intended for stage2,
 * so that it can reuse stage1's SDMMC struct instance(s).
 *
 * @param mmc The SDMMC structure to be imported.
 */
int sdmmc_import_struct(struct mmc *mmc)
{
    int rc;
    bool uses_block_addressing = mmc->uses_block_addressing;
    mmc->regs = sdmmc_get_regs(mmc->controller);

    rc = sdmmc_initialize_defaults(mmc);
    if (rc) {
        printk("ERROR: controller SDMMC%d not currently supported!\n", mmc->controller + 1);
        return rc;
    }

    mmc->uses_block_addressing = uses_block_addressing;
    return 0;
}


/**
 * Selects the active MMC partition. Can be used to select
 * boot partitions for access. Affects all operations going forward.
 *
 * @param mmc The MMC controller whose card is to be used.
 * @param partition The partition number to be selected.
 *
 * @return 0 on success, or an error code on failure.
 */
int sdmmc_select_partition(struct mmc *mmc, enum sdmmc_partition partition)
{
    uint16_t argument = partition;
    int rc;

    // If we're trying to access hardware partitions on a device that doesn't support them,
    // bail out.
    if (!sdmmc_supports_hardware_partitions(mmc))
        return ENOTTY;

    // Set the PARTITION_CONFIG register to select the active partition.
    mmc_print(mmc, "switching to partition %d", partition);
    rc = mmc->switch_mode(mmc, MMC_SWITCH_MODE_WRITE_BYTE, MMC_PARTITION_CONFIG, argument, 0, NULL);
    if (rc) {
        mmc_print(mmc, "failed to select partition %d (%02x, rc=%d)", partition, argument, rc);
    }

    mmc_print(mmc, "waiting for %d us", mmc->partition_switch_time);
    udelay(mmc->partition_switch_time);

    return rc;
}


/**
 * Reads a sector or sectors from a given SD/MMC card.
 *
 * @param mmc The MMC device to work with.
 * @param buffer The output buffer to target.
 * @param block The sector number to read.
 * @param count The number of sectors to read.
 *
 * @return 0 on success, or an error code on failure.
 */
int sdmmc_read(struct mmc *mmc, void *buffer, uint32_t block, unsigned int count)
{
    uint32_t command = (count == 1) ? CMD_READ_SINGLE_BLOCK : CMD_READ_MULTIPLE_BLOCK;

    // Determine the argument, which indicates which address we're reading/writing.
    uint32_t extent = block;

    // If this card uses byte addressing rather than sector addressing,
    // multiply by the block size.
    if (!mmc->uses_block_addressing) {
        extent *= sdmmc_get_block_size(mmc, false);
    }

    // Execute the relevant read.
    return sdmmc_send_command(mmc, command, MMC_RESPONSE_LEN48, MMC_CHECKS_ALL, extent, NULL, count, false, count > 1, buffer);
}

/**
 * Releases the SDMMC write lockout, enabling access to the card.
 * Note that by default, setting this to WRITE_ENABLED will not allow access to eMMC.
 * Check the source for a third constant that can be used to enable eMMC writes.
 *
 * @param perms The permissions to apply-- typically WRITE_DISABLED or WRITE_ENABLED.
 */
void sdmmc_set_write_enable(struct mmc *mmc, enum sdmmc_write_permission perms)
{
    mmc->write_enable = perms;
}


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
int sdmmc_write(struct mmc *mmc, const void *buffer, uint32_t block, unsigned int count)
{
    // Sanity check variables: we're especially careful about allowing writes to the switch eMMC.
    bool is_emmc = (mmc->controller == SWITCH_EMMC);
    bool allow_mmc_write = (mmc->write_enable == SDMMC_WRITE_ENABLED_INCLUDING_EMMC);

    uint32_t command = (count == 1) ? CMD_WRITE_SINGLE_BLOCK : CMD_WRITE_MULTIPLE_BLOCK;

    // Determine the argument, which indicates which address we're reading/writing.
    uint32_t extent = block;

    // If we don't have an explict write enable, don't allow writes.
    if (mmc->write_enable == SDMMC_WRITE_DISABLED) {
        mmc_print(mmc, "tried to write to an external card, but write was not enabled!");
        return EACCES;
    }

    // Explicitly protect the switch's eMMC to prevent bricks.
    if (is_emmc && !allow_mmc_write) {
        mmc_print(mmc, "cowardly refusing to write to the switch's eMMMC");
        return EACCES;
    }

    // If this card uses byte addressing rather than sector addressing,
    // multiply by the block size.
    if (!mmc->uses_block_addressing) {
        extent *= sdmmc_get_block_size(mmc, true);
    }

    // Execute the relevant read.
    return sdmmc_send_command(mmc, command, MMC_RESPONSE_LEN48, MMC_CHECKS_ALL, extent, NULL, count, true, count > 1, (void *)buffer);
}

/**
 * Checks to see whether an SD card is present.
 *
 * @mmc mmc The controller with which to check for card presence.
 * @return true iff a card is present
 */
bool sdmmc_card_present(struct mmc *mmc)
{
    return mmc->card_present(mmc);
}

/**
 * Prints out all of the tegra_mmc struct fields
 *
 * @mmc mmc The controller with which to dump registers.
 */
void sdmmc_dump_regs(struct mmc *mmc) {
    mmc_debug(mmc, "dma_address: 0x%08" PRIX32 "\n",mmc->regs->dma_address);
    mmc_debug(mmc, "block_size: 0x%04" PRIX16 "\n",mmc->regs->block_size);
    mmc_debug(mmc, "block_count: 0x%04" PRIX16 "\n",mmc->regs->block_count);
    mmc_debug(mmc, "argument: 0x%08" PRIX32 "\n",mmc->regs->argument);
    mmc_debug(mmc, "transfer_mode: 0x%04" PRIX16 "\n",mmc->regs->transfer_mode);
    mmc_debug(mmc, "command: 0x%04" PRIX16 "\n",mmc->regs->command);
    mmc_debug(mmc, "response[0]: 0x%08" PRIX32 "\n",mmc->regs->response[0]);
    mmc_debug(mmc, "response[1]: 0x%08" PRIX32 "\n",mmc->regs->response[1]);
    mmc_debug(mmc, "response[2]: 0x%08" PRIX32 "\n",mmc->regs->response[2]);
    mmc_debug(mmc, "response[3]: 0x%08" PRIX32 "\n",mmc->regs->response[3]);
    mmc_debug(mmc, "buffer: 0x%08" PRIX32 "\n",mmc->regs->buffer);
    mmc_debug(mmc, "present_state: 0x%08" PRIX32 "\n",mmc->regs->present_state);
    mmc_debug(mmc, "host_control: 0x%02" PRIX8 "\n",mmc->regs->host_control);
    mmc_debug(mmc, "power_control: 0x%02" PRIX8 "\n",mmc->regs->power_control);
    mmc_debug(mmc, "block_gap_control: 0x%02" PRIX8 "\n",mmc->regs->block_gap_control);
    mmc_debug(mmc, "wake_up_control: 0x%02" PRIX8 "\n",mmc->regs->wake_up_control);
    mmc_debug(mmc, "clock_control: 0x%04" PRIX16 "\n",mmc->regs->clock_control);
    mmc_debug(mmc, "timeout_control: 0x%02" PRIX8 "\n",mmc->regs->timeout_control);
    mmc_debug(mmc, "software_reset: 0x%02" PRIX8 "\n",mmc->regs->software_reset);
    mmc_debug(mmc, "int_status: 0x%08" PRIX32 "\n",mmc->regs->int_status);
    mmc_debug(mmc, "int_enable: 0x%08" PRIX32 "\n",mmc->regs->int_enable);
    mmc_debug(mmc, "signal_enable: 0x%08" PRIX32 "\n",mmc->regs->signal_enable);
    mmc_debug(mmc, "acmd12_err: 0x%04" PRIX16 "\n",mmc->regs->acmd12_err);
    mmc_debug(mmc, "host_control2: 0x%04" PRIX16 "\n",mmc->regs->host_control2);
    mmc_debug(mmc, "capabilities: 0x%08" PRIX32 "\n",mmc->regs->capabilities);
    mmc_debug(mmc, "capabilities_1: 0x%08" PRIX32 "\n",mmc->regs->capabilities_1);
    mmc_debug(mmc, "max_current: 0x%08" PRIX32 "\n",mmc->regs->max_current);
    mmc_debug(mmc, "set_acmd12_error: 0x%04" PRIX16 "\n",mmc->regs->set_acmd12_error);
    mmc_debug(mmc, "set_int_error: 0x%04" PRIX16 "\n",mmc->regs->set_int_error);
    mmc_debug(mmc, "adma_error: 0x%04" PRIX16 "\n",mmc->regs->adma_error);
    mmc_debug(mmc, "adma_address: 0x%08" PRIX32 "\n",mmc->regs->adma_address);
    mmc_debug(mmc, "upper_adma_address: 0x%08" PRIX32 "\n",mmc->regs->upper_adma_address);
    mmc_debug(mmc, "preset_for_init: 0x%04" PRIX16 "\n",mmc->regs->preset_for_init);
    mmc_debug(mmc, "preset_for_default: 0x%04" PRIX16 "\n",mmc->regs->preset_for_default);
    mmc_debug(mmc, "preset_for_high: 0x%04" PRIX16 "\n",mmc->regs->preset_for_high);
    mmc_debug(mmc, "preset_for_sdr12: 0x%04" PRIX16 "\n",mmc->regs->preset_for_sdr12);
    mmc_debug(mmc, "preset_for_sdr25: 0x%04" PRIX16 "\n",mmc->regs->preset_for_sdr25);
    mmc_debug(mmc, "preset_for_sdr50: 0x%04" PRIX16 "\n",mmc->regs->preset_for_sdr50);
    mmc_debug(mmc, "preset_for_sdr104: 0x%04" PRIX16 "\n",mmc->regs->preset_for_sdr104);
    mmc_debug(mmc, "preset_for_ddr50: 0x%04" PRIX16 "\n",mmc->regs->preset_for_ddr50);
    mmc_debug(mmc, "slot_int_status: 0x%04" PRIX16 "\n",mmc->regs->slot_int_status);
    mmc_debug(mmc, "host_version: 0x%04" PRIX16 "\n",mmc->regs->host_version);
    mmc_debug(mmc, "vendor_clock_cntrl: 0x%08" PRIX32 "\n",mmc->regs->vendor_clock_cntrl);
    mmc_debug(mmc, "vendor_sys_sw_cntrl: 0x%08" PRIX32 "\n",mmc->regs->vendor_sys_sw_cntrl);
    mmc_debug(mmc, "vendor_err_intr_status: 0x%08" PRIX32 "\n",mmc->regs->vendor_err_intr_status);
    mmc_debug(mmc, "vendor_cap_overrides: 0x%08" PRIX32 "\n",mmc->regs->vendor_cap_overrides);
    mmc_debug(mmc, "vendor_boot_cntrl: 0x%08" PRIX32 "\n",mmc->regs->vendor_boot_cntrl);
    mmc_debug(mmc, "vendor_boot_ack_timeout: 0x%08" PRIX32 "\n",mmc->regs->vendor_boot_ack_timeout);
    mmc_debug(mmc, "vendor_boot_dat_timeout: 0x%08" PRIX32 "\n",mmc->regs->vendor_boot_dat_timeout);
    mmc_debug(mmc, "vendor_debounce_count: 0x%08" PRIX32 "\n",mmc->regs->vendor_debounce_count);
    mmc_debug(mmc, "vendor_misc_cntrl: 0x%08" PRIX32 "\n",mmc->regs->vendor_misc_cntrl);
    mmc_debug(mmc, "max_current_override: 0x%08" PRIX32 "\n",mmc->regs->max_current_override);
    mmc_debug(mmc, "max_current_override_hi: 0x%08" PRIX32 "\n",mmc->regs->max_current_override_hi);
    mmc_debug(mmc, "vendor_io_trim_cntrl: 0x%08" PRIX32 "\n",mmc->regs->vendor_io_trim_cntrl);
    mmc_debug(mmc, "vendor_dllcal_cfg: 0x%08" PRIX32 "\n",mmc->regs->vendor_dllcal_cfg);
    mmc_debug(mmc, "vendor_dll_ctrl0: 0x%08" PRIX32 "\n",mmc->regs->vendor_dll_ctrl0);
    mmc_debug(mmc, "vendor_dll_ctrl1: 0x%08" PRIX32 "\n",mmc->regs->vendor_dll_ctrl1);
    mmc_debug(mmc, "vendor_dllcal_cfg_sta: 0x%08" PRIX32 "\n",mmc->regs->vendor_dllcal_cfg_sta);
    mmc_debug(mmc, "vendor_tuning_cntrl0: 0x%08" PRIX32 "\n",mmc->regs->vendor_tuning_cntrl0);
    mmc_debug(mmc, "vendor_tuning_cntrl1: 0x%08" PRIX32 "\n",mmc->regs->vendor_tuning_cntrl1);
    mmc_debug(mmc, "vendor_tuning_status0: 0x%08" PRIX32 "\n",mmc->regs->vendor_tuning_status0);
    mmc_debug(mmc, "vendor_tuning_status1: 0x%08" PRIX32 "\n",mmc->regs->vendor_tuning_status1);
    mmc_debug(mmc, "vendor_clk_gate_hysteresis_count: 0x%08" PRIX32 "\n",mmc->regs->vendor_clk_gate_hysteresis_count);
    mmc_debug(mmc, "vendor_preset_val0: 0x%08" PRIX32 "\n",mmc->regs->vendor_preset_val0);
    mmc_debug(mmc, "vendor_preset_val1: 0x%08" PRIX32 "\n",mmc->regs->vendor_preset_val1);
    mmc_debug(mmc, "vendor_preset_val2: 0x%08" PRIX32 "\n",mmc->regs->vendor_preset_val2);
    mmc_debug(mmc, "sdmemcomppadctrl: 0x%08" PRIX32 "\n",mmc->regs->sdmemcomppadctrl);
    mmc_debug(mmc, "auto_cal_config: 0x%08" PRIX32 "\n",mmc->regs->auto_cal_config);
    mmc_debug(mmc, "auto_cal_interval: 0x%08" PRIX32 "\n",mmc->regs->auto_cal_interval);
    mmc_debug(mmc, "auto_cal_status: 0x%08" PRIX32 "\n",mmc->regs->auto_cal_status);
    mmc_debug(mmc, "io_spare: 0x%08" PRIX32 "\n",mmc->regs->io_spare);
    mmc_debug(mmc, "sdmmca_mccif_fifoctrl: 0x%08" PRIX32 "\n",mmc->regs->sdmmca_mccif_fifoctrl);
    mmc_debug(mmc, "timeout_wcoal_sdmmca: 0x%08" PRIX32 "\n",mmc->regs->timeout_wcoal_sdmmca);
}