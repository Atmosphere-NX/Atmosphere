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

#include "../soc/clock.h"
#include "../soc/t210.h"
#include "../utils/util.h"
#include "../emmc/sdmmc.h"

static const sclock_t _clock_i2c5 = {
	CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_CLK_SOURCE_I2C5, 0xF, 0, 4 //81.6MHz -> 400KHz
};

static sclock_t _clock_sdmmc_legacy_tm = {
	CLK_RST_CONTROLLER_RST_DEVICES_Y, CLK_RST_CONTROLLER_CLK_OUT_ENB_Y, CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC_LEGACY_TM, 1, 4, 66
};

void clock_enable(const sclock_t *clk)
{
	// Put clock into reset.
	CLOCK(clk->reset) = (CLOCK(clk->reset) & ~(1 << clk->index)) | (1 << clk->index);
	// Disable.
	CLOCK(clk->enable) &= ~(1 << clk->index);
	// Configure clock source if required.
	if (clk->source)
		CLOCK(clk->source) = clk->clk_div | (clk->clk_src << 29);
	// Enable.
	CLOCK(clk->enable) = (CLOCK(clk->enable) & ~(1 << clk->index)) | (1 << clk->index);
	usleep(2);

	// Take clock off reset.
	CLOCK(clk->reset) &= ~(1 << clk->index);
}

void clock_disable(const sclock_t *clk)
{
	// Put clock into reset.
	CLOCK(clk->reset) = (CLOCK(clk->reset) & ~(1 << clk->index)) | (1 << clk->index);
	// Disable.
	CLOCK(clk->enable) &= ~(1 << clk->index);
}

void clock_enable_i2c5()
{
	clock_enable(&_clock_i2c5);
}

void clock_disable_i2c5()
{
	clock_disable(&_clock_i2c5);
}

static void _clock_enable_pllc4()
{
	if ((CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) & (PLLCX_BASE_ENABLE | PLLCX_BASE_LOCK | 0xFFFFFF))
		== (PLLCX_BASE_ENABLE | PLLCX_BASE_LOCK | (104 << 8) | 4))
		return;

	// Enable Phase and Frequency lock detection.
	//CLOCK(CLK_RST_CONTROLLER_PLLC4_MISC) = PLLC4_MISC_EN_LCKDET;

	// Disable PLL and IDDQ in case they are on.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLLCX_BASE_ENABLE;
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) &= ~PLLC4_BASE_IDDQ;
	(void)CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE);
	usleep(10);

	// Set PLLC4 dividers.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) = (104 << 8) | 4; // DIVM: 4, DIVP: 1.

	// Enable PLLC4 and wait for Phase and Frequency lock.
	CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) |= PLLCX_BASE_ENABLE;
	(void)CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE);
	while (!(CLOCK(CLK_RST_CONTROLLER_PLLC4_BASE) & PLLCX_BASE_LOCK))
		;

	msleep(1); // Wait a bit for PLL to stabilize.
}

#define L_SWR_SDMMC1_RST (1 << 14)
#define L_SWR_SDMMC2_RST (1 << 9)
#define L_SWR_SDMMC4_RST (1 << 15)
#define U_SWR_SDMMC3_RST (1 << 5)

#define L_CLK_ENB_SDMMC1 (1 << 14)
#define L_CLK_ENB_SDMMC2 (1 << 9)
#define L_CLK_ENB_SDMMC4 (1 << 15)
#define U_CLK_ENB_SDMMC3 (1 << 5)

#define L_SET_SDMMC1_RST (1 << 14)
#define L_SET_SDMMC2_RST (1 << 9)
#define L_SET_SDMMC4_RST (1 << 15)
#define U_SET_SDMMC3_RST (1 << 5)

#define L_CLR_SDMMC1_RST (1 << 14)
#define L_CLR_SDMMC2_RST (1 << 9)
#define L_CLR_SDMMC4_RST (1 << 15)
#define U_CLR_SDMMC3_RST (1 << 5)

#define L_SET_CLK_ENB_SDMMC1 (1 << 14)
#define L_SET_CLK_ENB_SDMMC2 (1 << 9)
#define L_SET_CLK_ENB_SDMMC4 (1 << 15)
#define U_SET_CLK_ENB_SDMMC3 (1 << 5)

#define L_CLR_CLK_ENB_SDMMC1 (1 << 14)
#define L_CLR_CLK_ENB_SDMMC2 (1 << 9)
#define L_CLR_CLK_ENB_SDMMC4 (1 << 15)
#define U_CLR_CLK_ENB_SDMMC3 (1 << 5)

static int _clock_sdmmc_is_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & L_SWR_SDMMC1_RST;
	case SDMMC_2:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & L_SWR_SDMMC2_RST;
	case SDMMC_3:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_U) & U_SWR_SDMMC3_RST;
	case SDMMC_4:
		return CLOCK(CLK_RST_CONTROLLER_RST_DEVICES_L) & L_SWR_SDMMC4_RST;
	}
	return 0;
}

static void _clock_sdmmc_set_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = L_SET_SDMMC1_RST;
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = L_SET_SDMMC2_RST;
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_U_SET) = U_SET_SDMMC3_RST;
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_SET) = L_SET_SDMMC4_RST;
		break;
	}
}

static void _clock_sdmmc_clear_reset(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = L_CLR_SDMMC1_RST;
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = L_CLR_SDMMC2_RST;
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_U_CLR) = U_CLR_SDMMC3_RST;
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_RST_DEV_L_CLR) = L_CLR_SDMMC4_RST;
		break;
	}
}

static int _clock_sdmmc_is_enabled(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & L_CLK_ENB_SDMMC1;
	case SDMMC_2:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & L_CLK_ENB_SDMMC2;
	case SDMMC_3:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_U) & U_CLK_ENB_SDMMC3;
	case SDMMC_4:
		return CLOCK(CLK_RST_CONTROLLER_CLK_OUT_ENB_L) & L_CLK_ENB_SDMMC4;
	}
	return 0;
}

static void _clock_sdmmc_set_enable(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = L_SET_CLK_ENB_SDMMC1;
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = L_SET_CLK_ENB_SDMMC2;
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_U_SET) = U_SET_CLK_ENB_SDMMC3;
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_SET) = L_SET_CLK_ENB_SDMMC4;
		break;
	}
}

static void _clock_sdmmc_clear_enable(u32 id)
{
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = L_CLR_CLK_ENB_SDMMC1;
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = L_CLR_CLK_ENB_SDMMC2;
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_U_CLR) = U_CLR_CLK_ENB_SDMMC3;
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_ENB_L_CLR) = L_CLR_CLK_ENB_SDMMC4;
		break;
	}
}

static void _clock_sdmmc_config_legacy_tm()
{
	sclock_t *clk = &_clock_sdmmc_legacy_tm;
	if (!(CLOCK(clk->enable) & (1 << clk->index)))
		clock_enable(clk);
}

typedef struct _clock_sdmmc_t
{
	u32 clock;
	u32 real_clock;
} clock_sdmmc_t;

static clock_sdmmc_t _clock_sdmmc_table[4] = { 0 };

#define SDMMC_CLOCK_SRC_PLLP_OUT0      0x0
#define SDMMC_CLOCK_SRC_PLLC4_OUT2     0x3
#define SDMMC4_CLOCK_SRC_PLLC4_OUT2_LJ 0x1

static int _clock_sdmmc_config_clock_host(u32 *pclock, u32 id, u32 val)
{
	u32 divisor = 0;
	u32 source = SDMMC_CLOCK_SRC_PLLP_OUT0;

	if (id > SDMMC_4)
		return 0;

	// Get IO clock divisor.
	switch (val)
	{
	case 25000:
		*pclock = 24728;
		divisor = 31; // 16.5 div.
		break;
	case 26000:
		*pclock = 25500;
		divisor = 30; // 16 div.
		break;
	case 40800:
		*pclock = 40800;
		divisor = 18; // 10 div.
		break;
	case 50000:
		*pclock = 48000;
		divisor = 15; // 8.5 div.
		break;
	case 52000:
		*pclock = 51000;
		divisor = 14; // 8 div.
		break;
	case 100000:
		source = SDMMC_CLOCK_SRC_PLLC4_OUT2;
		*pclock = 99840;
		divisor = 2;  // 2 div.
		break;
	case 164000:
		*pclock = 163200;
		divisor = 3;  // 2.5 div.
		break;
	case 200000:
		switch (id)
		{
		case SDMMC_1:
			source = SDMMC_CLOCK_SRC_PLLC4_OUT2;
			break;
		case SDMMC_2:
			source = SDMMC4_CLOCK_SRC_PLLC4_OUT2_LJ;
			break;
		case SDMMC_3:
			source = SDMMC_CLOCK_SRC_PLLC4_OUT2;
			break;
		case SDMMC_4:
			source = SDMMC4_CLOCK_SRC_PLLC4_OUT2_LJ;
			break;
		}
		*pclock = 199680;
		divisor = 0;  // 1 div.
		break;
	default:
		*pclock = 24728;
		divisor = 31; // 16.5 div.
	}

	_clock_sdmmc_table[id].clock = val;
	_clock_sdmmc_table[id].real_clock = *pclock;

	// PLLC4 and LEGACY_TM clocks are already initialized,
	// because we init at the first eMMC read.
	// // Enable PLLC4 if in use by any SDMMC.
	// if (source)
	// 	_clock_enable_pllc4();

	// // Set SDMMC legacy timeout clock.
	// _clock_sdmmc_config_legacy_tm();


	// Set SDMMC clock.
	switch (id)
	{
	case SDMMC_1:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC1) = (source << 29) | divisor;
		break;
	case SDMMC_2:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC2) = (source << 29) | divisor;
		break;
	case SDMMC_3:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC3) = (source << 29) | divisor;
		break;
	case SDMMC_4:
		CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_SDMMC4) = (source << 29) | divisor;
		break;
	}

	return 1;
}

void clock_sdmmc_config_clock_source(u32 *pclock, u32 id, u32 val)
{
	if (_clock_sdmmc_table[id].clock == val)
	{
		*pclock = _clock_sdmmc_table[id].real_clock;
	}
	else
	{
		int is_enabled = _clock_sdmmc_is_enabled(id);
		if (is_enabled)
			_clock_sdmmc_clear_enable(id);
		_clock_sdmmc_config_clock_host(pclock, id, val);
		if (is_enabled)
			_clock_sdmmc_set_enable(id);
		_clock_sdmmc_is_reset(id);
	}
}

void clock_sdmmc_get_card_clock_div(u32 *pclock, u16 *pdivisor, u32 type)
{
	// Get Card clock divisor.
	switch (type)
	{
	case SDHCI_TIMING_MMC_ID: // Actual IO Freq: 380.59 KHz.
		*pclock = 26000;
		*pdivisor = 66;
		break;
	case SDHCI_TIMING_MMC_LS26:
		*pclock = 26000;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_MMC_HS52:
		*pclock = 52000;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_MMC_HS200:
	case SDHCI_TIMING_MMC_HS400:
	case SDHCI_TIMING_UHS_SDR104:
		*pclock = 200000;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_SD_ID: // Actual IO Freq: 380.43 KHz.
		*pclock = 25000;
		*pdivisor = 64;
		break;
	case SDHCI_TIMING_SD_DS12:
	case SDHCI_TIMING_UHS_SDR12:
		*pclock = 25000;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_SD_HS25:
	case SDHCI_TIMING_UHS_SDR25:
		*pclock = 50000;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_UHS_SDR50:
		*pclock = 100000;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_UHS_SDR82:
		*pclock = 164000;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_UHS_DDR50:
		*pclock = 40800;
		*pdivisor = 1;
		break;
	case SDHCI_TIMING_MMC_HS102: // Actual IO Freq: 99.84 MHz.
		*pclock = 200000;
		*pdivisor = 2;
		break;
	}
}

int clock_sdmmc_is_not_reset_and_enabled(u32 id)
{
	return !_clock_sdmmc_is_reset(id) && _clock_sdmmc_is_enabled(id);
}

void clock_sdmmc_enable(u32 id, u32 val)
{
	u32 clock = 0;

	if (_clock_sdmmc_is_enabled(id))
		_clock_sdmmc_clear_enable(id);
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_config_clock_host(&clock, id, val);
	_clock_sdmmc_set_enable(id);
	_clock_sdmmc_is_reset(id);
	usleep((100000 + clock - 1) / clock);
	_clock_sdmmc_clear_reset(id);
	_clock_sdmmc_is_reset(id);
}

void clock_sdmmc_disable(u32 id)
{
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_clear_enable(id);
	_clock_sdmmc_is_reset(id);
}
