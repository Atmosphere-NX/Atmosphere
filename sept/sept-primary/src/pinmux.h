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
 
#ifndef FUSEE_PINMUX_H
#define FUSEE_PINMUX_H

#define PINMUX_BASE 0x70003000
#define MAKE_PINMUX_REG(n) MAKE_REG32(PINMUX_BASE + n)

#define PINMUX_TRISTATE             (1 << 4)
#define PINMUX_PARKED               (1 << 5)
#define PINMUX_INPUT                (1 << 6)
#define PINMUX_PULL_NONE            (0 << 2)
#define PINMUX_PULL_DOWN            (1 << 2)
#define PINMUX_PULL_UP              (2 << 2)
#define PINMUX_SELECT_FUNCTION0     0
#define PINMUX_SELECT_FUNCTION1     1
#define PINMUX_SELECT_FUNCTION2     2
#define PINMUX_SELECT_FUNCTION3     3    
#define PINMUX_DRIVE_1X             (0 << 13)
#define PINMUX_DRIVE_2X             (1 << 13)
#define PINMUX_DRIVE_3X             (2 << 13)
#define PINMUX_DRIVE_4X             (3 << 13)

typedef struct {
    uint32_t sdmmc1_clk;
    uint32_t sdmmc1_cmd;
    uint32_t sdmmc1_dat3;
    uint32_t sdmmc1_dat2;
    uint32_t sdmmc1_dat1;
    uint32_t sdmmc1_dat0;
    uint32_t _r18;
    uint32_t sdmmc3_clk;
    uint32_t sdmmc3_cmd;
    uint32_t sdmmc3_dat0;
    uint32_t sdmmc3_dat1;
    uint32_t sdmmc3_dat2;
    uint32_t sdmmc3_dat3;
    uint32_t _r34;
    uint32_t pex_l0_rst_n;
    uint32_t pex_l0_clkreq_n;
    uint32_t pex_wake_n;
    uint32_t pex_l1_rst_n;
    uint32_t pex_l1_clkreq_n;
    uint32_t sata_led_active;
    uint32_t spi1_mosi;
    uint32_t spi1_miso;
    uint32_t spi1_sck;
    uint32_t spi1_cs0;
    uint32_t spi1_cs1;
    uint32_t spi2_mosi;
    uint32_t spi2_miso;
    uint32_t spi2_sck;
    uint32_t spi2_cs0;
    uint32_t spi2_cs1;
    uint32_t spi4_mosi;
    uint32_t spi4_miso;
    uint32_t spi4_sck;
    uint32_t spi4_cs0;
    uint32_t qspi_sck;
    uint32_t qspi_cs_n;
    uint32_t qspi_io0;
    uint32_t qspi_io1;
    uint32_t qspi_io2;
    uint32_t qspi_io3;
    uint32_t _ra0;
    uint32_t dmic1_clk;
    uint32_t dmic1_dat;
    uint32_t dmic2_clk;
    uint32_t dmic2_dat;
    uint32_t dmic3_clk;
    uint32_t dmic3_dat;
    uint32_t gen1_i2c_scl;
    uint32_t gen1_i2c_sda;
    uint32_t gen2_i2c_scl;
    uint32_t gen2_i2c_sda;
    uint32_t gen3_i2c_scl;
    uint32_t gen3_i2c_sda;
    uint32_t cam_i2c_scl;
    uint32_t cam_i2c_sda;
    uint32_t pwr_i2c_scl;
    uint32_t pwr_i2c_sda;
    uint32_t uart1_tx;
    uint32_t uart1_rx;
    uint32_t uart1_rts;
    uint32_t uart1_cts;
    uint32_t uart2_tx;
    uint32_t uart2_rx;
    uint32_t uart2_rts;
    uint32_t uart2_cts;
    uint32_t uart3_tx;
    uint32_t uart3_rx;
    uint32_t uart3_rts;
    uint32_t uart3_cts;
    uint32_t uart4_tx;
    uint32_t uart4_rx;
    uint32_t uart4_rts;
    uint32_t uart4_cts;
    uint32_t dap1_fs;
    uint32_t dap1_din;
    uint32_t dap1_dout;
    uint32_t dap1_sclk;
    uint32_t dap2_fs;
    uint32_t dap2_din;
    uint32_t dap2_dout;
    uint32_t dap2_sclk;
    uint32_t dap4_fs;
    uint32_t dap4_din;
    uint32_t dap4_dout;
    uint32_t dap4_sclk;
    uint32_t cam1_mclk;
    uint32_t cam2_mclk;
    uint32_t jtag_rtck;
    uint32_t clk_32k_in;
    uint32_t clk_32k_out;
    uint32_t batt_bcl;
    uint32_t clk_req;
    uint32_t cpu_pwr_req;
    uint32_t pwr_int_n;
    uint32_t shutdown;
    uint32_t core_pwr_req;
    uint32_t aud_mclk;
    uint32_t dvfs_pwm;
    uint32_t dvfs_clk;
    uint32_t gpio_x1_aud;
    uint32_t gpio_x3_aud;
    uint32_t pcc7;
    uint32_t hdmi_cec;
    uint32_t hdmi_int_dp_hpd;
    uint32_t spdif_out;
    uint32_t spdif_in;
    uint32_t usb_vbus_en0;
    uint32_t usb_vbus_en1;
    uint32_t dp_hpd0;
    uint32_t wifi_en;
    uint32_t wifi_rst;
    uint32_t wifi_wake_ap;
    uint32_t ap_wake_bt;
    uint32_t bt_rst;
    uint32_t bt_wake_ap;
    uint32_t ap_wake_nfc;
    uint32_t nfc_en;
    uint32_t nfc_int;
    uint32_t gps_en;
    uint32_t gps_rst;
    uint32_t cam_rst;
    uint32_t cam_af_en;
    uint32_t cam_flash_en;
    uint32_t cam1_pwdn;
    uint32_t cam2_pwdn;
    uint32_t cam1_strobe;
    uint32_t lcd_te;
    uint32_t lcd_bl_pwm;
    uint32_t lcd_bl_en;
    uint32_t lcd_rst;
    uint32_t lcd_gpio1;
    uint32_t lcd_gpio2;
    uint32_t ap_ready;
    uint32_t touch_rst;
    uint32_t touch_clk;
    uint32_t modem_wake_ap;
    uint32_t touch_int;
    uint32_t motion_int;
    uint32_t als_prox_int;
    uint32_t temp_alert;
    uint32_t button_power_on;
    uint32_t button_vol_up;
    uint32_t button_vol_down;
    uint32_t button_slide_sw;
    uint32_t button_home;
    uint32_t pa6;
    uint32_t pe6;
    uint32_t pe7;
    uint32_t ph6;
    uint32_t pk0;
    uint32_t pk1;
    uint32_t pk2;
    uint32_t pk3;
    uint32_t pk4;
    uint32_t pk5;
    uint32_t pk6;
    uint32_t pk7;
    uint32_t pl0;
    uint32_t pl1;
    uint32_t pz0;
    uint32_t pz1;
    uint32_t pz2;
    uint32_t pz3;
    uint32_t pz4;
    uint32_t pz5;
} tegra_pinmux_t;

static inline volatile tegra_pinmux_t *pinmux_get_regs(void)
{
    return (volatile tegra_pinmux_t *)PINMUX_BASE;
}

#endif
