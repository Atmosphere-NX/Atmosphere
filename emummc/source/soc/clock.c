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

#include "../soc/clock.h"
#include "../soc/t210.h"
#include "../utils/util.h"
#include "../emmc/sdmmc.h"

static const sclock_t _clock_i2c5 = {
	CLK_RST_CONTROLLER_RST_DEVICES_H, CLK_RST_CONTROLLER_CLK_OUT_ENB_H, CLK_RST_CONTROLLER_CLK_SOURCE_I2C5, 0xF, 0, 4 //81.6MHz -> 400KHz
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

static u32 _clock_sdmmc_table[8] = { 0 };

#define PLLP_OUT0      0x0

static int _clock_sdmmc_config_clock_source_inner(u32 *pout, u32 id, u32 val)
{
	u32 divisor = 0;
	u32 source = PLLP_OUT0;

	switch (val)
	{
	case 25000:
		*pout = 24728;
		divisor = 31;
		break;
	case 26000:
		*pout = 25500;
		divisor = 30;
		break;
	case 40800:
		*pout = 40800;
		divisor = 18;
		break;
	case 50000:
		*pout = 48000;
		divisor = 15;
		break;
	case 52000:
		*pout = 51000;
		divisor = 14;
		break;
	case 100000:
		*pout = 90667;
		divisor = 7;
		break;
	case 200000:
		*pout = 163200;
		divisor = 3;
		break;
	case 208000:
		*pout = 204000;
		divisor = 2;
		break;
	default:
		*pout = 24728;
		divisor = 31;
	}

	_clock_sdmmc_table[2 * id] = val;
	_clock_sdmmc_table[2 * id + 1] = *pout;

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

void clock_sdmmc_config_clock_source(u32 *pout, u32 id, u32 val)
{
	if (_clock_sdmmc_table[2 * id] == val)
	{
		*pout = _clock_sdmmc_table[2 * id + 1];
	}
	else
	{
		int is_enabled = _clock_sdmmc_is_enabled(id);
		if (is_enabled)
			_clock_sdmmc_clear_enable(id);
		_clock_sdmmc_config_clock_source_inner(pout, id, val);
		if (is_enabled)
			_clock_sdmmc_set_enable(id);
		_clock_sdmmc_is_reset(id);
	}
}

void clock_sdmmc_get_params(u32 *pout, u16 *pdivisor, u32 type)
{
	switch (type)
	{
	case 0:
		*pout = 26000;
		*pdivisor = 66;
		break;
	case 1:
		*pout = 26000;
		*pdivisor = 1;
		break;
	case 2:
		*pout = 52000;
		*pdivisor = 1;
		break;
	case 3:
	case 4:
	case 11:
		*pout = 200000;
		*pdivisor = 1;
		break;
	case 5:
		*pout = 25000;
		*pdivisor = 64;
		break;
	case 6:
	case 8:
		*pout = 25000;
		*pdivisor = 1;
		break;
	case 7:
		*pout = 50000;
		*pdivisor = 1;
		break;
	case 10:
		*pout = 100000;
		*pdivisor = 1;
		break;
	case 13:
		*pout = 40800;
		*pdivisor = 1;
		break;
	case 14:
		*pout = 200000;
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
	u32 div = 0;

	if (_clock_sdmmc_is_enabled(id))
		_clock_sdmmc_clear_enable(id);
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_config_clock_source_inner(&div, id, val);
	_clock_sdmmc_set_enable(id);
	_clock_sdmmc_is_reset(id);
	usleep((100000 + div - 1) / div);
	_clock_sdmmc_clear_reset(id);
	_clock_sdmmc_is_reset(id);
}

void clock_sdmmc_disable(u32 id)
{
	_clock_sdmmc_set_reset(id);
	_clock_sdmmc_clear_enable(id);
	_clock_sdmmc_is_reset(id);
}
