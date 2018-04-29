#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "sdmmc.h"
#include "car.h"
#include "timers.h"
#include "apb_misc.h"
#include "lib/printk.h"

#define TEGRA_SDMMC_BASE (0x700B0000)
#define TEGRA_SDMMC_SIZE (0x200)

/**
 * Map of tegra SDMMC registers
 */
struct PACKED tegra_sdmmc {

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
    uint8_t _0x55[0x3];
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
    uint8_t _0x70[0x3];
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
    uint32_t _0x12c[0x21];
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
enum sdmmc_response_sizes {
    /* Bytes in a LEN136 response */
    MMC_RESPONSE_SIZE_LEN136    = 15,
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

    /* Block size register */
    MMC_DMA_BOUNDARY_MAXIMUM = (0x3 << 12),
    MMC_DMA_BOUNDARY_512K    = (0x3 << 12),
    MMC_DMA_BOUNDARY_16K     = (0x2 << 12),
    MMC_TRANSFER_BLOCK_512B  = (0x1FF << 0),

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
    MMC_TRANSFER_AUTO_CMD12 = (1 <<2),
    MMC_TRANSFER_CARD_TO_HOST = (1 << 4),

    /* Interrupt status */
    MMC_STATUS_COMMAND_COMPLETE = (1 << 0),
    MMC_STATUS_TRANSFER_COMPLETE = (1 << 1),
    MMC_STATUS_DMA_INTERRUPT = (1 << 3),
    MMC_STATUS_COMMAND_TIMEOUT = (1 << 16),
    MMC_STATUS_COMMAND_CRC_ERROR = (1 << 17),
    MMC_STATUS_COMMAND_END_BIT_ERROR = (1 << 18),
    MMC_STATUS_COMMAND_INDEX_ERROR = (1 << 19),

    MMC_STATUS_ERROR_MASK =  (0xF << 16),

    /* Host control */
    MMC_DMA_SELECT_MASK = (0x3 << 3),

    /* Software reset */
    MMC_SOFT_RESET_FULL = (1 << 0),
};


/**
 * SDMMC commands
 */
enum sdmmc_command {
    CMD_GO_IDLE_OR_INIT = 0,
    CMD_SEND_OPERATING_CONDITIONS = 1,
    CMD_ALL_SEND_CID = 2,
    CMD_SET_RELATIVE_ADDR = 3,
    CMD_SET_DSR = 4,
    CMD_TOGGLE_SLEEP_AWAKE = 5,
    CMD_SWITCH_MODE = 6,
    CMD_TOGGLE_CARD_SELECT = 7,
    CMD_SEND_EXT_CSD = 8,
    CMD_SEND_IF_COND = 8,
    CMD_SEND_CSD = 9,
    CMD_SEND_CID = 10, 
    CMD_STOP_TRANSMISSION = 12,
    CMD_READ_STATUS = 13,
    CMD_BUS_TEST = 14,
    CMD_GO_INACTIVE = 15,
    CMD_SET_BLKLEN = 16,
    CMD_READ_SINGLE_BLOCK = 17,
    CMD_READ_MULTIPLE_BLOCK = 18,

    CMD_APP_CMD = 55,
};

/**
 * SDMMC command argument numbers
 */
enum sdmmc_command_magic {
    MMC_EMMC_OPERATING_COND_CAPACITY_MAGIC = 0x00ff8080,
    MMC_EMMC_OPERATING_COND_CAPACITY_MASK  = 0x0fffffff,
    MMC_EMMC_OPERATING_COND_BUSY           = (0x04 << 28),
    MMC_EMMC_OPERATING_COND_READY          = (0x0c << 28),
    MMC_EMMC_OPERATING_READINESS_MASK      = (0x0f << 28),
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
 * Page-aligned bounce buffer to target with SDMMC DMA.
 * If the size of this buffer is changed, the block_size
 */
static uint8_t ALIGN(4096) sdmmc_bounce_buffer[4096 * 4];
static const uint16_t sdmmc_bounce_dma_boundary = MMC_DMA_BOUNDARY_16K;

/**
 * Debug print for SDMMC information.
 */
void mmc_print(struct mmc *mmc, char *fmt, ...)
{
    va_list list;

    // TODO: check SDMMC log level before printing

    va_start(list, fmt);
    printk("%s: ", mmc->name);
    vprintk(fmt, list);
    printk("\n");
    va_end(list);
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
        mmc_print(mmc, "error: command response had invalid end bit");

    if (command_errno & MMC_STATUS_COMMAND_INDEX_ERROR)
        mmc_print(mmc, "error: response appears not to be for the last issued command");

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


static int sdmmc_hardware_reset(struct mmc *mmc)
{
    uint32_t timebase;

    // Reset the MMC controller...
    mmc->regs->software_reset |= MMC_SOFT_RESET_FULL;

    timebase = get_time();

    // Wait for the SDMMC controller to come back up...
    while(mmc->regs->software_reset & MMC_SOFT_RESET_FULL) {
        if (get_time_since(timebase) > mmc->timeout) {
            mmc_print(mmc, "failed to bring up SDMMC controller");
            return ETIMEDOUT;
        }
    }

    return 0;
}


/**
 *
 */
static int sdmmc_hardware_init(struct mmc *mmc)
{
    volatile struct tegra_car *car = car_get_regs();
    volatile struct tegra_sdmmc *regs = mmc->regs;

    uint32_t timebase;
    bool is_timeout;

    int rc;

    /* XXX fixme XXX */
    bool is_hs400_hs667 = false;

    mmc_print(mmc, "initializing in %s-speed mode...", is_hs400_hs667 ? "high" : "low");

    // FIXME: set up clock and reset to fetch the relevant clock register offsets

    // Put SDMMC4 in reset
    car->rst_dev_l_set |= 0x8000;

    // Set SDMMC4 clock source (PLLP_OUT0) and divisor (32).
    // We use 32 beacuse Nintendo does, and they probably know what they're doing?
    car->clk_src[CLK_SOURCE_SDMMC4] = CLK_SOURCE_FIRST | CLK_DIVIDER_32;

    // Set the legacy divier used for 
    car->clk_src_y[CLK_SOURCE_SDMMC_LEGACY] = CLK_SOURCE_FIRST | CLK_DIVIDER_32;

    // Set SDMMC4 clock enable
    car->clk_dev_l_set |= 0x8000;

    // host_clk_delay(0x64, clk_freq) -> Delay 100 host clock cycles
    udelay(5000);

    // Take SDMMC4 out of reset
    car->rst_dev_l_clr |= 0x8000;

    // Software reset the SDMMC device
    mmc_print(mmc, "resetting controller...");
    rc = sdmmc_hardware_reset(mmc);
    if (rc) {
        mmc_print(mmc, "failed to reset!");
        return rc;
    }

    // Set IO_SPARE[19] (one cycle delay)
    regs->io_spare |= 0x80000;

    // Clear SEL_VREG
    regs->vendor_io_trim_cntrl &= ~(0x04);

    // Set trimmer value to 0x08 (SDMMC4)
    regs->vendor_clock_cntrl &= ~(0x1F000000);
    regs->vendor_clock_cntrl |= 0x08000000;

    // The boootrom sets TAP_VAL to be 4.
    // We'll do that too. FIXME: should we?
    regs->vendor_clock_cntrl |= 0x40000;

    // Set SDMMC2TMC_CFG_SDMEMCOMP_VREF_SEL to 0x07
    regs->sdmemcomppadctrl &= ~(0x0F);
    regs->sdmemcomppadctrl |= 0x07;

    // Set auto-calibration PD/PU offsets
    regs->auto_cal_config = ((regs->auto_cal_config & ~(0x7F)) | 0x05);
    regs->auto_cal_config = ((regs->auto_cal_config & ~(0x7F00)) | 0x05);

    // Set PAD_E_INPUT_OR_E_PWRD (relevant for eMMC only)
    regs->sdmemcomppadctrl |= 0x80000000;

    // Wait one milisecond
    udelay(1000);

    // Set AUTO_CAL_START and AUTO_CAL_ENABLE
    regs->auto_cal_config |= 0xA0000000;

    // Wait one second
    udelay(1);

    // Program a timeout of 10ms
    is_timeout = false;
    timebase = get_time();

    // Wait for AUTO_CAL_ACTIVE to be cleared
    mmc_print(mmc, "initialing autocal...");
    while((regs->auto_cal_status & 0x80000000) && !is_timeout) {
        // Keep checking if timeout expired
        is_timeout = get_time_since(timebase) > 10000;
    }
    
    // AUTO_CAL_ACTIVE was not cleared in time
    if (is_timeout)
    {
        mmc_print(mmc, "autocal timed out!");

        // Set CFG2TMC_EMMC4_PAD_DRVUP_COMP and CFG2TMC_EMMC4_PAD_DRVDN_COMP
        APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 = ((APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 & ~(0x3F00)) | 0x1000);
        APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 = ((APB_MISC_GP_EMMC4_PAD_CFGPADCTRL_0 & ~(0xFC)) | 0x40);
        
        // Clear AUTO_CAL_ENABLE
        regs->auto_cal_config &= ~(0x20000000);
    }

    mmc_print(mmc, "autocal complete.");

    // Clear PAD_E_INPUT_OR_E_PWRD (relevant for eMMC only)
    regs->sdmemcomppadctrl &= ~(0x80000000);
    
    // Set SDHCI_CLOCK_INT_EN
    regs->clock_control |= 0x01;
   
    // Program a timeout of 2000ms
    timebase = get_time();
    is_timeout = false;
    
    // Wait for SDHCI_CLOCK_INT_STABLE to be set
    mmc_print(mmc, "waiting for internal clock to stabalize...");
    while(!(regs->clock_control & 0x02) && !is_timeout) {
        // Keep checking if timeout expired
        is_timeout = get_time_since(timebase) > 2000000;
    }
    
    // Clock failed to stabilize
    if (is_timeout) {
        mmc_print(mmc, "clock never stabalized!");
        return -1;
    } else {
        mmc_print(mmc, "clock stabalized.");
    }

    // Clear upper 17 bits
    regs->host_control2 &= ~(0xFFFF8000);

    // Clear SDHCI_PROG_CLOCK_MODE
    regs->clock_control &= ~(0x20);

    // Set SDHCI_CTRL2_HOST_V4_ENABLE
    regs->host_control2 |= 0x1000;

    // SDHCI_CAN_64BIT must be set
    if (!(regs->capabilities & 0x10000000)) {
        mmc_print(mmc, "missing CAN_64bit capability");
        return -1;
    }

    // Set SDHCI_CTRL2_64BIT_ENABLE
    regs->host_control2 |= 0x2000;

    // Clear SDHCI_CTRL_SDMA and SDHCI_CTRL_ADMA2
    regs->host_control &= 0xE7;
    
    // Set the timeout to be the maximum value
    regs->timeout_control &= ~(0x0F);
    regs->timeout_control |= 0x0E;
    
    // Clear SDHCI_CTRL_4BITBUS and SDHCI_CTRL_8BITBUS
    regs->host_control &= 0xFD;
    regs->host_control &= 0xDF;

    // Set SDHCI_POWER_180
    regs->power_control &= 0xF1;
    regs->power_control |= 0x0A;

    regs->power_control |= 0x01;

    if (is_hs400_hs667)
    {
        // Set DQS_TRIM_VAL
        regs->vendor_cap_overrides &= ~(0x3F00);
        regs->vendor_cap_overrides |= 0x2800;
    }

    // Clear TAP_VAL_UPDATED_BY_HW
    regs->vendor_tuning_cntrl0 &= ~(0x20000);

    // Software tap value should be 0 for SDMMC4, but HS400/HS667 modes
    // must take this value from the tuning procedure
    uint32_t tap_value = is_hs400_hs667 ? 1 : 0;

    // Set TAP_VAL
    regs->vendor_clock_cntrl &= ~(0xFF0000);
    regs->vendor_clock_cntrl |= (tap_value << 16);

    // Clear SDHCI_CTRL_HISPD
    regs->host_control &= 0xFB;

    // Clear SDHCI_CTRL_VDD_180 
    regs->host_control2 &= ~(0x08);

    // Set SDHCI_DIVIDER and SDHCI_DIVIDER_HI
    // FIXME: divider SD if necessary
    regs->clock_control &= ~(0xFFC0);
    regs->clock_control |= (0x80 << 8);  // use the slowest setting, for now
    //regs->clock_control |= ((sd_divider_lo << 0x08) | (sd_divider_hi << 0x06));

    // HS400/HS667 modes require additional DLL calibration
    if (is_hs400_hs667)
    {
        // Set CALIBRATE
        regs->vendor_dllcal_cfg |= 0x80000000;

        // Program a timeout of 5ms
        timebase = get_time();
        is_timeout = false;

        // Wait for CALIBRATE to be cleared
        mmc_print(mmc, "starting calibration...");
        while(regs->vendor_dllcal_cfg & 0x80000000 && !is_timeout) {
            // Keep checking if timeout expired
            is_timeout = get_time_since(timebase) > 5000;
        }

        // Failed to calibrate in time
        if (is_timeout) {
            mmc_print(mmc, "calibration failed!");
            return -1;
        }

        mmc_print(mmc, "calibration okay.");

        // Program a timeout of 10ms
        timebase = get_time();
        is_timeout = false;

        // Wait for DLL_CAL_ACTIVE to be cleared
        mmc_print(mmc, "waiting for calibration to finalize.... ");
        while((regs->vendor_dllcal_cfg_sta & 0x80000000) && !is_timeout) {
            // Keep checking if timeout expired
            is_timeout = get_time_since(timebase) > 10000;
        }

        // Failed to calibrate in time
        if (is_timeout) {
            mmc_print(mmc, "calibration failed to finalize!");
            return -1;
        }

        mmc_print(mmc, "calibration complete!");
    }

    // Set SDHCI_CLOCK_CARD_EN
    regs->clock_control |= 0x04;

    // Ensure we're using System DMA (SDMA) mode for DMA.
    regs->host_control &= ~MMC_DMA_SELECT_MASK;

    mmc_print(mmc, "initialized.");
    return 0;
}

/**
 * Blocks until the SD driver is ready for a command,
 * or the MMC controller's timeout interval is met.
 *
 * @param mmc The MMC controller
 */
static int sdmmc_wait_for_command_readiness(struct mmc *mmc)
{
    uint32_t timebase = get_time();

    // Wait until we either wind up ready, or until we've timed out.
    while(true) {
        if (get_time_since(timebase) > mmc->timeout) {
            mmc_print(mmc, "timed out waiting for command readiness!");
            return ETIMEDOUT;
        }

        // Wait until we're not inhibited from sending commands...
        if (!(mmc->regs->present_state & MMC_COMMAND_INHIBIT))
            return 0;
    }
}


/**
 * Blocks until the SD driver is ready to transmit data,
 * or the MMC controller's timeout interval is met.
 *
 * @param mmc The MMC controller
 */
static int sdmmc_wait_for_data_readiness(struct mmc *mmc)
{
    uint32_t timebase = get_time();

    // Wait until we either wind up ready, or until we've timed out.
    while(true) {
        if (get_time_since(timebase) > mmc->timeout) {
            mmc_print(mmc, "timed out waiting for command readiness!");
            return ETIMEDOUT;
        }

        // Wait until we're not inhibited from sending commands...
        if (!(mmc->regs->present_state & MMC_DATA_INHIBIT))
            return 0;
    }
}



/**
 * Blocks until the SD driver has completed issuing a command.
 *
 * @param mmc The MMC controller
 */
static int sdmmc_wait_for_command_completion(struct mmc *mmc)
{
    uint32_t timebase = get_time();

    // Wait until we either wind up ready, or until we've timed out.
    while(true) {
        if (get_time_since(timebase) > mmc->timeout) {
            mmc_print(mmc, "timed out waiting for command completion!");
            return ETIMEDOUT;
        }

        // If the command completes, return that.
        if (mmc->regs->int_status & MMC_STATUS_COMMAND_COMPLETE)
            return 0;

        // If an error occurs, return it.
        if (mmc->regs->int_status & MMC_STATUS_ERROR_MASK)
            return (mmc->regs->int_status & MMC_STATUS_ERROR_MASK);
    }
}


/**
 * Blocks until the SD driver has completed issuing a command.
 *
 * @param mmc The MMC controller
 */
static int sdmmc_wait_for_transfer_completion(struct mmc *mmc)
{
    uint32_t timebase = get_time();


    // Wait until we either wind up ready, or until we've timed out.
    while(true) {

        if (get_time_since(timebase) > mmc->timeout)
            return -ETIMEDOUT;

        // If the command completes, return that.
        if (mmc->regs->int_status & MMC_STATUS_TRANSFER_COMPLETE)
            return 0;

        // If we've hit a DMA page boundary, fault.
        if (mmc->regs->int_status & MMC_STATUS_DMA_INTERRUPT) {
            mmc_print(mmc, "transaction would overrun the DMA buffer!");
            return -EFAULT;
        }

        // If an error occurs, return it.
        if (mmc->regs->int_status & MMC_STATUS_ERROR_MASK)
            return (mmc->regs->int_status & MMC_STATUS_ERROR_MASK) >> 16;
    }
}


/**
 *  Returns the block size for a given operation on the MMC controller.
 *
 *  @param mmc The MMC controller for which we're quierying block size.
 *  @param is_write True iff the given operation is a write.
 */
static uint32_t sdmmc_get_block_size(struct mmc *mmc, bool is_write)
{
    // FIXME: support write blocks?
    (void)is_write;

    return (1 << mmc->read_block_order);
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
                mmc->regs->buffer = *buffer;
            } else {
                *buffer = mmc->regs->buffer;
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
 */
static void sdmmc_prepare_command_data(struct mmc *mmc, uint16_t blocks, bool is_write, int argument)
{
    if (blocks) {
        // If we're using DMA, target our bounce buffer.
        if (mmc->use_dma)
            mmc->regs->dma_address = (uint32_t)sdmmc_bounce_buffer;

        // Set up the DMA block size and count.
        // This is synchronized with the size of our bounce buffer.
        mmc->regs->block_size = sdmmc_bounce_dma_boundary | MMC_TRANSFER_BLOCK_512B;
        mmc->regs->block_count = blocks;
    }

    // Populate the command argument.
    mmc->regs->argument = argument;

    // Always use DMA mode for data, as that's what Nintendo does. :)
    if (blocks) {
        uint32_t to_write = MMC_TRANSFER_LIMIT_BLOCK_COUNT;

        // If this controller should use DMA, set that up.
        if (mmc->use_dma)
            to_write |= MMC_TRANSFER_DMA_ENABLE;

        // If this is a multi-block datagram, indicate so.
        // Also, configure the host to automatically stop the card when transfers are complete.
        if (blocks > 1) 
            to_write |= (MMC_TRANSFER_MULTIPLE_BLOCKS | MMC_TRANSFER_AUTO_CMD12);

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
    mmc->regs->int_status |= all_interrupts;

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
static void sdmmc_handle_command_response(struct mmc *mmc,
        enum sdmmc_response_type type, void *response_buffer)
{
    uint32_t *buffer = (uint32_t *)response_buffer;

    // If we don't have a place to put the response,
    // skip copying it out.
    if (!response_buffer) {
        return;
    }

    switch(type) {
        // Easy case: we don't have a response. We don't need to do anything.
        case MMC_RESPONSE_NONE:
            break;

        // If we have a 48-bit response, then we have 32 bits of response and 16 bits of CRC/command.
        // The naming is a little odd, but that's thanks to the SDMMC standard.
        case MMC_RESPONSE_LEN48:
        case MMC_RESPONSE_LEN48_CHK_BUSY:
            *buffer = mmc->regs->response[0];

            mmc_print(mmc, "response: %08x", *buffer);
            break;

        // If we have a 136-bit response, we have 128 of response and 8 bits of CRC.
        // TODO: validate that this is the right format/endianness/everything
        case MMC_RESPONSE_LEN136:

            // Copy the response to the buffer manually.
            // We avoid memcpy here, because this is volatile.
            for(int i = 0; i < 4; ++i)
                buffer[i] = mmc->regs->response[i];

            mmc_print(mmc, "response: %08x%08x%08x%08x", buffer[0], buffer[1], buffer[2], buffer[3]);
            break;

        default:
            mmc_print(mmc, "invalid response type; not handling response");
    }
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
 * @param data_buffer A byte buffer that either contains the data to be sent, or which should
 *      receive data, depending on the is_write argument.
 *
 * @returns 0 on success, an error number on failure
 */
static int sdmmc_send_command(struct mmc *mmc, enum sdmmc_command command,
        enum sdmmc_response_type response_type, enum sdmmc_response_checks checks,
        uint32_t argument, void *response_buffer, uint16_t blocks_to_transfer, 
        bool is_write, void *data_buffer)
{
    uint32_t total_data_to_xfer = sdmmc_get_block_size(mmc, is_write) * blocks_to_transfer;
    int rc;

    // XXX: get rid of
    mmc_print(mmc, "issuing CMD%d", command);

    // If this transfer would have us send more than we can, fail out.
    if (total_data_to_xfer > sizeof(sdmmc_bounce_buffer)) {
        mmc_print(mmc, "ERROR: transfer is larger than our maximum DMA transfer size!");
        return -E2BIG;
    }

    // Sanity check: if this is a data transfer, make sure we have a data buffer...
    if (blocks_to_transfer && !data_buffer) {
        mmc_print(mmc, "ERROR: no data buffer provided, but this is a data transfer!");
        return -EINVAL;
    }

    // Wait until we can issue commands to the device.
    rc = sdmmc_wait_for_command_readiness(mmc);
    if (rc) {
        mmc_print(mmc, "card not willing to accept commands (%d / %08x)", rc, mmc->regs->present_state);
        return -EBUSY;
    }

    // If this is a data command, wait until we can use the data lines.
    if (blocks_to_transfer) {
        rc = sdmmc_wait_for_data_readiness(mmc);
        if (rc) {
            mmc_print(mmc, "card not willing to accept data-commands (%d / %08x)", rc, mmc->regs->present_state);
            return -EBUSY;
        }
    }

    // If we have data to send, prepare it.
    sdmmc_prepare_command_data(mmc, blocks_to_transfer, is_write, argument);

    // If this is a write and we have data, we'll need to populate the bounce buffer before
    // issuing the command.
    if (blocks_to_transfer && is_write && mmc->use_dma) {
        memcpy(sdmmc_bounce_buffer, data_buffer, total_data_to_xfer);
    }

    // Configure the controller to send the command.
    sdmmc_prepare_command_registers(mmc, blocks_to_transfer, command, response_type, checks);

    // Ensure we get the status response we want.
    sdmmc_enable_interrupts(mmc, true);

    // Wait for the command to be completed.
    rc = sdmmc_wait_for_command_completion(mmc);
    if (rc) {
        mmc_print(mmc, "failed to issue CMD%d (%d / %08x)", command, rc, mmc->regs->int_status);
        mmc_print_command_errors(mmc, rc);

        sdmmc_enable_interrupts(mmc, false);
        return rc;
    }

    // Copy the response received to the output buffer, if applicable.
    sdmmc_handle_command_response(mmc, response_type, response_buffer);

    // If we had a data stage, handle it.
    if (blocks_to_transfer) {

        // If this is a DMA transfer, wait for its completion.
        if (mmc->use_dma) {

            // Wait for the transfer to be complete...
            mmc_print(mmc, "waiting for transfer completion...");
            rc = sdmmc_wait_for_transfer_completion(mmc);
            if (rc) {
                mmc_print(mmc, "failed to complete CMD%d data stage via DMA (%d)", command, rc);
                sdmmc_enable_interrupts(mmc, false);
                return rc;
            }

            // If this is a read, and we've just finished a transfer, copy the data from
            // our bounce buffer to the target data buffer.
            if (!is_write) {
                memcpy(data_buffer, sdmmc_bounce_buffer, total_data_to_xfer);
            }
        }
        // Otherwise, perform the transfer using the CPU.
        else {
            mmc_print(mmc, "transferring data...");
            rc = sdmmc_handle_cpu_transfer(mmc, blocks_to_transfer, is_write, data_buffer);
            if (rc) {
                mmc_print(mmc, "failed to complete CMD%d data stage via CPU (%d)", command, rc);
                sdmmc_enable_interrupts(mmc, false);
                return rc;
            }
        }

        // XXX: get rid of this
        printk("read result (%d):\n", total_data_to_xfer);
        for (int i = 0; i < 64; ++i) {

            if(i % 8 == 0) {
                printk("\n");
            }

            printk("%02x ", ((uint8_t *)data_buffer)[i]);
        }
        printk("\n");
    }

    // Disable resporting psuedo-interrupts.
    // (This is mostly for when the GIC is brought up)
    sdmmc_enable_interrupts(mmc, false);

    mmc_print(mmc, "CMD%d success!", command);
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
    return sdmmc_send_command(mmc, command, response_type, checks, argument, response_buffer, 0, 0, NULL);
}


/**
 * Handles eMMC-specific card initialization.
 */
static int emmc_card_init(struct mmc *mmc)
{
    int rc;
    uint32_t response[4];

    mmc_print(mmc, "setting up card as eMMC");

    // We only support Switch eMMC addressing, which is alawys block-based.
    mmc->uses_block_addressing = true;

    // Bring the bus out of its idle state.
    rc = sdmmc_send_simple_command(mmc, CMD_GO_IDLE_OR_INIT, MMC_RESPONSE_NONE, 0, NULL);
    if (rc) {
        mmc_print(mmc, "could not bring bus to idle!");
        return rc;
    }

    // Wait for the card to finish being busy.
    while (true) {

        uint32_t response_masked;

        // Ask the SD card to identify its state. It will respond with readiness and a capacity magic.
        rc = sdmmc_send_command(mmc, CMD_SEND_OPERATING_CONDITIONS, MMC_RESPONSE_LEN48, MMC_CHECKS_NONE, 0x40000080, response, 0, 0, NULL);
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

    // Print a summary of the read CSD.
    mmc_print(mmc, "CSD summary:");
    mmc_print(mmc, "  read_block_order: %d", mmc->read_block_order);

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
    switch(csd_version) {

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
    uint8_t ext_csd[512];

    // Read the single EXT CSD block.
    // FIXME: support block sizes other than 512B?
    rc = sdmmc_send_command(mmc, CMD_SEND_EXT_CSD, MMC_RESPONSE_LEN48, MMC_CHECKS_ALL, 0, NULL, 1, false, ext_csd);
    if (rc) {
        mmc_print(mmc, "ERROR: failed to read the extended CSD!");
        return rc;
    }

    // Parse the extended CSD here.
    mmc_print(mmc, "extended CSD looks like:");
    for (int i = 0; i < 64; ++i) {

        if(i % 8 == 0) {
            printk("\n");
        }

        printk("%02x ", ext_csd[i]);
    }

    printk("\n");
    return 0;
}


/**
 * Decides on a block transfer sized based on the information observed,
 * and applies it to the card.
 */
static int sdmmc_set_up_block_transfer_size(struct mmc *mmc)
{
    int rc;

    // For now, we'll only ever set up 512B blocks, because
    // 1) every card supports this, and 2) we use SDMA, which only supports up to 512B
    mmc->read_block_order = 9;

    // Inform the card of the block size we'll want to use.
    rc = sdmmc_send_simple_command(mmc, CMD_SET_BLKLEN, MMC_RESPONSE_LEN48, 1 << mmc->read_block_order, NULL);
    if (rc) {
        mmc_print(mmc, "could not fetch the CID");
        return ENODEV;
    }

    return 0;
}

/**
 * Optimize our SDMMC transfer mode to fully utilize the bus.
 */
static int sdmmc_optimize_transfer_mode(struct mmc *mmc)
{
    // TODO: FIXME
    return 0;
}



/**
 * Retrieves information about the card, and populates the MMC structure accordingly.
 * Used as part of the SDMMC initialization process.
 */
static int sdmmc_card_init(struct mmc *mmc)
{
    int rc;
    uint32_t response[4];

    // Retreive the card ID.
    rc = sdmmc_send_simple_command(mmc, CMD_ALL_SEND_CID, MMC_RESPONSE_LEN136, 0, response);
    if (rc) {
        mmc_print(mmc, "could not fetch the CID");
        return ENODEV;
    }

    // Store the card ID for later.
    memcpy(mmc->cid, response, sizeof(mmc->cid));

    // Set up the card's relative address.
    rc = sdmmc_send_simple_command(mmc, CMD_SET_RELATIVE_ADDR, MMC_RESPONSE_LEN48, mmc->relative_address << 16, response);
    if (rc) {
        mmc_print(mmc, "could not set the card's relative address! (%d)", rc);
        return EPIPE;
    }

    // Read and handle card's Card Specific Data (CSD).
    rc = sdmmc_read_and_parse_csd(mmc);
    if (rc) {
        mmc_print(mmc, "could not populate CSD attributes! (%d)", rc);
        return EPIPE;
    }

    // Select our eMMC card, so it knows we're talking to it.
    rc = sdmmc_send_simple_command(mmc, CMD_TOGGLE_CARD_SELECT, MMC_RESPONSE_LEN48, mmc->relative_address << 16, response);
    if (rc) {
        mmc_print(mmc, "could not select the active card for use! (%d)", rc);
        return EPIPE;
    }

    // Determine the block size we want to work with, and then set up the size accordingly.
    rc = sdmmc_set_up_block_transfer_size(mmc);
    if (rc) {
        mmc_print(mmc, "could not set up block transfer sizes! (%d)", rc);
        return EPIPE;
    }

    // Switch to a transfer mode that can more efficiently utilize the bus.
    rc = sdmmc_optimize_transfer_mode(mmc);
    if (rc) {
        mmc_print(mmc, "could not optimize bus utlization! (%d)", rc);
    }

    // Read and handle card's Extended Card Specific Data (ext-CSD).
    rc = sdmmc_read_and_parse_ext_csd(mmc);
    if (rc) {
        mmc_print(mmc, "could not populate extended-CSD attributes! (%d)", rc);
        return EPIPE;
    }

    return rc;
}


/**
 * Handle any speciailized initialization required by the given device type.
 *
 * @param mmc The device to initialize.
 */
static int sdmmc_handle_card_type_init(struct mmc *mmc) 
{
    int rc;

    switch(mmc->card_type) {

        // Handle initialization of eMMC cards.
        case MMC_CARD_EMMC:
            // FIXME: also handle MMC and SD cards that aren't eMMC
            rc = emmc_card_init(mmc);
            break;

        default:
            mmc_print(mmc,"initialization of this device not yet supported!");
            rc = ENOTTY;
            break;
    }

    return rc;
}



/**
 * Set up a new SDMMC driver.
 * FIXME: clean up!
 *
 * @param mmc The SDMMC structure to be initiailized with the device state.
 * @param controler The controller description to be used; usually SWITCH_EMMC
 *      or SWTICH_MICROSD.
 */
int sdmmc_init(struct mmc *mmc, enum sdmmc_controller controller)
{
    int rc;

    // Get a reference to the registers for the relevant SDMMC controller.
    mmc->regs = sdmmc_get_regs(controller);
    mmc->name = "eMMC";
    mmc->card_type = MMC_CARD_EMMC;

    // Default to a timeout of 1S.
    // FIXME: lower
    // FIXME: abstract
    mmc->timeout = 1000000;

    // FIXME: make this configurable?
    mmc->use_dma = false;

    // Default to relative address of zero.
    mmc->relative_address = 0;

    // Initialize the raw SDMMC controller.
    rc = sdmmc_hardware_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set up controller! (%d)", rc);
        return rc;
    }

    // Handle the initialization that's specific to the card type.
    rc = sdmmc_handle_card_type_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set run card-specific initialization (%d)!", rc);
        return rc;
    }

    // Handle the initialization that's common to all card types.
    rc = sdmmc_card_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set up card (%d)!", rc);
        return rc;
    }

    return 0;
}


/**
 * Reads a sector or sectors from a given SD card.
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
    // Determine if we need to perform a single-block or multi-block read.
    uint32_t command = (count == 1) ? CMD_READ_SINGLE_BLOCK : CMD_READ_MULTIPLE_BLOCK;

    // Determine the argument, which indicates which address we're reading/writing.
    uint32_t extent = block;

    // If this card uses byte addressing rather than sector addressing,
    // multiply by the block size.
    if (!mmc->uses_block_addressing) {
        extent *= sdmmc_get_block_size(mmc, false);
    }

    // Execute the relevant read.
    return sdmmc_send_command(mmc, command, MMC_RESPONSE_LEN48, MMC_CHECKS_ALL, extent, NULL, count, false, buffer);
}
