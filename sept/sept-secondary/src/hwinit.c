/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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

#include "hwinit.h"
#include "apb_misc.h"
#include "car.h"
#include "di.h"
#include "fuse.h"
#include "gpio.h"
#include "i2c.h"
#include "max77620.h"
#include "mc.h"
#include "pinmux.h"
#include "pmc.h"
#include "se.h"
#include "sdram.h"
#include "sysctr0.h"
#include "sysreg.h"
#include "timers.h"
#include "uart.h"

/* Determine the current SoC for Mariko specific code. */
static bool is_soc_mariko() {
    return (fuse_get_soc_type() == 1);
}

static void config_oscillators(void) {
    volatile tegra_car_t *car = car_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();

    car->spare_reg0 = ((car->spare_reg0 & 0xFFFFFFF3) | 4);

    SYSCTR0_CNTFID0_0 = 19200000;
    TIMERUS_USEC_CFG_0 = 0x45F;

    car->osc_ctrl = 0x50000071;
    pmc->osc_edpd_over = ((pmc->osc_edpd_over & 0xFFFFFF81) | 0xE);
    pmc->osc_edpd_over = ((pmc->osc_edpd_over & 0xFFBFFFFF) | 0x400000);
    pmc->cntrl2 = ((pmc->cntrl2 & 0xFFFFEFFF) | 0x1000);
    pmc->scratch188 = ((pmc->scratch188 & 0xFCFFFFFF) | 0x2000000);
    car->clk_sys_rate = 0x10;
    car->pllmb_base &= 0xBFFFFFFF;
    pmc->tsc_mult = ((pmc->tsc_mult & 0xFFFF0000) | 0x249F);      /* 0x249F = 19200000 * (16 / 32.768 kHz) */
    car->sclk_brst_pol = 0x20004444;
    car->super_sclk_div = 0x80000000;
    car->clk_sys_rate = 2;
}

static void config_gpios(void) {
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    bool is_mariko = is_soc_mariko();

    if (is_mariko) {
        uint32_t hardware_type = fuse_get_hardware_type();

        /* Only for HardwareType_Iowa and HardwareType_Five. */
        if ((hardware_type == 3) || (hardware_type == 5)) {
            pinmux->uart2_tx = 0;
            pinmux->uart3_tx = 0;
            gpio_configure_mode(TEGRA_GPIO(G, 0), GPIO_MODE_GPIO);
            gpio_configure_mode(TEGRA_GPIO(D, 1), GPIO_MODE_GPIO);
            gpio_configure_direction(TEGRA_GPIO(G, 0), GPIO_DIRECTION_INPUT);
            gpio_configure_direction(TEGRA_GPIO(D, 1), GPIO_DIRECTION_INPUT);
        }
    } else {
        pinmux->uart2_tx = 0;
        pinmux->uart3_tx = 0;
    }

    pinmux->pe6 = PINMUX_INPUT;
    pinmux->ph6 = PINMUX_INPUT;
    if (!is_mariko) {
        gpio_configure_mode(TEGRA_GPIO(G, 0), GPIO_MODE_GPIO);
        gpio_configure_mode(TEGRA_GPIO(D, 1), GPIO_MODE_GPIO);
    }
    gpio_configure_mode(TEGRA_GPIO(E, 6), GPIO_MODE_GPIO);
    gpio_configure_mode(TEGRA_GPIO(H, 6), GPIO_MODE_GPIO);
    if (!is_mariko) {
        gpio_configure_direction(TEGRA_GPIO(G, 0), GPIO_DIRECTION_INPUT);
        gpio_configure_direction(TEGRA_GPIO(D, 1), GPIO_DIRECTION_INPUT);
    }
    gpio_configure_direction(TEGRA_GPIO(E, 6), GPIO_DIRECTION_INPUT);
    gpio_configure_direction(TEGRA_GPIO(H, 6), GPIO_DIRECTION_INPUT);

    i2c_config(I2C_1);
    i2c_config(I2C_5);
    uart_config(UART_A);

    /* Configure volume up/down buttons as inputs. */
    gpio_configure_mode(GPIO_BUTTON_VOL_UP, GPIO_MODE_GPIO);
    gpio_configure_mode(GPIO_BUTTON_VOL_DOWN, GPIO_MODE_GPIO);
    gpio_configure_direction(GPIO_BUTTON_VOL_UP, GPIO_DIRECTION_INPUT);
    gpio_configure_direction(GPIO_BUTTON_VOL_DOWN, GPIO_DIRECTION_INPUT);

    if (is_mariko) {
        /* Configure home button as input. */
        gpio_configure_mode(TEGRA_GPIO(Y, 1), GPIO_MODE_GPIO);
        gpio_configure_direction(TEGRA_GPIO(Y, 1), GPIO_DIRECTION_INPUT);
    }
}

static void mbist_workaround(void) {
    volatile tegra_car_t *car = car_get_regs();

    car->clk_source_sor1 = ((car->clk_source_sor1 | 0x8000) & 0xFFFFBFFF);
    car->plld_base |= 0x40800000u;
    car->rst_dev_y_clr = 0x40;
    car->rst_dev_x_clr = 0x40000;
    car->rst_dev_l_clr = 0x18000000;
    udelay(2);

    /* Setup I2S. */
    MAKE_I2S_REG(0x0A0) |= 0x400;
    MAKE_I2S_REG(0x088) &= 0xFFFFFFFE;
    MAKE_I2S_REG(0x1A0) |= 0x400;
    MAKE_I2S_REG(0x188) &= 0xFFFFFFFE;
    MAKE_I2S_REG(0x2A0) |= 0x400;
    MAKE_I2S_REG(0x288) &= 0xFFFFFFFE;
    MAKE_I2S_REG(0x3A0) |= 0x400;
    MAKE_I2S_REG(0x388) &= 0xFFFFFFFE;
    MAKE_I2S_REG(0x4A0) |= 0x400;
    MAKE_I2S_REG(0x488) &= 0xFFFFFFFE;

    MAKE_DI_REG(DC_COM_DSC_TOP_CTL) |= 4;
    MAKE_VIC_REG(0x8C) = 0xFFFFFFFF;
    udelay(2);

    /* Set devices in reset. */
    car->rst_dev_y_set = 0x40;
    car->rst_dev_l_set = 0x18000000;
    car->rst_dev_x_set = 0x40000;

    /* Clock out enables. */
    car->clk_out_enb_h = 0xC0;
    car->clk_out_enb_l = 0x80000130;
    car->clk_out_enb_u = 0x1F00200;
    car->clk_out_enb_v = 0x80400808;
    car->clk_out_enb_w = 0x402000FC;
    car->clk_out_enb_x = 0x23000780;
    car->clk_out_enb_y = 0x300;

    /* LVL2 clock gate overrides. */
    car->lvl2_clk_gate_ovra = 0;
    car->lvl2_clk_gate_ovrb = 0;
    car->lvl2_clk_gate_ovrc = 0;
    car->lvl2_clk_gate_ovrd = 0;
    car->lvl2_clk_gate_ovre = 0;

    /* Configure clock sources. */
    car->plld_base &= 0x1F7FFFFF;
    car->clk_source_sor1 &= 0xFFFF3FFF;
    car->clk_source_vi = ((car->clk_source_vi & 0x1FFFFFFF) | 0x80000000);
    car->clk_source_host1x = ((car->clk_source_host1x & 0x1FFFFFFF) | 0x80000000);
    car->clk_source_nvenc = ((car->clk_source_nvenc & 0x1FFFFFFF) | 0x80000000);
}

static void config_se_brom(void) {
    volatile tegra_fuse_chip_common_t *fuse_chip = fuse_chip_common_get_regs();
    volatile tegra_se_t *se = se_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();

    /* Bootrom part we skipped. */
    uint32_t sbk[4] = {fuse_chip->FUSE_PRIVATE_KEY[0], fuse_chip->FUSE_PRIVATE_KEY[1], fuse_chip->FUSE_PRIVATE_KEY[2], fuse_chip->FUSE_PRIVATE_KEY[3]};
    set_aes_keyslot(0xE, sbk, 0x10);

    /* Lock SBK from being read. */
    se->SE_CRYPTO_KEYTABLE_ACCESS[0xE] = 0x7E;

    /* This memset needs to happen here, else TZRAM will behave weirdly later on. */
    memset((void *)0x7C010000, 0, 0x10000);

    pmc->crypto_op = 0;
    se->SE_INT_STATUS = 0x1F;

    /* Lock SSK (although it's not set and unused anyways). */
    se->SE_CRYPTO_KEYTABLE_ACCESS[0xF] = 0x7E;

    /* Clear the boot reason to avoid problems later */
    pmc->scratch200 = 0;
    pmc->rst_status = 0;
}

void nx_hwinit(bool enable_log) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    volatile tegra_car_t *car = car_get_regs();
    bool is_mariko = is_soc_mariko();

    if (!is_mariko) {
        /* Bootrom stuff we skipped by going through RCM. */
        config_se_brom();

        AHB_AHB_SPARE_REG_0 &= 0xFFFFFF9F;
        pmc->scratch49 = (((pmc->scratch49 >> 1) << 1) & 0xFFFFFFFD);

        /* Apply the memory built-in self test workaround. */
        mbist_workaround();
    }

    /* Enable SE clock. */
    clkrst_reboot(CARDEVICE_SE);
    if (is_mariko) {
        /* Lock the SE clock. */
        car->clk_source_se |= 0x100;
    }

    /* Initialize the fuse driver. */
    fuse_init();

    if (!is_mariko) {
        /* Initialize the memory controller. */
        mc_enable();
    }

    /* Configure oscillators. */
    config_oscillators();

    /* Disable pinmux tristate input clamping. */
    APB_MISC_PP_PINMUX_GLOBAL_0 = 0;

    /* Configure GPIOs. */
    config_gpios();

    /* UART debugging. */
    if (enable_log) {
        clkrst_reboot(CARDEVICE_UARTA);
        uart_init(UART_A, 115200);
    }

   /* Enable CL-DVFS clock. */
    clkrst_reboot(CARDEVICE_CL_DVFS);

    /* Enable I2C1 clock. */
    clkrst_reboot(CARDEVICE_I2C1);

    /* Enable I2C5 clock. */
    clkrst_reboot(CARDEVICE_I2C5);

    /* Enable TZRAM clock. */
    clkrst_reboot(CARDEVICE_TZRAM);

    /* Initialize I2C5. */
    i2c_init(I2C_5);

    /* Configure the PMIC. */
    if (is_mariko) {
        uint8_t val = 0x40;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_CNFGBBC, &val, 1);
        val = 0x78;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, &val, 1);
    } else {
        uint8_t val = 0x40;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_CNFGBBC, &val, 1);
        val = 0x60;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_ONOFFCNFG1, &val, 1);
        val = 0x38;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_CFG0, &val, 1);
        val = 0x3A;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_CFG1, &val, 1);
        val = 0x38;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_CFG2, &val, 1);
        val = 0xF;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_LDO4, &val, 1);
        val = 0xC7;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_LDO8, &val, 1);
        val = 0x4F;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_SD0, &val, 1);
        val = 0x29;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_SD1, &val, 1);
        val = 0x1B;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_SD3, &val, 1);

        /* NOTE: [3.0.0+] This was added. */
        val = 0x22;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_FPS_GPIO3, &val, 1);

        /* TODO: In 3.x+, if the unit is SDEV, the MBLPD bit is set. */
        /*
        i2c_query(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_CNFGGLBL1, &val, 1);
        val |= 0x40;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_CNFGGLBL1, &val, 1);
        */
    }

    /* Configure SD0 voltage. */
    uint8_t val = 0x24;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD0, &val, 1);

    /* Enable LDO8 in HardwareType_Hoag only. */
    if (is_mariko && (fuse_get_hardware_type() == 2)) {
        val = 0xE8;
        i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_LDO8_CFG, &val, 1);
    }

    /* Initialize I2C1. */
    i2c_init(I2C_1);

    /* Set super clock burst policy. */
    car->sclk_brst_pol = ((car->sclk_brst_pol & 0xFFFF8888) | 0x3333);

    if (is_mariko) {
        /* Mariko only PMC configuration for TZRAM. */
        pmc->tzram_pwr_cntrl &= 0xFFFFFFFE;
        pmc->tzram_non_sec_disable = 0x3;
        pmc->tzram_sec_disable = 0x3;
    }

    /* Save SDRAM parameters to scratch. */
    sdram_save_params(sdram_get_params(fuse_get_dram_id()));

    /* Initialize SDRAM. */
    sdram_init();
}
