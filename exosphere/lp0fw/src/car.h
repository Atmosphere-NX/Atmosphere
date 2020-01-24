/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
 
#ifndef EXOSPHERE_WARMBOOT_BIN_CLOCK_AND_RESET_H
#define EXOSPHERE_WARMBOOT_BIN_CLOCK_AND_RESET_H

#include <stdint.h>

#define CAR_BASE 0x60006000

#define MAKE_CAR_REG(n) MAKE_REG32(CAR_BASE + n)

#define CLK_RST_CONTROLLER_MISC_CLK_ENB_0 MAKE_CAR_REG(0x048)
#define CLK_RST_CONTROLLER_OSC_CTRL_0 MAKE_CAR_REG(0x050)
#define CLK_RST_CONTROLLER_PLLX_BASE_0 MAKE_CAR_REG(0x0E0)
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD_0 MAKE_CAR_REG(0x3A4)
#define CLK_RST_CONTROLLER_RST_CPUG_CMPLX_SET_0 MAKE_CAR_REG(0x450)
#define CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR_0 MAKE_CAR_REG(0x454)

#define CLK_RST_CONTROLLER_SUPER_CCLKG_DIVIDER_0 MAKE_CAR_REG(0x36C)
#define CLK_RST_CONTROLLER_SUPER_CCLKP_DIVIDER_0 MAKE_CAR_REG(0x374)

#define CLK_RST_CONTROLLER_CLK_SOURCE_I2C5_0 MAKE_CAR_REG(0x128)

#define CLK_RST_CONTROLLER_CLK_SOURCE_DVFS_REF_0 MAKE_CAR_REG(0x62C)
#define CLK_RST_CONTROLLER_CLK_SOURCE_DVFS_SOC_0 MAKE_CAR_REG(0x630)

#define CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2_0 MAKE_CAR_REG(0x388)
#define CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT_0 MAKE_CAR_REG(0x3B4)

#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA_0 MAKE_CAR_REG(0x0F8)
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB_0 MAKE_CAR_REG(0x0FC)
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC_0 MAKE_CAR_REG(0x3A0)
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD_0 MAKE_CAR_REG(0x3A4)
#define CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE_0 MAKE_CAR_REG(0x554)

#define CLK_RST_CONTROLLER_CCLKG_BURST_POLICY_0 MAKE_CAR_REG(0x368)
#define CLK_RST_CONTROLLER_CCLKP_BURST_POLICY_0 MAKE_CAR_REG(0x370)

#define CLK_RST_CONTROLLER_RST_DEVICES_H_0 MAKE_CAR_REG(0x008)

#define CLK_RST_CONTROLLER_SPARE_REG0_0 MAKE_CAR_REG(0x55C)

#define CLK_RST_CONTROLLER_RST_DEV_H_SET_0 MAKE_CAR_REG(0x308)
#define CLK_RST_CONTROLLER_RST_DEV_U_SET_0 MAKE_CAR_REG(0x310)

#define CLK_RST_CONTROLLER_RST_DEV_H_CLR_0 MAKE_CAR_REG(0x30C)
#define CLK_RST_CONTROLLER_RST_DEV_U_CLR_0 MAKE_CAR_REG(0x314)
#define CLK_RST_CONTROLLER_RST_DEV_V_CLR_0 MAKE_CAR_REG(0x434)

#define CLK_RST_CONTROLLER_CLK_ENB_L_SET_0 MAKE_CAR_REG(0x320)
#define CLK_RST_CONTROLLER_CLK_ENB_H_SET_0 MAKE_CAR_REG(0x328)
#define CLK_RST_CONTROLLER_CLK_ENB_U_SET_0 MAKE_CAR_REG(0x330)
#define CLK_RST_CONTROLLER_CLK_ENB_V_SET_0 MAKE_CAR_REG(0x440)
#define CLK_RST_CONTROLLER_CLK_ENB_W_SET_0 MAKE_CAR_REG(0x448)
#define CLK_RST_CONTROLLER_CLK_ENB_Y_SET_0 MAKE_CAR_REG(0x29C)

#define CLK_RST_CONTROLLER_CLK_ENB_H_CLR_0 MAKE_CAR_REG(0x32C)
#define CLK_RST_CONTROLLER_CLK_ENB_W_CLR_0 MAKE_CAR_REG(0x44C)

#define NUM_CAR_BANKS 7

typedef enum {
    CARDEVICE_UARTA = ((0 << 5) | 0x6),
    CARDEVICE_UARTB = ((0 << 5) | 0x7),
    CARDEVICE_UARTC = ((1 << 5) | 0x17),
    CARDEVICE_I2C1 = ((0 << 5) | 0xC),
    CARDEVICE_I2C5 = ((1 << 5) | 0xF),
    CARDEVICE_TZRAM = ((3 << 5) | 0x1E),
    CARDEVICE_SE = ((3 << 5) | 0x1F),
    CARDEVICE_HOST1X = ((0 << 5) | 0x1C),
    CARDEVICE_TSEC = ((2 << 5) | 0x13),
    CARDEVICE_SOR_SAFE = ((6 << 5) | 0x1E),
    CARDEVICE_SOR0 = ((5 << 5) | 0x16),
    CARDEVICE_SOR1 = ((5 << 5) | 0x17),
    CARDEVICE_KFUSE = ((1 << 5) | 0x8),
    CARDEVICE_CL_DVFS = ((4 << 5) | 0x1B),
    CARDEVICE_CORESIGHT = ((2 << 5) | 0x9),
    CARDEVICE_ACTMON = ((3 << 5) | 0x17),
    CARDEVICE_BPMP = ((0 << 5) | 0x1)
} CarDevice;

void car_configure_oscillators(void);
void car_mbist_workaround(void);

void clk_enable(CarDevice dev);
void clk_disable(CarDevice dev);
void rst_enable(CarDevice dev);
void rst_disable(CarDevice dev);

void clkrst_enable(CarDevice dev);
void clkrst_disable(CarDevice dev);
void clkrst_reboot(CarDevice dev);

void clkrst_enable_fuse_regs(bool enable);

#endif
