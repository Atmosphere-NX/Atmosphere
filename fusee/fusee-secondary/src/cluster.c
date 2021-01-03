/*
 * Copyright (c) 2018 naehrwert
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

#include "cluster.h"
#include "flow.h"
#include "sysreg.h"
#include "i2c.h"
#include "car.h"
#include "fuse.h"
#include "mc.h"
#include "timers.h"
#include "pmc.h"
#include "max77620.h"
#include "max77812.h"

/* Determine the current SoC for Mariko specific code. */
static bool is_soc_mariko() {
    return (fuse_get_soc_type() == 1);
}

static void cluster_enable_power(uint32_t regulator) {
    switch (regulator) {
        case 0:     /* Regulator_Max77621 */
            {
                uint8_t val = 0;
                i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_AME_GPIO, &val, 1);
                val &= 0xDF;
                i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_AME_GPIO, &val, 1);
                val = 0x09;
                i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_GPIO5, &val, 1);
                val = 0xB0;
                i2c_send(I2C_5, MAX77621_CPU_I2C_ADDR, 0x02, &val, 1);
                val = 0xC1;
                i2c_send(I2C_5, MAX77621_CPU_I2C_ADDR, 0x03, &val, 1);
                val = 0xB7;
                i2c_send(I2C_5, MAX77621_CPU_I2C_ADDR, 0x00, &val, 1);
                val = 0xB7;
                i2c_send(I2C_5, MAX77621_CPU_I2C_ADDR, 0x01, &val, 1);
            }
            break;
        case 1:     /* Regulator_Max77812PhaseConfiguration31 */
            {
                uint8_t val = 0;
                i2c_query(I2C_5, MAX77812_PHASE31_CPU_I2C_ADDR, MAX77812_REG_EN_CTRL, &val, 1);
                val |= 0x40;
                i2c_send(I2C_5, MAX77812_PHASE31_CPU_I2C_ADDR, MAX77812_REG_EN_CTRL, &val, 1);
                val = 0x6E;
                i2c_send(I2C_5, MAX77812_PHASE31_CPU_I2C_ADDR, MAX77812_REG_M4_VOUT, &val, 1);
            }
            break;
        case 2:     /* Regulator_Max77812PhaseConfiguration211 */
            {
                uint8_t val = 0;
                i2c_query(I2C_5, MAX77812_PHASE211_CPU_I2C_ADDR, MAX77812_REG_EN_CTRL, &val, 1);
                val |= 0x40;
                i2c_send(I2C_5, MAX77812_PHASE211_CPU_I2C_ADDR, MAX77812_REG_EN_CTRL, &val, 1);
                val = 0x6E;
                i2c_send(I2C_5, MAX77812_PHASE211_CPU_I2C_ADDR, MAX77812_REG_M4_VOUT, &val, 1);
            }
            break;
        default: return;
    }
}

static bool cluster_is_partition_powered(volatile tegra_pmc_t *pmc, uint32_t status) {
    return (pmc->pwrgate_status & status) == status;
}

static void cluster_pmc_enable_partition(uint32_t part, uint32_t toggle) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    const uint32_t status = (toggle << part);

    /* Check if the partition has already been turned on. */
    if (cluster_is_partition_powered(pmc, status)) {
        return;
    }

    int timeout = 5000;
    while (true) {
        if ((pmc->pwrgate_toggle & 0x100) == 0) {
            break;
        }
        udelay(1);
        if ((--timeout) < 0) {
            return;
        }
    }

    /* Turn the partition on. */
    pmc->pwrgate_toggle = (part | 0x100);

    timeout = 5000;
    while (true) {
        if (cluster_is_partition_powered(pmc, status)) {
            break;
        }
        udelay(1);
        if ((--timeout) < 0) {
            return;
        }
    }
}

void cluster_boot_cpu0(uint32_t entry) {
    volatile tegra_car_t *car = car_get_regs();
    bool is_mariko = is_soc_mariko();

    /* Set ACTIVE_CLUSER to FAST. */
    FLOW_CTLR_BPMP_CLUSTER_CONTROL_0 &= 0xFFFFFFFE;

    /* Enable VddCpu. */
    cluster_enable_power(is_mariko ? fuse_get_regulator() : 0);

    if (!(car->pllx_base & 0x40000000)) {
        car->pllx_misc3 &= 0xFFFFFFF7;
        udelay(2);
        car->pllx_base = 0x80404E02;
        car->pllx_base = 0x404E02;
        car->pllx_misc = ((car->pllx_misc & 0xFFFBFFFF) | 0x40000);
        car->pllx_base = 0x40404E02;
    }

    while (!(car->pllx_base & 0x8000000)) {
        /* Wait. */
    }

    /* Set MSELECT clock. */
    clk_enable(CARDEVICE_MSELECT);

    /* Configure initial CPU clock frequency and enable clock. */
    car->cclk_brst_pol = 0x20008888;
    car->super_cclk_div = 0x80000000;
    car->clk_enb_v_set = 1;

    /* Reboot CORESIGHT. */
    clkrst_reboot(CARDEVICE_CORESIGHT);

    /* Set CAR2PMC_CPU_ACK_WIDTH to 0. */
    car->cpu_softrst_ctrl2 &= 0xFFFFF000;

    /* Enable CPU rail. */
    cluster_pmc_enable_partition(0, 1);

    /* Enable cluster 0 non-CPU. */
    cluster_pmc_enable_partition(15, 1);

    /* Enable CE0. */
    cluster_pmc_enable_partition(14, 1);

    /* Request and wait for RAM repair. */
    FLOW_CTLR_RAM_REPAIR_0 = 1;
    while (!(FLOW_CTLR_RAM_REPAIR_0 & 2)) {
        /* Wait. */
    }

    MAKE_EXCP_VEC_REG(0x100) = 0;

    /* Set reset vector. */
    SB_AA64_RESET_LOW_0 = (entry | 1);
    SB_AA64_RESET_HIGH_0 = 0;

    /* Non-secure reset vector write disable. */
    SB_CSR_0 = 2;
    (void)SB_CSR_0;

    /* Set CPU_STRICT_TZ_APERTURE_CHECK. */
    /* NOTE: This breaks Exosphère. */
    /* MAKE_MC_REG(MC_TZ_SECURITY_CTRL) = 1; */

    /* Clear MSELECT reset. */
    rst_disable(CARDEVICE_MSELECT);

    /* Clear NONCPU reset. */
    car->rst_cpug_cmplx_clr = 0x20000000;

    /* Clear CPU{0} POR and CORE, CX0, L2, and DBG reset.*/
    car->rst_cpug_cmplx_clr = 0x41010001;
}
