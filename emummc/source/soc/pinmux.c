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

#include "../soc/pinmux.h"
#include "../soc/t210.h"

void pinmux_config_i2c(u32 idx)
{
	PINMUX_AUX(PINMUX_AUX_X_I2C_SCL(idx)) = PINMUX_INPUT_ENABLE;
	PINMUX_AUX(PINMUX_AUX_X_I2C_SDA(idx)) = PINMUX_INPUT_ENABLE;
}
