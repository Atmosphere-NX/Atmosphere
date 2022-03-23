/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2019 CTCaer
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

#define TEGRA_MMC_VNDR_TUN_CTRL0_TAP_VAL_UPDATED_BY_HW 0x20000
#define TEGRA_MMC_DLLCAL_CFG_EN_CALIBRATE 0x80000000
#define TEGRA_MMC_DLLCAL_CFG_STATUS_DLL_ACTIVE 0x80000000
#define TEGRA_MMC_SDMEMCOMPPADCTRL_PAD_E_INPUT_PWRD 0x80000000
#define TEGRA_MMC_SDMEMCOMPPADCTRL_COMP_VREF_SEL_MASK 0xFFFFFFF0
#define TEGRA_MMC_AUTOCALCFG_AUTO_CAL_ENABLE 0x20000000
#define TEGRA_MMC_AUTOCALCFG_AUTO_CAL_START 0x80000000
#define TEGRA_MMC_AUTOCALSTS_AUTO_CAL_ACTIVE 0x80000000

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
	vu8  hostctl;
	vu8  pwrcon;
	vu8  blkgap;
	vu8  wakcon;
	vu16 clkcon;
	vu8  timeoutcon;
	vu8  swrst;
	vu16 norintsts;
	vu16 errintsts;
	vu16 norintstsen; // Enable irq status.
	vu16 errintstsen; // Enable irq status.
	vu16 norintsigen; // Enable irq signal to LIC/GIC.
	vu16 errintsigen; // Enable irq signal to LIC/GIC.
	vu16 acmd12errsts;
	vu16 hostctl2;
	vu32 capareg;
	vu32 capareg_1;
	vu32 maxcurr;
	vu8  rsvd0[4]; // 4C-4F reserved for more max current.
	vu16 setacmd12err;
	vu16 setinterr;
	vu8  admaerr;
	vu8  rsvd1[3]; // 55-57 reserved.
	vu32 admaaddr;
	vu32 admaaddr_hi;
	vu8  rsvd2[156]; // 60-FB reserved.
	vu16 slotintsts;
	vu16 hcver;
	vu32 venclkctl;
	vu32 vensysswctl;
	vu32 venerrintsts;
	vu32 vencapover;
	vu32 venbootctl;
	vu32 venbootacktout;
	vu32 venbootdattout;
	vu32 vendebouncecnt;
	vu32 venmiscctl;
	vu32 maxcurrover;
	vu32 maxcurrover_hi;
	vu32 unk0[32]; // 0x12C
	vu32 veniotrimctl;
	vu32 vendllcalcfg;
	vu32 vendllctl0;
	vu32 vendllctl1;
	vu32 vendllcalcfgsts;
	vu32 ventunctl0;
	vu32 ventunctl1;
	vu32 ventunsts0;
	vu32 ventunsts1;
	vu32 venclkgatehystcnt;
	vu32 venpresetval0;
	vu32 venpresetval1;
	vu32 venpresetval2;
	vu32 sdmemcmppadctl;
	vu32 autocalcfg;
	vu32 autocalintval;
	vu32 autocalsts;
	vu32 iospare;
	vu32 mcciffifoctl;
	vu32 timeoutwcoal;
	vu32 unk1;
} t210_sdmmc_t;

#endif
