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

#ifndef _I2C_H_
#define _I2C_H_

#include "../utils/types.h"

#define I2C_1 0
#define I2C_2 1
#define I2C_3 2
#define I2C_4 3
#define I2C_5 4
#define I2C_6 5

#define I2C_CNFG        0x00
#define I2C_CMD_ADDR0   0x01
#define I2C_CMD_DATA1   0x03
#define I2C_CMD_DATA2   0x04
#define I2C_STATUS 0x07
#define INTERRUPT_STATUS_REGISTER 0x1A
#define I2C_CLK_DIVISOR_REGISTER 0x1B
#define I2C_BUS_CLEAR_CONFIG 0x21
#define I2C_BUS_CLEAR_STATUS 0x22
#define I2C_CONFIG_LOAD 0x23

void i2c_init(u32 idx);
int i2c_send_buf_small(u32 idx, u32 x, u32 y, u8 *buf, u32 size);
int i2c_recv_buf_small(u8 *buf, u32 size, u32 idx, u32 x, u32 y);
int i2c_send_byte(u32 idx, u32 x, u32 y, u8 b);
u8 i2c_recv_byte(u32 idx, u32 x, u32 y);

#endif
