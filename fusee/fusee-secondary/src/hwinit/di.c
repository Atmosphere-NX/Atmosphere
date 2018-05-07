#include "di.h"
#include "t210.h"
#include "util.h"
#include "i2c.h"
#include "pmc.h"

#include "di.inl"

static u32 _display_ver = 0;

static void _display_dsi_wait(u32 timeout, u32 off, u32 mask)
{
	u32 end = TMR(0x10) + timeout;
	while (TMR(0x10) < end && DSI(off) & mask)
		;
	sleep(5);
}

void display_init()
{
	//Power on.
	i2c_send_byte(I2C_5, 0x3C, 0x23, 0xD0);
	i2c_send_byte(I2C_5, 0x3C, 0x3D, 0x09);

	//Enable MIPI CAL, DSI, DISP1, HOST1X, UART_FST_MIPI_CAL, DSIA LP clocks.
	CLOCK(0x30C) = 0x1010000;
	CLOCK(0x328) = 0x1010000;
	CLOCK(0x304) = 0x18000000;
	CLOCK(0x320) = 0x18000000;
	CLOCK(0x284) = 0x20000;
	CLOCK(0x66C) = 0xA;
	CLOCK(0x448) = 0x80000;
	CLOCK(0x620) = 0xA;

	//DPD idle.
	PMC(APBDEV_PMC_IO_DPD_REQ) = 0x40000000;
	PMC(APBDEV_PMC_IO_DPD2_REQ) = 0x40000000;

	//Config pins.
	PINMUX_AUX(0x1D0) &= 0xFFFFFFEF;
	PINMUX_AUX(0x1D4) &= 0xFFFFFFEF;
	PINMUX_AUX(0x1FC) &= 0xFFFFFFEF;
	PINMUX_AUX(0x200) &= 0xFFFFFFEF;
	PINMUX_AUX(0x204) &= 0xFFFFFFEF;

	GPIO_3(0x00) = GPIO_3(0x00) & 0xFFFFFFFC | 0x3;
	GPIO_3(0x10) = GPIO_3(0x10) & 0xFFFFFFFC | 0x3;
	GPIO_3(0x20) = GPIO_3(0x20) & 0xFFFFFFFE | 0x1;

	sleep(10000u);

	GPIO_3(0x20) = GPIO_3(0x20) & 0xFFFFFFFD | 0x2;

	sleep(10000);

	GPIO_6(0x04) = GPIO_6(0x04) & 0xFFFFFFF8 | 0x7;
	GPIO_6(0x14) = GPIO_6(0x14) & 0xFFFFFFF8 | 0x7;
	GPIO_6(0x24) = GPIO_6(0x24) & 0xFFFFFFFD | 0x2;

	//Config display interface and display.
	MIPI_CAL(0x60) = 0;

	exec_cfg((u32 *)CLOCK_BASE, _display_config_1, 4);
	exec_cfg((u32 *)DISPLAY_A_BASE, _display_config_2, 94);
	exec_cfg((u32 *)DSI_BASE, _display_config_3, 60);

	sleep(10000);

	GPIO_6(0x24) = GPIO_6(0x24) & 0xFFFFFFFB | 0x4;

	sleep(60000);

	DSI(_DSIREG(DSI_DSI_BTA_TIMING)) = 0x50204;
	DSI(_DSIREG(DSI_DSI_WR_DATA)) = 0x337;
	DSI(_DSIREG(DSI_DSI_TRIGGER)) = 0x2;
	_display_dsi_wait(250000, _DSIREG(DSI_DSI_TRIGGER), 3);

	DSI(_DSIREG(DSI_DSI_WR_DATA)) = 0x406;
	DSI(_DSIREG(DSI_DSI_TRIGGER)) = 0x2;
	_display_dsi_wait(250000, _DSIREG(DSI_DSI_TRIGGER), 3);

	DSI(_DSIREG(DSI_HOST_DSI_CONTROL)) = 0x200B;
	_display_dsi_wait(150000, _DSIREG(DSI_HOST_DSI_CONTROL), 8);

	sleep(5000);

	_display_ver = DSI(_DSIREG(DSI_DSI_RD_DATA));
	if (_display_ver == 0x10)
		exec_cfg((u32 *)DSI_BASE, _display_config_4, 43);

	DSI(_DSIREG(DSI_DSI_WR_DATA)) = 0x1105;
	DSI(_DSIREG(DSI_DSI_TRIGGER)) = 0x2;

	sleep(180000);

	DSI(_DSIREG(DSI_DSI_WR_DATA)) = 0x2905;
	DSI(_DSIREG(DSI_DSI_TRIGGER)) = 0x2;

	sleep(20000);

	exec_cfg((u32 *)DSI_BASE, _display_config_5, 21);
	exec_cfg((u32 *)CLOCK_BASE, _display_config_6, 3);
	DISPLAY_A(_DIREG(DC_DISP_DISP_CLOCK_CONTROL)) = 4;
	exec_cfg((u32 *)DSI_BASE, _display_config_7, 10);

	sleep(10000);

	exec_cfg((u32 *)MIPI_CAL_BASE, _display_config_8, 6);
	exec_cfg((u32 *)DSI_BASE, _display_config_9, 4);
	exec_cfg((u32 *)MIPI_CAL_BASE, _display_config_10, 16);

	sleep(10000);

	exec_cfg((u32 *)DISPLAY_A_BASE, _display_config_11, 113);
}

void display_end()
{
	GPIO_6(0x24) &= 0xFFFFFFFE;
	DSI(_DSIREG(DSI_DSI_VID_MODE_CONTROL)) = 1;
	DSI(_DSIREG(DSI_DSI_WR_DATA)) = 0x2805;

	u32 end = HOST1X(0x30A4) + 5;
	while (HOST1X(0x30A4) < end)
		;

	DISPLAY_A(_DIREG(DC_CMD_STATE_ACCESS)) = 5;
	DSI(_DSIREG(DSI_DSI_VID_MODE_CONTROL)) = 0;

	exec_cfg((u32 *)DISPLAY_A_BASE, _display_config_12, 17);
	exec_cfg((u32 *)DSI_BASE, _display_config_13, 16);

	sleep(10000);

	if (_display_ver == 0x10)
		exec_cfg((u32 *)DSI_BASE, _display_config_14, 22);

	DSI(_DSIREG(DSI_DSI_WR_DATA)) = 0x1005;
	DSI(_DSIREG(DSI_DSI_TRIGGER)) = 2;

	sleep(50000);

	GPIO_6(0x24) &= 0xFFFFFFFB;

	sleep(10000);

	GPIO_3(0x20) &= 0xFFFFFFFD;

	sleep(10000);

	GPIO_3(0x20) = (GPIO_3(0x20) >> 1) << 1;

	sleep(10000);

	//Disable clocks.
	CLOCK(0x308) = 0x1010000;
	CLOCK(0x32C) = 0x1010000;
	CLOCK(0x300) = 0x18000000;
	CLOCK(0x324) = 0x18000000;

	DSI(_DSIREG(DSI_PAD_CONTROL)) = 0x10F010F;
	DSI(_DSIREG(DSI_DSI_POWER_CONTROL)) = 0;

	GPIO_6(0x04) &= 0xFFFFFFFE;

	PINMUX_AUX(0x1FC) = PINMUX_AUX(0x1FC) & 0xFFFFFFEF | 0x10;
	PINMUX_AUX(0x1FC) = (PINMUX_AUX(0x1FC) >> 2) << 2 | 1;
}

void display_color_screen(u32 color)
{
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_one_color, 8);

	//Configure display to show single color.
	DISPLAY_A(_DIREG(DC_WIN_AD_WIN_OPTIONS)) = 0;
	DISPLAY_A(_DIREG(DC_WIN_BD_WIN_OPTIONS)) = 0;
	DISPLAY_A(_DIREG(DC_WIN_CD_WIN_OPTIONS)) = 0;
	DISPLAY_A(_DIREG(DC_DISP_BLEND_BACKGROUND_COLOR)) = color;
	DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) = DISPLAY_A(_DIREG(DC_CMD_STATE_CONTROL)) & 0xFFFFFFFE | 1;

	sleep(35000);

	GPIO_6(0x24) = GPIO_6(0x24) & 0xFFFFFFFE | 1;
}

void display_enable_backlight(bool on) {
	GPIO_6(0x24) = GPIO_6(0x24) & 0xFFFFFFFE | !!on;
}


u32 *display_init_framebuffer(void *address)
{
	u32 *lfb_addr = (u32 *)address;

	//This configures the framebuffer @ 0xC0000000 with a resolution of 1280x720 (line stride 768).
	exec_cfg((u32 *)DISPLAY_A_BASE, cfg_display_framebuffer, 32);

	sleep(35000);

	return lfb_addr;
}
