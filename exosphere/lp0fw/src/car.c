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
#include "timer.h"
#include "pmc.h"
#include "emc.h"
#include "lp0.h"

static inline uint32_t get_special_clk_reg(CarDevice dev) {
    switch (dev) {
        case CARDEVICE_UARTA: return 0x178;
        case CARDEVICE_UARTB: return 0x17C;
        case CARDEVICE_I2C1: return 0x124;
        case CARDEVICE_I2C5: return 0x128;
        case CARDEVICE_ACTMON: return 0x3E8;
        case CARDEVICE_BPMP: return 0;
        default: reboot();
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
        default: reboot();
    }
}

static uint32_t g_clk_reg_offsets[NUM_CAR_BANKS] = {0x010, 0x014, 0x018, 0x360, 0x364, 0x280, 0x298};
static uint32_t g_rst_reg_offsets[NUM_CAR_BANKS] = {0x004, 0x008, 0x00C, 0x358, 0x35C, 0x28C, 0x2A4};

static uint32_t g_clk_clr_reg_offsets[NUM_CAR_BANKS] = {0x324, 0x32C, 0x334, 0x444, 0x44C, 0x228, 0x2A0};

void car_configure_oscillators(void) {
    /* Enable the crystal oscillator, setting drive strength to the saved value in PMC. */
    CLK_RST_CONTROLLER_OSC_CTRL_0 = (CLK_RST_CONTROLLER_OSC_CTRL_0 & 0xFFFFFC0E) | 1 | (((APBDEV_PMC_OSC_EDPD_OVER_0 >> 1) & 0x3F) << 4);

    /* Set CLK_M_DIVISOR to 1 (causes actual division by 2.) */
    CLK_RST_CONTROLLER_SPARE_REG0_0 = (1 << 2);
    /* Reading the register after writing it is required to ensure value takes. */
    (void)(CLK_RST_CONTROLLER_SPARE_REG0_0);

    /* Set TIMERUS_USEC_CFG to cycle at 0x60 / 0x5 = 19.2 MHz. */
    /* Value is (dividend << 8) | (divisor). */
    TIMERUS_USEC_CFG_0 = 0x45F;
}

void car_mbist_workaround(void) {
    /* This code works around MBIST bug. */

    /* Clear LVL2_CLK_GATE_OVR* registers. */
    CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRA_0 = 0;
    CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRB_0 = 0;
    CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRC_0 = 0;
    CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRD_0 = 0;
    CLK_RST_CONTROLLER_LVL2_CLK_GATE_OVRE_0 = 0;

    /* Clear bit patterns in CAR. */
    /* L: Reset all but RTC, TMR, GPIO, BPMP Cache (CACHE2). */
    MAKE_CAR_REG(g_clk_clr_reg_offsets[0]) = MAKE_CAR_REG(g_clk_reg_offsets[0]) & 0x7FFFFECF;
    /* H: Reset all but MC, PMC, FUSE, EMC. */
    MAKE_CAR_REG(g_clk_clr_reg_offsets[1]) = MAKE_CAR_REG(g_clk_reg_offsets[1]) & 0xFDFFFF3E;
    /* U: Reset all but CSITE, IRAM[A-D], BPMP Cache RAM (CRAM2). */
    MAKE_CAR_REG(g_clk_clr_reg_offsets[2]) = MAKE_CAR_REG(g_clk_reg_offsets[2]) & 0xFE0FFDFF;
    /* V: Reset all but MSELECT, S/PDIF audio doubler, TZRAM, SE. */
    MAKE_CAR_REG(g_clk_clr_reg_offsets[3]) = MAKE_CAR_REG(g_clk_reg_offsets[3]) & 0x3FBFFFF7;
    /* W: Reset all but PCIERX[0-5], ENTROPY. */
    MAKE_CAR_REG(g_clk_clr_reg_offsets[4]) = MAKE_CAR_REG(g_clk_reg_offsets[4]) & 0xFFDFFF03;
    /* X: Reset all but ETC, MCLK, MCLK2, I2C6, EMC_DLL, GPU, DBGAPB, PLLG_REF, . */
    MAKE_CAR_REG(g_clk_clr_reg_offsets[5]) = MAKE_CAR_REG(g_clk_reg_offsets[5]) & 0xDCFFB87F;
    /* Y: Reset all but MC_CDPA, MC_CCPA. */
    MAKE_CAR_REG(g_clk_clr_reg_offsets[6]) = MAKE_CAR_REG(g_clk_reg_offsets[6]) & 0xFFFFFCFF;

    /* Enable clock to MC1, if CH1 is enabled in EMC. */
    if (EMC_FBIO_CFG7_0 & 4) { /* CH1_ENABLE */
        CLK_RST_CONTROLLER_CLK_ENB_W_SET_0 |= 0x40000000; /* SET_CLK_ENB_MC1 */
    }
}

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