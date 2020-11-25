/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2020 CTCaer
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

void i2c_init();
int i2c_send_buf_small(u32 i2c_idx, u32 dev_addr, u32 reg, u8 *buf, u32 size);
int i2c_recv_buf_small(u8 *buf, u32 size, u32 i2c_idx, u32 dev_addr, u32 reg);
int i2c_send_byte(u32 i2c_idx, u32 dev_addr, u32 reg, u8 val);
u8  i2c_recv_byte(u32 i2c_idx, u32 dev_addr, u32 reg);

#endif
