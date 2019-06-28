/*
 * Copyright (c) 2018 naehrwert
 * Copyright (C) 2018 CTCaer
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
#include "../utils/types.h"
#include "../nx/cache.h"
#include "sdmmc.h"
#include "../utils/util.h"
#include "../soc/clock.h"
#include "mmc.h"
#include "../power/max7762x.h"
#include "../soc/t210.h"
#include "../soc/pmc.h"
#include "../soc/pinmux.h"
#include "../soc/gpio.h"
#include "../utils/fatal.h"

#define DPRINTF(...)

/*! SCMMC controller base addresses. */
static const u64 _sdmmc_bases[4] = {
	0x700B0000,
	0x700B0200,
	0x700B0400,
	0x700B0600,
};

int sdmmc_get_voltage(sdmmc_t *sdmmc)
{
	u32 p = sdmmc->regs->pwrcon;
	if (!(p & TEGRA_MMC_PWRCTL_SD_BUS_POWER))
		return SDMMC_POWER_OFF;
	if (p & TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V1_8)
		return SDMMC_POWER_1_8;
	if (p & TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V3_3)
		return SDMMC_POWER_3_3;
	return -1;
}

static int _sdmmc_set_voltage(sdmmc_t *sdmmc, u32 power)
{
	u8 pwr = 0;

	switch (power)
	{
	case SDMMC_POWER_OFF:
		sdmmc->regs->pwrcon &= ~TEGRA_MMC_PWRCTL_SD_BUS_POWER;
		break;
	case SDMMC_POWER_1_8:
		sdmmc->regs->pwrcon = TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V1_8;
		pwr = TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V1_8;
		break;
	case SDMMC_POWER_3_3:
		sdmmc->regs->pwrcon = TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V3_3;
		pwr = TEGRA_MMC_PWRCTL_SD_BUS_VOLTAGE_V3_3;
		break;
	default:
		return 0;
	}

	if (power != SDMMC_POWER_OFF)
	{
		pwr |= TEGRA_MMC_PWRCTL_SD_BUS_POWER;
		sdmmc->regs->pwrcon = pwr;
	}	

	return 1;
}

u32 sdmmc_get_bus_width(sdmmc_t *sdmmc)
{
	u32 h = sdmmc->regs->hostctl;
	if (h & TEGRA_MMC_HOSTCTL_8BIT)
		return SDMMC_BUS_WIDTH_8;
	if (h & TEGRA_MMC_HOSTCTL_4BIT)
		return SDMMC_BUS_WIDTH_4;
	return SDMMC_BUS_WIDTH_1;
}

void sdmmc_set_bus_width(sdmmc_t *sdmmc, u32 bus_width)
{
	if (bus_width == SDMMC_BUS_WIDTH_1)
		sdmmc->regs->hostctl &= ~(TEGRA_MMC_HOSTCTL_4BIT | TEGRA_MMC_HOSTCTL_8BIT);
	else if (bus_width == SDMMC_BUS_WIDTH_4)
	{
		sdmmc->regs->hostctl |= TEGRA_MMC_HOSTCTL_4BIT;
		sdmmc->regs->hostctl &= ~TEGRA_MMC_HOSTCTL_8BIT;
	}
	else if (bus_width == SDMMC_BUS_WIDTH_8)
		sdmmc->regs->hostctl |= TEGRA_MMC_HOSTCTL_8BIT;
}

void sdmmc_get_venclkctl(sdmmc_t *sdmmc)
{
	sdmmc->venclkctl_tap = sdmmc->regs->venclkctl >> 16;
	sdmmc->venclkctl_set = 1;
}

static int _sdmmc_config_ven_ceata_clk(sdmmc_t *sdmmc, u32 id)
{
	u32 tap_val = 0;

	if (id == 4)
		sdmmc->regs->venceatactl = (sdmmc->regs->venceatactl & 0xFFFFC0FF) | 0x2800;
	sdmmc->regs->ventunctl0 &= 0xFFFDFFFF;
	if (id == 4)
	{
		if (!sdmmc->venclkctl_set)
			return 0;
		tap_val = sdmmc->venclkctl_tap;
	}
	else
	{
		static const u32 tap_values[] = { 4, 0, 3, 0 };
		tap_val = tap_values[sdmmc->id];
	}
	sdmmc->regs->venclkctl = (sdmmc->regs->venclkctl & 0xFF00FFFF) | (tap_val << 16);

	return 1;
}

static int _sdmmc_get_clkcon(sdmmc_t *sdmmc)
{
	return sdmmc->regs->clkcon;
}

static void _sdmmc_pad_config_fallback(sdmmc_t *sdmmc, u32 power)
{
	_sdmmc_get_clkcon(sdmmc);
	switch (sdmmc->id)
	{
	case SDMMC_1:
		if (power == SDMMC_POWER_OFF)
			break;
		if (power == SDMMC_POWER_1_8)
			APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL) = 0x304; // Up: 3, Dn: 4.
		else if (power == SDMMC_POWER_3_3)
			APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL) = 0x808; // Up: 8, Dn: 8.
		break;
	case SDMMC_4:
		APB_MISC(APB_MISC_GP_EMMC4_PAD_CFGPADCTRL) = (APB_MISC(APB_MISC_GP_EMMC4_PAD_CFGPADCTRL) & 0x3FFC) | 0x1040;
		break;
	}
	//TODO: load standard values for other controllers, can depend on power.
}

static int _sdmmc_wait_type4(sdmmc_t *sdmmc)
{
	int res = 1, should_disable_sd_clock = 0;

	if (!(sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE))
	{
		should_disable_sd_clock = 1;
		sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
	}

	sdmmc->regs->vendllcal |= 0x80000000;
	_sdmmc_get_clkcon(sdmmc);

	u32 timeout = get_tmr_ms() + 5;
	while (sdmmc->regs->vendllcal & 0x80000000)
	{
		if (get_tmr_ms() > timeout)
		{
			res = 0;
			goto out;
		}
	}

	timeout = get_tmr_ms() + 10;
	while (sdmmc->regs->dllcfgstatus & 0x80000000)
	{
		if (get_tmr_ms() > timeout)
		{
			res = 0;
			goto out;
		}
	}

out:;
	if (should_disable_sd_clock)
		sdmmc->regs->clkcon &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
	return res;
}

int sdmmc_setup_clock(sdmmc_t *sdmmc, u32 type)
{
	//Disable the SD clock if it was enabled, and reenable it later.
	bool should_enable_sd_clock = false;
	if (sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE)
	{
		should_enable_sd_clock = true;
		sdmmc->regs->clkcon &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
	}

	_sdmmc_config_ven_ceata_clk(sdmmc, type);

	switch (type)
	{
	case 0:
	case 1:
	case 5:
	case 6:
		sdmmc->regs->hostctl  &= 0xFB; //Should this be 0xFFFB (~4) ?
		sdmmc->regs->hostctl2 &= SDHCI_CTRL_VDD_330;
		break;
	case 2:
	case 7:
		sdmmc->regs->hostctl  |= 4;
		sdmmc->regs->hostctl2 &= SDHCI_CTRL_VDD_330;
		break;
	case 3:
	case 11:
	case 13:
	case 14:
		sdmmc->regs->hostctl2  = (sdmmc->regs->hostctl2 & SDHCI_CTRL_UHS_MASK) | UHS_SDR104_BUS_SPEED;
		sdmmc->regs->hostctl2 |= SDHCI_CTRL_VDD_180;
		break;
	case 4:
		 //Non standard
		sdmmc->regs->hostctl2  = (sdmmc->regs->hostctl2 & SDHCI_CTRL_UHS_MASK) | HS400_BUS_SPEED;
		sdmmc->regs->hostctl2 |= SDHCI_CTRL_VDD_180;
		break;
	case 8:
		sdmmc->regs->hostctl2  = (sdmmc->regs->hostctl2 & SDHCI_CTRL_UHS_MASK) | UHS_SDR12_BUS_SPEED;
		sdmmc->regs->hostctl2 |= SDHCI_CTRL_VDD_180;
		break;
	case 10:
		//T210 Errata for SDR50, the host must be set to SDR104.
		sdmmc->regs->hostctl2  = (sdmmc->regs->hostctl2 & SDHCI_CTRL_UHS_MASK) | UHS_SDR104_BUS_SPEED;
		sdmmc->regs->hostctl2 |= SDHCI_CTRL_VDD_180;
		break;
	}

	_sdmmc_get_clkcon(sdmmc);

	u32 tmp;
	u16 divisor;
	clock_sdmmc_get_params(&tmp, &divisor, type);
	clock_sdmmc_config_clock_source(&tmp, sdmmc->id, tmp);
	sdmmc->divisor = (tmp + divisor - 1) / divisor;

	//if divisor != 1 && divisor << 31 -> error

	u16 div = divisor >> 1;
	divisor = 0;
	if (div > 0xFF)
		divisor = div >> 8;
	sdmmc->regs->clkcon = (sdmmc->regs->clkcon & 0x3F) | (div << 8) | (divisor << 6);

	//Enable the SD clock again.
	if (should_enable_sd_clock)
		sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;

	if (type == 4)
		return _sdmmc_wait_type4(sdmmc);
	return 1;
}

static void _sdmmc_sd_clock_enable(sdmmc_t *sdmmc)
{
	if (!sdmmc->no_sd)
	{
		if (!(sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE))
			sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
	}
	sdmmc->sd_clock_enabled = 1;
}

static void _sdmmc_sd_clock_disable(sdmmc_t *sdmmc)
{
	sdmmc->sd_clock_enabled = 0;
	sdmmc->regs->clkcon &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
}

void sdmmc_sd_clock_ctrl(sdmmc_t *sdmmc, int no_sd)
{
	sdmmc->no_sd = no_sd;
	if (no_sd)
	{
		if (!(sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE))
			return;
		sdmmc->regs->clkcon &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
		return;
	}
	if (sdmmc->sd_clock_enabled)
		if (!(sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE))
			sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
}

static int _sdmmc_cache_rsp(sdmmc_t *sdmmc, u32 *rsp, u32 size, u32 type)
{
	switch (type)
	{
	case SDMMC_RSP_TYPE_1:
	case SDMMC_RSP_TYPE_3:
	case SDMMC_RSP_TYPE_4:
	case SDMMC_RSP_TYPE_5:
		if (size < 4)
			return 0;
		rsp[0] = sdmmc->regs->rspreg0;
		break;
	case SDMMC_RSP_TYPE_2:
		if (size < 0x10)
			return 0;
		// CRC is stripped, so shifting is needed.
		u32 tempreg;
		for (int i = 0; i < 4; i++)
		{
			switch(i)
			{
			case 0:
				tempreg = sdmmc->regs->rspreg3;
				break;
			case 1:
				tempreg = sdmmc->regs->rspreg2;
				break;
			case 2:
				tempreg = sdmmc->regs->rspreg1;
				break;
			case 3:
				tempreg = sdmmc->regs->rspreg0;
				break;
			}
			rsp[i] = tempreg << 8;

			if (i != 0)
				rsp[i - 1] |= (tempreg >> 24) & 0xFF;
		}
		break;
	default:
		return 0;
		break;
	}

	return 1;
}

int sdmmc_get_rsp(sdmmc_t *sdmmc, u32 *rsp, u32 size, u32 type)
{
	if (!rsp || sdmmc->expected_rsp_type != type)
		return 0;

	switch (type)
	{
	case SDMMC_RSP_TYPE_1:
	case SDMMC_RSP_TYPE_3:
	case SDMMC_RSP_TYPE_4:
	case SDMMC_RSP_TYPE_5:
		if (size < 4)
			return 0;
		rsp[0] = sdmmc->rsp[0];
		break;
	case SDMMC_RSP_TYPE_2:
		if (size < 0x10)
			return 0;
		rsp[0] = sdmmc->rsp[0];
		rsp[1] = sdmmc->rsp[1];
		rsp[2] = sdmmc->rsp[2];
		rsp[3] = sdmmc->rsp[3];
		break;
	default:
		return 0;
		break;
	}

	return 1;
}

static void _sdmmc_reset(sdmmc_t *sdmmc)
{
	sdmmc->regs->swrst |= 
		TEGRA_MMC_SWRST_SW_RESET_FOR_CMD_LINE | TEGRA_MMC_SWRST_SW_RESET_FOR_DAT_LINE;
	_sdmmc_get_clkcon(sdmmc);
	u32 timeout = get_tmr_ms() + 2000;
	while (sdmmc->regs->swrst << 29 >> 30 && get_tmr_ms() < timeout)
		;
}

static int _sdmmc_wait_prnsts_type0(sdmmc_t *sdmmc, u32 wait_dat)
{
	_sdmmc_get_clkcon(sdmmc);

	u32 timeout = get_tmr_ms() + 2000;
	while(sdmmc->regs->prnsts & 1) //CMD inhibit.
		if (get_tmr_ms() > timeout)
		{
			_sdmmc_reset(sdmmc);
			return 0;
		}

	if (wait_dat)
	{
		timeout = get_tmr_ms() + 2000;
		while (sdmmc->regs->prnsts & 2) //DAT inhibit.
			if (get_tmr_ms() > timeout)
			{
				_sdmmc_reset(sdmmc);
				return 0;
			}
	}

	return 1;
}

static int _sdmmc_wait_prnsts_type1(sdmmc_t *sdmmc)
{
	_sdmmc_get_clkcon(sdmmc);

	u32 timeout = get_tmr_ms() + 2000;
	while (!(sdmmc->regs->prnsts & 0x100000)) //DAT0 line level.
		if (get_tmr_ms() > timeout)
		{
			_sdmmc_reset(sdmmc);
			return 0;
		}

	return 1;
}

static int _sdmmc_setup_read_small_block(sdmmc_t *sdmmc)
{
	switch (sdmmc_get_bus_width(sdmmc))
	{
	case SDMMC_BUS_WIDTH_1:
		return 0;
		break;
	case SDMMC_BUS_WIDTH_4:
		sdmmc->regs->blksize = 0x40;
		break;
	case SDMMC_BUS_WIDTH_8:
		sdmmc->regs->blksize = 0x80;
		break;
	}
	sdmmc->regs->blkcnt = 1;
	sdmmc->regs->trnmod = TEGRA_MMC_TRNMOD_DATA_XFER_DIR_SEL_READ;
	return 1;
}

static int _sdmmc_parse_cmdbuf(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, bool is_data_present)
{
	u16 cmdflags = 0;
	
	switch (cmd->rsp_type)
	{
	case SDMMC_RSP_TYPE_0:
		break;
	case SDMMC_RSP_TYPE_1:
	case SDMMC_RSP_TYPE_4:
	case SDMMC_RSP_TYPE_5:
		if (cmd->check_busy)
			cmdflags = TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48_BUSY |
				TEGRA_MMC_TRNMOD_CMD_INDEX_CHECK |
				TEGRA_MMC_TRNMOD_CMD_CRC_CHECK;
		else
			cmdflags = TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48 |
				TEGRA_MMC_TRNMOD_CMD_INDEX_CHECK |
				TEGRA_MMC_TRNMOD_CMD_CRC_CHECK;
		break;
	case SDMMC_RSP_TYPE_2:
		cmdflags = TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_136 |
			TEGRA_MMC_TRNMOD_CMD_CRC_CHECK;
		break;
	case SDMMC_RSP_TYPE_3:
		cmdflags = TEGRA_MMC_CMDREG_RESP_TYPE_SELECT_LENGTH_48;
		break;
	default:
		return 0;
		break;
	}

	if (is_data_present)
		cmdflags |= TEGRA_MMC_TRNMOD_DATA_PRESENT_SELECT_DATA_TRANSFER;
	sdmmc->regs->argument = cmd->arg;
	sdmmc->regs->cmdreg = (cmd->cmd << 8) | cmdflags;

	return 1;
}

static void _sdmmc_parse_cmd_48(sdmmc_t *sdmmc, u32 cmd)
{
	sdmmc_cmd_t cmdbuf;
	cmdbuf.cmd = cmd;
	cmdbuf.arg = 0;
	cmdbuf.rsp_type = SDMMC_RSP_TYPE_1;
	cmdbuf.check_busy = 0;
	_sdmmc_parse_cmdbuf(sdmmc, &cmdbuf, true);
}

static int _sdmmc_config_tuning_once(sdmmc_t *sdmmc, u32 cmd)
{
	if (sdmmc->no_sd)
		return 0;
	if (!_sdmmc_wait_prnsts_type0(sdmmc, 1))
		return 0;

	_sdmmc_setup_read_small_block(sdmmc);
	sdmmc->regs->norintstsen |= TEGRA_MMC_NORINTSTSEN_BUFFER_READ_READY;
	sdmmc->regs->norintsts = sdmmc->regs->norintsts;
	sdmmc->regs->clkcon &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
	_sdmmc_parse_cmd_48(sdmmc, cmd);
	_sdmmc_get_clkcon(sdmmc);
	usleep(1);
	_sdmmc_reset(sdmmc);
	sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
	_sdmmc_get_clkcon(sdmmc);

	u32 timeout = get_tmr_us() + 5000;
	while (get_tmr_us() < timeout)
	{
		if (sdmmc->regs->norintsts & 0x20)
		{
			sdmmc->regs->norintsts = 0x20;
			sdmmc->regs->norintstsen &= 0xFFDF;
			_sdmmc_get_clkcon(sdmmc);
			usleep((1000 * 8 + sdmmc->divisor - 1) / sdmmc->divisor);
			return 1;
		}
	}
	_sdmmc_reset(sdmmc);
	sdmmc->regs->norintstsen &= 0xFFDF;
	_sdmmc_get_clkcon(sdmmc);
	usleep((1000 * 8 + sdmmc->divisor - 1) / sdmmc->divisor);
	return 0;
}

int sdmmc_config_tuning(sdmmc_t *sdmmc, u32 type, u32 cmd)
{
	u32 max = 0, flag = 0;

	sdmmc->regs->field_1C4 = 0;
	switch (type)
	{
	case 3:
	case 4:
	case 11:
		max = 0x80;
		flag = 0x4000;
		break;
	case 10:
	case 13:
	case 14:
		max = 0x100;
		flag = 0x8000;
		break;
	default:
		return 0;
	}

	sdmmc->regs->ventunctl0 = (sdmmc->regs->ventunctl0 & 0xFFFF1FFF) | flag;
	sdmmc->regs->ventunctl0 = (sdmmc->regs->ventunctl0 & 0xFFFFE03F) | 0x40;
	sdmmc->regs->ventunctl0 |= 0x20000;
	sdmmc->regs->hostctl2  |= SDHCI_CTRL_EXEC_TUNING;

	for (u32 i = 0; i < max; i++)
	{
		_sdmmc_config_tuning_once(sdmmc, cmd);
		if (!(sdmmc->regs->hostctl2 & SDHCI_CTRL_EXEC_TUNING))
			break;
	}

	if (sdmmc->regs->hostctl2 & SDHCI_CTRL_TUNED_CLK)
		return 1;
	return 0;
}

static int _sdmmc_enable_internal_clock(sdmmc_t *sdmmc)
{
	//Enable internal clock and wait till it is stable.
	sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_INTERNAL_CLOCK_ENABLE;
	_sdmmc_get_clkcon(sdmmc);
	u32 timeout = get_tmr_ms() + 2000;
	while (!(sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_INTERNAL_CLOCK_STABLE))
	{
		if (get_tmr_ms() > timeout)
			return 0;
	}

	sdmmc->regs->hostctl2 &= ~SDHCI_CTRL_PRESET_VAL_EN;
	sdmmc->regs->clkcon   &= ~TEGRA_MMC_CLKCON_CLKGEN_SELECT;
	sdmmc->regs->hostctl2 |= SDHCI_HOST_VERSION_4_EN;

	if (!(sdmmc->regs->capareg & 0x10000000))
		return 0;

	sdmmc->regs->hostctl2 |= SDHCI_ADDRESSING_64BIT_EN;
	sdmmc->regs->hostctl &= 0xE7;
	sdmmc->regs->timeoutcon = (sdmmc->regs->timeoutcon & 0xF0) | 0xE;

	return 1;
}

static int _sdmmc_autocal_config_offset(sdmmc_t *sdmmc, u32 power)
{
	u32 off_pd = 0;
	u32 off_pu = 0;

	switch (sdmmc->id)
	{
	case SDMMC_2:
	case SDMMC_4:
		if (power != SDMMC_POWER_1_8)
			return 0;
		off_pd = 5;
		off_pu = 5;
		break;
	case SDMMC_1:
	case SDMMC_3:
		if (power == SDMMC_POWER_1_8)
		{
			off_pd = 123;
			off_pu = 123;
		}
		else if (power == SDMMC_POWER_3_3)
		{
			off_pd = 125;
			off_pu = 0;
		}
		else
			return 0;
		break;
	}

	sdmmc->regs->autocalcfg = (((sdmmc->regs->autocalcfg & 0xFFFF80FF) | (off_pd << 8)) >> 7 << 7) | off_pu;
	return 1;
}

static void _sdmmc_autocal_execute(sdmmc_t *sdmmc, u32 power)
{
	bool should_enable_sd_clock = false;
	if (sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE)
	{
		should_enable_sd_clock = true;
		sdmmc->regs->clkcon &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
	}

	if (!(sdmmc->regs->sdmemcmppadctl & 0x80000000))
	{
		sdmmc->regs->sdmemcmppadctl |= 0x80000000;
		_sdmmc_get_clkcon(sdmmc);
		usleep(1);
	}

	sdmmc->regs->autocalcfg |= 0xA0000000;
	_sdmmc_get_clkcon(sdmmc);
	usleep(1);

	u32 timeout = get_tmr_ms() + 10;
	while (sdmmc->regs->autocalcfg & 0x80000000)
	{
		if (get_tmr_ms() > timeout)
		{
			//In case autocalibration fails, we load suggested standard values.
			_sdmmc_pad_config_fallback(sdmmc, power);
			sdmmc->regs->autocalcfg &= 0xDFFFFFFF;
			break;
		}
	}

	sdmmc->regs->sdmemcmppadctl &= 0x7FFFFFFF;

	if(should_enable_sd_clock)
		sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
}

static void _sdmmc_enable_interrupts(sdmmc_t *sdmmc)
{
	sdmmc->regs->norintstsen |= 0xB;
	sdmmc->regs->errintstsen |= 0x17F;
	sdmmc->regs->norintsts = sdmmc->regs->norintsts;
	sdmmc->regs->errintsts = sdmmc->regs->errintsts;
}

static void _sdmmc_mask_interrupts(sdmmc_t *sdmmc)
{
	sdmmc->regs->errintstsen &= 0xFE80;
	sdmmc->regs->norintstsen &= 0xFFF4;
}

static int _sdmmc_check_mask_interrupt(sdmmc_t *sdmmc, u16 *pout, u16 mask)
{
	u16 norintsts = sdmmc->regs->norintsts;
	u16 errintsts = sdmmc->regs->errintsts;

	DPRINTF("norintsts %08X; errintsts %08X\n", norintsts, errintsts);

	if (pout)
		*pout = norintsts;

	//Check for error interrupt.
	if (norintsts & TEGRA_MMC_NORINTSTS_ERR_INTERRUPT)
	{
		sdmmc->regs->errintsts = errintsts;
		return SDMMC_MASKINT_ERROR;
	}
	else if (norintsts & mask)
	{
		sdmmc->regs->norintsts = norintsts & mask;
		return SDMMC_MASKINT_MASKED;
	}
	
	return SDMMC_MASKINT_NOERROR;
}

static int _sdmmc_wait_request(sdmmc_t *sdmmc)
{
	_sdmmc_get_clkcon(sdmmc);

	u32 timeout = get_tmr_ms() + 2000;
	while (1)
	{
		int res = _sdmmc_check_mask_interrupt(sdmmc, 0, TEGRA_MMC_NORINTSTS_CMD_COMPLETE);
		if (res == SDMMC_MASKINT_MASKED)
			break;
		if (res != SDMMC_MASKINT_NOERROR || get_tmr_ms() > timeout)
		{
			_sdmmc_reset(sdmmc);
			return 0;
		}
	}

	return 1;
}

static int _sdmmc_stop_transmission_inner(sdmmc_t *sdmmc, u32 *rsp)
{
	sdmmc_cmd_t cmd;

	if (!_sdmmc_wait_prnsts_type0(sdmmc, 0))
		return 0;

	_sdmmc_enable_interrupts(sdmmc);
	cmd.cmd = MMC_STOP_TRANSMISSION;
	cmd.arg = 0;
	cmd.rsp_type = SDMMC_RSP_TYPE_1;
	cmd.check_busy = 1;
	_sdmmc_parse_cmdbuf(sdmmc, &cmd, false);
	int res = _sdmmc_wait_request(sdmmc);
	_sdmmc_mask_interrupts(sdmmc);

	if (!res)
		return 0;
	
	_sdmmc_cache_rsp(sdmmc, rsp, 4, SDMMC_RSP_TYPE_1);
	return _sdmmc_wait_prnsts_type1(sdmmc);
}

int sdmmc_stop_transmission(sdmmc_t *sdmmc, u32 *rsp)
{
	if (!sdmmc->sd_clock_enabled)
		return 0;

	bool should_disable_sd_clock = false;
	if (!(sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE))
	{
		should_disable_sd_clock = true;
		sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
		_sdmmc_get_clkcon(sdmmc);
		usleep((8000 + sdmmc->divisor - 1) / sdmmc->divisor);
	}

	int res = _sdmmc_stop_transmission_inner(sdmmc, rsp);
	usleep((8000 + sdmmc->divisor - 1) / sdmmc->divisor);
	if (should_disable_sd_clock)
		sdmmc->regs->clkcon &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;

	return res;
}

static int _sdmmc_config_dma(sdmmc_t *sdmmc, u32 *blkcnt_out, sdmmc_req_t *req)
{
	if (!req->blksize || !req->num_sectors)
		return 0;

	u32 blkcnt = req->num_sectors;
	if (blkcnt >= 0xFFFF)
		blkcnt = 0xFFFF;
		
	u64 admaaddr = (u64)sdmmc_calculate_dma_addr(_current_accessor, req->buf, blkcnt);
	if (!admaaddr)
	{
		// buf is on a heap
		int dma_idx = sdmmc_calculate_fitting_dma_index(_current_accessor, blkcnt);
		admaaddr = (u64)&_current_accessor->parent->dmaBuffers[dma_idx].device_addr_buffer_masked[0];
		sdmmc->last_dma_idx = dma_idx;
	}

	//Check alignment.
	if (admaaddr & 7)
		return 0;

    sdmmc->regs->admaaddr = admaaddr & 0xFFFFFFFFF;
    sdmmc->regs->admaaddr_hi = (admaaddr >> 32) & 0xFFFFFFFFF;

	sdmmc->dma_addr_next = (admaaddr + 0x80000) & 0xFFFFFFFFFFF80000;

	sdmmc->regs->blksize = req->blksize | 0x7000;
	sdmmc->regs->blkcnt = blkcnt;

	if (blkcnt_out)
		*blkcnt_out = blkcnt;

	u32 trnmode = TEGRA_MMC_TRNMOD_DMA_ENABLE;
	if (req->is_multi_block)
		trnmode = TEGRA_MMC_TRNMOD_MULTI_BLOCK_SELECT |
			TEGRA_MMC_TRNMOD_BLOCK_COUNT_ENABLE |
			TEGRA_MMC_TRNMOD_DMA_ENABLE;
	if (!req->is_write)
		trnmode |= TEGRA_MMC_TRNMOD_DATA_XFER_DIR_SEL_READ;
	if (req->is_auto_cmd12)
		trnmode = (trnmode & 0xFFF3) | TEGRA_MMC_TRNMOD_AUTO_CMD12;

	sdmmc->regs->trnmod = trnmode;

	return 1;
}

static int _sdmmc_update_dma(sdmmc_t *sdmmc)
{
	u16 blkcnt = 0;
	do
	{
		blkcnt = sdmmc->regs->blkcnt;
		u32 timeout = get_tmr_ms() + 1500;
		do
		{
			int res = 0;
			while (1)
			{
				u16 intr = 0;
				res = _sdmmc_check_mask_interrupt(sdmmc, &intr, 
					TEGRA_MMC_NORINTSTS_XFER_COMPLETE | TEGRA_MMC_NORINTSTS_DMA_INTERRUPT);
				if (res < 0)
					break;
				if (intr & TEGRA_MMC_NORINTSTS_XFER_COMPLETE)
					return 1; //Transfer complete.
				if (intr & TEGRA_MMC_NORINTSTS_DMA_INTERRUPT)
				{
					//Update DMA.
					sdmmc->regs->admaaddr = sdmmc->dma_addr_next & 0xFFFFFFFFF;
					sdmmc->regs->admaaddr_hi = (sdmmc->dma_addr_next >> 32) & 0xFFFFFFFFF;
					sdmmc->dma_addr_next += 0x80000;
				}
			}
			if (res != SDMMC_MASKINT_NOERROR)
			{
				_sdmmc_reset(sdmmc);
				return 0;
			}
		} while (get_tmr_ms() < timeout);
	} while (sdmmc->regs->blkcnt != blkcnt);

	_sdmmc_reset(sdmmc);
	return 0;
}

static int _sdmmc_execute_cmd_inner(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, sdmmc_req_t *req, u32 *blkcnt_out)
{
	int has_req_or_check_busy = req || cmd->check_busy;
	if (!_sdmmc_wait_prnsts_type0(sdmmc, has_req_or_check_busy))
		return 0;

	u32 blkcnt = 0;
	bool is_data_present = false;
	if (req)
	{
		_sdmmc_config_dma(sdmmc, &blkcnt, req);
		if(!sdmmc_memcpy_buf)
		{
			// Flush from/to phys
			armDCacheFlush(req->buf, req->blksize * blkcnt);
		}
		else
		{
			if(req->is_write)
			{
				void* dma_addr = &_current_accessor->parent->dmaBuffers[sdmmc->last_dma_idx].device_addr_buffer[0];
				memcpy(dma_addr, req->buf, req->blksize * blkcnt);

				// Flush to phys
				armDCacheFlush(dma_addr, req->blksize * blkcnt);
			}
		}

		_sdmmc_enable_interrupts(sdmmc);
		is_data_present = true;
	}
	else
	{
		_sdmmc_enable_interrupts(sdmmc);
		is_data_present = false;
	}

	_sdmmc_parse_cmdbuf(sdmmc, cmd, is_data_present);

	int res = _sdmmc_wait_request(sdmmc);
	DPRINTF("rsp(%d): %08X, %08X, %08X, %08X\n", res, 
		sdmmc->regs->rspreg0, sdmmc->regs->rspreg1, sdmmc->regs->rspreg2, sdmmc->regs->rspreg3);
	if (res)
	{
		if (cmd->rsp_type)
		{
			sdmmc->expected_rsp_type = cmd->rsp_type;
			_sdmmc_cache_rsp(sdmmc, sdmmc->rsp, 0x10, cmd->rsp_type);
            
			/*if(sdmmc->rsp[0] & 0xFDF9A080)
			{
				res = 0;
				sdmmc->rsp[0] = 0; // Reset error
			}*/
		}
		
		if (res && req) 
			_sdmmc_update_dma(sdmmc);
	}

	_sdmmc_mask_interrupts(sdmmc);

	if (res)
	{
		if (req)
		{
			if(!req->is_write)
			{
				if(!sdmmc_memcpy_buf)
				{
					// Flush from phys
					armDCacheFlush(req->buf, req->blksize * blkcnt);
				}
				else
				{
					void* dma_addr = &_current_accessor->parent->dmaBuffers[sdmmc->last_dma_idx].device_addr_buffer[0];
					// Flush from phys
					armDCacheFlush(dma_addr, req->blksize * blkcnt);
					// Copy to buffer
					memcpy(req->buf, dma_addr, req->blksize * blkcnt);
				}
			}

			if (blkcnt_out)
				*blkcnt_out = blkcnt;

			if (req->is_auto_cmd12)
				sdmmc->rsp3 = sdmmc->regs->rspreg3;
		}

		if (cmd->check_busy || req)
			return _sdmmc_wait_prnsts_type1(sdmmc);
	}

	return res;
}

static int _sdmmc_config_sdmmc1()
{
	//Configure SD card detect.
	PINMUX_AUX(PINMUX_AUX_GPIO_PZ1) = PINMUX_INPUT_ENABLE | PINMUX_PULL_UP | 1; //GPIO control, pull up.
	APB_MISC(APB_MISC_GP_VGPIO_GPIO_MUX_SEL) = 0;
	gpio_config(GPIO_PORT_Z, GPIO_PIN_1, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_Z, GPIO_PIN_1, GPIO_OUTPUT_DISABLE);
	usleep(100);
	if(!!gpio_read(GPIO_PORT_Z, GPIO_PIN_1))
		return 0;

	/*
	* Pinmux config:
	*  DRV_TYPE = DRIVE_2X
	*  E_SCHMT = ENABLE (for 1.8V),  DISABLE (for 3.3V)
	*  E_INPUT = ENABLE
	*  TRISTATE = PASSTHROUGH
	*  APB_MISC_GP_SDMMCx_CLK_LPBK_CONTROL = SDMMCx_CLK_PAD_E_LPBK for CLK
	*/

	//Configure SDMMC1 pinmux.
	APB_MISC(APB_MISC_GP_SDMMC1_CLK_LPBK_CONTROL) = 1;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_CLK)  = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PARKED;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_CMD)  = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PARKED | PINMUX_PULL_UP;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT3) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PARKED | PINMUX_PULL_UP;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT2) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PARKED | PINMUX_PULL_UP;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT1) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PARKED | PINMUX_PULL_UP;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT0) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PARKED | PINMUX_PULL_UP;

	//Make sure SDMMC1 controller is reset.
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_NO_IOPOWER, (1 << 12), (1 << 12));
	usleep(1000);

	//Make sure the SDMMC1 controller is powered.
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_NO_IOPOWER, ~(1 << 12), (1 << 12));
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_PWR_DET_VAL, (1 << 12), (1 << 12));

	//Set enable SD card power.
	PINMUX_AUX(PINMUX_AUX_DMIC3_CLK) = PINMUX_INPUT_ENABLE | PINMUX_PULL_DOWN | 1; //GPIO control, pull down.
	gpio_config(GPIO_PORT_E, GPIO_PIN_4, GPIO_MODE_GPIO);
	gpio_write(GPIO_PORT_E, GPIO_PIN_4, GPIO_HIGH);
	gpio_output_enable(GPIO_PORT_E, GPIO_PIN_4, GPIO_OUTPUT_ENABLE);

	usleep(1000);

	//Enable SD card power.
	max77620_regulator_set_voltage(REGULATOR_LDO2, 3300000);
	max77620_regulator_enable(REGULATOR_LDO2, 1);

	usleep(1000);

	//For good measure.
	APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL) = 0x10000000;

	usleep(1000);

	return 1;
}

int sdmmc_init(sdmmc_t *sdmmc, u32 id, u32 power, u32 bus_width, u32 type, int no_sd)
{
	if (id > SDMMC_4)
		return 0;

	if (id == SDMMC_1)
		if (!_sdmmc_config_sdmmc1())
			return 0;

	memset(sdmmc, 0, sizeof(sdmmc_t));

	sdmmc->regs = (t210_sdmmc_t *)QueryIoMapping(_sdmmc_bases[id], 0x200);
	sdmmc->id = id;
	sdmmc->clock_stopped = 1;

	if (clock_sdmmc_is_not_reset_and_enabled(id))
	{
		_sdmmc_sd_clock_disable(sdmmc);
		_sdmmc_get_clkcon(sdmmc);
	}

	u32 clock;
	u16 divisor;
	clock_sdmmc_get_params(&clock, &divisor, type);
	clock_sdmmc_enable(id, clock);

	sdmmc->clock_stopped = 0;

	//TODO: make this skip-able.
	sdmmc->regs->iospare |= 0x80000;
	sdmmc->regs->veniotrimctl &= 0xFFFFFFFB;
	static const u32 trim_values[] = { 2, 8, 3, 8 };
	sdmmc->regs->venclkctl = (sdmmc->regs->venclkctl & 0xE0FFFFFF) | (trim_values[sdmmc->id] << 24);
	sdmmc->regs->sdmemcmppadctl = (sdmmc->regs->sdmemcmppadctl & 0xFFFFFFF0) | 7;
	if (!_sdmmc_autocal_config_offset(sdmmc, power))
		return 0;
	_sdmmc_autocal_execute(sdmmc, power);
	if (_sdmmc_enable_internal_clock(sdmmc))
	{
		sdmmc_set_bus_width(sdmmc, bus_width);
		_sdmmc_set_voltage(sdmmc, power);
		if (sdmmc_setup_clock(sdmmc, type))
		{
			sdmmc_sd_clock_ctrl(sdmmc, no_sd);
			_sdmmc_sd_clock_enable(sdmmc);
			_sdmmc_get_clkcon(sdmmc);
			return 1;
		}
		return 0;
	}
	return 0;
}

void sdmmc_end(sdmmc_t *sdmmc)
{
	if (!sdmmc->clock_stopped)
	{
		_sdmmc_sd_clock_disable(sdmmc);
		// Disable SDMMC power. 
		_sdmmc_set_voltage(sdmmc, SDMMC_POWER_OFF);

		// Disable SD card power.
		if (sdmmc->id == SDMMC_1)
		{
			gpio_output_enable(GPIO_PORT_E, GPIO_PIN_4, GPIO_OUTPUT_DISABLE);
			msleep(1); // To power cycle min 1ms without power is needed.
			max77620_regulator_enable(REGULATOR_LDO2, 0);
			msleep(100); // Some extra.
		}

		_sdmmc_get_clkcon(sdmmc);
		clock_sdmmc_disable(sdmmc->id);
		sdmmc->clock_stopped = 1;
	}
}

void sdmmc_init_cmd(sdmmc_cmd_t *cmdbuf, u16 cmd, u32 arg, u32 rsp_type, u32 check_busy)
{
	cmdbuf->cmd = cmd;
	cmdbuf->arg = arg;
	cmdbuf->rsp_type = rsp_type;
	cmdbuf->check_busy = check_busy;
}

int sdmmc_execute_cmd(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, sdmmc_req_t *req, u32 *blkcnt_out)
{
	if (!sdmmc->sd_clock_enabled)
		return 0;

	//Recalibrate periodically for SDMMC1.
	if (sdmmc->id == SDMMC_1 && sdmmc->no_sd)
		_sdmmc_autocal_execute(sdmmc, sdmmc_get_voltage(sdmmc));

	int should_disable_sd_clock = 0;
	if (!(sdmmc->regs->clkcon & TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE))
	{
		should_disable_sd_clock = 1;
		sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
		_sdmmc_get_clkcon(sdmmc);
		usleep((8000 + sdmmc->divisor - 1) / sdmmc->divisor);
	}

	int res = _sdmmc_execute_cmd_inner(sdmmc, cmd, req, blkcnt_out);
	usleep((8000 + sdmmc->divisor - 1) / sdmmc->divisor);
	if (should_disable_sd_clock)
		sdmmc->regs->clkcon &= ~TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;

	return res;
}

int sdmmc_enable_low_voltage(sdmmc_t *sdmmc)
{
	if(sdmmc->id != SDMMC_1)
		return 0;

	if (!sdmmc_setup_clock(sdmmc, 8))
		return 0;

	_sdmmc_get_clkcon(sdmmc);

	max77620_regulator_set_voltage(REGULATOR_LDO2, 1800000);
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_PWR_DET_VAL, ~(1 << 12), (1 << 12));

	_sdmmc_autocal_config_offset(sdmmc, SDMMC_POWER_1_8);
	_sdmmc_autocal_execute(sdmmc, SDMMC_POWER_1_8);
	_sdmmc_set_voltage(sdmmc, SDMMC_POWER_1_8);
	_sdmmc_get_clkcon(sdmmc);
	msleep(5);
	
	if (sdmmc->regs->hostctl2 & SDHCI_CTRL_VDD_180)
	{
		sdmmc->regs->clkcon |= TEGRA_MMC_CLKCON_SD_CLOCK_ENABLE;
		_sdmmc_get_clkcon(sdmmc);
		msleep(1);
		if ((sdmmc->regs->prnsts & 0xF00000) == 0xF00000)
			return 1;
	}

	return 0;
}
