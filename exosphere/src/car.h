#ifndef EXOSPHERE_CLOCK_AND_RESET_H
#define EXOSPHERE_CLOCK_AND_RESET_H

#include <stdint.h>

#include "memory_map.h"

/* Exosphere Driver for the Tegra X1 Clock and Reset registers. */

#define CAR_BASE (MMIO_GET_DEVICE_ADDRESS(MMIO_DEVID_CLKRST))

#define MAKE_CAR_REG(n) MAKE_REG32(CAR_BASE + n)

#define CLK_RST_CONTROLLER_MISC_CLK_ENB_0 MAKE_CAR_REG(0x048)
#define CLK_RST_CONTROLLER_RST_DEVICES_H_0 MAKE_CAR_REG(0x008)
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD_0 MAKE_CAR_REG(0x3A4)

#define NUM_CAR_BANKS 7

typedef enum {
    CARDEVICE_UARTA = 6,
    CARDEVICE_UARTB = 7,
    CARDEVICE_I2C1 = 12,
    CARDEVICE_I2C5 = 47,
    CARDEVICE_ACTMON = 119,
    CARDEVICE_BPMP = 1
} CarDevice;

void clk_enable(CarDevice dev);
void clk_disable(CarDevice dev);
void rst_enable(CarDevice dev);
void rst_disable(CarDevice dev);

void clkrst_enable(CarDevice dev);
void clkrst_disable(CarDevice dev);

void clkrst_reboot(CarDevice dev);

#endif
