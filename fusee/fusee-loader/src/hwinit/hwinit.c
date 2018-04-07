#include "clock.h"
#include "uart.h"
#include "i2c.h"
#include "sdram.h"
#include "di.h"
#include "mc.h"
#include "t210.h"
#include "pmc.h"
#include "pinmux.h"
#include "fuse.h"
#include "util.h"

void config_oscillators()
{
	CLOCK(CLK_RST_CONTROLLER_SPARE_REG0) = CLOCK(CLK_RST_CONTROLLER_SPARE_REG0) & 0xFFFFFFF3 | 4;
	SYSCTR0(SYSCTR0_CNTFID0) = 19200000;
	TMR(0x14) = 0x45F;
	CLOCK(CLK_RST_CONTROLLER_OSC_CTRL) = 0x50000071;
	PMC(APBDEV_PMC_OSC_EDPD_OVER) = PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFFFFF81 | 0xE;
	PMC(APBDEV_PMC_OSC_EDPD_OVER) = PMC(APBDEV_PMC_OSC_EDPD_OVER) & 0xFFBFFFFF | 0x400000;
	PMC(APBDEV_PMC_CNTRL2) = PMC(APBDEV_PMC_CNTRL2) & 0xFFFFEFFF | 0x1000;
	PMC(APBDEV_PMC_SCRATCH188) = PMC(APBDEV_PMC_SCRATCH188) & 0xFCFFFFFF | 0x2000000;
	CLOCK(CLK_RST_CONTROLLER_PLLMB_BASE) &= 0xBFFFFFFF;
	PMC(APBDEV_PMC_TSC_MULT) = PMC(APBDEV_PMC_TSC_MULT) & 0xFFFF0000 | 0x249F; //0x249F = 19200000 * (16 / 32.768 kHz)
	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = 0x20004444;
	CLOCK(CLK_RST_CONTROLLER_SUPER_SCLK_DIVIDER) = 0x80000000;
	CLOCK(CLK_RST_CONTROLLER_CLK_SYSTEM_RATE) = 2;
}

void config_gpios()
{
	PINMUX_AUX(PINMUX_AUX_UART2_TX) = 0;
	PINMUX_AUX(PINMUX_AUX_UART3_TX) = 0;

	PINMUX_AUX(PINMUX_AUX_GPIO_PE6) = 0x40;
	PINMUX_AUX(PINMUX_AUX_GPIO_PH6) = 0x40;

	GPIO_2(0x8) = GPIO_2(0x8) & 0xFFFFFFFE | 1;
	GPIO_1(0xC) = GPIO_1(0xC) & 0xFFFFFFFD | 2;
	GPIO_2(0x0) = GPIO_2(0x0) & 0xFFFFFFBF | 0x40;
	GPIO_2(0xC) = GPIO_2(0xC) & 0xFFFFFFBF | 0x40;
	GPIO_2(0x18) &= 0xFFFFFFFE;
	GPIO_1(0x1C) &= 0xFFFFFFFD;
	GPIO_2(0x10) &= 0xFFFFFFBF;
	GPIO_2(0x1C) &= 0xFFFFFFBF;

	pinmux_config_i2c(I2C_1);
	pinmux_config_i2c(I2C_5);
	pinmux_config_uart(UART_A);

	GPIO_6(0xC) = GPIO_6(0xC) & 0xFFFFFFBF | 0x40;
	GPIO_6(0xC) = GPIO_6(0xC) & 0xFFFFFF7F | 0x80;
	GPIO_6(0x1C) &= 0xFFFFFFBF;
	GPIO_6(0x1C) &= 0xFFFFFF7F;
}

void config_pmc_scratch()
{
	PMC(APBDEV_PMC_SCRATCH20) &= 0xFFF3FFFF;
	PMC(APBDEV_PMC_SCRATCH190) &= 0xFFFFFFFE;
	PMC(APBDEV_PMC_SECURE_SCRATCH21) |= 0x10;
}

void mc_config_tsec_carveout(u32 bom, u32 size1mb, int lock)
{
	MC(0x670) = 0x90000000;
	MC(0x674) = 1;
	if (lock)
		MC(0x678) = 1;
}

void mc_config_carveout()
{
	*(vu32 *)0x8005FFFC = 0xC0EDBBCC;
	MC(0x984) = 1;
	MC(0x988) = 0;
	MC(0x648) = 0;
	MC(0x64C) = 0;
	MC(0x650) = 1;

	//Official code disables and locks the carveout here.
	//mc_config_tsec_carveout(0, 0, 1);

	MC(0x9A0) = 0;
	MC(0x9A4) = 0;
	MC(0x9A8) = 0;
	MC(0x9AC) = 1;
	MC(0xC0C) = 0;
	MC(0xC10) = 0;
	MC(0xC14) = 0;
	MC(0xC18) = 0;
	MC(0xC1C) = 0;
	MC(0xC20) = 0;
	MC(0xC24) = 0;
	MC(0xC28) = 0;
	MC(0xC2C) = 0;
	MC(0xC30) = 0;
	MC(0xC34) = 0;
	MC(0xC38) = 0;
	MC(0xC3C) = 0;
	MC(0xC08) = 0x4000006;
	MC(0xC5C) = 0x80020000;
	MC(0xC60) = 0;
	MC(0xC64) = 2;
	MC(0xC68) = 0;
	MC(0xC6C) = 0;
	MC(0xC70) = 0x3000000;
	MC(0xC74) = 0;
	MC(0xC78) = 0x300;
	MC(0xC7C) = 0;
	MC(0xC80) = 0;
	MC(0xC84) = 0;
	MC(0xC88) = 0;
	MC(0xC8C) = 0;
	MC(0xC58) = 0x440167E;
	MC(0xCAC) = 0;
	MC(0xCB0) = 0;
	MC(0xCB4) = 0;
	MC(0xCB8) = 0;
	MC(0xCBC) = 0;
	MC(0xCC0) = 0x3000000;
	MC(0xCC4) = 0;
	MC(0xCC8) = 0x300;
	MC(0xCCC) = 0;
	MC(0xCD0) = 0;
	MC(0xCD4) = 0;
	MC(0xCD8) = 0;
	MC(0xCDC) = 0;
	MC(0xCA8) = 0x4401E7E;
	MC(0xCFC) = 0;
	MC(0xD00) = 0;
	MC(0xD04) = 0;
	MC(0xD08) = 0;
	MC(0xD0C) = 0;
	MC(0xD10) = 0;
	MC(0xD14) = 0;
	MC(0xD18) = 0;
	MC(0xD1C) = 0;
	MC(0xD20) = 0;
	MC(0xD24) = 0;
	MC(0xD28) = 0;
	MC(0xD2C) = 0;
	MC(0xCF8) = 0x8F;
	MC(0xD4C) = 0;
	MC(0xD50) = 0;
	MC(0xD54) = 0;
	MC(0xD58) = 0;
	MC(0xD5C) = 0;
	MC(0xD60) = 0;
	MC(0xD64) = 0;
	MC(0xD68) = 0;
	MC(0xD6C) = 0;
	MC(0xD70) = 0;
	MC(0xD74) = 0;
	MC(0xD78) = 0;
	MC(0xD7C) = 0;
	MC(0xD48) = 0x8F;
}

void enable_clocks()
{
	CLOCK(0x410) = (CLOCK(0x410) | 0x8000) & 0xFFFFBFFF;
	CLOCK(0xD0) |= 0x40800000u;
	CLOCK(0x2AC) = 64;
	CLOCK(0x294) = 0x40000;
	CLOCK(0x304) = 0x18000000;
	sleep(2);

	I2S(0x0A0) |= 0x400;
	I2S(0x088) &= 0xFFFFFFFE;
	I2S(0x1A0) |= 0x400;
	I2S(0x188) &= 0xFFFFFFFE;
	I2S(0x2A0) |= 0x400;
	I2S(0x288) &= 0xFFFFFFFE;
	I2S(0x3A0) |= 0x400;
	I2S(0x388) &= 0xFFFFFFFE;
	I2S(0x4A0) |= 0x400;
	I2S(0x488) &= 0xFFFFFFFE;
	DISPLAY_A(0xCF8) |= 4;
	VIC(0x8C) = 0xFFFFFFFF;
	sleep(2);

	CLOCK(0x2A8) = 0x40;
	CLOCK(0x300) = 0x18000000;
	CLOCK(0x290) = 0x40000;
	CLOCK(0x14) = 0xC0;
	CLOCK(0x10) = 0x80000130;
	CLOCK(0x18) = 0x1F00200;
	CLOCK(0x360) = 0x80400808;
	CLOCK(0x364) = 0x402000FC;
	CLOCK(0x280) = 0x23000780;
	CLOCK(0x298) = 0x300;
	CLOCK(0xF8) = 0;
	CLOCK(0xFC) = 0;
	CLOCK(0x3A0) = 0;
	CLOCK(0x3A4) = 0;
	CLOCK(0x554) = 0;
	CLOCK(0xD0) &= 0x1F7FFFFFu;
	CLOCK(0x410) &= 0xFFFF3FFF;
	CLOCK(0x148) = CLOCK(0x148) & 0x1FFFFFFF | 0x80000000;
	CLOCK(0x180) = CLOCK(0x180) & 0x1FFFFFFF | 0x80000000;
	CLOCK(0x6A0) = CLOCK(0x6A0) & 0x1FFFFFFF | 0x80000000;
}

void mc_enable_ahb_redirect()
{
	CLOCK(0x3A4) = CLOCK(0x3A4) & 0xFFF7FFFF | 0x80000;
	//MC(MC_IRAM_REG_CTRL) &= 0xFFFFFFFE;
	MC(MC_IRAM_BOM) = 0x40000000;
	MC(MC_IRAM_TOM) = 0x4003F000;
}

void mc_disable_ahb_redirect()
{
	MC(MC_IRAM_BOM) = 0xFFFFF000;
	MC(MC_IRAM_TOM) = 0;
	//Disable IRAM_CFG_WRITE_ACCESS (sticky).
	//MC(MC_IRAM_REG_CTRL) = MC(MC_IRAM_REG_CTRL) & 0xFFFFFFFE | 1;
	CLOCK(0x3A4) &= 0xFFF7FFFF;
}

void mc_enable()
{
	CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) = CLOCK(CLK_RST_CONTROLLER_CLK_SOURCE_EMC) & 0x1FFFFFFF | 0x40000000;
	//Enable MIPI CAL clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) & 0xFDFFFFFF | 0x2000000;
	//Enable MC clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) = CLOCK(CLK_RST_CONTROLLER_CLK_ENB_H_SET) & 0xFFFFFFFE | 1;
	//Enable EMC DLL clock.
	CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) = CLOCK(CLK_RST_CONTROLLER_CLK_ENB_X_SET) & 0xFFFFBFFF | 0x4000;
	CLOCK(CLK_RST_CONTROLLER_RST_DEV_H_SET) = 0x2000001; //Clear EMC and MC reset.
	sleep(5);
}

clock_t clock_unk1 = { 0x358, 0x360, 0x42C, 0x1F, 0, 0 };
clock_t clock_unk2 = { 0x358, 0x360, 0, 0x1E, 0, 0 };

void nx_hwinit()
{
	enable_clocks();
	clock_enable_se();
	clock_enable_fuse(1);
	fuse_disable_program();

	mc_enable();

	config_oscillators();
	_REG(0x70000000, 0x40) = 0;

	config_gpios();

	clock_enable_i2c(I2C_1);
	clock_enable_i2c(I2C_5);
	clock_enable(&clock_unk1);
	clock_enable(&clock_unk2);

	i2c_init(I2C_1);
	i2c_init(I2C_5);

	//Config PMIC (TODO: use max77620.h)
	i2c_send_byte(I2C_5, 0x3C, 4, 0x40);
	i2c_send_byte(I2C_5, 0x3C, 0x41, 0x78);
	i2c_send_byte(I2C_5, 0x3C, 0x43, 0x38);
	i2c_send_byte(I2C_5, 0x3C, 0x44, 0x3A);
	i2c_send_byte(I2C_5, 0x3C, 0x45, 0x38);
	i2c_send_byte(I2C_5, 0x3C, 0x4A, 0xF);
	i2c_send_byte(I2C_5, 0x3C, 0x4E, 0xC7);
	i2c_send_byte(I2C_5, 0x3C, 0x4F, 0x4F);
	i2c_send_byte(I2C_5, 0x3C, 0x50, 0x29);
	i2c_send_byte(I2C_5, 0x3C, 0x52, 0x1B);
	i2c_send_byte(I2C_5, 0x3C, 0x16, 42); //42 = (1000 * 1125 - 600000) / 12500

	config_pmc_scratch();

	CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) = CLOCK(CLK_RST_CONTROLLER_SCLK_BURST_POLICY) & 0xFFFF8888 | 0x3333;

	mc_config_carveout();

	sdram_init();
}
