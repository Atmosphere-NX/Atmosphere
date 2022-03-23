/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2019 CTCaer
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

#include "max7762x.h"
#include "max77620.h"
#include "../soc/i2c.h"
#include "../utils/util.h"

#define REGULATOR_SD 0
#define REGULATOR_LDO 1

typedef struct _max77620_regulator_t
{
	u8 type;
	const char *name;
	u8 reg_sd;

	u32 mv_step;
	u32 mv_min;
	u32 mv_default;
	u32 mv_max;

	u8 volt_addr;
	u8 cfg_addr;

	u8 volt_mask;
	u8 enable_mask;
	u8 enable_shift;
	u8 status_mask;

	u8 fps_addr;
	u8 fps_src;
	u8 pd_period;
	u8 pu_period;
} max77620_regulator_t;

static const max77620_regulator_t _pmic_regulators[] = {
	{  REGULATOR_SD,  "sd0", 0x16,  12500, 600000,  625000, 1400000,      MAX77620_REG_SD0,   MAX77620_REG_SD0_CFG, MAX77620_SD0_VOLT_MASK, MAX77620_SD_POWER_MODE_MASK,  MAX77620_SD_POWER_MODE_SHIFT,  0x80,  MAX77620_REG_FPS_SD0,  1, 7, 1 },
	{  REGULATOR_SD,  "sd1", 0x17,  12500, 600000, 1125000, 1125000,      MAX77620_REG_SD1,   MAX77620_REG_SD1_CFG, MAX77620_SD1_VOLT_MASK, MAX77620_SD_POWER_MODE_MASK,  MAX77620_SD_POWER_MODE_SHIFT,  0x40,  MAX77620_REG_FPS_SD1,  0, 1, 5 },
	{  REGULATOR_SD,  "sd2", 0x18,  12500, 600000, 1325000, 1350000,      MAX77620_REG_SD2,   MAX77620_REG_SD2_CFG, MAX77620_SDX_VOLT_MASK, MAX77620_SD_POWER_MODE_MASK,  MAX77620_SD_POWER_MODE_SHIFT,  0x20,  MAX77620_REG_FPS_SD2,  1, 5, 2 },
	{  REGULATOR_SD,  "sd3", 0x19,  12500, 600000, 1800000, 1800000,      MAX77620_REG_SD3,   MAX77620_REG_SD3_CFG, MAX77620_SDX_VOLT_MASK, MAX77620_SD_POWER_MODE_MASK,  MAX77620_SD_POWER_MODE_SHIFT,  0x10,  MAX77620_REG_FPS_SD3,  0, 3, 3 },
	{ REGULATOR_LDO, "ldo0", 0x00,  25000, 800000, 1200000, 1200000, MAX77620_REG_LDO0_CFG, MAX77620_REG_LDO0_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO0, 3, 7, 0 },
	{ REGULATOR_LDO, "ldo1", 0x00,  25000, 800000, 1050000, 1050000, MAX77620_REG_LDO1_CFG, MAX77620_REG_LDO1_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO1, 3, 7, 0 },
	{ REGULATOR_LDO, "ldo2", 0x00,  50000, 800000, 1800000, 3300000, MAX77620_REG_LDO2_CFG, MAX77620_REG_LDO2_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO2, 3, 7, 0 },
	{ REGULATOR_LDO, "ldo3", 0x00,  50000, 800000, 3100000, 3100000, MAX77620_REG_LDO3_CFG, MAX77620_REG_LDO3_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO3, 3, 7, 0 },
	{ REGULATOR_LDO, "ldo4", 0x00,  12500, 800000,  850000,  850000, MAX77620_REG_LDO4_CFG, MAX77620_REG_LDO4_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO4, 0, 7, 1 },
	{ REGULATOR_LDO, "ldo5", 0x00,  50000, 800000, 1800000, 1800000, MAX77620_REG_LDO5_CFG, MAX77620_REG_LDO5_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO5, 3, 7, 0 },
	{ REGULATOR_LDO, "ldo6", 0x00,  50000, 800000, 2900000, 2900000, MAX77620_REG_LDO6_CFG, MAX77620_REG_LDO6_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO6, 3, 7, 0 },
	{ REGULATOR_LDO, "ldo7", 0x00,  50000, 800000, 1050000, 1050000, MAX77620_REG_LDO7_CFG, MAX77620_REG_LDO7_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO7, 1, 4, 3 },
	{ REGULATOR_LDO, "ldo8", 0x00,  50000, 800000, 1050000, 1050000, MAX77620_REG_LDO8_CFG, MAX77620_REG_LDO8_CFG2, MAX77620_LDO_VOLT_MASK, MAX77620_LDO_POWER_MODE_MASK, MAX77620_LDO_POWER_MODE_SHIFT, 0x00,  MAX77620_REG_FPS_LDO8, 3, 7, 0 }
};

int max77620_regulator_get_status(u32 id)
{
	if (id > REGULATOR_MAX)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	if (reg->type == REGULATOR_SD)
		return (i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_STATSD) & reg->status_mask) ? 0 : 1;
	return (i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, reg->cfg_addr) & 8) ? 1 : 0;
}

int max77620_regulator_config_fps(u32 id)
{
	if (id > REGULATOR_MAX)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, reg->fps_addr,
		(reg->fps_src << MAX77620_FPS_SRC_SHIFT) | (reg->pu_period << MAX77620_FPS_PU_PERIOD_SHIFT) | (reg->pd_period));

	return 1;
}

int max77620_regulator_set_voltage(u32 id, u32 mv)
{
	if (id > REGULATOR_MAX)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	if (mv < reg->mv_min || mv > reg->mv_max)
		return 0;

	u32 mult = (mv + reg->mv_step - 1 - reg->mv_min) / reg->mv_step;
	u8 val = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, reg->volt_addr);
	val = (val & ~reg->volt_mask) | (mult & reg->volt_mask);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, reg->volt_addr, val);
	usleep(1000);

	return 1;
}

int max77620_regulator_enable(u32 id, int enable)
{
	if (id > REGULATOR_MAX)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	u32 addr = reg->type == REGULATOR_SD ? reg->cfg_addr : reg->volt_addr;
	u8 val = i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, addr);
	if (enable)
		val = (val & ~reg->enable_mask) | ((MAX77620_POWER_MODE_NORMAL << reg->enable_shift) & reg->enable_mask);
	else
		val &= ~reg->enable_mask;
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, addr, val);
	usleep(1000);

	return 1;
}

int max77620_regulator_set_volt_and_flags(u32 id, u32 mv, u8 flags)
{
	if (id > REGULATOR_MAX)
		return 0;

	const max77620_regulator_t *reg = &_pmic_regulators[id];

	if (mv < reg->mv_min || mv > reg->mv_max)
		return 0;

	u32 mult = (mv + reg->mv_step - 1 - reg->mv_min) / reg->mv_step;
	u8 val = ((flags << reg->enable_shift) & ~reg->volt_mask) | (mult & reg->volt_mask);
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, reg->volt_addr, val);
	usleep(1000);

	return 1;
}

void max77620_config_default()
{
	for (u32 i = 1; i <= REGULATOR_MAX; i++)
	{
		i2c_recv_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_CID4);
		max77620_regulator_config_fps(i);
		max77620_regulator_set_voltage(i, _pmic_regulators[i].mv_default);
		if (_pmic_regulators[i].fps_src != MAX77620_FPS_SRC_NONE)
			max77620_regulator_enable(i, 1);
	}
	i2c_send_byte(I2C_5, MAX77620_I2C_ADDR, MAX77620_REG_SD_CFG2, 4);
}
