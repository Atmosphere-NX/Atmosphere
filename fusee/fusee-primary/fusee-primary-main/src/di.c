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

#include <string.h>

#include "di.h"
#include "timers.h"
#include "i2c.h"
#include "pmc.h"
#include "max77620.h"
#include "gpio.h"
#include "pinmux.h"
#include "car.h"
#include "apb_misc.h"

#include "di.inl"

static uint32_t g_lcd_vendor = 0;

static void do_dsi_sleep_or_register_writes(const dsi_sleep_or_register_write_t *writes, uint32_t num_writes) {
    for (uint32_t i = 0; i < num_writes; i++) {
        if (writes[i].kind == 1) {
            udelay(1000 * writes[i].offset);
        } else {
            *(volatile uint32_t *)(DSI_BASE + sizeof(uint32_t) * writes[i].offset) = writes[i].value;
        }
    }
}

static void do_register_writes(uint32_t base_address, const register_write_t *writes, uint32_t num_writes) {
    for (uint32_t i = 0; i < num_writes; i++) {
        *(volatile uint32_t *)(base_address + writes[i].offset) = writes[i].value;
    }
}

static void dsi_wait(uint32_t timeout, uint32_t offset, uint32_t mask, uint32_t delay) {
    uint32_t end = get_time_us() + timeout;
    while ((get_time_us() < end) && (MAKE_DSI_REG(offset) & mask)) {
        /* Wait. */
    }
    udelay(delay);
}

void display_init_erista(void) {
    volatile tegra_car_t *car = car_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    
    /* Power on. */
    uint8_t val = 0xD0;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_LDO0_CFG, &val, 1);
    
    /* Enable MIPI CAL, DSI, DISP1, HOST1X, UART_FST_MIPI_CAL, DSIA LP clocks. */
    car->rst_dev_h_clr = 0x1010000;
    car->clk_enb_h_set = 0x1010000;
    car->rst_dev_l_clr = 0x18000000;
    car->clk_enb_l_set = 0x18000000;
    car->clk_enb_x_set = 0x20000;
    car->clk_source_uart_fst_mipi_cal = 0xA;
    car->clk_enb_w_set = 0x80000;
    car->clk_source_dsia_lp = 0xA;

    /* DPD idle. */
    pmc->io_dpd_req = 0x40000000;
    pmc->io_dpd2_req = 0x40000000;

    /* Configure pins. */
    pinmux->nfc_en &= ~PINMUX_TRISTATE;
    pinmux->nfc_int &= ~PINMUX_TRISTATE;
    pinmux->lcd_bl_pwm &= ~PINMUX_TRISTATE;
    pinmux->lcd_bl_en &= ~PINMUX_TRISTATE;
    pinmux->lcd_rst &= ~PINMUX_TRISTATE;
    
    /* Configure Backlight +-5V GPIOs. */
    gpio_configure_mode(GPIO_LCD_BL_P5V, GPIO_MODE_GPIO);
    gpio_configure_mode(GPIO_LCD_BL_N5V, GPIO_MODE_GPIO);
    gpio_configure_direction(GPIO_LCD_BL_P5V, GPIO_DIRECTION_OUTPUT);
    gpio_configure_direction(GPIO_LCD_BL_N5V, GPIO_DIRECTION_OUTPUT);
        
    /* Enable Backlight +5V. */
    gpio_write(GPIO_LCD_BL_P5V, GPIO_LEVEL_HIGH); 

    udelay(10000);

    /* Enable Backlight -5V. */
    gpio_write(GPIO_LCD_BL_N5V, GPIO_LEVEL_HIGH); 

    udelay(10000);

    /* Configure Backlight PWM, EN and RST GPIOs. */
    gpio_configure_mode(GPIO_LCD_BL_PWM, GPIO_MODE_GPIO);
    gpio_configure_mode(GPIO_LCD_BL_EN, GPIO_MODE_GPIO);
    gpio_configure_mode(GPIO_LCD_BL_RST, GPIO_MODE_GPIO);    
    gpio_configure_direction(GPIO_LCD_BL_PWM, GPIO_DIRECTION_OUTPUT);
    gpio_configure_direction(GPIO_LCD_BL_EN, GPIO_DIRECTION_OUTPUT);
    gpio_configure_direction(GPIO_LCD_BL_RST, GPIO_DIRECTION_OUTPUT);
    
    /* Enable Backlight EN. */
    gpio_write(GPIO_LCD_BL_EN, GPIO_LEVEL_HIGH);

    /* Configure display interface and display. */
    MAKE_MIPI_CAL_REG(MIPI_CAL_MIPI_BIAS_PAD_CFG2) = 0;

    do_register_writes(CAR_BASE, display_config_plld_01_erista, 4);
    do_register_writes(DI_BASE, display_config_dc_01, 94);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_01, 8);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_02_erista, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_03, 13);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_04_erista, 0);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_05, 11);
    do_register_writes(DSI_BASE, display_config_dsi_phy_timing_erista, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_06, 12);
    do_register_writes(DSI_BASE, display_config_dsi_phy_timing_erista, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_07, 14);

    udelay(10000);
    
    /* Enable Backlight RST. */
    gpio_write(GPIO_LCD_BL_RST, GPIO_LEVEL_HIGH);

    udelay(60000);

    MAKE_DSI_REG(DSI_BTA_TIMING) = 0x50204;
    MAKE_DSI_REG(DSI_WR_DATA) = 0x337;
    MAKE_DSI_REG(DSI_TRIGGER) = DSI_TRIGGER_HOST;
    dsi_wait(250000, DSI_TRIGGER, (DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO), 5);

    MAKE_DSI_REG(DSI_WR_DATA) = 0x406;
    MAKE_DSI_REG(DSI_TRIGGER) = DSI_TRIGGER_HOST;
    dsi_wait(250000, DSI_TRIGGER, (DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO), 5);

    MAKE_DSI_REG(DSI_HOST_CONTROL) = (DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_IMM_BTA | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC);
    dsi_wait(150000, DSI_HOST_CONTROL, DSI_HOST_CONTROL_IMM_BTA, 5000);

    /* Parse LCD vendor. */
    uint32_t host_response[3];
    for (uint32_t i = 0; i < 3; i++) {
        host_response[i] = MAKE_DSI_REG(DSI_RD_DATA);
    }

    /* The last word from host response is:
        Bits 0-7: FAB
        Bits 8-15: REV
        Bits 16-23: Minor REV
    */
    if ((host_response[2] & 0xFF) == 0x10) {
        g_lcd_vendor = 0;
    } else {
        g_lcd_vendor = (host_response[2] >> 8) & 0xFF00;
    }
    g_lcd_vendor = (g_lcd_vendor & 0xFFFFFF00) | (host_response[2] & 0xFF);
    
    /* LCD vendor specific configuration. */
    switch (g_lcd_vendor) {
        case 0x10: /* Japan Display Inc screens. */
            do_dsi_sleep_or_register_writes(display_config_jdi_specific_init_01, 48);
            break;
        case 0xF20: /* Innolux first revision screens. */
            do_dsi_sleep_or_register_writes(display_config_innolux_rev1_specific_init_01, 14);
            break;
        case 0xF30: /* AUO first revision screens. */
            do_dsi_sleep_or_register_writes(display_config_auo_rev1_specific_init_01, 14);
            break;
        default:
            /* Innolux and AUO second revision screens. */
            if ((g_lcd_vendor | 0x10) == 0x1030) {
                do_dsi_sleep_or_register_writes(display_config_innolux_auo_rev2_specific_init_01, 5);
            }
            break;
    }
    
    udelay(20000);
    
    do_register_writes(CAR_BASE, display_config_plld_02_erista, 3);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_08, 1);
    do_register_writes(DSI_BASE, display_config_dsi_phy_timing_erista, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_09, 19);
    MAKE_DI_REG(DC_DISP_DISP_CLOCK_CONTROL) = 4;
    do_register_writes(DSI_BASE, display_config_dsi_01_init_10, 10);

    udelay(10000);

    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_01, 4);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_02_erista, 2);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_11_erista, 4);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_03_erista, 6);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_04, 10);
    
    udelay(10000);

    do_register_writes(DI_BASE, display_config_dc_02, 113);
}

void display_init_mariko(void) {
    volatile tegra_car_t *car = car_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    
    /* Power on. */
    uint8_t val = 0x3A;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD2, &val, 1);
    val = 0x71;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD2_CFG, &val, 1);
    val = 0xD0;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_LDO0_CFG, &val, 1);
    
    /* Enable MIPI CAL, DSI, DISP1, HOST1X, UART_FST_MIPI_CAL, DSIA LP clocks. */
    car->rst_dev_h_clr = 0x1010000;
    car->clk_enb_h_set = 0x1010000;
    car->rst_dev_l_clr = 0x18000000;
    car->clk_enb_l_set = 0x18000000;
    car->clk_enb_x_set = 0x20000;
    car->clk_source_uart_fst_mipi_cal = 0xA;
    car->clk_enb_w_set = 0x80000;
    car->clk_source_dsia_lp = 0xA;

    /* DPD idle. */
    pmc->io_dpd_req = 0x40000000;
    pmc->io_dpd2_req = 0x40000000;

    /* Configure pins. */
    pinmux->nfc_en &= ~PINMUX_TRISTATE;
    pinmux->nfc_int &= ~PINMUX_TRISTATE;
    pinmux->lcd_bl_pwm &= ~PINMUX_TRISTATE;
    pinmux->lcd_bl_en &= ~PINMUX_TRISTATE;
    pinmux->lcd_rst &= ~PINMUX_TRISTATE;
    
    /* Configure Backlight +-5V GPIOs. */
    gpio_configure_mode(GPIO_LCD_BL_P5V, GPIO_MODE_GPIO);
    gpio_configure_mode(GPIO_LCD_BL_N5V, GPIO_MODE_GPIO);
    gpio_configure_direction(GPIO_LCD_BL_P5V, GPIO_DIRECTION_OUTPUT);
    gpio_configure_direction(GPIO_LCD_BL_N5V, GPIO_DIRECTION_OUTPUT);
        
    /* Enable Backlight +5V. */
    gpio_write(GPIO_LCD_BL_P5V, GPIO_LEVEL_HIGH); 

    udelay(10000);

    /* Enable Backlight -5V. */
    gpio_write(GPIO_LCD_BL_N5V, GPIO_LEVEL_HIGH); 

    udelay(10000);

    /* Configure Backlight PWM, EN and RST GPIOs. */
    gpio_configure_mode(GPIO_LCD_BL_PWM, GPIO_MODE_GPIO);
    gpio_configure_mode(GPIO_LCD_BL_EN, GPIO_MODE_GPIO);
    gpio_configure_mode(GPIO_LCD_BL_RST, GPIO_MODE_GPIO);    
    gpio_configure_direction(GPIO_LCD_BL_PWM, GPIO_DIRECTION_OUTPUT);
    gpio_configure_direction(GPIO_LCD_BL_EN, GPIO_DIRECTION_OUTPUT);
    gpio_configure_direction(GPIO_LCD_BL_RST, GPIO_DIRECTION_OUTPUT);
    
    /* Enable Backlight EN. */
    gpio_write(GPIO_LCD_BL_EN, GPIO_LEVEL_HIGH);

    /* Configure display interface and display. */
    MAKE_MIPI_CAL_REG(MIPI_CAL_MIPI_BIAS_PAD_CFG2) = 0;
    MAKE_MIPI_CAL_REG(MIPI_CAL_MIPI_BIAS_PAD_CFG0) = 0;
    MAKE_APB_MISC_REG(0xAC0) = 0;

    do_register_writes(CAR_BASE, display_config_plld_01_mariko, 4);
    do_register_writes(DI_BASE, display_config_dc_01, 94);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_01, 8);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_02_mariko, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_03, 13);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_04_mariko, 7);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_05, 11);
    do_register_writes(DSI_BASE, display_config_dsi_phy_timing_mariko, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_06, 12);
    do_register_writes(DSI_BASE, display_config_dsi_phy_timing_mariko, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_07, 14);

    udelay(10000);
    
    /* Enable Backlight RST. */
    gpio_write(GPIO_LCD_BL_RST, GPIO_LEVEL_HIGH);

    udelay(60000);

    MAKE_DSI_REG(DSI_BTA_TIMING) = 0x50204;
    MAKE_DSI_REG(DSI_WR_DATA) = 0x337;
    MAKE_DSI_REG(DSI_TRIGGER) = DSI_TRIGGER_HOST;
    dsi_wait(250000, DSI_TRIGGER, (DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO), 5);

    MAKE_DSI_REG(DSI_WR_DATA) = 0x406;
    MAKE_DSI_REG(DSI_TRIGGER) = DSI_TRIGGER_HOST;
    dsi_wait(250000, DSI_TRIGGER, (DSI_TRIGGER_HOST | DSI_TRIGGER_VIDEO), 5);

    MAKE_DSI_REG(DSI_HOST_CONTROL) = (DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_IMM_BTA | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC);
    dsi_wait(150000, DSI_HOST_CONTROL, DSI_HOST_CONTROL_IMM_BTA, 5000);

    /* Parse LCD vendor. */
    uint32_t host_response[3];
    for (uint32_t i = 0; i < 3; i++) {
        host_response[i] = MAKE_DSI_REG(DSI_RD_DATA);
    }

    /* The last word from host response is:
        Bits 0-7: FAB
        Bits 8-15: REV
        Bits 16-23: Minor REV
    */
    if ((host_response[2] & 0xFF) == 0x10) {
        g_lcd_vendor = 0;
    } else {
        g_lcd_vendor = (host_response[2] >> 8) & 0xFF00;
    }
    g_lcd_vendor = (g_lcd_vendor & 0xFFFFFF00) | (host_response[2] & 0xFF);
    
    /* LCD vendor specific configuration. */
    switch (g_lcd_vendor) {
        case 0x10: /* Japan Display Inc screens. */
            do_dsi_sleep_or_register_writes(display_config_jdi_specific_init_01, 48);
            break;
        case 0xF20: /* Innolux first revision screens. */
            do_dsi_sleep_or_register_writes(display_config_innolux_rev1_specific_init_01, 14);
            break;
        case 0xF30: /* AUO first revision screens. */
            do_dsi_sleep_or_register_writes(display_config_auo_rev1_specific_init_01, 14);
            break;
        default:
            /* Innolux and AUO second revision screens. */
            if ((g_lcd_vendor | 0x10) == 0x1030) {
                do_dsi_sleep_or_register_writes(display_config_innolux_auo_rev2_specific_init_01, 5);
            }
            break;
    }
    
    udelay(20000);
    
    do_register_writes(CAR_BASE, display_config_plld_02_mariko, 3);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_08, 1);
    do_register_writes(DSI_BASE, display_config_dsi_phy_timing_mariko, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_09, 19);
    MAKE_DI_REG(DC_DISP_DISP_CLOCK_CONTROL) = 4;
    do_register_writes(DSI_BASE, display_config_dsi_01_init_10, 10);

    udelay(10000);

    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_01, 4);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_02_mariko, 2);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_11_mariko, 7);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_03_mariko, 6);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_04, 10);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_02_mariko, 2);
    do_register_writes(DSI_BASE, display_config_dsi_01_init_11_mariko, 7);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_03_mariko, 6);
    do_register_writes(MIPI_CAL_BASE, display_config_mipi_cal_04, 10);

    udelay(10000);

    do_register_writes(DI_BASE, display_config_dc_02, 113);
}

void display_end_erista(void) {
    volatile tegra_car_t *car = car_get_regs();
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    
    /* Disable Backlight. */
    display_backlight(false);
    
    MAKE_DSI_REG(DSI_VIDEO_MODE_CONTROL) = 1;
    MAKE_DSI_REG(DSI_WR_DATA) = 0x2805;
    
    /* Wait 5 frames. */
    uint32_t start_val = MAKE_HOST1X_REG(0x30A4);
    while (MAKE_HOST1X_REG(0x30A4) < start_val + 5) {
        /* Wait. */
    }

    MAKE_DI_REG(DC_CMD_STATE_ACCESS) = (READ_MUX | WRITE_MUX);
    MAKE_DSI_REG(DSI_VIDEO_MODE_CONTROL) = 0;
    
    do_register_writes(CAR_BASE, display_config_plld_01_erista, 4);
    do_register_writes(DSI_BASE, display_config_dsi_01_fini_01, 2);
    do_register_writes(DSI_BASE, display_config_dsi_phy_timing_erista, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_fini_02, 13);

    udelay(10000);
    
    /* LCD vendor specific shutdown. */
    switch (g_lcd_vendor) {
        case 0x10: /* Japan Display Inc screens. */
            do_dsi_sleep_or_register_writes(display_config_jdi_specific_fini_01, 22);
            break;
        case 0xF30: /* AUO first revision screens. */
            do_dsi_sleep_or_register_writes(display_config_auo_rev1_specific_fini_01, 38);
            break;
        case 0x1020: /* Innolux second revision screens. */
            do_dsi_sleep_or_register_writes(display_config_innolux_rev2_specific_fini_01, 10);
            break;
        case 0x1030: /* AUO second revision screens. */
            do_dsi_sleep_or_register_writes(display_config_auo_rev2_specific_fini_01, 10);
            break;
        default:
            break;
    }
    
    udelay(5000);

    MAKE_DSI_REG(DSI_WR_DATA) = 0x1005;
    MAKE_DSI_REG(DSI_TRIGGER) = DSI_TRIGGER_HOST;

    udelay(50000);
    
    /* Disable Backlight RST. */
    gpio_write(GPIO_LCD_BL_RST, GPIO_LEVEL_LOW); 

    udelay(10000);
    
    /* Disable Backlight -5V. */
    gpio_write(GPIO_LCD_BL_N5V, GPIO_LEVEL_LOW); 

    udelay(10000);

    /* Disable Backlight +5V. */
    gpio_write(GPIO_LCD_BL_P5V, GPIO_LEVEL_LOW); 

    udelay(10000);

    /* Disable clocks. */
    car->rst_dev_h_set = 0x1010000;
    car->clk_enb_h_clr = 0x1010000;
    car->rst_dev_l_set = 0x18000000;
    car->clk_enb_l_clr = 0x18000000;

    MAKE_DSI_REG(DSI_PAD_CONTROL_0) = (DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) | DSI_PAD_CONTROL_VS1_PDIO_CLK | DSI_PAD_CONTROL_VS1_PDIO(0xF));
    MAKE_DSI_REG(DSI_POWER_CONTROL) = 0;
    
    /* Backlight PWM. */
    gpio_configure_mode(GPIO_LCD_BL_PWM, GPIO_MODE_SFIO);
    
    pinmux->lcd_bl_pwm = ((pinmux->lcd_bl_pwm & ~PINMUX_TRISTATE) | PINMUX_TRISTATE);
    pinmux->lcd_bl_pwm = (((pinmux->lcd_bl_pwm >> 2) << 2) | 1);
}

void display_end_mariko(void) {
    volatile tegra_car_t *car = car_get_regs();
    volatile tegra_pinmux_t *pinmux = pinmux_get_regs();
    
    /* Disable Backlight. */
    display_backlight(false);
    
    MAKE_DSI_REG(DSI_VIDEO_MODE_CONTROL) = 1;
    MAKE_DSI_REG(DSI_WR_DATA) = 0x2805;
    
    /* Wait 5 frames. */
    uint32_t start_val = MAKE_HOST1X_REG(0x30A4);
    while (MAKE_HOST1X_REG(0x30A4) < start_val + 5) {
        /* Wait. */
    }

    MAKE_DI_REG(DC_CMD_STATE_ACCESS) = (READ_MUX | WRITE_MUX);
    MAKE_DSI_REG(DSI_VIDEO_MODE_CONTROL) = 0;
    
    do_register_writes(CAR_BASE, display_config_plld_01_mariko, 4);
    do_register_writes(DSI_BASE, display_config_dsi_01_fini_01, 2);
    do_register_writes(DSI_BASE, display_config_dsi_phy_timing_mariko, 1);
    do_register_writes(DSI_BASE, display_config_dsi_01_fini_02, 13);

    udelay(10000);
    
    /* LCD vendor specific shutdown. */
    switch (g_lcd_vendor) {
        case 0x10: /* Japan Display Inc screens. */
            do_dsi_sleep_or_register_writes(display_config_jdi_specific_fini_01, 22);
            break;
        case 0xF30: /* AUO first revision screens. */
            do_dsi_sleep_or_register_writes(display_config_auo_rev1_specific_fini_01, 38);
            break;
        case 0x1020: /* Innolux second revision screens. */
            do_dsi_sleep_or_register_writes(display_config_innolux_rev2_specific_fini_01, 10);
            break;
        case 0x1030: /* AUO second revision screens. */
            do_dsi_sleep_or_register_writes(display_config_auo_rev2_specific_fini_01, 10);
            break;
        default:
            break;
    }
    
    udelay(5000);

    MAKE_DSI_REG(DSI_WR_DATA) = 0x1005;
    MAKE_DSI_REG(DSI_TRIGGER) = DSI_TRIGGER_HOST;

    udelay(50000);
    
    /* Disable Backlight RST. */
    gpio_write(GPIO_LCD_BL_RST, GPIO_LEVEL_LOW); 

    udelay(10000);
    
    /* Disable Backlight -5V. */
    gpio_write(GPIO_LCD_BL_N5V, GPIO_LEVEL_LOW); 

    udelay(10000);

    /* Disable Backlight +5V. */
    gpio_write(GPIO_LCD_BL_P5V, GPIO_LEVEL_LOW); 

    udelay(10000);

    /* Disable clocks. */
    car->rst_dev_h_set = 0x1010000;
    car->clk_enb_h_clr = 0x1010000;
    car->rst_dev_l_set = 0x18000000;
    car->clk_enb_l_clr = 0x18000000;

    MAKE_DSI_REG(DSI_PAD_CONTROL_0) = (DSI_PAD_CONTROL_VS1_PULLDN_CLK | DSI_PAD_CONTROL_VS1_PULLDN(0xF) | DSI_PAD_CONTROL_VS1_PDIO_CLK | DSI_PAD_CONTROL_VS1_PDIO(0xF));
    MAKE_DSI_REG(DSI_POWER_CONTROL) = 0;
    
    /* Backlight PWM. */
    gpio_configure_mode(GPIO_LCD_BL_PWM, GPIO_MODE_SFIO);
    
    pinmux->lcd_bl_pwm = ((pinmux->lcd_bl_pwm & ~PINMUX_TRISTATE) | PINMUX_TRISTATE);
    pinmux->lcd_bl_pwm = (((pinmux->lcd_bl_pwm >> 2) << 2) | 1);
}

void display_backlight(bool enable) {
    /* Enable Backlight PWM. */
    gpio_write(GPIO_LCD_BL_PWM, enable ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
}

void display_color_screen(uint32_t color) {
    do_register_writes(DI_BASE, display_config_solid_color, 8);

    /* Configure display to show single color. */
    MAKE_DI_REG(DC_WIN_AD_WIN_OPTIONS) = 0;
    MAKE_DI_REG(DC_WIN_BD_WIN_OPTIONS) = 0;
    MAKE_DI_REG(DC_WIN_CD_WIN_OPTIONS) = 0;
    MAKE_DI_REG(DC_DISP_BLEND_BACKGROUND_COLOR) = color;
    MAKE_DI_REG(DC_CMD_STATE_CONTROL) = ((MAKE_DI_REG(DC_CMD_STATE_CONTROL) & 0xFFFFFFFE) | GENERAL_ACT_REQ);

    udelay(35000);

    display_backlight(true);
}

uint32_t *display_init_framebuffer(void *address) {
    static register_write_t conf[sizeof(display_config_frame_buffer)/sizeof(register_write_t)] = {0};
    if (conf[0].value == 0) {
        for (uint32_t i = 0; i < sizeof(display_config_frame_buffer)/sizeof(register_write_t); i++) {
            conf[i] = display_config_frame_buffer[i];
        }
    }

    uint32_t *lfb_addr = (uint32_t *)address;
    conf[19].value = (uint32_t)address;
    
    /* This configures the framebuffer @ address with a resolution of 1280x720 (line stride 768). */
    do_register_writes(DI_BASE, conf, 32);

    udelay(35000);

    return lfb_addr;
}
