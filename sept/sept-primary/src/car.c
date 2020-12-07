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
 
#include "car.h"
#include "timers.h"
#include "utils.h"

static inline uint32_t get_clk_source_reg(CarDevice dev) {
    switch (dev) {
        case CARDEVICE_UARTA: return 0x178;
        case CARDEVICE_UARTB: return 0x17C;
        case CARDEVICE_UARTC: return 0x1A0;
        case CARDEVICE_I2C1: return 0x124;
        case CARDEVICE_I2C5: return 0x128;
        case CARDEVICE_TZRAM: return 0;
        case CARDEVICE_SE: return 0x42C;
        case CARDEVICE_HOST1X: return 0x180;
        case CARDEVICE_TSEC: return 0x1F4;
        case CARDEVICE_SOR_SAFE: return 0;
        case CARDEVICE_SOR0: return 0;
        case CARDEVICE_SOR1: return 0x410;
        case CARDEVICE_KFUSE: return 0;
        case CARDEVICE_CL_DVFS: return 0;
        case CARDEVICE_CORESIGHT: return 0x1D4;
        case CARDEVICE_MSELECT: return 0x3B4;
        case CARDEVICE_ACTMON: return 0x3E8;
        case CARDEVICE_BPMP: return 0;
        default: generic_panic();
    }
} 

static inline uint32_t get_clk_source_val(CarDevice dev) {
    switch (dev) {
        case CARDEVICE_UARTA: return 0;
        case CARDEVICE_UARTB: return 0;
        case CARDEVICE_UARTC: return 0;
        case CARDEVICE_I2C1: return 6;
        case CARDEVICE_I2C5: return 6;
        case CARDEVICE_TZRAM: return 0;
        case CARDEVICE_SE: return 0;
        case CARDEVICE_HOST1X: return 4;
        case CARDEVICE_TSEC: return 0;
        case CARDEVICE_SOR_SAFE: return 0;
        case CARDEVICE_SOR0: return 0;
        case CARDEVICE_SOR1: return 0;
        case CARDEVICE_KFUSE: return 0;
        case CARDEVICE_CL_DVFS: return 0;
        case CARDEVICE_CORESIGHT: return 0;
        case CARDEVICE_MSELECT: return 0;
        case CARDEVICE_ACTMON: return 6;
        case CARDEVICE_BPMP: return 0;
        default: generic_panic();
    }
}

static inline uint32_t get_clk_source_div(CarDevice dev) {
    switch (dev) {
        case CARDEVICE_UARTA: return 0;
        case CARDEVICE_UARTB: return 0;
        case CARDEVICE_UARTC: return 0;
        case CARDEVICE_I2C1: return 0;
        case CARDEVICE_I2C5: return 0;
        case CARDEVICE_TZRAM: return 0;
        case CARDEVICE_SE: return 0;
        case CARDEVICE_HOST1X: return 3;
        case CARDEVICE_TSEC: return 2;
        case CARDEVICE_SOR_SAFE: return 0;
        case CARDEVICE_SOR0: return 0;
        case CARDEVICE_SOR1: return 2;
        case CARDEVICE_KFUSE: return 0;
        case CARDEVICE_CL_DVFS: return 0;
        case CARDEVICE_CORESIGHT: return 4;
        case CARDEVICE_MSELECT: return 6;
        case CARDEVICE_ACTMON: return 0;
        case CARDEVICE_BPMP: return 0;
        default: generic_panic();
    }
}

static uint32_t g_clk_reg_offsets[NUM_CAR_BANKS] = {0x010, 0x014, 0x018, 0x360, 0x364, 0x280, 0x298};
static uint32_t g_rst_reg_offsets[NUM_CAR_BANKS] = {0x004, 0x008, 0x00C, 0x358, 0x35C, 0x28C, 0x2A4}; 

void clk_enable(CarDevice dev) {
    uint32_t clk_source_reg;
    if ((clk_source_reg = get_clk_source_reg(dev))) {
        MAKE_CAR_REG(clk_source_reg) = (get_clk_source_val(dev) << 29) | get_clk_source_div(dev);
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
    if (dev == CARDEVICE_KFUSE) {
        /* Workaround for KFUSE clock. */
        clk_enable(dev);
        udelay(100);
        rst_disable(dev);
        udelay(200);
    } else {
        clkrst_enable(dev);
    }
}

void clkrst_enable_fuse_regs(bool enable) {
    volatile tegra_car_t *car = car_get_regs();
    car->misc_clk_enb = ((car->misc_clk_enb & 0xEFFFFFFF) | ((enable & 1) << 28));
}
