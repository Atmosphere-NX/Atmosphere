#include <string.h>

#include "i2c.h"
#include "util.h"

static u32 i2c_addrs[] = { 0x7000C000, 0x7000C400, 0x7000C500, 0x7000C700, 0x7000D000, 0x7000D100 };

static void _i2c_wait(vu32 *base)
{
	base[0x23] = 0x25;
	for (u32 i = 0; i < 20; i++)
	{
		sleep(1);
		if (!(base[0x23] & 1))
			break;
	}
}

static int _i2c_send_pkt(u32 idx, u32 x, u8 *buf, u32 size)
{
	if (size > 4)
		return 0;

	u32 tmp = 0;
	memcpy(&tmp, buf, size);

	vu32 *base = (vu32 *)i2c_addrs[idx];
	base[1] = x << 1; //Set x (send mode).
	base[3] = tmp; //Set value.
	base[0] = (2 * size - 2) | 0x2800; //Set size and send mode.
	_i2c_wait(base); //Kick transaction.

	base[0] = base[0] & 0xFFFFFDFF | 0x200;
	while (base[7] & 0x100)
		;

	if (base[7] << 28)
		return 0;

	return 1;
}

static int _i2c_recv_pkt(u32 idx, u8 *buf, u32 size, u32 x)
{
	if (size > 4)
		return 0;

	vu32 *base = (vu32 *)i2c_addrs[idx];
	base[1] = (x << 1) | 1; //Set x (recv mode).
	base[0] = (2 * size - 2) | 0x2840; //Set size and recv mode.
	_i2c_wait(base); //Kick transaction.

	base[0] = base[0] & 0xFFFFFDFF | 0x200;
	while (base[7] & 0x100)
		;

	if (base[7] << 28)
		return 0;

	u32 tmp = base[3]; //Get value.
	memcpy(buf, &tmp, size);

	return 1;
}

void i2c_init(u32 idx)
{
	vu32 *base = (vu32 *)i2c_addrs[idx];

	base[0x1B] = 0x50001;
	base[0x21] = 0x90003;
	_i2c_wait(base);

	for (u32 i = 0; i < 10; i++)
	{
		sleep(20000);
		if (base[0x1A] & 0x800)
			break;
	}

	vu32 dummy = base[0x22];
	base[0x1A] = base[0x1A];
}

int i2c_send_buf_small(u32 idx, u32 x, u32 y, u8 *buf, u32 size)
{
	u8 tmp[4];

	if  (size > 3)
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

void i2c_send_byte(u32 idx, u32 x, u32 y, u8 b)
{
	i2c_send_buf_small(idx, x, y, &b, 1);
}

u8 i2c_recv_byte(u32 idx, u32 x, u32 y)
{
	u8 tmp;
	i2c_recv_buf_small(&tmp, 1, idx, x, y);
	return tmp;
}
