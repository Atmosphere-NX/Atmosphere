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
#include "cluster.h"
#include "car.h"
#include "timer.h"
#include "pmc.h"
#include "misc.h"
#include "i2c.h"
#include "flow.h"
#include "sysreg.h"

static void cluster_pmc_enable_partition(uint32_t mask, uint32_t toggle) {
    /* Set toggle if unset. */
    if (!(APBDEV_PMC_PWRGATE_STATUS_0 & mask)) {
        APBDEV_PMC_PWRGATE_TOGGLE_0 = toggle;
    }
    
    /* Wait until toggle set. */
    while (!(APBDEV_PMC_PWRGATE_STATUS_0 & mask)) { }

    /* Remove clamping. */
    APBDEV_PMC_REMOVE_CLAMPING_CMD_0 = mask;
    while (APBDEV_PMC_CLAMP_STATUS_0 & mask) { }
}

void cluster_initialize_cpu(void) {
    /* Hold CoreSight in reset. */
    CLK_RST_CONTROLLER_RST_DEV_U_SET_0 = 0x200;

    /* CAR2PMC_CPU_ACK_WIDTH should be set to 0. */
    CLK_RST_CONTROLLER_CPU_SOFTRST_CTRL2_0 &= 0xFFFFF000;
    
    /* Restore SB_AA64_RESET values from PMC scratch. */
    SB_AA64_RESET_LOW_0 = APBDEV_PMC_SECURE_SCRATCH34_0 | 1;
    SB_AA64_RESET_HIGH_0 = APBDEV_PMC_SECURE_SCRATCH35_0;
    
    /* Set CDIV_ENB for CCLKG/CCLKP. */
    CLK_RST_CONTROLLER_SUPER_CCLKG_DIVIDER_0 = 0x80000000;
    CLK_RST_CONTROLLER_SUPER_CCLKP_DIVIDER_0 = 0x80000000;
    
    /* Enable CoreSight clock, take CoreSight out of reset. */
    CLK_RST_CONTROLLER_CLK_ENB_U_SET_0 = 0x200;
    CLK_RST_CONTROLLER_RST_DEV_U_CLR_0 = 0x200;
    
    /* Configure MSELECT to divide by 4, enable MSELECT clock. */
    CLK_RST_CONTROLLER_CLK_SOURCE_MSELECT_0 = 6; /* (6/2) + 1 = 4. */
    CLK_RST_CONTROLLER_CLK_ENB_V_SET_0 = 0x8;
    
    /* Wait 2 us, then take MSELECT out of reset. */
    timer_wait(2);
    CLK_RST_CONTROLLER_RST_DEV_V_CLR_0 = 0x8;
    
    /* Set MSELECT WRAP_TO_SLAVE_INCR[0-2], clear ERR_RESP_EN_SLAVE[1-2]. */
    MSELECT_CONFIG_0 = (MSELECT_CONFIG_0 & 0xFCFFFFFF) | 0x38000000;
    
    /* Clear PLLX_ENABLE. */
    CLK_RST_CONTROLLER_PLLX_BASE_0 &= 0xBFFFFFFF;
    
    /* Clear PMC scratch 190, disable PMC DPD then wait 10 us. */
    APBDEV_PMC_SCRATCH190_0 &= 0xFFFFFFFE;
    APBDEV_PMC_DPD_SAMPLE_0 = 0;
    timer_wait(10);
    
    /* Configure UART2 via GPIO controller 2 G. */
    MAKE_REG32(0x6000D108) |= 4;  /* GPIO_CNF */
    MAKE_REG32(0x6000D118) |= 4;  /* GPIO_OE */
    MAKE_REG32(0x6000D128) &= ~4; /* GPIO_OUT */
    
    /* Set CL_DVFS RSVD0 + TRISTATE, read register to make it stick. */
    PINMUX_AUX_DVFS_PWM_0 = 0x11;
    (void)PINMUX_AUX_DVFS_PWM_0;
    
    /* Configure I2C. */
    PINMUX_AUX_PWR_I2C_SCL_0 = 0x40;
    PINMUX_AUX_PWR_I2C_SDA_0 = 0x40;
    
    /* Enable clock to CL_DVFS, and set its source/divider. */
    CLK_RST_CONTROLLER_CLK_ENB_W_SET_0 = 0x08000000;
    CLK_RST_CONTROLLER_CLK_SOURCE_DVFS_REF_0 = 0xE;
    CLK_RST_CONTROLLER_CLK_SOURCE_DVFS_SOC_0 = 0xE;
    
    /* Power on I2C5, wait 5 us, set source + take out of reset. */
    CLK_RST_CONTROLLER_CLK_ENB_H_SET_0 = 0x8000;
    CLK_RST_CONTROLLER_RST_DEV_H_SET_0 = 0x8000;
    timer_wait(5);
    CLK_RST_CONTROLLER_CLK_SOURCE_I2C5_0 = 0x4;
    CLK_RST_CONTROLLER_RST_DEV_H_CLR_0 = 0x8000;
    
    /* Enable the PMIC, wait 2ms. */
    i2c_enable_pmic();
    timer_wait(2000);
    
    /* Enable power to the CRAIL partition. */
    cluster_pmc_enable_partition(1, 0x100);
    
    /* Remove SW clamp to CRAIL. */
    APBDEV_PMC_SET_SW_CLAMP_0 = 0;
    APBDEV_PMC_REMOVE_CLAMPING_CMD_0 = 1;
    while (APBDEV_PMC_CLAMP_STATUS_0 & 1) { }
    
    /* Nintendo manually counts down from 8. I am not sure why this happens. */
    {
        volatile int32_t counter = 8;
        while (counter >= 0) {
            counter--;
        }
    }
    
    /* Power off I2C5. */
    CLK_RST_CONTROLLER_RST_DEV_H_SET_0 = 0x8000;
    CLK_RST_CONTROLLER_CLK_ENB_H_CLR_0 = 0x8000;
    
    /* Disable clock to CL_DVFS */
    CLK_RST_CONTROLLER_CLK_ENB_W_CLR_0 = 0x08000000;
    
    /* Perform RAM repair if necessary. */
    flow_perform_ram_repair();
    
    /* Enable power to the non-CPU partition. */
    cluster_pmc_enable_partition(0x8000, 0x10F);
    
    /* Enable clock to PLLP_OUT_CPU, wait 2 us. */
    CLK_RST_CONTROLLER_CLK_ENB_Y_SET_0 = 0x80000000;
    timer_wait(2);
    
    /* Enable clock to CPU, CPUG, wait 10 us. */
    CLK_RST_CONTROLLER_CLK_ENB_L_SET_0 = 1;
    CLK_RST_CONTROLLER_CLK_ENB_V_SET_0 = 1;
    timer_wait(10);
    
    /* Set CPU clock sources to PLLP_OUT_0 + state to RUN, wait 10 us. */
    CLK_RST_CONTROLLER_CCLKG_BURST_POLICY_0 = 0x20004444;
    CLK_RST_CONTROLLER_CCLKP_BURST_POLICY_0 = 0x20004444;
    timer_wait(10);
    
    /* Take non-CPU out of reset (write CLR_NONCPURESET). */
    CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR_0 = 0x20000000;
}

void cluster_power_on_cpu(void) {
    /*  Enable power to CE0 partition. */
    cluster_pmc_enable_partition(0x4000, 0x10E);
    
    /* Clear CPU reset. */
    CLK_RST_CONTROLLER_RST_CPUG_CMPLX_CLR_0 = 0x10001;
}