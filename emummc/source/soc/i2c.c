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

#include "i2c.h"
#include "../utils/util.h"
#include "t210.h"

#define I2C_PACKET_PROT_I2C  (1 << 4)
#define I2C_HEADER_CONT_XFER (1 << 15)
#define I2C_HEADER_REP_START (1 << 16)
#define I2C_HEADER_IE_ENABLE (1 << 17)
#define I2C_HEADER_READ      (1 << 19)

#define I2C_CNFG              (0x00 / 4)
#define  CMD1_WRITE           (0 << 6)
#define  CMD1_READ            (1 << 6)
#define  NORMAL_MODE_GO       (1 << 9)
#define  PACKET_MODE_GO       (1 << 10)
#define  NEW_MASTER_FSM       (1 << 11)
#define  DEBOUNCE_CNT_4T      (2 << 12)

#define I2C_CMD_ADDR0         (0x04 / 4)
#define  ADDR0_WRITE          0
#define  ADDR0_READ           1

#define I2C_CMD_DATA1         (0x0C / 4)
#define I2C_CMD_DATA2         (0x10 / 4)

#define I2C_STATUS            (0x1C / 4)
#define  I2C_STATUS_NOACK     (0xF << 0)
#define  I2C_STATUS_BUSY      (1 << 8)

#define I2C_TX_FIFO           (0x50 / 4)
#define I2C_RX_FIFO           (0x54 / 4)

#define I2C_FIFO_CONTROL      (0x5C / 4)
#define  RX_FIFO_FLUSH        (1 << 0)
#define  TX_FIFO_FLUSH        (1 << 1)

#define I2C_FIFO_STATUS       (0x60 / 4)
#define  RX_FIFO_FULL_CNT     (0xF << 0)
#define  TX_FIFO_EMPTY_CNT    (0xF << 4)

#define I2C_INT_EN            (0x64 / 4)
#define I2C_INT_STATUS        (0x68 / 4)
#define I2C_INT_SOURCE        (0x70 / 4)
#define  RX_FIFO_DATA_REQ     (1 << 0)
#define  TX_FIFO_DATA_REQ     (1 << 1)
#define  ARB_LOST             (1 << 2)
#define  NO_ACK               (1 << 3)
#define  RX_FIFO_UNDER        (1 << 4)
#define  TX_FIFO_OVER         (1 << 5)
#define  ALL_PACKETS_COMPLETE (1 << 6)
#define  PACKET_COMPLETE      (1 << 7)
#define  BUS_CLEAR_DONE       (1 << 11)

#define I2C_CLK_DIVISOR       (0x6C / 4)

#define I2C_BUS_CLEAR_CONFIG  (0x84 / 4)
#define  BC_ENABLE            (1 << 0)
#define  BC_TERMINATE         (1 << 1)

#define I2C_BUS_CLEAR_STATUS  (0x88 / 4)

#define I2C_CONFIG_LOAD       (0x8C / 4)
#define  MSTR_CONFIG_LOAD     (1 << 0)
#define  TIMEOUT_CONFIG_LOAD  (1 << 2)

static const u64 i2c_addrs[] = {
	0x7000C000, // I2C_1.
	0x7000C400, // I2C_2.
	0x7000C500, // I2C_3.
	0x7000C700, // I2C_4.
	0x7000D000, // I2C_5.
	0x7000D100  // I2C_6.
};

static void _i2c_load_cfg_wait(vu32 *base)
{
	base[I2C_CONFIG_LOAD] = (1 << 5) | TIMEOUT_CONFIG_LOAD | MSTR_CONFIG_LOAD;
	for (u32 i = 0; i < 20; i++)
	{
		usleep(1);
		if (!(base[I2C_CONFIG_LOAD] & MSTR_CONFIG_LOAD))
			break;
	}
}

static int _i2c_send_single(u32 i2c_idx, u32 dev_addr, u8 *buf, u32 size)
{
	if (size > 4)
		return 0;

	u32 tmp = 0;
	memcpy(&tmp, buf, size);

	vu32 *base = (vu32 *)QueryIoMapping(i2c_addrs[I2C_5], 0x1000);

	// Set device address and send mode.
	base[I2C_CMD_ADDR0] = dev_addr << 1 | ADDR0_WRITE;
	base[I2C_CMD_DATA1] = tmp;    //Set value.
	// Set size and send mode.
	base[I2C_CNFG] = ((size - 1) << 1) | DEBOUNCE_CNT_4T | NEW_MASTER_FSM | CMD1_WRITE;

	// Load configuration.
	_i2c_load_cfg_wait(base);

	// Initiate transaction on normal mode.
	base[I2C_CNFG] = (base[I2C_CNFG] & 0xFFFFF9FF) | NORMAL_MODE_GO;

	u64 timeout = get_tmr_ms() + 100;
	while (base[I2C_STATUS] & I2C_STATUS_BUSY)
	{
		if (get_tmr_ms() > timeout)
			return 0;
	}

	if (base[I2C_STATUS] & I2C_STATUS_NOACK)
		return 0;

	return 1;
}

static int _i2c_recv_single(u32 i2c_idx, u8 *buf, u32 size, u32 dev_addr)
{
	if (size > 4)
		return 0;

	vu32 *base = (vu32 *)QueryIoMapping(i2c_addrs[I2C_5], 0x1000);
	// Set device address and recv mode.
	base[I2C_CMD_ADDR0] = (dev_addr << 1) | ADDR0_READ;

	// Set size and recv mode.
	base[I2C_CNFG] = ((size - 1) << 1) | DEBOUNCE_CNT_4T | NEW_MASTER_FSM | CMD1_READ;

	// Load configuration.
	_i2c_load_cfg_wait(base);

	// Initiate transaction on normal mode.
	base[I2C_CNFG] = (base[I2C_CNFG] & 0xFFFFF9FF) | NORMAL_MODE_GO;

	u64 timeout = get_tmr_ms() + 100;
	while (base[I2C_STATUS] & I2C_STATUS_BUSY)
	{
		if (get_tmr_ms() > timeout)
			return 0;
	}

	if (base[I2C_STATUS] & I2C_STATUS_NOACK)
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
    
	base[I2C_CLK_DIVISOR] = (5 << 16) | 1; // SF mode Div: 6, HS mode div: 2.
	base[I2C_BUS_CLEAR_CONFIG] = (9 << 16) | BC_TERMINATE | BC_ENABLE;

	// Load configuration.
	_i2c_load_cfg_wait(base);

	for (u32 i = 0; i < 10; i++)
	{
		if (base[I2C_INT_STATUS] & BUS_CLEAR_DONE)
			break;
	}

	(vu32)base[I2C_BUS_CLEAR_STATUS];
	base[I2C_INT_STATUS] = base[I2C_INT_STATUS];
}

int i2c_send_buf_small(u32 i2c_idx, u32 dev_addr, u32 reg, u8 *buf, u32 size)
{
	u8 tmp[4];

	if (size > 3)
		return 0;

	tmp[0] = reg;
	memcpy(tmp + 1, buf, size);

	return _i2c_send_single(i2c_idx, dev_addr, tmp, size + 1);
}

int i2c_recv_buf_small(u8 *buf, u32 size, u32 i2c_idx, u32 dev_addr, u32 reg)
{
	int res = _i2c_send_single(i2c_idx, dev_addr, (u8 *)&reg, 1);
	if (res)
		res = _i2c_recv_single(i2c_idx, buf, size, dev_addr);
	return res;
}

int i2c_send_byte(u32 i2c_idx, u32 dev_addr, u32 reg, u8 val)
{
	return i2c_send_buf_small(i2c_idx, dev_addr, reg, &val, 1);
}

u8 i2c_recv_byte(u32 i2c_idx, u32 dev_addr, u32 reg)
{
	u8 tmp = 0;
	i2c_recv_buf_small(&tmp, 1, i2c_idx, dev_addr, reg);
	return tmp;
}

