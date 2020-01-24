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

#include <stdint.h>

#include "utils.h"
#include "car.h"
#include "timers.h"

static inline uint32_t get_special_clk_reg(CarDevice dev) {
    switch (dev) {
        case CARDEVICE_UARTA: return 0x178;
        case CARDEVICE_UARTB: return 0x17C;
        case CARDEVICE_I2C1: return 0x124;
        case CARDEVICE_I2C5: return 0x128;
        case CARDEVICE_ACTMON: return 0x3E8;
        case CARDEVICE_BPMP: return 0;
        default: generic_panic();
    }
}

static inline uint32_t get_special_clk_val(CarDevice dev) {
    switch (dev) {
        case CARDEVICE_UARTA: return 0;
        case CARDEVICE_UARTB: return 0;
        case CARDEVICE_I2C1: return (6 << 29);
        case CARDEVICE_I2C5: return (6 << 29);
        case CARDEVICE_ACTMON: return (6 << 29);
        case CARDEVICE_BPMP: return 0;
        default: generic_panic();
    }
}

static uint32_t g_clk_reg_offsets[NUM_CAR_BANKS] = {0x010, 0x014, 0x018, 0x360, 0x364, 0x280, 0x298};
static uint32_t g_rst_reg_offsets[NUM_CAR_BANKS] = {0x004, 0x008, 0x00C, 0x358, 0x35C, 0x28C, 0x2A4};

void clk_enable(CarDevice dev) {
    uint32_t special_reg;
    if ((special_reg = get_special_clk_reg(dev))) {
        MAKE_CAR_REG(special_reg) = get_special_clk_val(dev);
    }
    MAKE_CAR_REG(g_clk_reg_offsets[dev >> 5]) |= BIT(dev & 0x1F);
}

void clk_disable(CarDevice dev) {
    MAKE_CAR_REG(g_clk_reg_offsets[dev >> 5]) &= ~(BIT(dev & 0x1F));
}

void rst_enable(CarDevice dev) {
    MAKE_CAR_REG(g_rst_reg_offsets[dev >> 5]) |= BIT(dev & 0x1F);
}

void rst_disable(CarDevice dev) {
    MAKE_CAR_REG(g_rst_reg_offsets[dev >> 5]) &= ~(BIT(dev & 0x1F));
}

void clkrst_enable(CarDevice dev) {
    clk_enable(dev);
    rst_disable(dev);
}

void clkrst_disable(CarDevice dev) {
    rst_enable(dev);
    clk_disable(dev);
}

void clkrst_reboot(CarDevice dev) {
    clkrst_disable(dev);
    clkrst_enable(dev);
}

void clkrst_enable_fuse_regs(bool enable) {
    CLK_RST_CONTROLLER_MISC_CLK_ENB_0 = ((CLK_RST_CONTROLLER_MISC_CLK_ENB_0 & 0xEFFFFFFF) | ((enable & 1) << 28));
}