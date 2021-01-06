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
 
#ifndef FUSEE_SDMMC_CORE_H
#define FUSEE_SDMMC_CORE_H

#include "sdmmc_tegra.h"

/* Bounce buffer */
#define SDMMC_BOUNCE_BUFFER_ADDRESS     0x90000000

/* Present state */
#define SDHCI_CMD_INHIBIT       0x00000001
#define SDHCI_DATA_INHIBIT      0x00000002
#define SDHCI_DOING_WRITE       0x00000100
#define SDHCI_DOING_READ        0x00000200
#define SDHCI_SPACE_AVAILABLE   0x00000400
#define SDHCI_DATA_AVAILABLE    0x00000800
#define SDHCI_CARD_PRESENT      0x00010000
#define SDHCI_WRITE_PROTECT     0x00080000
#define SDHCI_DATA_LVL_MASK     0x00F00000
#define SDHCI_DATA_LVL_SHIFT    20
#define SDHCI_DATA_0_LVL_MASK   0x00100000
#define SDHCI_CMD_LVL           0x01000000

/* SDHCI clock control */
#define SDHCI_DIVIDER_SHIFT     8
#define SDHCI_DIVIDER_HI_SHIFT  6
#define SDHCI_DIV_MASK          0xFF
#define SDHCI_DIV_MASK_LEN      8
#define SDHCI_DIV_HI_MASK       0x300
#define SDHCI_PROG_CLOCK_MODE   0x0020
#define SDHCI_CLOCK_CARD_EN     0x0004
#define SDHCI_CLOCK_INT_STABLE  0x0002
#define SDHCI_CLOCK_INT_EN      0x0001

/* SDHCI host control */
#define SDHCI_CTRL_LED          0x01
#define SDHCI_CTRL_4BITBUS      0x02
#define SDHCI_CTRL_HISPD        0x04
#define SDHCI_CTRL_DMA_MASK     0x18
#define SDHCI_CTRL_SDMA         0x00
#define SDHCI_CTRL_ADMA1        0x08
#define SDHCI_CTRL_ADMA32       0x10
#define SDHCI_CTRL_ADMA64       0x18
#define SDHCI_CTRL_8BITBUS      0x20
#define SDHCI_CTRL_CDTEST_INS   0x40
#define SDHCI_CTRL_CDTEST_EN    0x80

/* SDHCI host control 2 */
#define SDHCI_CTRL_UHS_MASK             0x0007
#define SDHCI_CTRL_UHS_SDR12            0x0000
#define SDHCI_CTRL_UHS_SDR25            0x0001
#define SDHCI_CTRL_UHS_SDR50            0x0002
#define SDHCI_CTRL_UHS_SDR104           0x0003
#define SDHCI_CTRL_UHS_DDR50            0x0004
#define SDHCI_CTRL_HS400                0x0005
#define SDHCI_CTRL_VDD_180              0x0008
#define SDHCI_CTRL_DRV_TYPE_MASK        0x0030
#define SDHCI_CTRL_DRV_TYPE_B           0x0000
#define SDHCI_CTRL_DRV_TYPE_A           0x0010
#define SDHCI_CTRL_DRV_TYPE_C           0x0020
#define SDHCI_CTRL_DRV_TYPE_D           0x0030
#define SDHCI_CTRL_EXEC_TUNING          0x0040
#define SDHCI_CTRL_TUNED_CLK            0x0080
#define SDHCI_UHS2_IF_EN                0x0100
#define SDHCI_HOST_VERSION_4_EN         0x1000
#define SDHCI_ADDRESSING_64BIT_EN       0x2000
#define SDHCI_ASYNC_INTR_EN             0x4000
#define SDHCI_CTRL_PRESET_VAL_ENABLE    0x8000

/* SDHCI capabilities */
#define SDHCI_CAN_DO_8BIT   0x00040000
#define SDHCI_CAN_DO_ADMA2  0x00080000
#define SDHCI_CAN_DO_ADMA1  0x00100000
#define SDHCI_CAN_DO_HISPD  0x00200000
#define SDHCI_CAN_DO_SDMA   0x00400000
#define SDHCI_CAN_VDD_330   0x01000000
#define SDHCI_CAN_VDD_300   0x02000000
#define SDHCI_CAN_VDD_180   0x04000000
#define SDHCI_CAN_64BIT     0x10000000
#define SDHCI_ASYNC_INTR    0x20000000

/* Vendor clock control */
#define SDMMC_CLOCK_TAP_MASK                    (0xFF << 16)
#define SDMMC_CLOCK_TAP_SDMMC1                  (0x04 << 16)
#define SDMMC_CLOCK_TAP_SDMMC2                  (0x00 << 16)
#define SDMMC_CLOCK_TAP_SDMMC3                  (0x03 << 16)
#define SDMMC_CLOCK_TAP_SDMMC4                  (0x00 << 16)
#define SDMMC_CLOCK_TRIM_MASK                   (0xFF << 24)
#define SDMMC_CLOCK_TRIM_SDMMC1_ERISTA          (0x02 << 24)
#define SDMMC_CLOCK_TRIM_SDMMC1_MARIKO          (0x0E << 24)
#define SDMMC_CLOCK_TRIM_SDMMC2_ERISTA          (0x08 << 24)
#define SDMMC_CLOCK_TRIM_SDMMC2_MARIKO          (0x0D << 24)
#define SDMMC_CLOCK_TRIM_SDMMC3                 (0x03 << 24)
#define SDMMC_CLOCK_TRIM_SDMMC4_ERISTA          (0x08 << 24)
#define SDMMC_CLOCK_TRIM_SDMMC4_MARIKO          (0x0D << 24)
#define SDMMC_CLOCK_SPI_MODE_CLKEN_OVERRIDE     (1 << 2)
#define SDMMC_CLOCK_PADPIPE_CLKEN_OVERRIDE      (1 << 3)

/* Autocal configuration */
#define SDMMC_AUTOCAL_PDPU_CONFIG_MASK          0x7F7F
#define SDMMC_AUTOCAL_PDPU_SDMMC1_1V8_ERISTA    0x7B7B
#define SDMMC_AUTOCAL_PDPU_SDMMC1_1V8_MARIKO    0x0606
#define SDMMC_AUTOCAL_PDPU_SDMMC1_3V3_ERISTA    0x7D00
#define SDMMC_AUTOCAL_PDPU_SDMMC1_3V3_MARIKO    0x0000
#define SDMMC_AUTOCAL_PDPU_SDMMC4_1V8           0x0505
#define SDMMC_AUTOCAL_START                     (1 << 31)
#define SDMMC_AUTOCAL_ENABLE                    (1 << 29)

/* Autocal status */
#define SDMMC_AUTOCAL_ACTIVE                    (1 << 31)

/* Vendor tuning control 0*/
#define SDMMC_VENDOR_TUNING_TRIES_MASK          (0x7 << 13)
#define SDMMC_VENDOR_TUNING_TRIES_SHIFT          13
#define SDMMC_VENDOR_TUNING_MULTIPLIER_MASK     (0x7F << 6)
#define SDMMC_VENDOR_TUNING_MULTIPLIER_UNITY    (1 << 6)
#define SDMMC_VENDOR_TUNING_DIVIDER_MASK        (0x7 << 3)
#define SDMMC_VENDOR_TUNING_SET_BY_HW           (1 << 17)

/* Vendor tuning control 1*/
#define SDMMC_VENDOR_TUNING_STEP_SIZE_SDR50_DEFAULT     (0 << 0)
#define SDMMC_VENDOR_TUNING_STEP_SIZE_SDR104_DEFAULT    (0 << 4)

/* Vendor capability overrides */
#define SDMMC_VENDOR_CAPABILITY_DQS_TRIM_MASK           (0x3F << 8)
#define SDMMC_VENDOR_CAPABILITY_DQS_TRIM_HS400          (0x11 << 8)

/* Timeouts */
#define SDMMC_AUTOCAL_TIMEOUT                           (10 * 1000)
#define SDMMC_TUNING_TIMEOUT                            (150 * 1000)

/* Command response flags */
#define SDMMC_RSP_PRESENT                               (1 << 0)
#define SDMMC_RSP_136                                   (1 << 1)
#define SDMMC_RSP_CRC                                   (1 << 2)
#define SDMMC_RSP_BUSY                                  (1 << 3)
#define SDMMC_RSP_OPCODE                                (1 << 4)

/* Command types */
#define SDMMC_CMD_MASK                                  (3 << 5)
#define SDMMC_CMD_AC                                    (0 << 5)
#define SDMMC_CMD_ADTC                                  (1 << 5)
#define SDMMC_CMD_BC                                    (2 << 5)
#define SDMMC_CMD_BCR                                   (3 << 5)

/* SPI command response flags */
#define SDMMC_RSP_SPI_S1                                (1 << 7)
#define SDMMC_RSP_SPI_S2                                (1 << 8)
#define SDMMC_RSP_SPI_B4                                (1 << 9)
#define SDMMC_RSP_SPI_BUSY                              (1 << 10)

/* Native response types for commands */
#define SDMMC_RSP_NONE          (0)
#define SDMMC_RSP_R1            (SDMMC_RSP_PRESENT|SDMMC_RSP_CRC|SDMMC_RSP_OPCODE)
#define SDMMC_RSP_R1B           (SDMMC_RSP_PRESENT|SDMMC_RSP_CRC|SDMMC_RSP_OPCODE|SDMMC_RSP_BUSY)
#define SDMMC_RSP_R2            (SDMMC_RSP_PRESENT|SDMMC_RSP_136|SDMMC_RSP_CRC)
#define SDMMC_RSP_R3            (SDMMC_RSP_PRESENT)
#define SDMMC_RSP_R4            (SDMMC_RSP_PRESENT)
#define SDMMC_RSP_R5            (SDMMC_RSP_PRESENT|SDMMC_RSP_CRC|SDMMC_RSP_OPCODE)
#define SDMMC_RSP_R6            (SDMMC_RSP_PRESENT|SDMMC_RSP_CRC|SDMMC_RSP_OPCODE)
#define SDMMC_RSP_R7            (SDMMC_RSP_PRESENT|SDMMC_RSP_CRC|SDMMC_RSP_OPCODE)
#define SDMMC_RSP_R1_NO_CRC     (SDMMC_RSP_PRESENT|SDMMC_RSP_OPCODE)

/* SPI response types for commands */
#define SDMMC_RSP_SPI_R1        (SDMMC_RSP_SPI_S1)
#define SDMMC_RSP_SPI_R1B       (SDMMC_RSP_SPI_S1|SDMMC_RSP_SPI_BUSY)
#define SDMMC_RSP_SPI_R2        (SDMMC_RSP_SPI_S1|SDMMC_RSP_SPI_S2)
#define SDMMC_RSP_SPI_R3        (SDMMC_RSP_SPI_S1|SDMMC_RSP_SPI_B4)
#define SDMMC_RSP_SPI_R4        (SDMMC_RSP_SPI_S1|SDMMC_RSP_SPI_B4)
#define SDMMC_RSP_SPI_R5        (SDMMC_RSP_SPI_S1|SDMMC_RSP_SPI_S2)
#define SDMMC_RSP_SPI_R7        (SDMMC_RSP_SPI_S1|SDMMC_RSP_SPI_B4)

/* SDMMC controllers */
typedef enum {
    SDMMC_1 = 0,
    SDMMC_2 = 1,
    SDMMC_3 = 2,
    SDMMC_4 = 3
} SdmmcControllerNum;

typedef enum {
    SDMMC_PARTITION_INVALID = -1,
    SDMMC_PARTITION_USER    = 0,
    SDMMC_PARTITION_BOOT0   = 1,
    SDMMC_PARTITION_BOOT1   = 2,
    SDMMC_PARTITION_RPMB    = 3
} SdmmcPartitionNum;

typedef enum {
    SDMMC_VOLTAGE_NONE  = 0,
    SDMMC_VOLTAGE_1V8   = 1,
    SDMMC_VOLTAGE_3V3   = 2
} SdmmcBusVoltage;

typedef enum {
    SDMMC_BUS_WIDTH_1BIT  = 0,
    SDMMC_BUS_WIDTH_4BIT  = 1,
    SDMMC_BUS_WIDTH_8BIT  = 2
} SdmmcBusWidth;

typedef enum {
    SDMMC_SPEED_MMC_IDENT       = 0,
    SDMMC_SPEED_MMC_LEGACY      = 1,
    SDMMC_SPEED_MMC_HS          = 2,
    SDMMC_SPEED_MMC_HS200       = 3,
    SDMMC_SPEED_MMC_HS400       = 4,
    SDMMC_SPEED_SD_IDENT        = 5,
    SDMMC_SPEED_SD_DS           = 6,
    SDMMC_SPEED_SD_HS           = 7,
    SDMMC_SPEED_SD_SDR12        = 8,
    SDMMC_SPEED_SD_SDR25        = 9,
    SDMMC_SPEED_SD_SDR50        = 10,
    SDMMC_SPEED_SD_SDR104       = 11,
    SDMMC_SPEED_SD_DDR50        = 12,
    SDMMC_SPEED_GC_ASIC_FPGA    = 13,
    SDMMC_SPEED_GC_ASIC         = 14,
    SDMMC_SPEED_EMU_SDR104      = 255, /* Custom speed mode. Prevents low voltage switch in MMC emulation. */
} SdmmcBusSpeed;

typedef enum {
    SDMMC_CAR_DIVIDER_MMC_LEGACY        = 30, /* (16 * 2) - 2 */
    SDMMC_CAR_DIVIDER_MMC_HS            = 14, /* (8 * 2) - 2 */
    SDMMC_CAR_DIVIDER_MMC_HS200         = 3,  /* (2.5 * 2) - 2 (for PLLP_OUT0, same as HS400) */
    SDMMC_CAR_DIVIDER_SD_SDR12          = 31, /* (16.5 * 2) - 2 */
    SDMMC_CAR_DIVIDER_SD_SDR25          = 15, /* (8.5 * 2) - 2 */
    SDMMC_CAR_DIVIDER_SD_SDR50          = 7,  /* (4.5 * 2) - 2 */
    SDMMC_CAR_DIVIDER_SD_SDR104         = 2,  /* (2 * 2) - 2 */
    SDMMC_CAR_DIVIDER_GC_ASIC_FPGA      = 18, /* (5 * 2 * 2) - 2 */
} SdmmcCarDivider;

/* Structure for describing a SDMMC device. */
typedef struct {
    /* Controller number */
    SdmmcControllerNum controller;
    
    /* Backing register space */
    volatile tegra_sdmmc_t *regs;
    
    /* Controller properties */
    const char *name;
    bool has_sd;
    bool is_clk_running;
    bool is_sd_clk_enabled;
    bool is_tuning_tap_val_set;
    bool use_adma;
    uint32_t tap_val;
    uint32_t internal_divider;
    uint32_t resp[4];
    uint32_t resp_auto_cmd12;
    uint32_t next_dma_addr;
    uint8_t* dma_bounce_buf;
    SdmmcBusVoltage bus_voltage;
    SdmmcBusWidth bus_width;
    
    /* Per-controller operations. */
    int (*sdmmc_config)();
} sdmmc_t;

/* Structure for describing a SDMMC command. */
typedef struct {
    uint32_t    opcode;
    uint32_t    arg;
    uint32_t    resp[4];
    uint32_t    flags;      /* Expected response type. */
} sdmmc_command_t;

/* Structure for describing a SDMMC request. */
typedef struct {
    void*       data;
    uint32_t    blksz;
    uint32_t    num_blocks;
    bool        is_multi_block;
    bool        is_read;
    bool        is_auto_cmd12;
} sdmmc_request_t;

int sdmmc_init(sdmmc_t *sdmmc, SdmmcControllerNum controller, SdmmcBusVoltage bus_voltage, SdmmcBusWidth bus_width, SdmmcBusSpeed bus_speed);
void sdmmc_finish(sdmmc_t *sdmmc);
int sdmmc_select_speed(sdmmc_t *sdmmc, SdmmcBusSpeed bus_speed);
void sdmmc_select_bus_width(sdmmc_t *sdmmc, SdmmcBusWidth width);
void sdmmc_select_voltage(sdmmc_t *sdmmc, SdmmcBusVoltage voltage);
void sdmmc_adjust_sd_clock(sdmmc_t *sdmmc);
int sdmmc_switch_voltage(sdmmc_t *sdmmc);
void sdmmc_set_tuning_tap_val(sdmmc_t *sdmmc);
int sdmmc_execute_tuning(sdmmc_t *sdmmc, SdmmcBusSpeed bus_speed, uint32_t opcode);
int sdmmc_send_cmd(sdmmc_t *sdmmc, sdmmc_command_t *cmd, sdmmc_request_t *req, uint32_t *num_blocks_out);
int sdmmc_load_response(sdmmc_t *sdmmc, uint32_t flags, uint32_t *resp);
int sdmmc_abort(sdmmc_t *sdmmc, uint32_t opcode);
void sdmmc_error(sdmmc_t *sdmmc, char *fmt, ...);
void sdmmc_warn(sdmmc_t *sdmmc, char *fmt, ...);
void sdmmc_info(sdmmc_t *sdmmc, char *fmt, ...);
void sdmmc_debug(sdmmc_t *sdmmc, char *fmt, ...);
void sdmmc_dump_regs(sdmmc_t *sdmmc);

#endif
