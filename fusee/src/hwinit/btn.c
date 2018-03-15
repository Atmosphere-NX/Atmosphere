#include "btn.h"
#include "i2c.h"
#include "t210.h"

u32 btn_read()
{
	u32 res = 0;
	if(!(GPIO_6(0x3C) & 0x80))
		res |= BTN_VOL_DOWN;
	if(!(GPIO_6(0x3C) & 0x40))
		res |= BTN_VOL_UP;
	if(i2c_recv_byte(4, 0x3C, 0x15) & 0x4)
		res |= BTN_POWER;
	return res;
}

u32 btn_wait()
{
	u32 res = 0, btn = btn_read();
	do
	{
		res = btn_read();
	} while (btn == res);
	return res;
}
