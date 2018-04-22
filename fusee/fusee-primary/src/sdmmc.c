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
    uint16_t response[0x8];
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
 * General masks for SDMMC registers.
 */
enum sdmmc_register_bits {

    /* Present state register */
    MMC_COMMAND_INHIBIT = 1 << 0,

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
    MMC_TRANSFER_HOST_TO_CARD = (1 << 4),

    /* Interrupt status */
    MMC_STATUS_COMMAND_COMPLETE = (1 << 0),
    MMC_STATUS_TRANSFER_COMPLETE = (1 << 1),
    MMC_STATUS_ERROR_MASK =  (0xF << 16),

    /* Host control */
    MMC_DMA_SELECT_MASK = (0x3 << 3),
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
    CMD_SEND_CSD = 9,
    CMD_SEND_CID = 10, 
    CMD_STOP_TRANSMISSION = 12,
    CMD_READ_STATUS = 13,
    CMD_BUS_TEST = 14,
    CMD_GO_INACTIVE = 15,
    CMD_SET_BLKLEN = 16,
    CMD_READ_SINGLE_BLOCK = 17,
    CMD_READ_MULTIPLE_BLOCK = 18,
};


/**
 * Page-aligned bounce buffer to target with SDMMC DMA.
 */
static uint8_t ALIGN(4096) sdmmc_bounce_buffer[4096 * 4];

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
 *
 */
static int sdmmc_hardware_init(struct mmc *mmc)
{
    volatile struct tegra_car *car = car_get_regs();
    volatile struct tegra_sdmmc *regs = mmc->regs;

    uint32_t timebase;
    bool is_timeout;

    /* XXX fixme XXX */
    bool is_hs400_hs667 = false;

    mmc_print(mmc, "initializing in %s-speed mode...", is_hs400_hs667 ? "high" : "low");


    // FIXME: set up clock and reset to fetch the relevant clock register offsets

    // Put SDMMC4 in reset
    car->rst_dev_l_set |= 0x8000;

    // Set SDMMC4 clock source (PLLP_OUT0) and divisor (1)
    car->clk_src[CLK_SOURCE_SDMMC4] = CLK_SOURCE_FIRST | CLK_DIVIDER_UNITY;

    // Set SDMMC4 clock enable
    car->clk_dev_l_set |= 0x8000;

    // host_clk_delay(0x64, clk_freq) -> Delay 100 host clock cycles
    udelay(5000);

    // Take SDMMC4 out of reset
    car->rst_dev_l_clr |= 0x8000;

    // Set IO_SPARE[19] (one cycle delay)
    regs->io_spare |= 0x80000;

    // Clear SEL_VREG
    regs->vendor_io_trim_cntrl &= ~(0x04);

    // Set trimmer value to 0x08 (SDMMC4)
    regs->vendor_clock_cntrl &= ~(0x1F000000);
    regs->vendor_clock_cntrl |= 0x08000000;

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
    regs->host_control2 &= ~(0xFFFE0000);

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
    regs->clock_control |= (0x80 << 8); // XXX wtf is this
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
            return -ETIMEDOUT;
        }

        // Wait until we're not inhibited from sending commands...
        if (!(mmc->regs->present_state & MMC_COMMAND_INHIBIT))
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
        if (get_time_since(timebase) > mmc->timeout)
            return -ETIMEDOUT;

        // If the command completes, return that.
        if (mmc->regs->int_status & MMC_STATUS_COMMAND_COMPLETE)
            return 0;

        // If the command's no longer active, and we don't have an error, use that.
        // This is what the bootrom does?
        if (!(mmc->regs->present_state & MMC_COMMAND_INHIBIT))
            return 0;

        // If an error occurs, return it.
        if (mmc->regs->int_status & MMC_STATUS_ERROR_MASK)
            return (mmc->regs->int_status & MMC_STATUS_ERROR_MASK) >> 16;
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

        // If an error occurs, return it.
        if (mmc->regs->int_status & MMC_STATUS_ERROR_MASK)
            return (mmc->regs->int_status & MMC_STATUS_ERROR_MASK) >> 16;
    }
}


/**
 * Prepare the data-related registers for command transmission.
 *
 * @param mmc The device to be used to transmit.
 * @param blocks The total number of blocks to be transferred.
 * @param is_write True iff we're sending data _to_ the card.
 */
static void sdmmc_prepare_command_data(struct mmc *mmc, uint16_t blocks, bool is_write)
{
    // Ensure we're targeting our bounce buffer.
    mmc->regs->dma_address = (uint32_t)sdmmc_bounce_buffer;

    // Ensure we're using System DMA mode for DMA.
    mmc->regs->host_control &= ~MMC_DMA_SELECT_MASK;

    // Set up the DMA block size and count.
    // FIXME: implement!
    mmc_print(mmc, "WARNING: block size and count register needs to be set up, but CSD code isnt done yet!");
    mmc->regs->block_size = 0;
    mmc->regs->block_count = 0;

    // Always use DMA mode for data, as that's what Nintendo does. :)
    mmc->regs->transfer_mode = MMC_TRANSFER_DMA_ENABLE | MMC_TRANSFER_LIMIT_BLOCK_COUNT;

    // If this is a multi-block datagram, indicate so.
    if (blocks > 1)
        mmc->regs->transfer_mode |= MMC_TRANSFER_MULTIPLE_BLOCKS;

    // If this is a write, set the WRITE mode.
    if (is_write)
        mmc->regs->transfer_mode |= MMC_TRANSFER_HOST_TO_CARD;

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
        enum sdmmc_command command, enum sdmmc_response_type response_type)
{
    // Populate the command number 
    uint16_t to_write = (command << MMC_COMMAND_NUMBER_SHIFT) | (response_type << MMC_COMMAND_RESPONSE_TYPE_SHIFT) | MMC_COMMAND_CHECK_NUMBER; 

    // If this is a "stop transmitting" command, set the abort flag.
    if (command == CMD_STOP_TRANSMISSION)
        to_write |= MMC_COMMAND_TYPE_ABORT;

    // TODO: do we want to support CRC or index checks?
    if (blocks_to_xfer)
        to_write |= MMC_COMMAND_HAS_DATA;

    // Write our command to the given register.
    // This must be all done at once, as writes to this register have semantic meaning.
    mmc->regs->command = to_write;
}



/**
 * Sends a command to the SD card, and awaits a response.
 */
static int sdmmc_send_command(struct mmc *mmc, enum sdmmc_command command,
        enum sdmmc_response_type response_type, uint32_t argument, int blocks_to_transfer, bool is_write)
{
    int rc;

    // Wait until we can issue commands to the device.
    mmc_print(mmc, "waiting for command readiness...");
    rc = sdmmc_wait_for_command_readiness(mmc);
    if(rc) {
        mmc_print(mmc, "card not willing to accept commands (%d / %08x)", rc, mmc->regs->present_state);
        return -EBUSY;
    }

    // Populate the command argument.
    mmc_print(mmc, "populating argument...");
    mmc->regs->argument = argument;

    // If we have data to send, prepare it.
    if (blocks_to_transfer) {
        mmc_print(mmc, "preparing data...");
        sdmmc_prepare_command_data(mmc, blocks_to_transfer, is_write);
    }

    // Configure the controller to send the command.
    mmc_print(mmc, "preparing command...");
    sdmmc_prepare_command_registers(mmc, blocks_to_transfer, command, response_type);

    // Wait for the command to be completed.
    mmc_print(mmc, "waiting for command completion...");
    rc = sdmmc_wait_for_command_completion(mmc);
    if(rc) {
        mmc_print(mmc, "failed to issue CMD%d (%d / %08x)", command, rc, mmc->regs->int_status);
        return rc;
    }

    // TODO: copy response to an out argument, if it we have one?

    // If we had a data stage, handle it.
    if(blocks_to_transfer) {

        // Wait for the transfer to be complete...
        mmc_print(mmc, "waiting for transfer completion...");
        rc = sdmmc_wait_for_transfer_completion(mmc);
        if(rc) {
            mmc_print(mmc, "failed to complete CMD%d data stage (%d)", command, rc);
            return rc;
        }

        // TODO: copy data from the bounce buffer to the output buffer, if this was a read
    }

    mmc_print(mmc, "command success!");
    return 0;
}


/**
 * Retrieves information about the card, and populates the MMC structure accordingly.
 * Used as part of the SDMMC initialization process.
 */
static int sdmmc_card_init(struct mmc *mmc)
{
    int rc;

    rc = sdmmc_send_command(mmc, CMD_GO_IDLE_OR_INIT, 0, 0, 0, 0);
    rc = sdmmc_send_command(mmc, CMD_SEND_OPERATING_CONDITIONS, MMC_RESPONSE_LEN48, 0, 0, 0);

    mmc_print(mmc, "response: %08x %08x %08x %08x %08x %08x %08x %08x", 
            mmc->regs->response[0], mmc->regs->response[1],
            mmc->regs->response[2], mmc->regs->response[2],
            mmc->regs->response[4], mmc->regs->response[5],
            mmc->regs->response[6], mmc->regs->response[7]);


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

    // Default to a timeout of 1S.
    // FIXME: lower
    // FIXME: abstract
    mmc->timeout = 1000000;

    // Initialize the raw SDMMC controller.
    mmc_print(mmc, "setting up hardware");
    rc = sdmmc_hardware_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set up controller! (%d)", rc);
        return rc;
    }

    // Initialize the SDMMC card.
    mmc_print(mmc, "setting up card");
    rc = sdmmc_card_init(mmc);
    if (rc) {
        mmc_print(mmc, "failed to set up card! (%d)", rc);
        return rc;
    }



    return rc;
}


/**
 * Reads a sector or sectors from a given SD card.
 *
 * @param mmc The MMC device to work with.
 * @param buffer The output buffer to target.
 * @param sector The sector number to read.
 * @param count The number of sectors to read.
 */
int sdmmc_read(struct mmc *mmc, void *buffer, uint32_t sector, unsigned int count)
{
    return -1;
}
