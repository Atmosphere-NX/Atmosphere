#include "clock.h"
#include "t210.h"
#include "util.h"

static const clock_t _clock_uart[] =  {
	/* UART A */ { 4, 0x10, 0x178, 6, 0, 0 },
	/* UART B */ { 4, 0x10, 0x17C, 7, 0, 0 },
	/* UART C */ { 8, 0x14, 0x1A0, 0x17, 0, 0 },
	/* UART D */ { 0 },
	/* UART E */ { 0 }
};

static const clock_t _clock_i2c[] = {
	/* I2C1 */ { 4, 0x10, 0x124, 0xC, 6, 0 },
	/* I2C2 */ { 0 },
	/* I2C3 */ { 0 },
	/* I2C4 */ { 0 },
	/* I2C5 */ { 8, 0x14, 0x128, 0xF, 6, 0 },
	/* I2C6 */ { 0 }
};

static clock_t _clock_se = { 0x358, 0x360, 0x42C, 0x1F, 0, 0 };

static clock_t _clock_host1x = { 4, 0x10, 0x180, 0x1C, 4, 3 };
static clock_t _clock_tsec = { 0xC, 0x18, 0x1F4, 0x13, 0, 2 };
static clock_t _clock_sor_safe = { 0x2A4, 0x298, 0, 0x1E, 0, 0 };
static clock_t _clock_sor0 = { 0x28C, 0x280, 0, 0x16, 0, 0 };
static clock_t _clock_sor1 = { 0x28C, 0x280, 0x410, 0x17, 0, 2 };
static clock_t _clock_kfuse = { 8, 0x14, 0, 8, 0, 0 };

static clock_t _clock_coresight = { 0xC, 0x18, 0x1D4, 9, 0, 4};

void clock_enable(const clock_t *clk)
{
	//Put clock into reset.
	CLOCK(clk->reset) = CLOCK(clk->reset) & ~(1 << clk->index) | (1 << clk->index);
	//Disable.
	CLOCK(clk->enable) &= ~(1 << clk->index);
	//Configure clock source if required.
	if (clk->source)
		CLOCK(clk->source) = clk->clk_div | (clk->clk_src << 29);
	//Enable.
	CLOCK(clk->enable) = CLOCK(clk->enable) & ~(1 << clk->index) | (1 << clk->index);
	//Take clock off reset.
	CLOCK(clk->reset) &= ~(1 << clk->index);
}

void clock_disable(const clock_t *clk)
{
	//Put clock into reset.
	CLOCK(clk->reset) = CLOCK(clk->reset) & ~(1 << clk->index) | (1 << clk->index);
	//Disable.
	CLOCK(clk->enable) &= ~(1 << clk->index);
}

void clock_enable_fuse(u32 enable)
{
	CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) = CLOCK(CLK_RST_CONTROLLER_MISC_CLK_ENB) & 0xEFFFFFFF | ((enable & 1) << 28) & 0x10000000;
}

void clock_enable_uart(u32 idx)
{
	clock_enable(&_clock_uart[idx]);
}

void clock_enable_i2c(u32 idx)
{
	clock_enable(&_clock_i2c[idx]);
}

void clock_enable_se()
{
	clock_enable(&_clock_se);
}

void clock_enable_host1x()
{
	clock_enable(&_clock_host1x);
}

void clock_enable_tsec()
{
	clock_enable(&_clock_tsec);
}

void clock_enable_sor_safe()
{
	clock_enable(&_clock_sor_safe);
}

void clock_enable_sor0()
{
	clock_enable(&_clock_sor0);
}

void clock_enable_sor1()
{
	clock_enable(&_clock_sor1);
}

void clock_enable_kfuse()
{
	//clock_enable(&_clock_kfuse);
	CLOCK(0x8) = CLOCK(0x8) & 0xFFFFFEFF | 0x100;
	CLOCK(0x14) &= 0xFFFFFEFF;
	CLOCK(0x14) = CLOCK(0x14) & 0xFFFFFEFF | 0x100;
	sleep(10);
	CLOCK(0x8) &= 0xFFFFFEFF;
	sleep(20);
}

void clock_disable_host1x()
{
	clock_disable(&_clock_host1x);
}

void clock_disable_tsec()
{
	clock_disable(&_clock_tsec);
}

void clock_disable_sor_safe()
{
	clock_disable(&_clock_sor_safe);
}

void clock_disable_sor0()
{
	clock_disable(&_clock_sor0);
}

void clock_disable_sor1()
{
	clock_disable(&_clock_sor1);
}

void clock_disable_kfuse()
{
	clock_disable(&_clock_kfuse);
}

void clock_enable_coresight()
{
	clock_enable(&_clock_coresight);
}
