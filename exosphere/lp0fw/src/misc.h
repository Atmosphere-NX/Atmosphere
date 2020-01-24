/*
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
 
#ifndef EXOSPHERE_WARMBOOT_BIN_MISC_H
#define EXOSPHERE_WARMBOOT_BIN_MISC_H

#include <stdint.h>

#include "utils.h"

#define MISC_BASE  (0x70000000)

#define MAKE_MISC_REG(n) MAKE_REG32(MISC_BASE + n)

#define APB_MISC_PP_CONFIG_CTL_0 MAKE_MISC_REG(0x024)
#define APB_MISC_GP_ASDBGREG_0 MAKE_MISC_REG(0x810)

#define PINMUX_AUX_PWR_I2C_SCL_0 MAKE_MISC_REG(0x30DC)
#define PINMUX_AUX_PWR_I2C_SDA_0 MAKE_MISC_REG(0x30E0)
#define PINMUX_AUX_DVFS_PWM_0    MAKE_MISC_REG(0x3184)
#define PINMUX_AUX_GPIO_PA6_0 MAKE_MISC_REG(0x3244)

void misc_configure_device_dbg_settings(void);
void misc_restore_ram_svop(void);

#endif
