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
 
#ifndef EXOSPHERE_BPMPFW_I2C_H
#define EXOSPHERE_BPMPFW_I2C_H

#include "utils.h"

/* I2C_BASE = I2C4. */
#define I2C_BASE (0x7000D000)

#define MAKE_I2C_REG(ofs) (MAKE_REG32(I2C_BASE + ofs))

#define I2C_I2C_CNFG_0                       MAKE_I2C_REG(0x000)

#define I2C_I2C_CMD_ADDR0_0                  MAKE_I2C_REG(0x004)

#define I2C_I2C_CMD_DATA1_0                  MAKE_I2C_REG(0x00C)

#define I2C_I2C_STATUS_0                     MAKE_I2C_REG(0x01C)

#define I2C_INTERRUPT_STATUS_REGISTER_0      MAKE_I2C_REG(0x068)

#define I2C_I2C_CLK_DIVISOR_REGISTER_0       MAKE_I2C_REG(0x06C)

#define I2C_I2C_BUS_CLEAR_CONFIG_0           MAKE_I2C_REG(0x084)

#define I2C_I2C_BUS_CLEAR_STATUS_0           MAKE_I2C_REG(0x088)


#define I2C_I2C_CONFIG_LOAD_0                MAKE_I2C_REG(0x08C)

void i2c_init(void);

int i2c_send_reset_cmd(void);

#endif
