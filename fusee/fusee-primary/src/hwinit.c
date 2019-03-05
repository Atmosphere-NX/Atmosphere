/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
 * Copyright (c) 2018 Atmosphère-NX
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

void config_oscillators()
{
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

void config_gpios()
{
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    
    pinmux->uart2_tx = 0;
    pinmux->uart3_tx = 0;
    pinmux->pe6 = PINMUX_INPUT;
    pinmux->ph6 = PINMUX_INPUT;
    
    gpio_configure_mode(TEGRA_GPIO(G, 0), GPIO_MODE_GPIO);
    gpio_configure_mode(TEGRA_GPIO(D, 1), GPIO_MODE_GPIO);
    gpio_configure_mode(TEGRA_GPIO(E, 6), GPIO_MODE_GPIO);
    gpio_configure_mode(TEGRA_GPIO(H, 6), GPIO_MODE_GPIO);
    gpio_configure_direction(TEGRA_GPIO(G, 0), GPIO_DIRECTION_INPUT);
    gpio_configure_direction(TEGRA_GPIO(D, 1), GPIO_DIRECTION_INPUT);
    gpio_configure_direction(TEGRA_GPIO(E, 6), GPIO_DIRECTION_INPUT);
    gpio_configure_direction(TEGRA_GPIO(H, 6), GPIO_DIRECTION_INPUT);

    pinmux->gen1_i2c_scl = PINMUX_INPUT;
    pinmux->gen1_i2c_sda = PINMUX_INPUT;
    pinmux->pwr_i2c_scl = PINMUX_INPUT;
    pinmux->pwr_i2c_sda = PINMUX_INPUT;
    pinmux->uart1_rx = 0;
    pinmux->uart1_tx = (PINMUX_INPUT | PINMUX_PULL_UP);
    pinmux->uart1_rts = 0;
    pinmux->uart1_cts = (PINMUX_INPUT | PINMUX_PULL_DOWN);

    /* Configure volume up/down as inputs. */
    gpio_configure_mode(GPIO_BUTTON_VOL_UP, GPIO_MODE_GPIO);
    gpio_configure_mode(GPIO_BUTTON_VOL_DOWN, GPIO_MODE_GPIO);
    gpio_configure_direction(GPIO_BUTTON_VOL_UP, GPIO_DIRECTION_INPUT);
    gpio_configure_direction(GPIO_BUTTON_VOL_DOWN, GPIO_DIRECTION_INPUT);
}

void config_pmc_scratch()
{
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    
    pmc->scratch20 &= 0xFFF3FFFF;
    pmc->scratch190 &= 0xFFFFFFFE;
    pmc->secure_scratch21 |= 0x10;
}

void mbist_workaround()
{
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

void config_se_brom()
{
    volatile tegra_fuse_chip_t *fuse_chip = fuse_chip_get_regs();
    volatile tegra_se_t *se = se_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    
    /* Bootrom part we skipped. */
    uint32_t sbk[4] = {fuse_chip->FUSE_PRIVATE_KEY[0], fuse_chip->FUSE_PRIVATE_KEY[1], fuse_chip->FUSE_PRIVATE_KEY[2], fuse_chip->FUSE_PRIVATE_KEY[3]};
    set_aes_keyslot(0xE, sbk, 0x10);
    
    /* Lock SBK from being read. */
    se->AES_KEYSLOT_FLAGS[0xE] = 0x7E;
    
    /* This memset needs to happen here, else TZRAM will behave weirdly later on. */
    memset((void *)0x7C010000, 0, 0x10000);
    
    pmc->crypto_op = 0;
    se->INT_STATUS_REG = 0x1F;
    
    /* Lock SSK (although it's not set and unused anyways). */
    se->AES_KEYSLOT_FLAGS[0xF] = 0x7E;
    
    /* Clear the boot reason to avoid problems later */
    pmc->scratch200 = 0;
    pmc->reset_status = 0;
}

void nx_hwinit()
{
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    volatile tegra_car_t *car = car_get_regs();
    
    /* Bootrom stuff we skipped by going through RCM. */
    config_se_brom();
    
    AHB_AHB_SPARE_REG_0 &= 0xFFFFFF9F;
    pmc->scratch49 = (((pmc->scratch49 >> 1) << 1) & 0xFFFFFFFD);
    
    /* Apply the memory built-in self test workaround. */
    mbist_workaround();
    
    /* Reboot SE. */
    clkrst_reboot(CARDEVICE_SE);

    /* Initialize the fuse driver. */
    fuse_init();

    /* Initialize the memory controller. */
    mc_enable();

    /* Configure oscillators. */
    config_oscillators();
    
    /* Disable pinmux tristate input clamping. */
    APB_MISC_PP_PINMUX_GLOBAL_0 = 0;
    
    /* Configure GPIOs. */
    /* NOTE: [3.0.0+] Part of the GPIO configuration is skipped if the unit is SDEV. */
    /* NOTE: [6.0.0+] The GPIO configuration's order was changed a bit. */
    config_gpios();

    /* Uncomment for UART debugging. */
    /*
    clkrst_reboot(CARDEVICE_UARTC);
    uart_init(UART_C, 115200);
    */
    
    /* Reboot CL-DVFS. */
    clkrst_reboot(CARDEVICE_CL_DVFS);
    
    /* Reboot I2C1. */
    clkrst_reboot(CARDEVICE_I2C1);
    
    /* Reboot I2C5. */
    clkrst_reboot(CARDEVICE_I2C5);
    
    /* Reboot SE. */
    /* NOTE: [4.0.0+] This was removed. */
    /* clkrst_reboot(CARDEVICE_SE); */
    
    /* Reboot unknown device. */
    clkrst_reboot(CARDEVICE_UNK);

    /* Initialize I2C1. */
    /* NOTE: [6.0.0+] This was moved to after the PMIC is configured. */
    i2c_init(I2C_1);
    
    /* Initialize I2C5. */
    i2c_init(I2C_5);
    
    /* Configure the PMIC. */
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

    /* Configure SD0 voltage. */
    val = 42;       /* 42 = (1125000 - 600000) / 12500 -> 1.125V */
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD0, &val, 1);

    /* Configure and lock PMC scratch registers. */
    /* NOTE: [4.0.0+] This was removed. */
    config_pmc_scratch();

    /* Set super clock burst policy. */
    car->sclk_brst_pol = ((car->sclk_brst_pol & 0xFFFF8888) | 0x3333);

    /* Configure memory controller carveouts. */
    /* NOTE: [4.0.0+] This is now done in the Secure Monitor. */
    /* mc_config_carveout(); */

    /* Initialize SDRAM. */
    sdram_init();
    
    /* Save SDRAM LP0 parameters. */
    sdram_lp0_save_params(sdram_get_params());
}