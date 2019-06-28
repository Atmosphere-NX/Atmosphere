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

#include <string.h>

#include "i2c.h"
#include "../utils/util.h"
#include "t210.h"

// TODO: not hardcode I2C_5
static u64 i2c_addrs[] = {
	0x7000C000, 0x7000C400, 0x7000C500,
	0x7000C700, 0x7000D000, 0x7000D100
};

static void _i2c_wait(vu32 *base)
{
	base[I2C_CONFIG_LOAD] = 0x25;

	for (u32 i = 0; i < 20; i++)
	{
		usleep(1);
		if (!(base[I2C_CONFIG_LOAD] & 1))
			break;
	}
}

static int _i2c_send_pkt(u32 idx, u32 x, u8 *buf, u32 size)
{
	if (size > 4)
		return 0;

	u32 tmp = 0;
	memcpy(&tmp, buf, size);

	vu32 *base = (vu32 *)QueryIoMapping(i2c_addrs[I2C_5], 0x1000);
	base[I2C_CMD_ADDR0] = x << 1; //Set x (send mode).
	base[I2C_CMD_DATA1] = tmp;    //Set value.
	base[I2C_CNFG] = (2 * size - 2) | 0x2800; //Set size and send mode.
	_i2c_wait(base);  //Kick transaction.

	base[I2C_CNFG] = (base[I2C_CNFG] & 0xFFFFFDFF) | 0x200;
	while (base[I2C_STATUS] & 0x100)
		;

	if (base[I2C_STATUS] << 28)
		return 0;

	return 1;
}

static int _i2c_recv_pkt(u32 idx, u8 *buf, u32 size, u32 x)
{
	if (size > 8)
		return 0;

	vu32 *base = (vu32 *)QueryIoMapping(i2c_addrs[I2C_5], 0x1000);

	base[I2C_CMD_ADDR0] = (x << 1) | 1; // Set x (recv mode).
	base[I2C_CNFG] = (size - 1) << 1 | 0x2840; // Set size and recv mode.
	_i2c_wait(base);        // Kick transaction.

	base[I2C_CNFG] = (base[I2C_CNFG] & 0xFFFFFDFF) | 0x200;
	while (base[I2C_STATUS] & 0x100)
		;

	if (base[I2C_STATUS] << 28)
		return 0;

	u32 tmp = base[I2C_CMD_DATA1]; // Get LS value.
	if (size > 4)
	{
		memcpy(buf, &tmp, 4);
		tmp = base[I2C_CMD_DATA2]; // Get MS value.
		memcpy(buf + 4, &tmp, size - 4);
	}
	else
		memcpy(buf, &tmp, size);

	return 1;
}

void i2c_init()
{
	vu32 *base = (vu32 *)QueryIoMapping(i2c_addrs[I2C_5], 0x1000);
    
	base[I2C_CLK_DIVISOR_REGISTER] = 0x50001;
	base[I2C_BUS_CLEAR_CONFIG] = 0x90003;
	_i2c_wait(base);

	for (u32 i = 0; i < 10; i++)
	{
		if (base[INTERRUPT_STATUS_REGISTER] & 0x800)
			break;
	}

	(vu32)base[I2C_BUS_CLEAR_STATUS];
	base[INTERRUPT_STATUS_REGISTER] = base[INTERRUPT_STATUS_REGISTER];
}

int i2c_send_buf_small(u32 idx, u32 x, u32 y, u8 *buf, u32 size)
{
	u8 tmp[4];

	if (size > 3)
		return 0;

	tmp[0] = y;
	memcpy(tmp + 1, buf, size);

	return _i2c_send_pkt(idx, x, tmp, size + 1);
}

int i2c_recv_buf_small(u8 *buf, u32 size, u32 idx, u32 x, u32 y)
{
	int res = _i2c_send_pkt(idx, x, (u8 *)&y, 1);
	if (res)
		res = _i2c_recv_pkt(idx, buf, size, x);
	return res;
}

int i2c_send_byte(u32 idx, u32 x, u32 y, u8 b)
{
	return i2c_send_buf_small(idx, x, y, &b, 1);
}

u8 i2c_recv_byte(u32 idx, u32 x, u32 y)
{
	u8 tmp = 0;
	i2c_recv_buf_small(&tmp, 1, idx, x, y);
	return tmp;
}

