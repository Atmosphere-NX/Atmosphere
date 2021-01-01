/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#include "sdmmc_core.h"
#if defined(FUSEE_STAGE1_SRC)
#include "../../../fusee/fusee-primary/fusee-primary-main/src/car.h"
#include "../../../fusee/fusee-primary/fusee-primary-main/src/fuse.h"
#include "../../../fusee/fusee-primary/fusee-primary-main/src/pinmux.h"
#include "../../../fusee/fusee-primary/fusee-primary-main/src/timers.h"
#include "../../../fusee/fusee-primary/fusee-primary-main/src/apb_misc.h"
#include "../../../fusee/fusee-primary/fusee-primary-main/src/gpio.h"
#include "../../../fusee/fusee-primary/fusee-primary-main/src/pmc.h"
#include "../../../fusee/fusee-primary/fusee-primary-main/src/max7762x.h"
#elif defined(FUSEE_STAGE2_SRC)
#include "../../../fusee/fusee-secondary/src/car.h"
#include "../../../fusee/fusee-secondary/src/fuse.h"
#include "../../../fusee/fusee-secondary/src/pinmux.h"
#include "../../../fusee/fusee-secondary/src/timers.h"
#include "../../../fusee/fusee-secondary/src/apb_misc.h"
#include "../../../fusee/fusee-secondary/src/gpio.h"
#include "../../../fusee/fusee-secondary/src/pmc.h"
#include "../../../fusee/fusee-secondary/src/max7762x.h"
#elif defined(SEPT_STAGE2_SRC)
#include "../../../sept/sept-secondary/src/car.h"
#include "../../../sept/sept-secondary/src/fuse.h"
#include "../../../sept/sept-secondary/src/pinmux.h"
#include "../../../sept/sept-secondary/src/timers.h"
#include "../../../sept/sept-secondary/src/apb_misc.h"
#include "../../../sept/sept-secondary/src/gpio.h"
#include "../../../sept/sept-secondary/src/pmc.h"
#include "../../../sept/sept-secondary/src/max7762x.h"
#endif
#include "../log.h"

static void sdmmc_print(sdmmc_t *sdmmc, ScreenLogLevel screen_log_level, char *fmt, va_list list) {
    if (screen_log_level > log_get_log_level()) {
        return;
    }
    print(screen_log_level, "%s: ", sdmmc->name);
    vprint(screen_log_level, fmt, list);
    print(screen_log_level | SCREEN_LOG_LEVEL_NO_PREFIX, "\n");
}

void sdmmc_error(sdmmc_t *sdmmc, char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    sdmmc_print(sdmmc, SCREEN_LOG_LEVEL_ERROR, fmt, list);
    va_end(list);
}

void sdmmc_warn(sdmmc_t *sdmmc, char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    sdmmc_print(sdmmc, SCREEN_LOG_LEVEL_WARNING, fmt, list);
    va_end(list);
}

void sdmmc_info(sdmmc_t *sdmmc, char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    sdmmc_print(sdmmc, SCREEN_LOG_LEVEL_INFO, fmt, list);
    va_end(list);
}

void sdmmc_debug(sdmmc_t *sdmmc, char *fmt, ...) {
    va_list list;
    va_start(list, fmt);
    sdmmc_print(sdmmc, SCREEN_LOG_LEVEL_SD_DEBUG, fmt, list);
    va_end(list);
}

void sdmmc_dump_regs(sdmmc_t *sdmmc) {
    sdmmc_debug(sdmmc, "dma_address: 0x%08" PRIX32, sdmmc->regs->dma_address);
    sdmmc_debug(sdmmc, "block_size: 0x%04" PRIX16, sdmmc->regs->block_size);
    sdmmc_debug(sdmmc, "block_count: 0x%04" PRIX16, sdmmc->regs->block_count);
    sdmmc_debug(sdmmc, "argument: 0x%08" PRIX32, sdmmc->regs->argument);
    sdmmc_debug(sdmmc, "transfer_mode: 0x%04" PRIX16, sdmmc->regs->transfer_mode);
    sdmmc_debug(sdmmc, "command: 0x%04" PRIX16, sdmmc->regs->command);
    sdmmc_debug(sdmmc, "response[0]: 0x%08" PRIX32, sdmmc->regs->response[0]);
    sdmmc_debug(sdmmc, "response[1]: 0x%08" PRIX32, sdmmc->regs->response[1]);
    sdmmc_debug(sdmmc, "response[2]: 0x%08" PRIX32, sdmmc->regs->response[2]);
    sdmmc_debug(sdmmc, "response[3]: 0x%08" PRIX32, sdmmc->regs->response[3]);
    sdmmc_debug(sdmmc, "buffer: 0x%08" PRIX32, sdmmc->regs->buffer);
    sdmmc_debug(sdmmc, "present_state: 0x%08" PRIX32, sdmmc->regs->present_state);
    sdmmc_debug(sdmmc, "host_control: 0x%02" PRIX8, sdmmc->regs->host_control);
    sdmmc_debug(sdmmc, "power_control: 0x%02" PRIX8, sdmmc->regs->power_control);
    sdmmc_debug(sdmmc, "block_gap_control: 0x%02" PRIX8, sdmmc->regs->block_gap_control);
    sdmmc_debug(sdmmc, "wake_up_control: 0x%02" PRIX8, sdmmc->regs->wake_up_control);
    sdmmc_debug(sdmmc, "clock_control: 0x%04" PRIX16, sdmmc->regs->clock_control);
    sdmmc_debug(sdmmc, "timeout_control: 0x%02" PRIX8, sdmmc->regs->timeout_control);
    sdmmc_debug(sdmmc, "software_reset: 0x%02" PRIX8, sdmmc->regs->software_reset);
    sdmmc_debug(sdmmc, "int_status: 0x%08" PRIX32, sdmmc->regs->int_status);
    sdmmc_debug(sdmmc, "int_enable: 0x%08" PRIX32, sdmmc->regs->int_enable);
    sdmmc_debug(sdmmc, "signal_enable: 0x%08" PRIX32, sdmmc->regs->signal_enable);
    sdmmc_debug(sdmmc, "acmd12_err: 0x%04" PRIX16, sdmmc->regs->acmd12_err);
    sdmmc_debug(sdmmc, "host_control2: 0x%04" PRIX16, sdmmc->regs->host_control2);
    sdmmc_debug(sdmmc, "capabilities: 0x%08" PRIX32, sdmmc->regs->capabilities);
    sdmmc_debug(sdmmc, "capabilities_1: 0x%08" PRIX32, sdmmc->regs->capabilities_1);
    sdmmc_debug(sdmmc, "max_current: 0x%08" PRIX32, sdmmc->regs->max_current);
    sdmmc_debug(sdmmc, "set_acmd12_error: 0x%04" PRIX16, sdmmc->regs->set_acmd12_error);
    sdmmc_debug(sdmmc, "set_int_error: 0x%04" PRIX16, sdmmc->regs->set_int_error);
    sdmmc_debug(sdmmc, "adma_error: 0x%02" PRIX8, sdmmc->regs->adma_error);
    sdmmc_debug(sdmmc, "adma_address: 0x%08" PRIX32, sdmmc->regs->adma_address);
    sdmmc_debug(sdmmc, "upper_adma_address: 0x%08" PRIX32, sdmmc->regs->upper_adma_address);
    sdmmc_debug(sdmmc, "preset_for_init: 0x%04" PRIX16, sdmmc->regs->preset_for_init);
    sdmmc_debug(sdmmc, "preset_for_default: 0x%04" PRIX16, sdmmc->regs->preset_for_default);
    sdmmc_debug(sdmmc, "preset_for_high: 0x%04" PRIX16, sdmmc->regs->preset_for_high);
    sdmmc_debug(sdmmc, "preset_for_sdr12: 0x%04" PRIX16, sdmmc->regs->preset_for_sdr12);
    sdmmc_debug(sdmmc, "preset_for_sdr25: 0x%04" PRIX16, sdmmc->regs->preset_for_sdr25);
    sdmmc_debug(sdmmc, "preset_for_sdr50: 0x%04" PRIX16, sdmmc->regs->preset_for_sdr50);
    sdmmc_debug(sdmmc, "preset_for_sdr104: 0x%04" PRIX16, sdmmc->regs->preset_for_sdr104);
    sdmmc_debug(sdmmc, "preset_for_ddr50: 0x%04" PRIX16, sdmmc->regs->preset_for_ddr50);
    sdmmc_debug(sdmmc, "slot_int_status: 0x%04" PRIX16, sdmmc->regs->slot_int_status);
    sdmmc_debug(sdmmc, "host_version: 0x%04" PRIX16, sdmmc->regs->host_version);
    sdmmc_debug(sdmmc, "vendor_clock_cntrl: 0x%08" PRIX32, sdmmc->regs->vendor_clock_cntrl);
    sdmmc_debug(sdmmc, "vendor_sys_sw_cntrl: 0x%08" PRIX32, sdmmc->regs->vendor_sys_sw_cntrl);
    sdmmc_debug(sdmmc, "vendor_err_intr_status: 0x%08" PRIX32, sdmmc->regs->vendor_err_intr_status);
    sdmmc_debug(sdmmc, "vendor_cap_overrides: 0x%08" PRIX32, sdmmc->regs->vendor_cap_overrides);
    sdmmc_debug(sdmmc, "vendor_boot_cntrl: 0x%08" PRIX32, sdmmc->regs->vendor_boot_cntrl);
    sdmmc_debug(sdmmc, "vendor_boot_ack_timeout: 0x%08" PRIX32, sdmmc->regs->vendor_boot_ack_timeout);
    sdmmc_debug(sdmmc, "vendor_boot_dat_timeout: 0x%08" PRIX32, sdmmc->regs->vendor_boot_dat_timeout);
    sdmmc_debug(sdmmc, "vendor_debounce_count: 0x%08" PRIX32, sdmmc->regs->vendor_debounce_count);
    sdmmc_debug(sdmmc, "vendor_misc_cntrl: 0x%08" PRIX32, sdmmc->regs->vendor_misc_cntrl);
    sdmmc_debug(sdmmc, "max_current_override: 0x%08" PRIX32, sdmmc->regs->max_current_override);
    sdmmc_debug(sdmmc, "max_current_override_hi: 0x%08" PRIX32, sdmmc->regs->max_current_override_hi);
    sdmmc_debug(sdmmc, "vendor_io_trim_cntrl: 0x%08" PRIX32, sdmmc->regs->vendor_io_trim_cntrl);
    sdmmc_debug(sdmmc, "vendor_dllcal_cfg: 0x%08" PRIX32, sdmmc->regs->vendor_dllcal_cfg);
    sdmmc_debug(sdmmc, "vendor_dll_ctrl0: 0x%08" PRIX32, sdmmc->regs->vendor_dll_ctrl0);
    sdmmc_debug(sdmmc, "vendor_dll_ctrl1: 0x%08" PRIX32, sdmmc->regs->vendor_dll_ctrl1);
    sdmmc_debug(sdmmc, "vendor_dllcal_cfg_sta: 0x%08" PRIX32, sdmmc->regs->vendor_dllcal_cfg_sta);
    sdmmc_debug(sdmmc, "vendor_tuning_cntrl0: 0x%08" PRIX32, sdmmc->regs->vendor_tuning_cntrl0);
    sdmmc_debug(sdmmc, "vendor_tuning_cntrl1: 0x%08" PRIX32, sdmmc->regs->vendor_tuning_cntrl1);
    sdmmc_debug(sdmmc, "vendor_tuning_status0: 0x%08" PRIX32, sdmmc->regs->vendor_tuning_status0);
    sdmmc_debug(sdmmc, "vendor_tuning_status1: 0x%08" PRIX32, sdmmc->regs->vendor_tuning_status1);
    sdmmc_debug(sdmmc, "vendor_clk_gate_hysteresis_count: 0x%08" PRIX32, sdmmc->regs->vendor_clk_gate_hysteresis_count);
    sdmmc_debug(sdmmc, "vendor_preset_val0: 0x%08" PRIX32, sdmmc->regs->vendor_preset_val0);
    sdmmc_debug(sdmmc, "vendor_preset_val1: 0x%08" PRIX32, sdmmc->regs->vendor_preset_val1);
    sdmmc_debug(sdmmc, "vendor_preset_val2: 0x%08" PRIX32, sdmmc->regs->vendor_preset_val2);
    sdmmc_debug(sdmmc, "sdmemcomppadctrl: 0x%08" PRIX32, sdmmc->regs->sdmemcomppadctrl);
    sdmmc_debug(sdmmc, "auto_cal_config: 0x%08" PRIX32, sdmmc->regs->auto_cal_config);
    sdmmc_debug(sdmmc, "auto_cal_interval: 0x%08" PRIX32, sdmmc->regs->auto_cal_interval);
    sdmmc_debug(sdmmc, "auto_cal_status: 0x%08" PRIX32, sdmmc->regs->auto_cal_status);
    sdmmc_debug(sdmmc, "io_spare: 0x%08" PRIX32, sdmmc->regs->io_spare);
    sdmmc_debug(sdmmc, "sdmmca_mccif_fifoctrl: 0x%08" PRIX32, sdmmc->regs->sdmmca_mccif_fifoctrl);
    sdmmc_debug(sdmmc, "timeout_wcoal_sdmmca: 0x%08" PRIX32, sdmmc->regs->timeout_wcoal_sdmmca);
}

typedef struct {
    uint32_t clk_source_val;
    uint32_t clk_div_val;
} sdmmc_clk_source_t;

static sdmmc_clk_source_t sdmmc_clk_sources[4] = {0};

/* Determine the current SoC for Mariko specific code. */
static bool is_soc_mariko() {
    return (fuse_get_soc_type() == 1);
}

/* Check if the SDMMC device clock is held in reset. */
static bool is_sdmmc_clk_rst(SdmmcControllerNum controller) {
    volatile tegra_car_t *car = car_get_regs();

    switch (controller) {
        case SDMMC_1:
            return (car->rst_dev_l & CLK_L_SDMMC1);
        case SDMMC_2:
            return (car->rst_dev_l & CLK_L_SDMMC2);
        case SDMMC_3:
            return (car->rst_dev_u & CLK_U_SDMMC3);
        case SDMMC_4:
            return (car->rst_dev_l & CLK_L_SDMMC4);
    }

    return false;
}

/* Put the SDMMC device clock in reset. */
static void sdmmc_clk_set_rst(SdmmcControllerNum controller) {
    volatile tegra_car_t *car = car_get_regs();

    switch (controller) {
        case SDMMC_1:
            car->rst_dev_l_set = CLK_L_SDMMC1;
            break;
        case SDMMC_2:
            car->rst_dev_l_set = CLK_L_SDMMC2;
            break;
        case SDMMC_3:
            car->rst_dev_u_set = CLK_U_SDMMC3;
            break;
        case SDMMC_4:
            car->rst_dev_l_set = CLK_L_SDMMC4;
            break;
    }
}

/* Take the SDMMC device clock out of reset. */
static void sdmmc_clk_clear_rst(SdmmcControllerNum controller) {
    volatile tegra_car_t *car = car_get_regs();

    switch (controller) {
        case SDMMC_1:
            car->rst_dev_l_clr = CLK_L_SDMMC1;
            break;
        case SDMMC_2:
            car->rst_dev_l_clr = CLK_L_SDMMC2;
            break;
        case SDMMC_3:
            car->rst_dev_u_clr = CLK_U_SDMMC3;
            break;
        case SDMMC_4:
            car->rst_dev_l_clr = CLK_L_SDMMC4;
            break;
    }
}

/* Check if the SDMMC device clock is enabled. */
static bool is_sdmmc_clk_enb(SdmmcControllerNum controller) {
    volatile tegra_car_t *car = car_get_regs();

    switch (controller) {
        case SDMMC_1:
            return (car->clk_out_enb_l & CLK_L_SDMMC1);
        case SDMMC_2:
            return (car->clk_out_enb_l & CLK_L_SDMMC2);
        case SDMMC_3:
            return (car->clk_out_enb_u & CLK_U_SDMMC3);
        case SDMMC_4:
            return (car->clk_out_enb_l & CLK_L_SDMMC4);
    }

    return false;
}

/* Enable the SDMMC device clock. */
static void sdmmc_clk_set_enb(SdmmcControllerNum controller) {
    volatile tegra_car_t *car = car_get_regs();

    switch (controller) {
        case SDMMC_1:
            car->clk_enb_l_set = CLK_L_SDMMC1;
            break;
        case SDMMC_2:
            car->clk_enb_l_set = CLK_L_SDMMC2;
            break;
        case SDMMC_3:
            car->clk_enb_u_set = CLK_U_SDMMC3;
            break;
        case SDMMC_4:
            car->clk_enb_l_set = CLK_L_SDMMC4;
            break;
    }
}

/* Disable the SDMMC device clock. */
static void sdmmc_clk_clear_enb(SdmmcControllerNum controller) {
    volatile tegra_car_t *car = car_get_regs();

    switch (controller) {
        case SDMMC_1:
            car->clk_enb_l_clr = CLK_L_SDMMC1;
            break;
        case SDMMC_2:
            car->clk_enb_l_clr = CLK_L_SDMMC2;
            break;
        case SDMMC_3:
            car->clk_enb_u_clr = CLK_U_SDMMC3;
            break;
        case SDMMC_4:
            car->clk_enb_l_clr = CLK_L_SDMMC4;
            break;
    }
}

/* Get the appropriate SDMMC maximum frequency. */
static int sdmmc_get_sdclk_freq(SdmmcBusSpeed bus_speed) {
    switch (bus_speed) {
        case SDMMC_SPEED_MMC_IDENT:
        case SDMMC_SPEED_MMC_LEGACY:
            return 26000;
        case SDMMC_SPEED_MMC_HS:
            return 52000;
        case SDMMC_SPEED_MMC_HS200:
        case SDMMC_SPEED_MMC_HS400:
        case SDMMC_SPEED_SD_SDR104:
        case SDMMC_SPEED_EMU_SDR104:
            return 200000;
        case SDMMC_SPEED_SD_IDENT:
        case SDMMC_SPEED_SD_DS:
        case SDMMC_SPEED_SD_SDR12:
            return 25000;
        case SDMMC_SPEED_SD_HS:
        case SDMMC_SPEED_SD_SDR25:
            return 50000;
        case SDMMC_SPEED_SD_SDR50:
            return 100000;
        case SDMMC_SPEED_GC_ASIC_FPGA:
            return 40800;
        case SDMMC_SPEED_GC_ASIC:
            return 200000;
        default:
            return 0;
    }
}

/* Get the appropriate SDMMC divider for the SDCLK. */
static int sdmmc_get_sdclk_div(SdmmcBusSpeed bus_speed) {
    switch (bus_speed) {
        case SDMMC_SPEED_MMC_IDENT:
            return 66;
        case SDMMC_SPEED_SD_IDENT:
         /* return 64; */
        case SDMMC_SPEED_MMC_LEGACY:
        case SDMMC_SPEED_MMC_HS:
        case SDMMC_SPEED_MMC_HS200:
        case SDMMC_SPEED_MMC_HS400:
        case SDMMC_SPEED_SD_DS:
        case SDMMC_SPEED_SD_HS:
        case SDMMC_SPEED_SD_SDR12:
        case SDMMC_SPEED_SD_SDR25:
        case SDMMC_SPEED_SD_SDR50:
        case SDMMC_SPEED_SD_SDR104:
        case SDMMC_SPEED_GC_ASIC_FPGA:
        case SDMMC_SPEED_EMU_SDR104:
            return 1;
        case SDMMC_SPEED_GC_ASIC:
            return 2;
        default:
            return 0;
    }
}

/* Set the device clock source and CAR divider. */
static int sdmmc_clk_set_source(SdmmcControllerNum controller, uint32_t clk_freq) {
    volatile tegra_car_t *car = car_get_regs();

    uint32_t car_div = 0;
    uint32_t out_freq = 0;

    switch (clk_freq) {
        case 25000:
            out_freq = 24728;
            car_div = SDMMC_CAR_DIVIDER_SD_SDR12;
            break;
        case 26000:
            out_freq = 25500;
            car_div = SDMMC_CAR_DIVIDER_MMC_LEGACY;
            break;
        case 40800:
            out_freq = 40800;
            car_div = SDMMC_CAR_DIVIDER_GC_ASIC_FPGA;
            break;
        case 50000:
            out_freq = 48000;
            car_div = SDMMC_CAR_DIVIDER_SD_SDR25;
            break;
        case 52000:
            out_freq = 51000;
            car_div = SDMMC_CAR_DIVIDER_MMC_HS;
            break;
        case 100000:
            out_freq = 90667;
            car_div = SDMMC_CAR_DIVIDER_SD_SDR50;
            break;
        case 200000:
            out_freq = 163200;
            car_div = SDMMC_CAR_DIVIDER_MMC_HS200;
            break;
        case 208000:
            out_freq = 204000;
            car_div = SDMMC_CAR_DIVIDER_SD_SDR104;
            break;
        default:
            return 0;
    }

    sdmmc_clk_sources[controller].clk_source_val = clk_freq;
    sdmmc_clk_sources[controller].clk_div_val = out_freq;

    switch (controller) {
        case SDMMC_1:
            car->clk_source_sdmmc1 = (CLK_SOURCE_FIRST | car_div);
            break;
        case SDMMC_2:
            car->clk_source_sdmmc2 = (CLK_SOURCE_FIRST | car_div);
            break;
        case SDMMC_3:
            car->clk_source_sdmmc3 = (CLK_SOURCE_FIRST | car_div);
            break;
        case SDMMC_4:
            car->clk_source_sdmmc4 = (CLK_SOURCE_FIRST | car_div);
            break;
    }

    return out_freq;
}

/* Adjust the device clock source value. */
static int sdmmc_clk_adjust_source(SdmmcControllerNum controller, uint32_t clk_source_val) {
    uint32_t out_val = 0;

    if (sdmmc_clk_sources[controller].clk_source_val == clk_source_val) {
        out_val = sdmmc_clk_sources[controller].clk_div_val;
    } else {
        bool was_sdmmc_clk_enb = is_sdmmc_clk_enb(controller);

        /* Clock was already enabled. Disable it. */
        if (was_sdmmc_clk_enb) {
            sdmmc_clk_clear_enb(controller);
        }

        out_val = sdmmc_clk_set_source(controller, clk_source_val);

        /* Clock was already enabled. Enable it back. */
        if (was_sdmmc_clk_enb) {
            sdmmc_clk_set_enb(controller);
        }

        /* Dummy read for value refreshing. */
        is_sdmmc_clk_rst(controller);
    }

    return out_val;
}

/* Enable the SD clock if possible. */
static void sdmmc_enable_sd_clock(sdmmc_t *sdmmc) {
    if ((sdmmc->has_sd) && !(sdmmc->regs->clock_control & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE)) {
        sdmmc->regs->clock_control |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
    }
    sdmmc->is_sd_clk_enabled = true;
}

/* Disable the SD clock. */
static void sdmmc_disable_sd_clock(sdmmc_t *sdmmc) {
    sdmmc->is_sd_clk_enabled = false;
    sdmmc->regs->clock_control &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
}

/* Automatically enable or disable the SD clock. */
void sdmmc_adjust_sd_clock(sdmmc_t *sdmmc) {
    if (!(sdmmc->has_sd) && (sdmmc->regs->clock_control & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE)) {
        sdmmc_disable_sd_clock(sdmmc);
    } else if (sdmmc->is_sd_clk_enabled && !(sdmmc->regs->clock_control & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE)) {
        sdmmc_enable_sd_clock(sdmmc);
    }
}

/* Return the clock control value. Used for dummy reads. */
static int sdmmc_get_sd_clock_control(sdmmc_t *sdmmc) {
    return sdmmc->regs->clock_control;
}

/* Start the SDMMC clock. */
static void sdmmc_clk_start(SdmmcControllerNum controller, uint32_t clk_source_val) {
    /* Clock was already enabled. Disable it. */
    if (is_sdmmc_clk_enb(controller)) {
        sdmmc_clk_clear_enb(controller);
    }

    /* Put the device clock in reset. */
    sdmmc_clk_set_rst(controller);

    /* Configure the device clock source. */
    uint32_t clk_div = sdmmc_clk_set_source(controller, clk_source_val);

    /* Enable the device clock. */
    sdmmc_clk_set_enb(controller);

    /* Dummy read for value refreshing. */
    is_sdmmc_clk_rst(controller);

    /* Synchronize. */
    udelay((100000 + clk_div - 1) / clk_div);

    /* Take the device clock out of reset. */
    sdmmc_clk_clear_rst(controller);

    /* Dummy read for value refreshing. */
    is_sdmmc_clk_rst(controller);
}

/* Stop the SDMMC clock. */
static void sdmmc_clk_stop(SdmmcControllerNum controller) {
    /* Put the device clock in reset. */
    sdmmc_clk_set_rst(controller);

    /* Disable the device clock. */
    sdmmc_clk_clear_enb(controller);

    /* Dummy read for value refreshing. */
    is_sdmmc_clk_rst(controller);
}

/* Configure clock trimming. */
static void sdmmc_vendor_clock_cntrl_config(sdmmc_t *sdmmc) {
    bool is_mariko = is_soc_mariko();

    /* Clear the I/O conditioning constants. */
    sdmmc->regs->vendor_clock_cntrl &= ~(SDMMC_CLOCK_TRIM_MASK | SDMMC_CLOCK_TAP_MASK);

    /* Set the PADPIPE clock enable */
    sdmmc->regs->vendor_clock_cntrl |= SDMMC_CLOCK_PADPIPE_CLKEN_OVERRIDE;

    /* Set the appropriate trim value. */
    switch (sdmmc->controller) {
        case SDMMC_1:
            sdmmc->regs->vendor_clock_cntrl |= (is_mariko ? SDMMC_CLOCK_TRIM_SDMMC1_MARIKO : SDMMC_CLOCK_TRIM_SDMMC1_ERISTA);
            break;
        case SDMMC_2:
            sdmmc->regs->vendor_clock_cntrl |= (is_mariko ? SDMMC_CLOCK_TRIM_SDMMC2_MARIKO : SDMMC_CLOCK_TRIM_SDMMC2_ERISTA);
            break;
        case SDMMC_3:
            sdmmc->regs->vendor_clock_cntrl |= SDMMC_CLOCK_TRIM_SDMMC3;
            break;
        case SDMMC_4:
            sdmmc->regs->vendor_clock_cntrl |= (is_mariko ? SDMMC_CLOCK_TRIM_SDMMC4_MARIKO : SDMMC_CLOCK_TRIM_SDMMC4_ERISTA);
            break;
    }

    /* Clear the SPI_MODE clock enable. */
    sdmmc->regs->vendor_clock_cntrl &= ~(SDMMC_CLOCK_SPI_MODE_CLKEN_OVERRIDE);
}

/* Configure automatic calibration. */
static int sdmmc_autocal_config(sdmmc_t *sdmmc, SdmmcBusVoltage voltage) {
    bool is_mariko = is_soc_mariko();

    switch (sdmmc->controller) {
        case SDMMC_1:
        case SDMMC_3:
            switch (voltage) {
                case SDMMC_VOLTAGE_1V8:
                    sdmmc->regs->auto_cal_config &= ~(SDMMC_AUTOCAL_PDPU_CONFIG_MASK);
                    sdmmc->regs->auto_cal_config |= (is_mariko ? SDMMC_AUTOCAL_PDPU_SDMMC1_1V8_MARIKO : SDMMC_AUTOCAL_PDPU_SDMMC1_1V8_ERISTA);
                    break;
                case SDMMC_VOLTAGE_3V3:
                    sdmmc->regs->auto_cal_config &= ~(SDMMC_AUTOCAL_PDPU_CONFIG_MASK);
                    sdmmc->regs->auto_cal_config |= (is_mariko ? SDMMC_AUTOCAL_PDPU_SDMMC1_3V3_MARIKO : SDMMC_AUTOCAL_PDPU_SDMMC1_3V3_ERISTA);
                    break;
                default:
                    sdmmc_error(sdmmc, "uSD does not support requested voltage!");
                    return 0;
            }

            break;
        case SDMMC_2:
        case SDMMC_4:
            if (voltage != SDMMC_VOLTAGE_1V8) {
                sdmmc_error(sdmmc, "eMMC can only run at 1V8!");
                return 0;
            }
            sdmmc->regs->auto_cal_config &= ~(SDMMC_AUTOCAL_PDPU_CONFIG_MASK);
            sdmmc->regs->auto_cal_config |= SDMMC_AUTOCAL_PDPU_SDMMC4_1V8;
            break;
    }

    return 1;
}

/* Run automatic calibration. */
static void sdmmc_autocal_run(sdmmc_t *sdmmc, SdmmcBusVoltage voltage) {
    volatile tegra_padctl_t *padctl = padctl_get_regs();
    bool restart_sd_clock = false;
    bool is_mariko = is_soc_mariko();

    /* SD clock is enabled. Disable it and restart later. */
    if (sdmmc->is_sd_clk_enabled) {
        restart_sd_clock = true;
        sdmmc_disable_sd_clock(sdmmc);
    }

    /* Set PAD_E_INPUT_OR_E_PWRD */
    if (!(sdmmc->regs->sdmemcomppadctrl & 0x80000000)) {
        sdmmc->regs->sdmemcomppadctrl |= 0x80000000;

        /* Force a register read to refresh the clock control value. */
        sdmmc_get_sd_clock_control(sdmmc);

        /* Delay. */
        udelay(1);
    }

    /* Start automatic calibration. */
    sdmmc->regs->auto_cal_config |= (SDMMC_AUTOCAL_START | SDMMC_AUTOCAL_ENABLE);

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Delay. */
    udelay(2);

    /* Get current time. */
    uint32_t timebase = get_time();

    /* Wait until the autocal is complete. */
    while ((sdmmc->regs->auto_cal_status & SDMMC_AUTOCAL_ACTIVE)) {
        /* Ensure we haven't timed out. */
        if (get_time_since(timebase) > SDMMC_AUTOCAL_TIMEOUT) {
            sdmmc_warn(sdmmc, "Auto-calibration timed out!");

            /* Force a register read to refresh the clock control value. */
            sdmmc_get_sd_clock_control(sdmmc);

            /* Upon timeout, fall back to standard values. */
            if (sdmmc->controller == SDMMC_1) {
                uint32_t drvup, drvdn = 0;
                if (is_mariko) {
                    drvup = 0x8;
                    drvdn = 0x8;
                } else {
                    drvup = (voltage == SDMMC_VOLTAGE_3V3) ? 0xC : 0xB;
                    drvdn = (voltage == SDMMC_VOLTAGE_3V3) ? 0xC : 0xF;
                }
                uint32_t value = padctl->sdmmc1_pad_cfgpadctrl;
                value &= ~(SDMMC1_PAD_CAL_DRVUP_MASK | SDMMC1_PAD_CAL_DRVDN_MASK);
                value |= (drvup << SDMMC1_PAD_CAL_DRVUP_SHIFT);
                value |= (drvdn << SDMMC1_PAD_CAL_DRVDN_SHIFT);
                padctl->sdmmc1_pad_cfgpadctrl = value;
            } else if (sdmmc->controller == SDMMC_2) {
                uint32_t drvup, drvdn = 0;
                if (is_mariko) {
                    drvup = 0xA;
                    drvdn = 0xA;
                    uint32_t value = padctl->emmc2_pad_cfgpadctrl;
                    value &= ~(SDMMC2_PAD_CAL_DRVUP_MASK | SDMMC2_PAD_CAL_DRVDN_MASK);
                    value |= (drvup << SDMMC2_PAD_CAL_DRVUP_SHIFT);
                    value |= (drvdn << SDMMC2_PAD_CAL_DRVDN_SHIFT);
                    padctl->emmc2_pad_cfgpadctrl = value;
                } else {
                    drvup = 0x10;
                    drvdn = 0x10;
                    uint32_t value = padctl->emmc2_pad_cfgpadctrl;
                    value &= ~(EMMC2_PAD_DRVUP_COMP_MASK | EMMC2_PAD_DRVDN_COMP_MASK);
                    value |= (drvup << EMMC2_PAD_DRVUP_COMP_SHIFT);
                    value |= (drvdn << EMMC2_PAD_DRVDN_COMP_SHIFT);
                    padctl->emmc2_pad_cfgpadctrl = value;
                }
            } else if (sdmmc->controller == SDMMC_4) {
                uint32_t drvup, drvdn = 0;
                if (is_mariko) {
                    drvup = 0xA;
                    drvdn = 0xA;
                } else {
                    drvup = 0x10;
                    drvdn = 0x10;
                }
                uint32_t value = padctl->emmc4_pad_cfgpadctrl;
                value &= ~(EMMC4_PAD_DRVUP_COMP_MASK | EMMC4_PAD_DRVDN_COMP_MASK);
                value |= (drvup << EMMC4_PAD_DRVUP_COMP_SHIFT);
                value |= (drvdn << EMMC4_PAD_DRVDN_COMP_SHIFT);
                padctl->emmc4_pad_cfgpadctrl = value;
            }

            /* Manually clear the autocal enable bit. */
            sdmmc->regs->auto_cal_config &= ~(SDMMC_AUTOCAL_ENABLE);
            break;
        }
    }

    /* Clear PAD_E_INPUT_OR_E_PWRD (relevant for eMMC only) */
    sdmmc->regs->sdmemcomppadctrl &= ~(0x80000000);

    /* If requested, enable the SD clock. */
    if (restart_sd_clock) {
        sdmmc_enable_sd_clock(sdmmc);
    }
}

static int sdmmc_int_clk_enable(sdmmc_t *sdmmc) {
    /* Enable the internal clock. */
    sdmmc->regs->clock_control |= TEGRA_MMC_CLKCON_INTERNAL_CLOCK_ENABLE;

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Program a timeout of 2000ms. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    /* Wait for the clock to stabilize. */
    while (!(sdmmc->regs->clock_control & TEGRA_MMC_CLKCON_INTERNAL_CLOCK_STABLE) && !is_timeout) {
        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 2000000);
    }

    /* Clock failed to stabilize. */
    if (is_timeout) {
        sdmmc_error(sdmmc, "Clock never stabilized!");
        return 0;
    }

    /* Configure clock control and host control 2. */
    sdmmc->regs->host_control2 &= ~SDHCI_CTRL_PRESET_VAL_ENABLE;
    sdmmc->regs->clock_control &= ~TEGRA_MMC_CLKCON_PROG_CLOCK_MODE;
    sdmmc->regs->host_control2 |= SDHCI_HOST_VERSION_4_EN;

    /* Ensure 64bit addressing is supported. */
    if (!(sdmmc->regs->capabilities & SDHCI_CAN_64BIT)) {
        sdmmc_error(sdmmc, "64bit addressing is unsupported!");
        return 0;
    }

    /* Enable 64bit addressing. */
    sdmmc->regs->host_control2 |= SDHCI_ADDRESSING_64BIT_EN;

    /* Use SDMA by default. */
    sdmmc->regs->host_control &= ~SDHCI_CTRL_DMA_MASK;

    /* Change to ADMA if possible. */
    if (sdmmc->regs->capabilities & SDHCI_CAN_DO_ADMA2) {
        sdmmc->use_adma = true;
    }

    /* Set the timeout to be the maximum value. */
    sdmmc->regs->timeout_control &= 0xF0;
    sdmmc->regs->timeout_control |= 0x0E;

    return 1;
}

void sdmmc_select_bus_width(sdmmc_t *sdmmc, SdmmcBusWidth width) {
    if (width == SDMMC_BUS_WIDTH_1BIT) {
        sdmmc->regs->host_control &= ~(SDHCI_CTRL_4BITBUS | SDHCI_CTRL_8BITBUS);
        sdmmc->bus_width = SDMMC_BUS_WIDTH_1BIT;
    } else if (width == SDMMC_BUS_WIDTH_4BIT) {
        sdmmc->regs->host_control |= SDHCI_CTRL_4BITBUS;
        sdmmc->regs->host_control &= ~SDHCI_CTRL_8BITBUS;
        sdmmc->bus_width = SDMMC_BUS_WIDTH_4BIT;
    } else if (width == SDMMC_BUS_WIDTH_8BIT) {
        sdmmc->regs->host_control |= SDHCI_CTRL_8BITBUS;
        sdmmc->bus_width = SDMMC_BUS_WIDTH_8BIT;
    } else {
        sdmmc_error(sdmmc, "Invalid bus width specified!");
    }
}

void sdmmc_select_voltage(sdmmc_t *sdmmc, SdmmcBusVoltage voltage) {
    if (voltage == SDMMC_VOLTAGE_NONE) {
        sdmmc->regs->power_control &= ~TEGRA_MMC_PWRCTL_SD_BUS_POWER;
        sdmmc->bus_voltage = SDMMC_VOLTAGE_NONE;
    } else if (voltage == SDMMC_VOLTAGE_1V8) {
        sdmmc->regs->power_control |= TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V1_8;
        sdmmc->regs->power_control |= TEGRA_MMC_PWRCTL_SD_BUS_POWER;
        sdmmc->bus_voltage = SDMMC_VOLTAGE_1V8;
    } else if (voltage == SDMMC_VOLTAGE_3V3) {
        sdmmc->regs->power_control |= TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V3_3;
        sdmmc->regs->power_control |= TEGRA_MMC_PWRCTL_SD_BUS_POWER;
        sdmmc->bus_voltage = SDMMC_VOLTAGE_3V3;
    } else {
        sdmmc_error(sdmmc, "Invalid power state specified!");
    }
}

static void sdmmc_tap_config(sdmmc_t *sdmmc, SdmmcBusSpeed bus_speed) {
    bool is_mariko = is_soc_mariko();

    if (bus_speed == SDMMC_SPEED_MMC_HS400) {
        /* Clear and set DQS_TRIM_VAL (used in HS400) */
        sdmmc->regs->vendor_cap_overrides &= ~(0x3F00);
        sdmmc->regs->vendor_cap_overrides |= 0x2800;
    }

    /* Clear TAP_VAL_UPDATED_BY_HW */
    sdmmc->regs->vendor_tuning_cntrl0 &= ~(0x20000);

    if (bus_speed == SDMMC_SPEED_MMC_HS400) {
        /* We must have obtained the tap value from the tuning procedure here. */
        if (sdmmc->is_tuning_tap_val_set) {
            /* Clear and set the tap value. */
            sdmmc->regs->vendor_clock_cntrl &= ~(0xFF0000);
            sdmmc->regs->vendor_clock_cntrl |= (sdmmc->tap_val << 16);
        }
    } else {
        /* Use the recommended values. */
        switch (sdmmc->controller) {
            case SDMMC_1:
                sdmmc->tap_val = (is_mariko ? 0xB : 4);
                break;
            case SDMMC_2:
            case SDMMC_4:
                sdmmc->tap_val = (is_mariko ? 0xB : 0);
                break;
            case SDMMC_3:
                sdmmc->tap_val = 3;
                break;
        }

        /* Clear and set the tap value. */
        sdmmc->regs->vendor_clock_cntrl &= ~(0xFF0000);
        sdmmc->regs->vendor_clock_cntrl |= (sdmmc->tap_val << 16);
    }
}

static int sdmmc_dllcal_run(sdmmc_t *sdmmc) {
    bool shutdown_sd_clock = false;

    /* SD clock is disabled. Enable it. */
    if (!sdmmc->is_sd_clk_enabled) {
        shutdown_sd_clock = true;
        sdmmc_enable_sd_clock(sdmmc);
    }

    /* Set the CALIBRATE bit. */
    sdmmc->regs->vendor_dllcal_cfg |= 0x80000000;

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Program a timeout of 5ms. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    /* Wait for CALIBRATE to be cleared. */
    while ((sdmmc->regs->vendor_dllcal_cfg & 0x80000000) && !is_timeout) {
        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 5000);
    }

    /* Calibration failed. */
    if (is_timeout) {
        sdmmc_error(sdmmc, "DLLCAL failed!");
        return 0;
    }

    /* Program a timeout of 10ms. */
    timebase = get_time();
    is_timeout = false;

    /* Wait for DLL_CAL_ACTIVE to be cleared. */
    while ((sdmmc->regs->vendor_dllcal_cfg_sta & 0x80000000) && !is_timeout) {
        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 10000);
    }

    /* Calibration failed. */
    if (is_timeout) {
        sdmmc_error(sdmmc, "DLLCAL failed!");
        return 0;
    }

    /* If requested, disable the SD clock. */
    if (shutdown_sd_clock) {
        sdmmc_disable_sd_clock(sdmmc);
    }

    return 1;
}

int sdmmc_select_speed(sdmmc_t *sdmmc, SdmmcBusSpeed bus_speed) {
    bool restart_sd_clock = false;

    /* SD clock is enabled. Disable it and restart later. */
    if (sdmmc->is_sd_clk_enabled) {
        restart_sd_clock = true;
        sdmmc_disable_sd_clock(sdmmc);
    }

    /* Configure tap values as necessary. */
    sdmmc_tap_config(sdmmc, bus_speed);

    /* Set the appropriate host speed. */
    switch (bus_speed) {
        /* 400kHz initialization mode and a few others. */
        case SDMMC_SPEED_MMC_IDENT:
        case SDMMC_SPEED_MMC_LEGACY:
        case SDMMC_SPEED_SD_IDENT:
        case SDMMC_SPEED_SD_DS:
            sdmmc->regs->host_control &= ~(SDHCI_CTRL_HISPD);
            sdmmc->regs->host_control2 &= ~(SDHCI_CTRL_VDD_180);
            break;

        /* 50MHz high speed (SD) and 52MHz high speed (MMC). */
        case SDMMC_SPEED_SD_HS:
        case SDMMC_SPEED_MMC_HS:
        case SDMMC_SPEED_SD_SDR25:
            sdmmc->regs->host_control |= SDHCI_CTRL_HISPD;
            sdmmc->regs->host_control2 &= ~(SDHCI_CTRL_VDD_180);
            break;

        /* 200MHz UHS-I (SD) and other modes due to errata. */
        case SDMMC_SPEED_MMC_HS200:
        case SDMMC_SPEED_SD_SDR104:
        case SDMMC_SPEED_GC_ASIC_FPGA:
        case SDMMC_SPEED_SD_SDR50:
        case SDMMC_SPEED_GC_ASIC:
        case SDMMC_SPEED_EMU_SDR104:
            sdmmc->regs->host_control2 &= ~(SDHCI_CTRL_UHS_MASK);
            sdmmc->regs->host_control2 |= SDHCI_CTRL_UHS_SDR104;
            sdmmc->regs->host_control2 |= SDHCI_CTRL_VDD_180;
            break;

        /* 200MHz single-data rate (MMC). */
        case SDMMC_SPEED_MMC_HS400:
            sdmmc->regs->host_control2 &= ~(SDHCI_CTRL_UHS_MASK);
            sdmmc->regs->host_control2 |= SDHCI_CTRL_HS400;
            sdmmc->regs->host_control2 |= SDHCI_CTRL_VDD_180;
            break;

        /* 25MHz default speed (SD). */
        case SDMMC_SPEED_SD_SDR12:
            sdmmc->regs->host_control2 &= ~(SDHCI_CTRL_UHS_MASK);
            sdmmc->regs->host_control2 |= SDHCI_CTRL_UHS_SDR12;
            sdmmc->regs->host_control2 |= SDHCI_CTRL_VDD_180;
            break;

        default:
            sdmmc_error(sdmmc, "Switching to unsupported speed!");
            return 0;
    }

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Get the clock's frequency and divider. */
    uint32_t freq_val = sdmmc_get_sdclk_freq(bus_speed);
    uint32_t div_val = sdmmc_get_sdclk_div(bus_speed);

    /* Adjust the CAR side of the clock. */
    uint32_t out_freq_val = sdmmc_clk_adjust_source(sdmmc->controller, freq_val);

    /* Save the internal divider value. */
    sdmmc->internal_divider = ((out_freq_val + div_val - 1) / div_val);

    uint16_t div_val_lo = div_val >> 1;
    uint16_t div_val_hi = 0;

    if (div_val_lo > 0xFF) {
        div_val_hi = (div_val_lo >> 8);
    }

    /* Set the clock control divider values. */
    sdmmc->regs->clock_control &= ~((SDHCI_DIV_HI_MASK | SDHCI_DIV_MASK) << 6);
    sdmmc->regs->clock_control |= ((div_val_hi << SDHCI_DIVIDER_HI_SHIFT) | (div_val_lo << SDHCI_DIVIDER_SHIFT));

    /* If requested, enable the SD clock. */
    if (restart_sd_clock) {
        sdmmc_enable_sd_clock(sdmmc);
    }

    /* Run DLLCAL for HS400 only */
    if (bus_speed == SDMMC_SPEED_MMC_HS400) {
        return sdmmc_dllcal_run(sdmmc);
    }

    return 1;
}

static int sdmmc1_config() {
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    volatile tegra_padctl_t *padctl = padctl_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();

    /* Set up the card detect pin as a GPIO input */
    pinmux->pz1 = PINMUX_SELECT_FUNCTION1 | PINMUX_PULL_UP | PINMUX_INPUT;
    padctl->vgpio_gpio_mux_sel = 0;
    gpio_configure_mode(GPIO_MICROSD_CARD_DETECT, GPIO_MODE_GPIO);
    gpio_configure_direction(GPIO_MICROSD_CARD_DETECT, GPIO_DIRECTION_INPUT);
    udelay(100);

    /* Check the GPIO. */
    if (gpio_read(GPIO_MICROSD_CARD_DETECT)) {
        return 0;
    }

    /* Enable loopback control.  */
    padctl->sdmmc1_clk_lpbk_control = 1;

    /* Set up the SDMMC1 pinmux. */
    pinmux->sdmmc1_clk  = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT;
    pinmux->sdmmc1_cmd  = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;
    pinmux->sdmmc1_dat3 = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;
    pinmux->sdmmc1_dat2 = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;
    pinmux->sdmmc1_dat1 = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;
    pinmux->sdmmc1_dat0 = PINMUX_DRIVE_2X | PINMUX_PARKED | PINMUX_SELECT_FUNCTION0 | PINMUX_INPUT | PINMUX_PULL_UP;

    /* Ensure the PMC is prepared for the SDMMC1 card to receive power. */
    pmc->no_iopower &= ~PMC_CONTROL_SDMMC1;

    /* Configure the enable line for the SD card power. */
    pinmux->dmic3_clk  =  PINMUX_SELECT_FUNCTION1 | PINMUX_PULL_DOWN | PINMUX_INPUT;
    gpio_configure_mode(GPIO_MICROSD_SUPPLY_ENABLE, GPIO_MODE_GPIO);
    gpio_write(GPIO_MICROSD_SUPPLY_ENABLE, GPIO_LEVEL_HIGH);
    gpio_configure_direction(GPIO_MICROSD_SUPPLY_ENABLE, GPIO_DIRECTION_OUTPUT);

    /* Delay. */
    udelay(10000);

    /* Configure Sdmmc1 IO as 3.3V. */
    pmc->pwr_det_val |= PMC_CONTROL_SDMMC1;
    max77620_regulator_set_voltage(REGULATOR_LDO2, 3300000);
    max77620_regulator_enable(REGULATOR_LDO2, 1);

    /* Delay. */
    udelay(130);

    return 1;
}

static int sdmmc2_config() {
    return 1;
}

static int sdmmc3_config() {
    return 1;
}

static int sdmmc4_config() {
    return 1;
}

static int sdmmc_init_controller(sdmmc_t *sdmmc, SdmmcControllerNum controller) {
    /* Sanitize input number for the controller. */
    if ((controller < SDMMC_1) || (controller > SDMMC_4)) {
        return 0;
    }

    /* Clear up memory for our struct. */
    memset(sdmmc, 0, sizeof(sdmmc_t));

    /* Bind the appropriate controller and it's register space to our struct. */
    sdmmc->controller = controller;
    sdmmc->regs = sdmmc_get_regs(controller);

    /* Set up per-device pointers and properties. */
    switch (sdmmc->controller) {
        case SDMMC_1:
            /* Controller properties. */
            sdmmc->name = "uSD";
            sdmmc->has_sd = true;
            sdmmc->is_clk_running = false;
            sdmmc->is_sd_clk_enabled = false;
            sdmmc->is_tuning_tap_val_set = false;
            sdmmc->use_adma = false;
            sdmmc->dma_bounce_buf = (uint8_t*)SDMMC_BOUNCE_BUFFER_ADDRESS;
            sdmmc->tap_val = 0;
            sdmmc->internal_divider = 0;
            sdmmc->bus_voltage = SDMMC_VOLTAGE_NONE;

            /* Function pointers. */
            sdmmc->sdmmc_config = sdmmc1_config;
            break;

        case SDMMC_2:
            /* Controller properties. */
            sdmmc->name = "GC";
            sdmmc->has_sd = true;
            sdmmc->is_clk_running = false;
            sdmmc->is_sd_clk_enabled = false;
            sdmmc->is_tuning_tap_val_set = false;
            sdmmc->use_adma = false;
            sdmmc->dma_bounce_buf = (uint8_t*)SDMMC_BOUNCE_BUFFER_ADDRESS;
            sdmmc->tap_val = 0;
            sdmmc->internal_divider = 0;
            sdmmc->bus_voltage = SDMMC_VOLTAGE_NONE;

            /* Function pointers. */
            sdmmc->sdmmc_config = sdmmc2_config;
            break;

        case SDMMC_3:
            /* Controller properties. */
            sdmmc->name = "UNUSED";
            sdmmc->has_sd = true;
            sdmmc->is_clk_running = false;
            sdmmc->is_sd_clk_enabled = false;
            sdmmc->is_tuning_tap_val_set = false;
            sdmmc->use_adma = false;
            sdmmc->dma_bounce_buf = (uint8_t*)SDMMC_BOUNCE_BUFFER_ADDRESS;
            sdmmc->tap_val = 0;
            sdmmc->internal_divider = 0;
            sdmmc->bus_voltage = SDMMC_VOLTAGE_NONE;

            /* Function pointers. */
            sdmmc->sdmmc_config = sdmmc3_config;
            break;

        case SDMMC_4:
            /* Controller properties. */
            sdmmc->name = "eMMC";
            sdmmc->has_sd = true;
            sdmmc->is_clk_running = false;
            sdmmc->is_sd_clk_enabled = false;
            sdmmc->is_tuning_tap_val_set = false;
            sdmmc->use_adma = false;
            sdmmc->dma_bounce_buf = (uint8_t*)SDMMC_BOUNCE_BUFFER_ADDRESS;
            sdmmc->tap_val = 0;
            sdmmc->internal_divider = 0;
            sdmmc->bus_voltage = SDMMC_VOLTAGE_NONE;

            /* Function pointers. */
            sdmmc->sdmmc_config = sdmmc4_config;
            break;
    }

    return 1;
}

int sdmmc_init(sdmmc_t *sdmmc, SdmmcControllerNum controller, SdmmcBusVoltage bus_voltage, SdmmcBusWidth bus_width, SdmmcBusSpeed bus_speed) {
    bool is_mariko = is_soc_mariko();

    /* Initialize our controller structure. */
    if (!sdmmc_init_controller(sdmmc, controller)) {
        sdmmc_error(sdmmc, "Failed to initialize SDMMC%d", controller + 1);
        return 0;
    }

    /* Perform initial configuration steps if necessary. */
    if (!sdmmc->sdmmc_config()) {
        sdmmc_error(sdmmc, "Failed to configure controller!");
        return 0;
    }

    /* Initialize the clock status. */
    sdmmc->is_clk_running = false;

    /* Clock is enabled and out of reset. Shouldn't happen. */
    if (!is_sdmmc_clk_rst(controller) && is_sdmmc_clk_enb(controller)) {
        /* Disable the SD clock. */
        sdmmc_disable_sd_clock(sdmmc);

        /* Force a register read to refresh the clock control value. */
        sdmmc_get_sd_clock_control(sdmmc);
    }

    /* Sort out the clock's frequency. */
    uint32_t clk_freq_val = sdmmc_get_sdclk_freq(bus_speed);

    /* Start the SDMMC clock. */
    sdmmc_clk_start(controller, clk_freq_val);

    /* Update the clock status. */
    sdmmc->is_clk_running = true;

    /* Set IO_SPARE[19] (one cycle delay) */
    sdmmc->regs->io_spare |= 0x80000;

    /* Clear SEL_VREG */
    sdmmc->regs->vendor_io_trim_cntrl &= ~(0x04);

    /* Configure vendor clocking. */
    sdmmc_vendor_clock_cntrl_config(sdmmc);

    /* Set slew codes for SDMMC1 (Erista only). */
    if ((controller == SDMMC_1) && !is_mariko) {
        volatile tegra_padctl_t *padctl = padctl_get_regs();
        uint32_t value = padctl->sdmmc1_pad_cfgpadctrl;
        value &= ~(SDMMC1_CLK_CFG_CAL_DRVDN_SLWR_MASK | SDMMC1_CLK_CFG_CAL_DRVDN_SLWF_MASK);
        value |= (0x01 << SDMMC1_CLK_CFG_CAL_DRVDN_SLWR_SHIFT);
        value |= (0x01 << SDMMC1_CLK_CFG_CAL_DRVDN_SLWF_SHIFT);
        padctl->sdmmc1_pad_cfgpadctrl = value;
    }

    /* Set vref sel. */
    sdmmc->regs->sdmemcomppadctrl &= 0x0F;
    if ((controller == SDMMC_1) && is_mariko) {
        sdmmc->regs->sdmemcomppadctrl |= 0x00;
    } else {
        sdmmc->regs->sdmemcomppadctrl |= 0x07;
    }

    /* Configure autocal offsets. */
    if (!sdmmc_autocal_config(sdmmc, bus_voltage)) {
        sdmmc_error(sdmmc, "Failed to configure automatic calibration!");
        return 0;
    }

    /* Do autocal. */
    sdmmc_autocal_run(sdmmc, bus_voltage);

    /* Enable the internal clock. */
    if (!sdmmc_int_clk_enable(sdmmc)) {
        sdmmc_error(sdmmc, "Failed to enable the internal clock!");
        return 0;
    }

    /* Select the desired bus width. */
    sdmmc_select_bus_width(sdmmc, bus_width);

    /* Select the desired voltage. */
    sdmmc_select_voltage(sdmmc, bus_voltage);

    /* Enable the internal clock. */
    if (!sdmmc_select_speed(sdmmc, bus_speed)) {
        sdmmc_error(sdmmc, "Failed to apply the correct bus speed!");
        return 0;
    }

    /* Correct any inconsistent states. */
    sdmmc_adjust_sd_clock(sdmmc);

    /* Enable the SD clock. */
    sdmmc_enable_sd_clock(sdmmc);

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    return 1;
}

void sdmmc_finish(sdmmc_t *sdmmc) {
    /* Stop everything. */
    if (sdmmc->is_clk_running) {
        /* Disable the SD clock. */
        sdmmc_disable_sd_clock(sdmmc);

        /* Disable SDMMC power. */
        sdmmc_select_voltage(sdmmc, SDMMC_VOLTAGE_NONE);

        /* Disable the SD card power. */
        if (sdmmc->controller == SDMMC_1) {
            /* Disable GPIO output. */
            gpio_configure_direction(GPIO_MICROSD_SUPPLY_ENABLE, GPIO_DIRECTION_INPUT);

            /* Power cycle for 100ms without power. */
            mdelay(100);

            /* Disable the regulator. */
            max77620_regulator_enable(REGULATOR_LDO2, 0);
        }

        /* Force a register read to refresh the clock control value. */
        sdmmc_get_sd_clock_control(sdmmc);

        /* Stop the SDMMC clock. */
        sdmmc_clk_stop(sdmmc->controller);

        /* Clock is no longer running by now. */
        sdmmc->is_clk_running = false;
    }
}

static void sdmmc_do_sw_reset(sdmmc_t *sdmmc) {
    /* Assert a software reset. */
    sdmmc->regs->software_reset |= (TEGRA_MMC_SWRST_SW_RESET_FOR_CMD_LINE | TEGRA_MMC_SWRST_SW_RESET_FOR_DAT_LINE);

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Program a timeout of 100ms. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    /* Wait for the register to be cleared. */
    while ((sdmmc->regs->software_reset & (TEGRA_MMC_SWRST_SW_RESET_FOR_CMD_LINE | TEGRA_MMC_SWRST_SW_RESET_FOR_DAT_LINE)) && !is_timeout) {
        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 100000);
    }
}

static int sdmmc_wait_for_inhibit(sdmmc_t *sdmmc, bool wait_for_dat) {
    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Program a timeout of 10ms. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    /* Wait on CMD inhibit to be cleared. */
    while ((sdmmc->regs->present_state & TEGRA_MMC_PRNSTS_CMD_INHIBIT_CMD) && !is_timeout) {
        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 10000);
    }

    /* Bit was never released. Reset. */
    if (is_timeout) {
        sdmmc_do_sw_reset(sdmmc);
        return 0;
    }

    if (wait_for_dat) {
        /* Program a timeout of 10ms. */
        timebase = get_time();
        is_timeout = false;

        /* Wait on DAT inhibit to be cleared. */
        while ((sdmmc->regs->present_state & TEGRA_MMC_PRNSTS_CMD_INHIBIT_DAT) && !is_timeout) {
            /* Keep checking if timeout expired. */
            is_timeout = (get_time_since(timebase) > 10000);
        }

        /* Bit was never released, reset. */
        if (is_timeout) {
            sdmmc_do_sw_reset(sdmmc);
            return 0;
        }
    }

    return 1;
}

static int sdmmc_wait_busy(sdmmc_t *sdmmc) {
    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Program a timeout of 10ms. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    /* Wait on DAT0 level mask to be set. */
    while (!(sdmmc->regs->present_state & SDHCI_DATA_0_LVL_MASK) && !is_timeout) {
        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 10000);
    }

    /* Bit was never released. Reset. */
    if (is_timeout) {
        sdmmc_do_sw_reset(sdmmc);
        return 0;
    }

    return 1;
}

static void sdmmc_intr_enable(sdmmc_t *sdmmc) {
    /* Enable the relevant interrupts and set all error bits. */
    sdmmc->regs->int_enable |= (TEGRA_MMC_NORINTSTSEN_CMD_COMPLETE | TEGRA_MMC_NORINTSTSEN_XFER_COMPLETE | TEGRA_MMC_NORINTSTSEN_DMA_INTERRUPT);
    sdmmc->regs->int_enable |= 0x017F0000;

    /* Refresh status. */
    sdmmc->regs->int_status = sdmmc->regs->int_status;
}

static void sdmmc_intr_disable(sdmmc_t *sdmmc) {
    /* Clear all error bits and disable the relevant interrupts. */
    sdmmc->regs->int_enable &= ~(0x017F0000);
    sdmmc->regs->int_enable &= ~(TEGRA_MMC_NORINTSTSEN_CMD_COMPLETE | TEGRA_MMC_NORINTSTSEN_XFER_COMPLETE | TEGRA_MMC_NORINTSTSEN_DMA_INTERRUPT);
}

static int sdmmc_intr_check(sdmmc_t *sdmmc, uint16_t *status_out, uint16_t status_mask) {
    uint32_t int_status = sdmmc->regs->int_status;

    sdmmc_debug(sdmmc, "INTSTS: %08X", int_status);

    /* Return the status, if necessary. */
    if (status_out) {
        *status_out = (int_status & 0xFFFF);
    }

    if (int_status & TEGRA_MMC_NORINTSTS_ERR_INTERRUPT) {
        /* Acknowledge error by refreshing status. */
        sdmmc->regs->int_status = int_status;
        return -1;
    } else if (int_status & status_mask) {
        /* Mask the status. */
        sdmmc->regs->int_status = (int_status & status_mask);
        return 1;
    }

    return 0;
}

static int sdmmc_dma_init(sdmmc_t *sdmmc, sdmmc_request_t *req) {
    /* Invalid block count or size. */
    if (!req->blksz || !req->num_blocks) {
        sdmmc_error(sdmmc, "Empty DMA request!");
        return 0;
    }

    uint32_t blkcnt = req->num_blocks;

    /* Truncate block count. Length can't be over 65536 bytes. */
    if (blkcnt >= 0xFFFF) {
        blkcnt = 0xFFFF;
    }

    /* Use our bounce buffer for SDMA or the request data buffer for ADMA. */
    uint32_t dma_base_addr = sdmmc->use_adma ? (uint32_t)req->data : (uint32_t)sdmmc->dma_bounce_buf;

    /* DMA buffer address must be aligned to 4 bytes. */
    if ((4 - (dma_base_addr & 0x03)) & 0x03) {
        sdmmc_error(sdmmc, "Invalid DMA request data buffer: 0x%08X", dma_base_addr);
        return 0;
    }

    /* Write our address to the registers. */
    if (sdmmc->use_adma) {
        /* Set ADMA registers. */
        sdmmc->regs->adma_address = dma_base_addr;
        sdmmc->regs->upper_adma_address = 0;
    } else {
        /* Set SDMA register. */
        sdmmc->regs->dma_address = dma_base_addr;
    }

    /* Store the next DMA block address for updating. */
    sdmmc->next_dma_addr = ((dma_base_addr + 0x80000) & 0xFFF80000);

    /* Set the block size ORed with the DMA boundary mask. */
    sdmmc->regs->block_size = req->blksz | 0x7000;

    /* Set the block count. */
    sdmmc->regs->block_count = blkcnt;

    /* Select basic DMA transfer mode. */
    uint32_t transfer_mode = TEGRA_MMC_TRNMOD_DMA_ENABLE;

    /* Select multi block. */
    if (req->is_multi_block) {
        transfer_mode |= (TEGRA_MMC_TRNMOD_MULTI_BLOCK_SELECT | TEGRA_MMC_TRNMOD_BLOCK_COUNT_ENABLE);
    }

    /* Select read mode. */
    if (req->is_read) {
        transfer_mode |= TEGRA_MMC_TRNMOD_DATA_XFER_DIR_SEL_READ;
    }

    /* Select AUTO_CMD12. */
    if (req->is_auto_cmd12) {
        transfer_mode &= ~(TEGRA_MMC_TRNMOD_AUTO_CMD12 & TEGRA_MMC_TRNMOD_AUTO_CMD23);
        transfer_mode |= TEGRA_MMC_TRNMOD_AUTO_CMD12;
    }

    /* Set the transfer mode in the register. */
    sdmmc->regs->transfer_mode = transfer_mode;

    return blkcnt;
}

static int sdmmc_dma_update(sdmmc_t *sdmmc) {
    uint16_t blkcnt = 0;

    /* Loop until all blocks have been consumed. */
    do {
        /* Update block count. */
        blkcnt = sdmmc->regs->block_count;

        /* Program a large timeout. */
        uint32_t timebase = get_time();
        bool is_timeout = false;

        /* Watch over the DMA transfer. */
        while (!is_timeout) {
            /* Check interrupts. */
            uint16_t intr_status = 0;
            int intr_res = sdmmc_intr_check(sdmmc, &intr_status, TEGRA_MMC_NORINTSTS_XFER_COMPLETE | TEGRA_MMC_NORINTSTS_DMA_INTERRUPT);

            /* An error has been raised. Reset. */
            if (intr_res < 0) {
                sdmmc_do_sw_reset(sdmmc);
                return 0;
            }

            /* Transfer is over. */
            if (intr_status & TEGRA_MMC_NORINTSTS_XFER_COMPLETE) {
                return 1;
            }

            /* We have a DMA interrupt. Restart the transfer where it was interrupted. */
            if (intr_status & TEGRA_MMC_NORINTSTS_DMA_INTERRUPT) {
                if (sdmmc->use_adma) {
                    /* Update ADMA registers. */
                    sdmmc->regs->adma_address = sdmmc->next_dma_addr;
                    sdmmc->regs->upper_adma_address = 0;
                } else {
                    /* Update SDMA register. */
                    sdmmc->regs->dma_address = sdmmc->next_dma_addr;
                }

                sdmmc->next_dma_addr += 0x80000;
            }

            /* Keep checking if timeout expired. */
            is_timeout = (get_time_since(timebase) > 2000000);
        }
    } while (sdmmc->regs->block_count < blkcnt);

    /* Should never get here. Reset. */
    sdmmc_do_sw_reset(sdmmc);
    return 0;
}

static void sdmmc_set_cmd_flags(sdmmc_t *sdmmc, sdmmc_command_t *cmd, bool is_dma) {
    uint16_t cmd_reg_flags = 0;

    /* Select length flags based on response type. */
    if (!(cmd->flags & SDMMC_RSP_PRESENT)) {
        cmd_reg_flags = TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_NO_RESPONSE;
    } else if (cmd->flags & SDMMC_RSP_136) {
        cmd_reg_flags = TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_136;
    } else if (cmd->flags & SDMMC_RSP_BUSY) {
        cmd_reg_flags = TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48_BUSY;
    } else {
        cmd_reg_flags = TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48;
    }

    /* Select CRC flag based on response type. */
    if (cmd->flags & SDMMC_RSP_CRC) {
        cmd_reg_flags |= TEGRA_MMC_TRNMOD_CMD_CRC_CHECK;
    }

    /* Select opcode flag based on response type. */
    if (cmd->flags & SDMMC_RSP_OPCODE) {
        cmd_reg_flags |= TEGRA_MMC_TRNMOD_CMD_INDEX_CHECK;
    }

    /* Select data present flag. */
    if (is_dma) {
        cmd_reg_flags |= TEGRA_MMC_TRNMOD_DATA_PRESENT_SELECT_DATA_TRANSFER;
    }

    /* Set the CMD's argument, opcode and flags. */
    sdmmc->regs->argument = cmd->arg;
    sdmmc->regs->command = ((cmd->opcode << 8) | cmd_reg_flags);
}

static int sdmmc_wait_for_cmd(sdmmc_t *sdmmc) {
    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Program a large timeout. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    /* Set this for error checking. */
    bool is_err = false;

    /* Wait for CMD to finish. */
    while (!is_err && !is_timeout) {
        /* Check interrupts. */
        int intr_res = sdmmc_intr_check(sdmmc, 0, TEGRA_MMC_NORINTSTS_CMD_COMPLETE);

        /* Command is done. */
        if (intr_res > 0) {
            return 1;
        }

        /* Check for any raised errors. */
        is_err = (intr_res < 0);

        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 2000000);
    }

    /* Should never get here. Reset. */
    sdmmc_do_sw_reset(sdmmc);
    return 0;
}

static int sdmmc_save_response(sdmmc_t *sdmmc, uint32_t flags) {
    /* We have a valid response. */
    if (flags & SDMMC_RSP_PRESENT) {
        if (flags & SDMMC_RSP_136) {
            /* CRC is stripped so we need to do some shifting. */
            for (int i = 0; i < 4; i++) {
                sdmmc->resp[i] = (sdmmc->regs->response[3 - i] << 0x08);

                if (i != 0) {
                    sdmmc->resp[i - 1] |= ((sdmmc->regs->response[3 - i] >> 24) & 0xFF);
                }
            }
        } else {
            /* Card is still busy. */
            if (flags & SDMMC_RSP_BUSY) {
                /* Wait for DAT0 level mask. */
                if (!sdmmc_wait_busy(sdmmc)) {
                    return 0;
                }
            }

            /* Save our response. */
            sdmmc->resp[0] = sdmmc->regs->response[0];
        }

        return 1;
    }

    /* Invalid response. */
    return 0;
}

int sdmmc_load_response(sdmmc_t *sdmmc, uint32_t flags, uint32_t *resp) {
    /* Make sure our output buffer is valid. */
    if (!resp) {
        return 0;
    }

    /* We have a valid response. */
    if (flags & SDMMC_RSP_PRESENT) {
        if (flags & SDMMC_RSP_136) {
            resp[0] = sdmmc->resp[0];
            resp[1] = sdmmc->resp[1];
            resp[2] = sdmmc->resp[2];
            resp[3] = sdmmc->resp[3];
        } else {
            resp[0] = sdmmc->resp[0];
        }

        return 1;
    }

    /* Invalid response. */
    return 0;
}

int sdmmc_send_cmd(sdmmc_t *sdmmc, sdmmc_command_t *cmd, sdmmc_request_t *req, uint32_t *num_blocks_out) {
    uint32_t cmd_result = 0;
    bool shutdown_sd_clock = false;
    bool is_mariko = is_soc_mariko();

    /* Run automatic calibration on each command submission for SDMMC1 (Erista only). */
    if ((sdmmc->controller == SDMMC_1) && !(sdmmc->has_sd) && !is_mariko) {
        sdmmc_autocal_run(sdmmc, sdmmc->bus_voltage);
    }

    /* SD clock is disabled. Enable it. */
    if (!sdmmc->is_sd_clk_enabled) {
        shutdown_sd_clock = true;
        sdmmc_enable_sd_clock(sdmmc);

        /* Force a register read to refresh the clock control value. */
        sdmmc_get_sd_clock_control(sdmmc);

        /* Provide 8 clock cycles after enabling the clock. */
        udelay((8000 + sdmmc->internal_divider - 1) / sdmmc->internal_divider);
    }

    /* Determine if we should wait for data inhibit. */
    bool wait_for_dat = (req || (cmd->flags & SDMMC_RSP_BUSY));

    /* Wait for CMD and DAT inhibit. */
    if (!sdmmc_wait_for_inhibit(sdmmc, wait_for_dat)) {
        return 0;
    }

    uint32_t dma_blkcnt = 0;
    bool is_dma = false;

    /* This is a data transfer. */
    if (req) {
        is_dma = true;
        dma_blkcnt = sdmmc_dma_init(sdmmc, req);

        /* Abort in case initialization failed. */
        if (!dma_blkcnt) {
            sdmmc_error(sdmmc, "Failed to initialize the DMA transfer!");
            return 0;
        }

        /* If this is a SDMA write operation, copy the data into our bounce buffer. */
        if (!sdmmc->use_adma && !req->is_read) {
            memcpy((void *)sdmmc->dma_bounce_buf, (void *)req->data, req->blksz * req->num_blocks);
        }
    }

    /* Enable interrupts. */
    sdmmc_intr_enable(sdmmc);

    /* Parse and set the CMD's flags. */
    sdmmc_set_cmd_flags(sdmmc, cmd, is_dma);

    /* Wait for the CMD to finish. */
    cmd_result = sdmmc_wait_for_cmd(sdmmc);

    sdmmc_debug(sdmmc, "CMD(%d): %08X, %08X, %08X, %08X", cmd_result, sdmmc->regs->response[0], sdmmc->regs->response[1], sdmmc->regs->response[2], sdmmc->regs->response[3]);

    if (cmd_result) {
        /* Save response, if necessary. */
        sdmmc_save_response(sdmmc, cmd->flags);

        /* Update the DMA request. */
        if (req) {
            /* Disable interrupts and abort in case updating failed. */
            if (!sdmmc_dma_update(sdmmc)) {
                sdmmc_warn(sdmmc, "Failed to update the DMA transfer!");
                sdmmc_intr_disable(sdmmc);
                return 0;
            }

            /* If this is a SDMA read operation, copy the data from our bounce buffer. */
            if (!sdmmc->use_adma && req->is_read) {
                uint32_t dma_data_size = (sdmmc->regs->dma_address - (uint32_t)sdmmc->dma_bounce_buf);
                memcpy((void *)req->data, (void *)sdmmc->dma_bounce_buf, dma_data_size);
            }
        }
    }

    /* Disable interrupts. */
    sdmmc_intr_disable(sdmmc);

    if (cmd_result) {
        if (req) {
            /* Save back the number of DMA blocks. */
            if (num_blocks_out) {
                *num_blocks_out = dma_blkcnt;
            }
            /* Save the response for AUTO_CMD12. */
            if (req->is_auto_cmd12) {
                sdmmc->resp_auto_cmd12 = sdmmc->regs->response[3];
            }
        }

        /* Wait for DAT0 to be 0. */
        if (req || (cmd->flags & SDMMC_RSP_BUSY)) {
            cmd_result = sdmmc_wait_busy(sdmmc);
        }
    }

    /* Provide 8 clock cycles before disabling the clock. */
    udelay((8000 + sdmmc->internal_divider - 1) / sdmmc->internal_divider);

    if (shutdown_sd_clock) {
        sdmmc_disable_sd_clock(sdmmc);
    }

    return cmd_result;
}

int sdmmc_switch_voltage(sdmmc_t *sdmmc) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();

    /* Disable the SD clock. */
    sdmmc_disable_sd_clock(sdmmc);

    /* Reconfigure the internal clock. */
    if (!sdmmc_select_speed(sdmmc, SDMMC_SPEED_SD_SDR12)) {
        sdmmc_error(sdmmc, "Failed to apply the correct bus speed for low voltage support!");
        return 0;
    }

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Reconfigure the regulator. */
    max77620_regulator_set_voltage(REGULATOR_LDO2, 1800000);
    max77620_regulator_enable(REGULATOR_LDO2, 1);
    udelay(150);
    pmc->pwr_det_val &= ~(PMC_CONTROL_SDMMC1);

    /* Reconfigure autocal offsets. */
    if (!sdmmc_autocal_config(sdmmc, SDMMC_VOLTAGE_1V8)) {
        sdmmc_error(sdmmc, "Failed to configure automatic calibration for low voltage support!");
        return 0;
    }

    /* Do autocal again. */
    sdmmc_autocal_run(sdmmc, SDMMC_VOLTAGE_1V8);

    /* Change the desired voltage. */
    sdmmc_select_voltage(sdmmc, SDMMC_VOLTAGE_1V8);

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Wait a while. */
    mdelay(5);

    /* Host control 2 flag should be set by now. */
    if (sdmmc->regs->host_control2 & SDHCI_CTRL_VDD_180) {
        /* Enable the SD clock. */
        sdmmc_enable_sd_clock(sdmmc);

        /* Force a register read to refresh the clock control value. */
        sdmmc_get_sd_clock_control(sdmmc);

        /* Wait a while. */
        mdelay(1);

        /* Data level is up. Voltage switching is done.*/
        if (sdmmc->regs->present_state & SDHCI_DATA_LVL_MASK) {
            return 1;
        }
    }

    return 0;
}

static int sdmmc_send_tuning(sdmmc_t *sdmmc, uint32_t opcode) {
    /* Nothing to do. */
    if (!sdmmc->has_sd) {
        return 0;
    }

    /* Wait for CMD and DAT inhibit. */
    if (!sdmmc_wait_for_inhibit(sdmmc, true)) {
        return 0;
    }

    /* Select the right size for sending the tuning block. */
    if (sdmmc->bus_width == SDMMC_BUS_WIDTH_4BIT) {
        sdmmc->regs->block_size = 0x40;
    } else if (sdmmc->bus_width == SDMMC_BUS_WIDTH_8BIT) {
        sdmmc->regs->block_size = 0x80;
    } else {
        return 0;
    }

    /* Select the block count and transfer mode. */
    sdmmc->regs->block_count = 1;
    sdmmc->regs->transfer_mode = TEGRA_MMC_TRNMOD_DATA_XFER_DIR_SEL_READ;

    /* Manually enable the Buffer Read Ready interrupt. */
    sdmmc->regs->int_enable |= TEGRA_MMC_NORINTSTSEN_BUFFER_READ_READY;

    /* Refresh status. */
    sdmmc->regs->int_status = sdmmc->regs->int_status;

    /* Disable the SD clock. */
    sdmmc_disable_sd_clock(sdmmc);

    /* Prepare the tuning command. */
    sdmmc_command_t cmd = {};
    cmd.opcode = opcode;
    cmd.arg = 0;
    cmd.flags = SDMMC_RSP_R1;

    /* Parse and set the CMD's flags. */
    sdmmc_set_cmd_flags(sdmmc, &cmd, true);

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Wait a while. */
    udelay(1);

    /* Reset. */
    sdmmc_do_sw_reset(sdmmc);

    /* Enable back the SD clock. */
    sdmmc_enable_sd_clock(sdmmc);

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Program a 50ms timeout. */
    uint32_t timebase = get_time();
    bool is_timeout = false;

    /* Wait for Buffer Read Ready interrupt. */
    while (!is_timeout) {
        /* Buffer Read Ready was asserted. */
        if (sdmmc_intr_check(sdmmc, 0, TEGRA_MMC_NORINTSTSEN_BUFFER_READ_READY) > 0) {
            /* Manually disable the Buffer Read Ready interrupt. */
            sdmmc->regs->int_enable &= ~(TEGRA_MMC_NORINTSTSEN_BUFFER_READ_READY);

            /* Force a register read to refresh the clock control value. */
            sdmmc_get_sd_clock_control(sdmmc);

            /* Provide 8 clock cycles. */
            udelay((8000 + sdmmc->internal_divider - 1) / sdmmc->internal_divider);

            return 1;
        }

        /* Keep checking if timeout expired. */
        is_timeout = (get_time_since(timebase) > 5000);
    }

    /* Reset. */
    sdmmc_do_sw_reset(sdmmc);

    /* Manually disable the Buffer Read Ready interrupt. */
    sdmmc->regs->int_enable &= ~(TEGRA_MMC_NORINTSTSEN_BUFFER_READ_READY);

    /* Force a register read to refresh the clock control value. */
    sdmmc_get_sd_clock_control(sdmmc);

    /* Provide 8 clock cycles. */
    udelay((8000 + sdmmc->internal_divider - 1) / sdmmc->internal_divider);

    return 0;
}

void sdmmc_set_tuning_tap_val(sdmmc_t *sdmmc) {
    sdmmc->tap_val = (sdmmc->regs->vendor_clock_cntrl >> 16);
    sdmmc->is_tuning_tap_val_set = true;
}

int sdmmc_execute_tuning(sdmmc_t *sdmmc, SdmmcBusSpeed bus_speed, uint32_t opcode) {
    uint32_t max_tuning_loop = 0;
    uint32_t tuning_cntrl_flag = 0;

    sdmmc->regs->vendor_tuning_cntrl1 = 0;

    switch (bus_speed) {
        case SDMMC_SPEED_MMC_HS200:
        case SDMMC_SPEED_MMC_HS400:
        case SDMMC_SPEED_SD_SDR104:
        case SDMMC_SPEED_EMU_SDR104:
            max_tuning_loop = 0x80;
            tuning_cntrl_flag = 0x4000;
            break;
        case SDMMC_SPEED_SD_SDR50:
        case SDMMC_SPEED_GC_ASIC_FPGA:
        case SDMMC_SPEED_GC_ASIC:
            max_tuning_loop = 0x100;
            tuning_cntrl_flag = 0x8000;
            break;
        default:
            return 0;
    }

    sdmmc->regs->vendor_tuning_cntrl0 &= ~(0xE000);
    sdmmc->regs->vendor_tuning_cntrl0 |= tuning_cntrl_flag;

    sdmmc->regs->vendor_tuning_cntrl0 &= ~(0x1FC0);
    sdmmc->regs->vendor_tuning_cntrl0 |= 0x40;

    sdmmc->regs->vendor_tuning_cntrl0 |= 0x20000;

    /* Start tuning. */
    sdmmc->regs->host_control2 |= SDHCI_CTRL_EXEC_TUNING;

    /* Repeat until Execute Tuning is set to 0 or the number of loops reaches the maximum value. */
    for (uint32_t i = 0; i < max_tuning_loop; i++) {
        sdmmc_send_tuning(sdmmc, opcode);

        /* Tuning is done. */
        if (!(sdmmc->regs->host_control2 & SDHCI_CTRL_EXEC_TUNING)) {
            break;
        }
    }

    /* Success! */
    if (sdmmc->regs->host_control2 & SDHCI_CTRL_TUNED_CLK) {
        return 1;
    }

    return 0;
}

int sdmmc_abort(sdmmc_t *sdmmc, uint32_t opcode) {
    uint32_t result = 0;
    uint32_t cmd_result = 0;
    bool shutdown_sd_clock = false;

    /* SD clock is disabled. Enable it. */
    if (!sdmmc->is_sd_clk_enabled) {
        shutdown_sd_clock = true;
        sdmmc_enable_sd_clock(sdmmc);

        /* Force a register read to refresh the clock control value. */
        sdmmc_get_sd_clock_control(sdmmc);

        /* Provide 8 clock cycles after enabling the clock. */
        udelay((8000 + sdmmc->internal_divider - 1) / sdmmc->internal_divider);
    }

    /* Wait for CMD and DAT inhibit. */
    if (sdmmc_wait_for_inhibit(sdmmc, false)) {
        /* Enable interrupts. */
        sdmmc_intr_enable(sdmmc);

        /* Prepare the command. */
        sdmmc_command_t cmd = {};
        cmd.opcode = opcode;
        cmd.arg = 0;
        cmd.flags = SDMMC_RSP_R1B;

        /* Parse and set the CMD's flags. */
        sdmmc_set_cmd_flags(sdmmc, &cmd, false);

        /* Wait for the CMD to finish. */
        cmd_result = sdmmc_wait_for_cmd(sdmmc);

        /* Disable interrupts. */
        sdmmc_intr_disable(sdmmc);

        if (cmd_result) {
            /* Save response, if necessary. */
            sdmmc_save_response(sdmmc, cmd.flags);

            /* Wait for DAT0 to be 0. */
            result = sdmmc_wait_busy(sdmmc);
        }
    }

    /* Provide 8 clock cycles before disabling the clock. */
    udelay((8000 + sdmmc->internal_divider - 1) / sdmmc->internal_divider);

    /* Disable the SD clock if requested. */
    if (shutdown_sd_clock) {
        sdmmc_disable_sd_clock(sdmmc);
    }

    return result;
}