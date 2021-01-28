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
 
#ifndef FUSEE_SDMMC_TEGRA_H
#define FUSEE_SDMMC_TEGRA_H

#include <stdbool.h>
#include <stdint.h>

#define TEGRA_MMC_PWRCTL_SD_BUS_POWER                           (1 << 0)
#define TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V1_8                    (5 << 1)
#define TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V3_0                    (6 << 1)
#define TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V3_3                    (7 << 1)

#define TEGRA_MMC_HOSTCTL_DMASEL_MASK                           (3 << 3)
#define TEGRA_MMC_HOSTCTL_DMASEL_SDMA                           (0 << 3)
#define TEGRA_MMC_HOSTCTL_DMASEL_ADMA2_32BIT                    (2 << 3)
#define TEGRA_MMC_HOSTCTL_DMASEL_ADMA2_64BIT                    (3 << 3)

#define TEGRA_MMC_TRNMOD_DMA_ENABLE                             (1 << 0)
#define TEGRA_MMC_TRNMOD_BLOCK_COUNT_ENABLE                     (1 << 1)
#define TEGRA_MMC_TRNMOD_AUTO_CMD12                             (1 << 2)
#define TEGRA_MMC_TRNMOD_AUTO_CMD23                             (1 << 3)
#define TEGRA_MMC_TRNMOD_DATA_XFER_DIR_SEL_WRITE                (0 << 4)
#define TEGRA_MMC_TRNMOD_DATA_XFER_DIR_SEL_READ                 (1 << 4)
#define TEGRA_MMC_TRNMOD_MULTI_BLOCK_SELECT                     (1 << 5)

#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_MASK                  (3 << 0)
#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_NO_RESPONSE           (0 << 0)
#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_136            (1 << 0)
#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48             (2 << 0)
#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48_BUSY        (3 << 0)

#define TEGRA_MMC_TRNMOD_CMD_CRC_CHECK                          (1 << 3)
#define TEGRA_MMC_TRNMOD_CMD_INDEX_CHECK                        (1 << 4)
#define TEGRA_MMC_TRNMOD_DATA_PRESENT_SELECT_DATA_TRANSFER      (1 << 5)

#define TEGRA_MMC_PRNSTS_CMD_INHIBIT_CMD                        (1 << 0)
#define TEGRA_MMC_PRNSTS_CMD_INHIBIT_DAT                        (1 << 1)

#define TEGRA_MMC_CLKCON_INTERNAL_CLOCK_ENABLE                  (1 << 0)
#define TEGRA_MMC_CLKCON_INTERNAL_CLOCK_STABLE                  (1 << 1)
#define TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE                        (1 << 2)
#define TEGRA_MMC_CLKCON_PROG_CLOCK_MODE                        (1 << 5)

#define TEGRA_MMC_CLKCON_SDCLK_FREQ_SEL_SHIFT                   8
#define TEGRA_MMC_CLKCON_SDCLK_FREQ_SEL_MASK                    (0xff << 8)

#define TEGRA_MMC_SWRST_SW_RESET_FOR_ALL                        (1 << 0)
#define TEGRA_MMC_SWRST_SW_RESET_FOR_CMD_LINE                   (1 << 1)
#define TEGRA_MMC_SWRST_SW_RESET_FOR_DAT_LINE                   (1 << 2)

#define TEGRA_MMC_NORINTSTS_CMD_COMPLETE                        (1 << 0)
#define TEGRA_MMC_NORINTSTS_XFER_COMPLETE                       (1 << 1)
#define TEGRA_MMC_NORINTSTS_DMA_INTERRUPT                       (1 << 3)
#define TEGRA_MMC_NORINTSTS_ERR_INTERRUPT                       (1 << 15)
#define TEGRA_MMC_NORINTSTS_CMD_TIMEOUT                         (1 << 16)

#define TEGRA_MMC_NORINTSTSEN_CMD_COMPLETE                      (1 << 0)
#define TEGRA_MMC_NORINTSTSEN_XFER_COMPLETE                     (1 << 1)
#define TEGRA_MMC_NORINTSTSEN_DMA_INTERRUPT                     (1 << 3)
#define TEGRA_MMC_NORINTSTSEN_BUFFER_WRITE_READY                (1 << 4)
#define TEGRA_MMC_NORINTSTSEN_BUFFER_READ_READY                 (1 << 5)

#define TEGRA_MMC_NORINTSIGEN_XFER_COMPLETE                     (1 << 1)

typedef struct {
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
    uint8_t adma_error;
    uint8_t _0x56[0x3];
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
    uint32_t _0x70[0x23];
    uint16_t slot_int_status;
    uint16_t host_version;

    /* Vendor specific registers */
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

    /* Start of sdmmc2/sdmmc4 only */
    uint32_t vendor_dllcal_cfg;
    uint32_t vendor_dll_ctrl0;
    uint32_t vendor_dll_ctrl1;
    uint32_t vendor_dllcal_cfg_sta;
    /* End of sdmmc2/sdmmc4 only */

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
} tegra_sdmmc_t;

static inline volatile tegra_sdmmc_t *sdmmc_get_regs(uint32_t idx)
{
    return (volatile tegra_sdmmc_t *)(0x700B0000 + (idx * 0x200));
}

#endif