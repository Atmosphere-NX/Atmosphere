/*
 * Copyright (c) 2018 naehrwert
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

#ifndef _SDMMC_T210_H_
#define _SDMMC_T210_H_

#include "../utils/types.h"

#define TEGRA_MMC_PWRCTL_SD_BUS_POWER 0x1
#define TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V1_8 0xA
#define TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V3_0 0xC
#define TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V3_3 0xE
#define TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_MASK 0xF1

#define TEGRA_MMC_HOSTCTL_1BIT 0x00
#define TEGRA_MMC_HOSTCTL_4BIT 0x02
#define TEGRA_MMC_HOSTCTL_8BIT 0x20

#define TEGRA_MMC_CLKCON_INTERNAL_CLOCK_ENABLE 0x1
#define TEGRA_MMC_CLKCON_INTERNAL_CLOCK_STABLE 0x2
#define TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE 0x4
#define TEGRA_MMC_CLKCON_CLKGEN_SELECT 0x20

#define TEGRA_MMC_SWRST_SW_RESET_FOR_ALL 0x1
#define TEGRA_MMC_SWRST_SW_RESET_FOR_CMD_LINE 0x2
#define TEGRA_MMC_SWRST_SW_RESET_FOR_DAT_LINE 0x4

#define TEGRA_MMC_TRNMOD_DMA_ENABLE 0x1
#define TEGRA_MMC_TRNMOD_BLOCK_COUNT_ENABLE 0x2
#define TEGRA_MMC_TRNMOD_AUTO_CMD12 0x4
#define TEGRA_MMC_TRNMOD_DATA_XFER_DIR_SEL_WRITE 0x0
#define TEGRA_MMC_TRNMOD_DATA_XFER_DIR_SEL_READ 0x10
#define TEGRA_MMC_TRNMOD_MULTI_BLOCK_SELECT 0x20

#define TEGRA_MMC_TRNMOD_CMD_CRC_CHECK 0x8
#define TEGRA_MMC_TRNMOD_CMD_INDEX_CHECK 0x10
#define TEGRA_MMC_TRNMOD_DATA_PRESENT_SELECT_DATA_TRANSFER 0x20

#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_MASK 0x3
#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_NO_RESPONSE 0x0
#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_136 0x1
#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48 0x2
#define TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48_BUSY 0x3

#define TEGRA_MMC_NORINTSTS_CMD_COMPLETE 0x1
#define TEGRA_MMC_NORINTSTS_XFER_COMPLETE 0x2
#define TEGRA_MMC_NORINTSTS_DMA_INTERRUPT 0x8
#define TEGRA_MMC_NORINTSTS_ERR_INTERRUPT 0x8000
#define TEGRA_MMC_NORINTSTS_CMD_TIMEOUT 0x10000

#define TEGRA_MMC_NORINTSTSEN_BUFFER_READ_READY 0x20

typedef struct _t210_sdmmc_t
{
	vu32 sysad;
	vu16 blksize;
	vu16 blkcnt;
	vu32 argument;
	vu16 trnmod;
	vu16 cmdreg;
	vu32 rspreg0;
	vu32 rspreg1;
	vu32 rspreg2;
	vu32 rspreg3;
	vu32 bdata;
	vu32 prnsts;
	vu8 hostctl;
	vu8 pwrcon;
	vu8 blkgap;
	vu8 wakcon;
	vu16 clkcon;
	vu8 timeoutcon;
	vu8 swrst;
	vu16 norintsts;
	vu16 errintsts;
	vu16 norintstsen;
	vu16 errintstsen;
	vu16 norintsigen;
	vu16 errintsigen;
	vu16 acmd12errsts;
	vu16 hostctl2;
	vu32 capareg;
	vu32 capareg_1;
	vu32 maxcurr;
	vu8 res3[4];
	vu16 setacmd12err;
	vu16 setinterr;
	vu8 admaerr;
	vu8 res4[3];
	vu32 admaaddr;
	vu32 admaaddr_hi;
	vu8 res5[156];
	vu16 slotintstatus;
	vu16 hcver;
	vu32 venclkctl;
	vu32 venspictl;
	vu32 venspiintsts;
	vu32 venceatactl;
	vu32 venbootctl;
	vu32 venbootacktout;
	vu32 venbootdattout;
	vu32 vendebouncecnt;
	vu32 venmiscctl;
	vu32 res6[34];
	vu32 veniotrimctl;
	vu32 vendllcal;
	vu8 res7[8];
	vu32 dllcfgstatus;
	vu32 ventunctl0;
	vu32 field_1C4;
	vu8 field_1C8[24];
	vu32 sdmemcmppadctl;
	vu32 autocalcfg;
	vu32 autocalintval;
	vu32 autocalsts;
	vu32 iospare;
} t210_sdmmc_t;

#endif
