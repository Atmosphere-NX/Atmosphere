#include "pinmux.h"
#include "t210.h"

void pinmux_config_uart(u32 idx)
{
	PINMUX_AUX(PINMUX_AUX_UARTX_RX(idx)) = 0;
	PINMUX_AUX(PINMUX_AUX_UARTX_TX(idx)) = 0x48;
	PINMUX_AUX(PINMUX_AUX_UARTX_RTS(idx)) = 0;
	PINMUX_AUX(PINMUX_AUX_UARTX_CTS(idx)) = 0x44;
}

void pinmux_config_i2c(u32 idx)
{
	PINMUX_AUX(PINMUX_AUX_X_I2C_SCL(idx)) = 0x40;
	PINMUX_AUX(PINMUX_AUX_X_I2C_SDA(idx)) = 0x40;
}
