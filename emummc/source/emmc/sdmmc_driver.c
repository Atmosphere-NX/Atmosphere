/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018-2020 CTCaer
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
#include "mmc.h"
#include "sdmmc.h"
#include "../nx/cache.h"
#include "../power/max7762x.h"
#include "../soc/clock.h"
#include "../soc/gpio.h"
#include "../soc/pinmux.h"
#include "../soc/pmc.h"
#include "../soc/t210.h"
#include "../utils/fatal.h"
#include "../utils/types.h"
#include "../utils/util.h"

#define DPRINTF(...)

/*! SCMMC controller base addresses. */
static const u64 _sdmmc_bases[4] = {
	0x700B0000,
	0x700B0200,
	0x700B0400,
	0x700B0600,
};

int sdmmc_get_io_power(sdmmc_t *sdmmc)
{
	u32 p = sdmmc->regs->pwrcon;
	if (!(p & SDHCI_POWER_ON))
		return SDMMC_POWER_OFF;
	if (p & SDHCI_POWER_180)
		return SDMMC_POWER_1_8;
	if (p & SDHCI_POWER_330)
		return SDMMC_POWER_3_3;
	return -1;
}

static int _sdmmc_set_io_power(sdmmc_t *sdmmc, u32 power)
{
	switch (power)
	{
	case SDMMC_POWER_OFF:
		sdmmc->regs->pwrcon &= ~SDHCI_POWER_ON;
		break;

	case SDMMC_POWER_1_8:
		sdmmc->regs->pwrcon = SDHCI_POWER_180;
		break;

	case SDMMC_POWER_3_3:
		sdmmc->regs->pwrcon = SDHCI_POWER_330;
		break;

	default:
		return 0;
	}

	if (power != SDMMC_POWER_OFF)
		sdmmc->regs->pwrcon |= SDHCI_POWER_ON;

	return 1;
}

u32 sdmmc_get_bus_width(sdmmc_t *sdmmc)
{
	u32 h = sdmmc->regs->hostctl;
	if (h & SDHCI_CTRL_8BITBUS)
		return SDMMC_BUS_WIDTH_8;
	if (h & SDHCI_CTRL_4BITBUS)
		return SDMMC_BUS_WIDTH_4;
	return SDMMC_BUS_WIDTH_1;
}

void sdmmc_set_bus_width(sdmmc_t *sdmmc, u32 bus_width)
{
	u32 host_control = sdmmc->regs->hostctl & ~(SDHCI_CTRL_4BITBUS | SDHCI_CTRL_8BITBUS);

	if (bus_width == SDMMC_BUS_WIDTH_1)
		sdmmc->regs->hostctl = host_control;
	else if (bus_width == SDMMC_BUS_WIDTH_4)
		sdmmc->regs->hostctl = host_control | SDHCI_CTRL_4BITBUS;
	else if (bus_width == SDMMC_BUS_WIDTH_8)
		sdmmc->regs->hostctl = host_control | SDHCI_CTRL_8BITBUS;
}

void sdmmc_save_tap_value(sdmmc_t *sdmmc)
{
	sdmmc->venclkctl_tap = sdmmc->regs->venclkctl >> 16;
	sdmmc->venclkctl_set = 1;
}

static int _sdmmc_config_tap_val(sdmmc_t *sdmmc, u32 type)
{
	const u32 dqs_trim_val = 0x28;
	const u32 tap_values_t210[] = { 4, 0, 3, 0 };

	u32 tap_val = 0;

	if (type == SDHCI_TIMING_MMC_HS400)
		sdmmc->regs->vencapover = (sdmmc->regs->vencapover & 0xFFFFC0FF) | (dqs_trim_val << 8);

	sdmmc->regs->ventunctl0 &= ~TEGRA_MMC_VNDR_TUN_CTRL0_TAP_VAL_UPDATED_BY_HW;

	if (type == SDHCI_TIMING_MMC_HS400)
	{
		if (!sdmmc->venclkctl_set)
			return 0;

		tap_val = sdmmc->venclkctl_tap;
	}
	else
		tap_val = sdmmc->t210b01 ? 11 : tap_values_t210[sdmmc->id];

	sdmmc->regs->venclkctl = (sdmmc->regs->venclkctl & 0xFF00FFFF) | (tap_val << 16);

	return 1;
}

static int _sdmmc_commit_changes(sdmmc_t *sdmmc)
{
	return sdmmc->regs->clkcon;
}

static void _sdmmc_pad_config_fallback(sdmmc_t *sdmmc, u32 power)
{
	_sdmmc_commit_changes(sdmmc);
	switch (sdmmc->id)
	{
	case SDMMC_1: // 33 Ohm 2X Driver.
		if (power == SDMMC_POWER_OFF)
			break;
		u32 sdmmc1_pad_cfg = APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL) & 0xF8080FFF;
		if (sdmmc->t210b01)
			sdmmc1_pad_cfg |= (0x808 << 12); // Up:  8, Dn:  8. For 33 ohm.
		else if (power == SDMMC_POWER_1_8)
			sdmmc1_pad_cfg |= (0xB0F << 12); // Up: 11, Dn: 15. For 33 ohm.
		else if (power == SDMMC_POWER_3_3)
			sdmmc1_pad_cfg |= (0xC0C << 12); // Up: 12, Dn: 12. For 33 ohm.
		APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL) = sdmmc1_pad_cfg;
		(void)APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL); // Commit write.
		break;

	case SDMMC_2:
		if (sdmmc->t210b01)
			APB_MISC(APB_MISC_GP_EMMC2_PAD_CFGPADCTRL) = (APB_MISC(APB_MISC_GP_EMMC2_PAD_CFGPADCTRL) & 0xF8080FFF) | 0xA0A000;
		else
			APB_MISC(APB_MISC_GP_EMMC2_PAD_CFGPADCTRL) = (APB_MISC(APB_MISC_GP_EMMC2_PAD_CFGPADCTRL) & 0xFFFFC003) | 0x1040; // PU:16, PD:16.
		(void)APB_MISC(APB_MISC_GP_EMMC2_PAD_CFGPADCTRL);
		break;

	case SDMMC_4: // 50 Ohm 2X Driver. PU:16, PD:16, B01: PU:10, PD:10.
		APB_MISC(APB_MISC_GP_EMMC4_PAD_CFGPADCTRL) =
			(APB_MISC(APB_MISC_GP_EMMC4_PAD_CFGPADCTRL) & 0xFFFFC003) | (sdmmc->t210b01 ? 0xA28 : 0x1040);
		(void)APB_MISC(APB_MISC_GP_EMMC4_PAD_CFGPADCTRL); // Commit write.
		break;
	}
}

static void _sdmmc_autocal_execute(sdmmc_t *sdmmc, u32 power)
{
	bool should_enable_sd_clock = false;
	if (sdmmc->regs->clkcon & SDHCI_CLOCK_CARD_EN)
	{
		should_enable_sd_clock = true;
		sdmmc->regs->clkcon &= ~SDHCI_CLOCK_CARD_EN;
	}

	// Enable E_INPUT power.
	if (!(sdmmc->regs->sdmemcmppadctl & TEGRA_MMC_SDMEMCOMPPADCTRL_PAD_E_INPUT_PWRD))
	{
		sdmmc->regs->sdmemcmppadctl |= TEGRA_MMC_SDMEMCOMPPADCTRL_PAD_E_INPUT_PWRD;
		_sdmmc_commit_changes(sdmmc);
		usleep(1);
	}

	// Enable auto calibration and start auto configuration.
	sdmmc->regs->autocalcfg |= TEGRA_MMC_AUTOCALCFG_AUTO_CAL_ENABLE | TEGRA_MMC_AUTOCALCFG_AUTO_CAL_START;
	_sdmmc_commit_changes(sdmmc);
	usleep(2);

	u64 timeout = get_tmr_ms() + 10;
	while (sdmmc->regs->autocalsts & TEGRA_MMC_AUTOCALSTS_AUTO_CAL_ACTIVE)
	{
		if (get_tmr_ms() > timeout)
		{
			timeout = 0; // Set timeout to 0 if we timed out.
			break;
		}
	}

#if 0
	// Check if Comp pad is open or short to ground.
	// SDMMC1: CZ pads - T210/T210B01: 7-bit/5-bit. SDMMC2/4: LV_CZ pads - 5-bit.
	u8 code_mask = (sdmmc->t210b01 || sdmmc->id != SDMMC_1) ? 0x1F : 0x7F;
	u8 autocal_pu_status = sdmmc->regs->autocalsts & code_mask;
	if (!autocal_pu_status)
		EPRINTF("SDMMC: Comp Pad short to gnd!");
	else if (autocal_pu_status == code_mask)
		EPRINTF("SDMMC: Comp Pad open!");
#endif

	// In case auto calibration fails, we load suggested standard values.
	if (!timeout)
	{
		_sdmmc_pad_config_fallback(sdmmc, power);
		sdmmc->regs->autocalcfg &= ~TEGRA_MMC_AUTOCALCFG_AUTO_CAL_ENABLE;
	}

	// Disable E_INPUT to conserve power.
	sdmmc->regs->sdmemcmppadctl &= ~TEGRA_MMC_SDMEMCOMPPADCTRL_PAD_E_INPUT_PWRD;

	if(should_enable_sd_clock)
		sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;
}

static int _sdmmc_dll_cal_execute(sdmmc_t *sdmmc)
{
	int result = 1, should_disable_sd_clock = 0;

	if (!(sdmmc->regs->clkcon & SDHCI_CLOCK_CARD_EN))
	{
		should_disable_sd_clock = 1;
		sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;
	}

	sdmmc->regs->vendllcalcfg |= TEGRA_MMC_DLLCAL_CFG_EN_CALIBRATE;
	_sdmmc_commit_changes(sdmmc);

	u64 timeout = get_tmr_ms() + 5;
	while (sdmmc->regs->vendllcalcfg & TEGRA_MMC_DLLCAL_CFG_EN_CALIBRATE)
	{
		if (get_tmr_ms() > timeout)
		{
			result = 0;
			goto out;
		}
	}

	timeout = get_tmr_ms() + 10;
	while (sdmmc->regs->vendllcalcfgsts & TEGRA_MMC_DLLCAL_CFG_STATUS_DLL_ACTIVE)
	{
		if (get_tmr_ms() > timeout)
		{
			result = 0;
			goto out;
		}
	}

out:;
	if (should_disable_sd_clock)
		sdmmc->regs->clkcon &= ~SDHCI_CLOCK_CARD_EN;
	return result;
}

static void _sdmmc_reset(sdmmc_t *sdmmc)
{
	sdmmc->regs->swrst |= SDHCI_RESET_CMD | SDHCI_RESET_DATA;
	_sdmmc_commit_changes(sdmmc);
	u64 timeout = get_tmr_ms() + 2000;
	while ((sdmmc->regs->swrst & (SDHCI_RESET_CMD | SDHCI_RESET_DATA)) && get_tmr_ms() < timeout)
		;
}

static void _sdmmc_reset_all(sdmmc_t *sdmmc)
{
	sdmmc->regs->swrst |= SDHCI_RESET_ALL;
	_sdmmc_commit_changes(sdmmc);
	u32 timeout = get_tmr_ms() + 2000;//100ms
	while ((sdmmc->regs->swrst & SDHCI_RESET_ALL) && get_tmr_ms() < timeout)
		;
}

int sdmmc_setup_clock(sdmmc_t *sdmmc, u32 type)
{
	// Disable the SD clock if it was enabled, and reenable it later.
	bool should_enable_sd_clock = false;
	if (sdmmc->regs->clkcon & SDHCI_CLOCK_CARD_EN)
	{
		should_enable_sd_clock = true;
		sdmmc->regs->clkcon &= ~SDHCI_CLOCK_CARD_EN;
	}

	_sdmmc_config_tap_val(sdmmc, type);

	_sdmmc_reset(sdmmc);

	switch (type)
	{
	case SDHCI_TIMING_MMC_ID:
	case SDHCI_TIMING_MMC_LS26:
	case SDHCI_TIMING_SD_ID:
	case SDHCI_TIMING_SD_DS12:
		sdmmc->regs->hostctl  &= ~SDHCI_CTRL_HISPD;
		sdmmc->regs->hostctl2 &= ~SDHCI_CTRL_VDD_180;
		break;

	case SDHCI_TIMING_MMC_HS52:
	case SDHCI_TIMING_SD_HS25:
		sdmmc->regs->hostctl  |= SDHCI_CTRL_HISPD;
		sdmmc->regs->hostctl2 &= ~SDHCI_CTRL_VDD_180;
		break;

	case SDHCI_TIMING_MMC_HS200:
	case SDHCI_TIMING_UHS_SDR50: // T210 Errata for SDR50, the host must be set to SDR104.
	case SDHCI_TIMING_UHS_SDR104:
	case SDHCI_TIMING_UHS_SDR82:
	case SDHCI_TIMING_UHS_DDR50:
	case SDHCI_TIMING_MMC_HS102:
		sdmmc->regs->hostctl2  = (sdmmc->regs->hostctl2 & SDHCI_CTRL_UHS_MASK) | UHS_SDR104_BUS_SPEED;
		sdmmc->regs->hostctl2 |= SDHCI_CTRL_VDD_180;
		break;

	case SDHCI_TIMING_MMC_HS400:
		// Non standard.
		sdmmc->regs->hostctl2  = (sdmmc->regs->hostctl2 & SDHCI_CTRL_UHS_MASK) | HS400_BUS_SPEED;
		sdmmc->regs->hostctl2 |= SDHCI_CTRL_VDD_180;
		break;

	case SDHCI_TIMING_UHS_SDR25:
		sdmmc->regs->hostctl2  = (sdmmc->regs->hostctl2 & SDHCI_CTRL_UHS_MASK) | UHS_SDR25_BUS_SPEED;
		sdmmc->regs->hostctl2 |= SDHCI_CTRL_VDD_180;
		break;

	case SDHCI_TIMING_UHS_SDR12:
		sdmmc->regs->hostctl2  = (sdmmc->regs->hostctl2 & SDHCI_CTRL_UHS_MASK) | UHS_SDR12_BUS_SPEED;
		sdmmc->regs->hostctl2 |= SDHCI_CTRL_VDD_180;
		break;
	}

	_sdmmc_commit_changes(sdmmc);

	u32 clock;
	u16 divisor;
	clock_sdmmc_get_card_clock_div(&clock, &divisor, type);
	clock_sdmmc_config_clock_source(&clock, sdmmc->id, clock);
	sdmmc->divisor = (clock + divisor - 1) / divisor;

	//if divisor != 1 && divisor << 31 -> error

	u16 div = divisor >> 1;
	divisor = 0;
	if (div > 0xFF)
		divisor = div >> SDHCI_DIVIDER_SHIFT;

	sdmmc->regs->clkcon = (sdmmc->regs->clkcon & ~(SDHCI_DIV_MASK | SDHCI_DIV_HI_MASK))
		| (div << SDHCI_DIVIDER_SHIFT) | (divisor << SDHCI_DIVIDER_HI_SHIFT);

	// Enable the SD clock again.
	if (should_enable_sd_clock)
		sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;

	if (type == SDHCI_TIMING_MMC_HS400)
		return _sdmmc_dll_cal_execute(sdmmc);
	return 1;
}

static void _sdmmc_card_clock_enable(sdmmc_t *sdmmc)
{
	// Recalibrate conditionally.
	if (sdmmc->manual_cal && !sdmmc->powersave_enabled)
		_sdmmc_autocal_execute(sdmmc, sdmmc_get_io_power(sdmmc));

	if (!sdmmc->powersave_enabled)
	{
		if (!(sdmmc->regs->clkcon & SDHCI_CLOCK_CARD_EN))
			sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;
	}
	sdmmc->card_clock_enabled = 1;
}

static void _sdmmc_sd_clock_disable(sdmmc_t *sdmmc)
{
	sdmmc->card_clock_enabled = 0;
	sdmmc->regs->clkcon &= ~SDHCI_CLOCK_CARD_EN;
}

void sdmmc_card_clock_powersave(sdmmc_t *sdmmc, int powersave_enable)
{
	// Recalibrate periodically for SDMMC1.
	if (sdmmc->manual_cal && !powersave_enable && sdmmc->card_clock_enabled)
		_sdmmc_autocal_execute(sdmmc, sdmmc_get_io_power(sdmmc));

	sdmmc->powersave_enabled = powersave_enable;
	if (powersave_enable)
	{
		if (sdmmc->regs->clkcon & SDHCI_CLOCK_CARD_EN)
			sdmmc->regs->clkcon &= ~SDHCI_CLOCK_CARD_EN;
		return;
	}

	if (sdmmc->card_clock_enabled)
		if (!(sdmmc->regs->clkcon & SDHCI_CLOCK_CARD_EN))
			sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;
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
	}

	return 1;
}

static int _sdmmc_wait_cmd_data_inhibit(sdmmc_t *sdmmc, bool wait_dat)
{
	_sdmmc_commit_changes(sdmmc);

	u64 timeout = get_tmr_ms() + 2000;
	while(sdmmc->regs->prnsts & SDHCI_CMD_INHIBIT)
		if (get_tmr_ms() > timeout)
		{
			_sdmmc_reset(sdmmc);
			return 0;
		}

	if (wait_dat)
	{
		timeout = get_tmr_ms() + 2000;
		while (sdmmc->regs->prnsts & SDHCI_DATA_INHIBIT)
			if (get_tmr_ms() > timeout)
			{
				_sdmmc_reset(sdmmc);
				return 0;
			}
	}

	return 1;
}

static int _sdmmc_wait_card_busy(sdmmc_t *sdmmc)
{
	_sdmmc_commit_changes(sdmmc);

	u64 timeout = get_tmr_ms() + 2000;
	while (!(sdmmc->regs->prnsts & SDHCI_DATA_0_LVL_MASK))
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

	case SDMMC_BUS_WIDTH_4:
		sdmmc->regs->blksize = 64;
		break;

	case SDMMC_BUS_WIDTH_8:
		sdmmc->regs->blksize = 128;
		break;
	}

	sdmmc->regs->blkcnt = 1;
	sdmmc->regs->trnmod = SDHCI_TRNS_READ;

	return 1;
}

static int _sdmmc_send_cmd(sdmmc_t *sdmmc, sdmmc_cmd_t *cmd, bool is_data_present)
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
			cmdflags = SDHCI_CMD_RESP_LEN48_BUSY | SDHCI_CMD_INDEX | SDHCI_CMD_CRC;
		else
			cmdflags = SDHCI_CMD_RESP_LEN48 | SDHCI_CMD_INDEX | SDHCI_CMD_CRC;
		break;

	case SDMMC_RSP_TYPE_2:
		cmdflags = SDHCI_CMD_RESP_LEN136 | SDHCI_CMD_CRC;
		break;

	case SDMMC_RSP_TYPE_3:
		cmdflags = SDHCI_CMD_RESP_LEN48;
		break;

	default:
		return 0;
	}

	if (is_data_present)
		cmdflags |= SDHCI_CMD_DATA;
	sdmmc->regs->argument = cmd->arg;
	sdmmc->regs->cmdreg = (cmd->cmd << 8) | cmdflags;

	return 1;
}

static void _sdmmc_send_tuning_cmd(sdmmc_t *sdmmc, u32 cmd)
{
	sdmmc_cmd_t cmdbuf;
	cmdbuf.cmd = cmd;
	cmdbuf.arg = 0;
	cmdbuf.rsp_type = SDMMC_RSP_TYPE_1;
	cmdbuf.check_busy = 0;
	_sdmmc_send_cmd(sdmmc, &cmdbuf, true);
}

static int _sdmmc_tuning_execute_once(sdmmc_t *sdmmc, u32 cmd)
{
	if (sdmmc->powersave_enabled)
		return 0;
	if (!_sdmmc_wait_cmd_data_inhibit(sdmmc, true))
		return 0;

	_sdmmc_setup_read_small_block(sdmmc);

	sdmmc->regs->norintstsen |= SDHCI_INT_DATA_AVAIL;
	sdmmc->regs->norintsts = sdmmc->regs->norintsts;
	sdmmc->regs->clkcon &= ~SDHCI_CLOCK_CARD_EN;

	_sdmmc_send_tuning_cmd(sdmmc, cmd);
	_sdmmc_commit_changes(sdmmc);
	usleep(1);

	_sdmmc_reset(sdmmc);

	sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;
	_sdmmc_commit_changes(sdmmc);

	u64 timeout = get_tmr_us() + 5000;
	while (get_tmr_us() < timeout)
	{
		if (sdmmc->regs->norintsts & SDHCI_INT_DATA_AVAIL)
		{
			sdmmc->regs->norintsts = SDHCI_INT_DATA_AVAIL;
			sdmmc->regs->norintstsen &= ~SDHCI_INT_DATA_AVAIL;
			_sdmmc_commit_changes(sdmmc);
			usleep((1000 * 8 + sdmmc->divisor - 1) / sdmmc->divisor);
			return 1;
		}
	}

	_sdmmc_reset(sdmmc);

	sdmmc->regs->norintstsen &= ~SDHCI_INT_DATA_AVAIL;
	_sdmmc_commit_changes(sdmmc);
	usleep((1000 * 8 + sdmmc->divisor - 1) / sdmmc->divisor);

	return 0;
}

int sdmmc_tuning_execute(sdmmc_t *sdmmc, u32 type, u32 cmd)
{
	u32 max = 0, flag = 0;

	switch (type)
	{
	case SDHCI_TIMING_MMC_HS200:
	case SDHCI_TIMING_MMC_HS400:
	case SDHCI_TIMING_UHS_SDR104:
	case SDHCI_TIMING_UHS_SDR82:
		max = 128;
		flag = (2 << 13); // 128 iterations.
		break;

	case SDHCI_TIMING_UHS_SDR50:
	case SDHCI_TIMING_UHS_DDR50:
	case SDHCI_TIMING_MMC_HS102:
		max = 256;
		flag = (4 << 13); // 256 iterations.
		break;

	case SDHCI_TIMING_UHS_SDR12:
	case SDHCI_TIMING_UHS_SDR25:
		return 1;

	default:
		return 0;
	}

	sdmmc->regs->ventunctl1 = 0; // step_size 1.

	sdmmc->regs->ventunctl0 = (sdmmc->regs->ventunctl0 & 0xFFFF1FFF) | flag; // Tries.
	sdmmc->regs->ventunctl0 = (sdmmc->regs->ventunctl0 & 0xFFFFE03F) | (1 << 6); // 1x Multiplier.
	sdmmc->regs->ventunctl0 |= TEGRA_MMC_VNDR_TUN_CTRL0_TAP_VAL_UPDATED_BY_HW;
	sdmmc->regs->hostctl2   |= SDHCI_CTRL_EXEC_TUNING;

	for (u32 i = 0; i < max; i++)
	{
		_sdmmc_tuning_execute_once(sdmmc, cmd);
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
	sdmmc->regs->clkcon |= SDHCI_CLOCK_INT_EN;
	_sdmmc_commit_changes(sdmmc);
	u64 timeout = get_tmr_ms() + 2000;
	while (!(sdmmc->regs->clkcon & SDHCI_CLOCK_INT_STABLE))
	{
		if (get_tmr_ms() > timeout)
			return 0;
	}

	sdmmc->regs->hostctl2 &= ~SDHCI_CTRL_PRESET_VAL_EN;
	sdmmc->regs->clkcon   &= ~SDHCI_PROG_CLOCK_MODE;
	sdmmc->regs->hostctl2 |= SDHCI_HOST_VERSION_4_EN;

	if (!(sdmmc->regs->capareg & SDHCI_CAN_64BIT))
		return 0;

	sdmmc->regs->hostctl2 |= SDHCI_ADDRESSING_64BIT_EN;
	sdmmc->regs->hostctl &= ~SDHCI_CTRL_DMA_MASK;
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
		if (power == SDMMC_POWER_1_8)
		{
			if (!sdmmc->t210b01)
			{
				off_pd = 123;
				off_pu = 123;
			}
			else
			{
				off_pd = 6;
				off_pu = 6;
			}
		}
		else if (power == SDMMC_POWER_3_3)
		{
			if (!sdmmc->t210b01)
			{
				off_pd = 125;
				off_pu = 0;
			}
		}
		else
			return 0;
		break;
	}

	sdmmc->regs->autocalcfg = (sdmmc->regs->autocalcfg & 0xFFFF8080) | (off_pd << 8) | off_pu;
	return 1;
}

static void _sdmmc_enable_interrupts(sdmmc_t *sdmmc)
{
	sdmmc->regs->norintstsen |= SDHCI_INT_DMA_END | SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE;
	sdmmc->regs->errintstsen |= SDHCI_ERR_INT_ALL_EXCEPT_ADMA_BUSPWR;
	sdmmc->regs->norintsts = sdmmc->regs->norintsts;
	sdmmc->regs->errintsts = sdmmc->regs->errintsts;
}

static void _sdmmc_mask_interrupts(sdmmc_t *sdmmc)
{
	sdmmc->regs->errintstsen &= ~SDHCI_ERR_INT_ALL_EXCEPT_ADMA_BUSPWR;
	sdmmc->regs->norintstsen &= ~(SDHCI_INT_DMA_END | SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE);
}

static int _sdmmc_check_mask_interrupt(sdmmc_t *sdmmc, u16 *pout, u16 mask)
{
	u16 norintsts = sdmmc->regs->norintsts;
	u16 errintsts = sdmmc->regs->errintsts;

DPRINTF("norintsts %08X, errintsts %08X\n", norintsts, errintsts);

	if (pout)
		*pout = norintsts;

	// Check for error interrupt.
	if (norintsts & SDHCI_INT_ERROR)
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

static int _sdmmc_wait_response(sdmmc_t *sdmmc)
{
	_sdmmc_commit_changes(sdmmc);

	u64 timeout = get_tmr_ms() + 2000;
	while (true)
	{
		int result = _sdmmc_check_mask_interrupt(sdmmc, NULL, SDHCI_INT_RESPONSE);
		if (result == SDMMC_MASKINT_MASKED)
			break;
		if (result != SDMMC_MASKINT_NOERROR || get_tmr_ms() > timeout)
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

	if (!_sdmmc_wait_cmd_data_inhibit(sdmmc, false))
		return 0;

	_sdmmc_enable_interrupts(sdmmc);

	cmd.cmd = MMC_STOP_TRANSMISSION;
	cmd.arg = 0;
	cmd.rsp_type = SDMMC_RSP_TYPE_1;
	cmd.check_busy = 1;

	_sdmmc_send_cmd(sdmmc, &cmd, false);

	int result = _sdmmc_wait_response(sdmmc);
	_sdmmc_mask_interrupts(sdmmc);

	if (!result)
		return 0;

	_sdmmc_cache_rsp(sdmmc, rsp, 4, SDMMC_RSP_TYPE_1);

	return _sdmmc_wait_card_busy(sdmmc);
}

int sdmmc_stop_transmission(sdmmc_t *sdmmc, u32 *rsp)
{
	if (!sdmmc->card_clock_enabled)
		return 0;

	// Recalibrate periodically for SDMMC1.
	if (sdmmc->manual_cal && sdmmc->powersave_enabled)
		_sdmmc_autocal_execute(sdmmc, sdmmc_get_io_power(sdmmc));

	bool should_disable_sd_clock = false;
	if (!(sdmmc->regs->clkcon & SDHCI_CLOCK_CARD_EN))
	{
		should_disable_sd_clock = true;
		sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;
		_sdmmc_commit_changes(sdmmc);
		usleep((8000 + sdmmc->divisor - 1) / sdmmc->divisor);
	}

	int result = _sdmmc_stop_transmission_inner(sdmmc, rsp);
	usleep((8000 + sdmmc->divisor - 1) / sdmmc->divisor);

	if (should_disable_sd_clock)
		sdmmc->regs->clkcon &= ~SDHCI_CLOCK_CARD_EN;

	return result;
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

	// Check alignment.
	if (admaaddr & 7)
		return 0;

    sdmmc->regs->admaaddr = admaaddr & 0xFFFFFFFFF;
    sdmmc->regs->admaaddr_hi = (admaaddr >> 32) & 0xFFFFFFFFF;

	sdmmc->dma_addr_next = (admaaddr + 0x80000) & 0xFFFFFFFFFFF80000;

	sdmmc->regs->blksize = req->blksize | 0x7000; // DMA 512KB (Detects A18 carry out).
	sdmmc->regs->blkcnt = blkcnt;

	if (blkcnt_out)
		*blkcnt_out = blkcnt;

	u32 trnmode = SDHCI_TRNS_DMA;
	if (req->is_multi_block)
		trnmode = SDHCI_TRNS_MULTI | SDHCI_TRNS_BLK_CNT_EN | SDHCI_TRNS_DMA;
	if (!req->is_write)
		trnmode |= SDHCI_TRNS_READ;
	if (req->is_auto_cmd12)
		trnmode = (trnmode & ~(SDHCI_TRNS_AUTO_CMD12 | SDHCI_TRNS_AUTO_CMD23)) | SDHCI_TRNS_AUTO_CMD12;

	sdmmc->regs->trnmod = trnmode;

	return 1;
}

static int _sdmmc_update_dma(sdmmc_t *sdmmc)
{
	u16 blkcnt = 0;
	do
	{
		blkcnt = sdmmc->regs->blkcnt;
		u64 timeout = get_tmr_ms() + 1500;
		do
		{
			int result = 0;
			while (true)
			{
				u16 intr = 0;
				result = _sdmmc_check_mask_interrupt(sdmmc, &intr,
					SDHCI_INT_DATA_END | SDHCI_INT_DMA_END);
				if (result < 0)
					break;

				if (intr & SDHCI_INT_DATA_END)
					return 1; // Transfer complete.

				if (intr & SDHCI_INT_DMA_END)
				{
					// Update DMA.
					sdmmc->regs->admaaddr = sdmmc->dma_addr_next & 0xFFFFFFFFF;
					sdmmc->regs->admaaddr_hi = (sdmmc->dma_addr_next >> 32) & 0xFFFFFFFFF;
					sdmmc->dma_addr_next += 0x80000;
				}
			}
			if (result != SDMMC_MASKINT_NOERROR)
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
	if (!_sdmmc_wait_cmd_data_inhibit(sdmmc, has_req_or_check_busy))
		return 0;

	u32 blkcnt = 0;
	bool is_data_present = false;
	if (req)
	{
		if (!_sdmmc_config_dma(sdmmc, &blkcnt, req))
			return 0;

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

		is_data_present = true;
	}

	_sdmmc_enable_interrupts(sdmmc);

	if (!_sdmmc_send_cmd(sdmmc, cmd, is_data_present))
		return 0;

	int result = _sdmmc_wait_response(sdmmc);
DPRINTF("rsp(%d): %08X, %08X, %08X, %08X\n", result,
		sdmmc->regs->rspreg0, sdmmc->regs->rspreg1, sdmmc->regs->rspreg2, sdmmc->regs->rspreg3);
	if (result)
	{
		if (cmd->rsp_type)
		{
			sdmmc->expected_rsp_type = cmd->rsp_type;
			result = _sdmmc_cache_rsp(sdmmc, sdmmc->rsp, 0x10, cmd->rsp_type);
		}

		if (req && result)
			result = _sdmmc_update_dma(sdmmc);
	}

	_sdmmc_mask_interrupts(sdmmc);

	if (result)
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
			return _sdmmc_wait_card_busy(sdmmc);
	}

	return result;
}

int sdmmc_get_sd_power_enabled()
{
	return gpio_read(GPIO_PORT_E, GPIO_PIN_4);
}

bool sdmmc_get_sd_inserted()
{
	return (!gpio_read(GPIO_PORT_Z, GPIO_PIN_1));
}

static void _sdmmc_config_sdmmc1_schmitt()
{
	PINMUX_AUX(PINMUX_AUX_SDMMC1_CLK)  |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_CMD)  |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT3) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT2) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT1) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT0) |= PINMUX_SCHMT;
}

static void _sdmmc_config_sdmmc2_schmitt()
{
	PINMUX_AUX(PINMUX_AUX_SDMMC2_CLK)  |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_CMD)  |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_DAT7) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_DAT6) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_DAT5) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_DAT4) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_DAT3) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_DAT2) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_DAT1) |= PINMUX_SCHMT;
	PINMUX_AUX(PINMUX_AUX_SDMMC2_DAT0) |= PINMUX_SCHMT;
}

static void _sdmmc_config_sdmmc1_pads(bool discharge)
{
	u32 sdmmc1_pin_mask = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;

	// Set values for Reset state.
	u32 function = GPIO_MODE_SPIO;
	u32 level    = GPIO_LOW;
	u32 output   = GPIO_OUTPUT_DISABLE;

	// Set values for dicharging.
	if (discharge)
	{
		function = GPIO_MODE_GPIO;
		level    = GPIO_HIGH;
		output   = GPIO_OUTPUT_ENABLE;
	}

	// Set all pads function.
	gpio_config(GPIO_PORT_M, sdmmc1_pin_mask, function);
	// Set all pads output level.
	gpio_write(GPIO_PORT_M, sdmmc1_pin_mask, level);
	// Set all pads output.
	gpio_output_enable(GPIO_PORT_M, sdmmc1_pin_mask, output);
}

static int _sdmmc_config_sdmmc1(bool t210b01)
{
	// Configure SD card detect.
	PINMUX_AUX(PINMUX_AUX_GPIO_PZ1) = PINMUX_INPUT_ENABLE | PINMUX_PULL_UP | 2; // GPIO control, pull up.
	APB_MISC(APB_MISC_GP_VGPIO_GPIO_MUX_SEL) = 0;
	gpio_config(GPIO_PORT_Z, GPIO_PIN_1, GPIO_MODE_GPIO);
	gpio_output_enable(GPIO_PORT_Z, GPIO_PIN_1, GPIO_OUTPUT_DISABLE);
	usleep(100);

	// Check if SD card is inserted.
	if(!sdmmc_get_sd_inserted())
		return 0;

	/*
	* Pinmux config:
	*  DRV_TYPE = DRIVE_2X (for 33 Ohm driver)
	*  E_SCHMT  = ENABLE (for 1.8V),  DISABLE (for 3.3V)
	*  E_INPUT  = ENABLE
	*  TRISTATE = PASSTHROUGH
	*  APB_MISC_GP_SDMMCx_CLK_LPBK_CONTROL = SDMMCx_CLK_PAD_E_LPBK for CLK
	*/

	// Enable deep loopback for SDMMC1 CLK pad.
	APB_MISC(APB_MISC_GP_SDMMC1_CLK_LPBK_CONTROL) = 1;

	// Configure SDMMC1 CLK pinmux, based on state and SoC type.
	if (PINMUX_AUX(PINMUX_AUX_SDMMC1_CLK) != (PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PULL_DOWN)) // Check if CLK pad is already configured.
		PINMUX_AUX(PINMUX_AUX_SDMMC1_CLK) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | (t210b01 ? PINMUX_PULL_NONE : PINMUX_PULL_DOWN);

	// Configure the reset of SDMMC1 pins pinmux.
	PINMUX_AUX(PINMUX_AUX_SDMMC1_CMD)  = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PULL_UP;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT3) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PULL_UP;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT2) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PULL_UP;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT1) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PULL_UP;
	PINMUX_AUX(PINMUX_AUX_SDMMC1_DAT0) = PINMUX_DRIVE_2X | PINMUX_INPUT_ENABLE | PINMUX_PULL_UP;

	// Force schmitt trigger for T210B01.
	if (t210b01)
		_sdmmc_config_sdmmc1_schmitt();

	// Make sure the SDMMC1 controller is powered.
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_NO_IOPOWER, ~PMC_NO_IOPOWER_SDMMC1_IO_EN, PMC_NO_IOPOWER_SDMMC1_IO_EN);
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_NO_IOPOWER, 0, 0); // Commit write.

	// Inform IO pads that voltage is gonna be 3.3V.
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_PWR_DET_VAL, PMC_PWR_DET_SDMMC1_IO_EN, PMC_PWR_DET_SDMMC1_IO_EN);
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_PWR_DET_VAL, 0, 0); // Commit write.

	// Set enable SD card power.
	//PINMUX_AUX(PINMUX_AUX_DMIC3_CLK) = PINMUX_PULL_DOWN | 2; // Proper pinmuxing. Breaks on HOS, takes over 1 minute to recover.
	PINMUX_AUX(PINMUX_AUX_DMIC3_CLK) = PINMUX_INPUT_ENABLE | PINMUX_PULL_DOWN | 1; // Wrong but working pinmuxing. Instant take over by FS.
	gpio_config(GPIO_PORT_E, GPIO_PIN_4, GPIO_MODE_GPIO);
	gpio_write(GPIO_PORT_E, GPIO_PIN_4, GPIO_HIGH);
	gpio_output_enable(GPIO_PORT_E, GPIO_PIN_4, GPIO_OUTPUT_ENABLE);
	usleep(10000);

	// Enable SD card IO power.
	max77620_regulator_set_voltage(REGULATOR_LDO2, 3300000);
	max77620_regulator_enable(REGULATOR_LDO2, 1);
	usleep(1000);

	// Set pad slew codes to get good quality clock.
	if (!t210b01)
	{
		APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL) = (APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL) & 0xFFFFFFF) | 0x50000000;
		(void)APB_MISC(APB_MISC_GP_SDMMC1_PAD_CFGPADCTRL); // Commit write.
		usleep(1000);
	}

	return 1;
}

static void _sdmmc_config_emmc(u32 id, bool t210b01)
{
	switch (id)
	{
	case SDMMC_2:
		if (!t210b01)
		{
			// Unset park for pads.
			APB_MISC(APB_MISC_GP_EMMC2_PAD_CFGPADCTRL) &= 0xF8003FFF;
			(void)APB_MISC(APB_MISC_GP_EMMC2_PAD_CFGPADCTRL); // Commit write.
		}
		else // Enable schmitt trigger for T210B01.
			_sdmmc_config_sdmmc2_schmitt();
		break;

	case SDMMC_4:
		// Unset park for pads.
		APB_MISC(APB_MISC_GP_EMMC4_PAD_CFGPADCTRL) &= 0xF8003FFF;
		// Set default pad cfg.
		if (t210b01)
			APB_MISC(APB_MISC_GP_EMMC4_PAD_PUPD_CFGPADCTRL) &= 0xFFBFFFF9; // Unset CMD/CLK/DQS powedown.
		// Enable schmitt trigger.
		APB_MISC(APB_MISC_GP_EMMC4_PAD_CFGPADCTRL) |= 1;
		(void)APB_MISC(APB_MISC_GP_EMMC4_PAD_CFGPADCTRL); // Commit write.
		break;
	}
}

int sdmmc_init(sdmmc_t *sdmmc, u32 id, u32 power, u32 bus_width, u32 type, int powersave_enable)
{
	u32 clock;
	u16 divisor;
	u8 vref_sel = 7;

	const u32 trim_values_t210[] = { 2, 8, 3, 8 };
	const u32 trim_values_t210b01[] = { 14, 13, 15, 13 };
	const u32 *trim_values = sdmmc->t210b01 ? trim_values_t210b01 : trim_values_t210;

	if (id > SDMMC_4 || id == SDMMC_3)
		return 0;

	memset(sdmmc, 0, sizeof(sdmmc_t));

	sdmmc->regs = (t210_sdmmc_t *)QueryIoMapping(_sdmmc_bases[id], 0x200);
	sdmmc->id = id;
	sdmmc->clock_stopped = 1;
	sdmmc->t210b01 = splGetSocType() == SplSocType_Mariko;

	// Do specific SDMMC HW configuration.
	switch (id)
	{
	case SDMMC_1:
		if (!_sdmmc_config_sdmmc1(sdmmc->t210b01))
			return 0;
		if (sdmmc->t210b01)
			vref_sel = 0;
		else
			sdmmc->manual_cal = 1;
		break;

	case SDMMC_2:
	case SDMMC_4:
		_sdmmc_config_emmc(id, sdmmc->t210b01);
		break;
	}

	// Disable clock if enabled.
	if (clock_sdmmc_is_not_reset_and_enabled(id))
	{
		_sdmmc_sd_clock_disable(sdmmc);
		_sdmmc_commit_changes(sdmmc);
	}

	// Configure and enable selected clock.
	clock_sdmmc_get_card_clock_div(&clock, &divisor, type);
	clock_sdmmc_enable(id, clock);

	// Make sure all sdmmc registers are reset.
	_sdmmc_reset_all(sdmmc);

	sdmmc->clock_stopped = 0;

	// Set default pad IO trimming configuration.
	sdmmc->regs->iospare |= 0x80000; // Enable muxing.
	sdmmc->regs->veniotrimctl &= 0xFFFFFFFB; // Set Band Gap VREG to supply DLL.
	sdmmc->regs->venclkctl = (sdmmc->regs->venclkctl & 0xE0FFFFFB) | (trim_values[sdmmc->id] << 24);
	sdmmc->regs->sdmemcmppadctl =
		(sdmmc->regs->sdmemcmppadctl & TEGRA_MMC_SDMEMCOMPPADCTRL_COMP_VREF_SEL_MASK) | vref_sel;

	// Configure auto calibration values.
	if (!_sdmmc_autocal_config_offset(sdmmc, power))
		return 0;

	// Calibrate pads.
	_sdmmc_autocal_execute(sdmmc, power);

	// Enable internal clock and power.
	if (_sdmmc_enable_internal_clock(sdmmc))
	{
		sdmmc_set_bus_width(sdmmc, bus_width);
		_sdmmc_set_io_power(sdmmc, power);

		if (sdmmc_setup_clock(sdmmc, type))
		{
			sdmmc_card_clock_powersave(sdmmc, powersave_enable);
			_sdmmc_card_clock_enable(sdmmc);
			_sdmmc_commit_changes(sdmmc);

			return 1;
		}
	}

	return 0;
}

void sdmmc1_disable_power()
{
	// Clear pull down from CLK pad.
	PINMUX_AUX(PINMUX_AUX_SDMMC1_CLK) &= ~PINMUX_PULL_MASK;

	// Set pads to discharge state.
	_sdmmc_config_sdmmc1_pads(true);

	// Disable SD card IO power regulator.
	max77620_regulator_enable(REGULATOR_LDO2, 0);
	usleep(4000);

	// Disable SD card IO power pin.
	gpio_write(GPIO_PORT_E, GPIO_PIN_4, GPIO_LOW);

	// T210/T210B01 WAR: Set start timer for IO and Controller power discharge.
	msleep(239);

	// Disable SDMMC1 controller power.
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_NO_IOPOWER, PMC_NO_IOPOWER_SDMMC1_IO_EN, PMC_NO_IOPOWER_SDMMC1_IO_EN);
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_NO_IOPOWER, 0, 0); // Commit write.

	// Inform IO pads that next voltage might be 3.3V.
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_PWR_DET_VAL, PMC_PWR_DET_SDMMC1_IO_EN, PMC_PWR_DET_SDMMC1_IO_EN);
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_PWR_DET_VAL, 0, 0); // Commit write.

	// Set pads to reset state.
	_sdmmc_config_sdmmc1_pads(false);

	// Set pull down to CLK pad.
	PINMUX_AUX(PINMUX_AUX_SDMMC1_CLK) |= PINMUX_PULL_DOWN;
}

void sdmmc_end(sdmmc_t *sdmmc)
{
	if (!sdmmc->clock_stopped)
	{
		_sdmmc_sd_clock_disable(sdmmc);
		// Disable SDMMC power.
		_sdmmc_set_io_power(sdmmc, SDMMC_POWER_OFF);

		// Disable SD card power.
		if (sdmmc->id == SDMMC_1)
			sdmmc1_disable_power();

		_sdmmc_commit_changes(sdmmc);
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
	if (!sdmmc->card_clock_enabled)
		return 0;

	// Recalibrate periodically for SDMMC1.
	if (sdmmc->manual_cal && sdmmc->powersave_enabled)
		_sdmmc_autocal_execute(sdmmc, sdmmc_get_io_power(sdmmc));

	int should_disable_sd_clock = 0;
	if (!(sdmmc->regs->clkcon & SDHCI_CLOCK_CARD_EN))
	{
		should_disable_sd_clock = 1;
		sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;
		_sdmmc_commit_changes(sdmmc);
		usleep((8000 + sdmmc->divisor - 1) / sdmmc->divisor);
	}

	int result = _sdmmc_execute_cmd_inner(sdmmc, cmd, req, blkcnt_out);
	usleep((8000 + sdmmc->divisor - 1) / sdmmc->divisor);

	if (should_disable_sd_clock)
		sdmmc->regs->clkcon &= ~SDHCI_CLOCK_CARD_EN;

	return result;
}

int sdmmc_enable_low_voltage(sdmmc_t *sdmmc)
{
	if(sdmmc->id != SDMMC_1)
		return 0;

	if (!sdmmc_setup_clock(sdmmc, SDHCI_TIMING_UHS_SDR12))
		return 0;

	_sdmmc_commit_changes(sdmmc);

	// Switch to 1.8V and wait for regulator to stabilize. Assume max possible wait needed.
	max77620_regulator_set_voltage(REGULATOR_LDO2, 1800000);
	usleep(150);

	// Inform IO pads that we switched to 1.8V.
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_PWR_DET_VAL, ~PMC_PWR_DET_SDMMC1_IO_EN, PMC_PWR_DET_SDMMC1_IO_EN);
	smcReadWriteRegister(PMC_BASE + APBDEV_PMC_PWR_DET_VAL, 0, 0); // Commit write.

	// Enable schmitt trigger for better duty cycle and low jitter clock.
	_sdmmc_config_sdmmc1_schmitt();

	_sdmmc_autocal_config_offset(sdmmc, SDMMC_POWER_1_8);
	_sdmmc_autocal_execute(sdmmc, SDMMC_POWER_1_8);
	_sdmmc_set_io_power(sdmmc, SDMMC_POWER_1_8);
	_sdmmc_commit_changes(sdmmc);
	msleep(5); // Wait minimum 5ms before turning on the card clock.

	// Turn on SDCLK.
	if (sdmmc->regs->hostctl2 & SDHCI_CTRL_VDD_180)
	{
		sdmmc->regs->clkcon |= SDHCI_CLOCK_CARD_EN;
		_sdmmc_commit_changes(sdmmc);
		usleep(1000);
		if ((sdmmc->regs->prnsts & SDHCI_DATA_LVL_MASK) == SDHCI_DATA_LVL_MASK)
			return 1;
	}

	return 0;
}
