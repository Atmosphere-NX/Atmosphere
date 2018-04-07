#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "types.h"

/*! Clock registers. */
#define CLK_RST_CONTROLLER_SCLK_BURST_POLICY 0x28
#define CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER 0x2C
#define CLK_RST_CONTROLLER_CLK_SYSTEM_RATE 0x30
#define CLK_RST_CONTROLLER_MISC_CLK_ENB 0x48
#define CLK_RST_CONTROLLER_OSC_CTRL 0x50
#define CLK_RST_CONTROLLER_CLK_SOURCE_EMC 0x19C
#define CLK_RST_CONTROLLER_CLK_ENB_X_SET 0x284
#define CLK_RST_CONTROLLER_RST_DEV_H_SET 0x308
#define CLK_RST_CONTROLLER_CLK_ENB_H_SET 0x328
#define CLK_RST_CONTROLLER_RST_DEVICES_V 0x358
#define CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR 0x454
#define CLK_RST_CONTROLLER_SPARE_REG0 0x55C
#define CLK_RST_CONTROLLER_PLLMB_BASE 0x5E8

typedef struct _clock_t
{
	u32 reset;
	u32 enable;
	u32 source;
	u8 index;
	u8 clk_src;
	u8 clk_div;
} clock_t;

void clock_enable(const clock_t *clk);
void clock_disable(const clock_t *clk);
void clock_enable_fuse(u32 enable);
void clock_enable_uart(u32 idx);
void clock_enable_i2c(u32 idx);
void clock_enable_se();
void clock_enable_host1x();
void clock_enable_tsec();
void clock_enable_sor_safe();
void clock_enable_sor0();
void clock_enable_sor1();
void clock_enable_kfuse();
void clock_disable_host1x();
void clock_disable_tsec();
void clock_disable_sor_safe();
void clock_disable_sor0();
void clock_disable_sor1();
void clock_disable_kfuse();
void clock_enable_coresight();

#endif
