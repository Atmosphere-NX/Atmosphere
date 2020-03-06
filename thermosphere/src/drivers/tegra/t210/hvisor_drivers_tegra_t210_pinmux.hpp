/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#pragma once

#include "../../../defines.hpp"

#define CONFIGURE_UART_DECL(n)\
void ConfigureUart##n() const\
{\
    m_regs->uart##n##_tx    = 0;\
    m_regs->uart##n##_rx    = INPUT | PULL_UP;\
    m_regs->uart##n##_rts   = 0;\
    m_regs->uart##n##_cts   = INPUT | PULL_DOWN;\
}

namespace ams::hvisor::drivers::tegra::t210 {

    class Pinmux final {
        private:
            struct Registers {
                u32 sdmmc1_clk;
                u32 sdmmc1_cmd;
                u32 sdmmc1_dat3;
                u32 sdmmc1_dat2;
                u32 sdmmc1_dat1;
                u32 sdmmc1_dat0;
                u32 _r18;
                u32 sdmmc3_clk;
                u32 sdmmc3_cmd;
                u32 sdmmc3_dat0;
                u32 sdmmc3_dat1;
                u32 sdmmc3_dat2;
                u32 sdmmc3_dat3;
                u32 _r34;
                u32 pex_l0_rst_n;
                u32 pex_l0_clkreq_n;
                u32 pex_wake_n;
                u32 pex_l1_rst_n;
                u32 pex_l1_clkreq_n;
                u32 sata_led_active;
                u32 spi1_mosi;
                u32 spi1_miso;
                u32 spi1_sck;
                u32 spi1_cs0;
                u32 spi1_cs1;
                u32 spi2_mosi;
                u32 spi2_miso;
                u32 spi2_sck;
                u32 spi2_cs0;
                u32 spi2_cs1;
                u32 spi4_mosi;
                u32 spi4_miso;
                u32 spi4_sck;
                u32 spi4_cs0;
                u32 qspi_sck;
                u32 qspi_cs_n;
                u32 qspi_io0;
                u32 qspi_io1;
                u32 qspi_io2;
                u32 qspi_io3;
                u32 _ra0;
                u32 dmic1_clk;
                u32 dmic1_dat;
                u32 dmic2_clk;
                u32 dmic2_dat;
                u32 dmic3_clk;
                u32 dmic3_dat;
                u32 gen1_i2c_scl;
                u32 gen1_i2c_sda;
                u32 gen2_i2c_scl;
                u32 gen2_i2c_sda;
                u32 gen3_i2c_scl;
                u32 gen3_i2c_sda;
                u32 cam_i2c_scl;
                u32 cam_i2c_sda;
                u32 pwr_i2c_scl;
                u32 pwr_i2c_sda;
                u32 uart1_tx;
                u32 uart1_rx;
                u32 uart1_rts;
                u32 uart1_cts;
                u32 uart2_tx;
                u32 uart2_rx;
                u32 uart2_rts;
                u32 uart2_cts;
                u32 uart3_tx;
                u32 uart3_rx;
                u32 uart3_rts;
                u32 uart3_cts;
                u32 uart4_tx;
                u32 uart4_rx;
                u32 uart4_rts;
                u32 uart4_cts;
                u32 dap1_fs;
                u32 dap1_din;
                u32 dap1_dout;
                u32 dap1_sclk;
                u32 dap2_fs;
                u32 dap2_din;
                u32 dap2_dout;
                u32 dap2_sclk;
                u32 dap4_fs;
                u32 dap4_din;
                u32 dap4_dout;
                u32 dap4_sclk;
                u32 cam1_mclk;
                u32 cam2_mclk;
                u32 jtag_rtck;
                u32 clk_32k_in;
                u32 clk_32k_out;
                u32 batt_bcl;
                u32 clk_req;
                u32 cpu_pwr_req;
                u32 pwr_int_n;
                u32 shutdown;
                u32 core_pwr_req;
                u32 aud_mclk;
                u32 dvfs_pwm;
                u32 dvfs_clk;
                u32 gpio_x1_aud;
                u32 gpio_x3_aud;
                u32 pcc7;
                u32 hdmi_cec;
                u32 hdmi_int_dp_hpd;
                u32 spdif_out;
                u32 spdif_in;
                u32 usb_vbus_en0;
                u32 usb_vbus_en1;
                u32 dp_hpd0;
                u32 wifi_en;
                u32 wifi_rst;
                u32 wifi_wake_ap;
                u32 ap_wake_bt;
                u32 bt_rst;
                u32 bt_wake_ap;
                u32 ap_wake_nfc;
                u32 nfc_en;
                u32 nfc_int;
                u32 gps_en;
                u32 gps_rst;
                u32 cam_rst;
                u32 cam_af_en;
                u32 cam_flash_en;
                u32 cam1_pwdn;
                u32 cam2_pwdn;
                u32 cam1_strobe;
                u32 lcd_te;
                u32 lcd_bl_pwm;
                u32 lcd_bl_en;
                u32 lcd_rst;
                u32 lcd_gpio1;
                u32 lcd_gpio2;
                u32 ap_ready;
                u32 touch_rst;
                u32 touch_clk;
                u32 modem_wake_ap;
                u32 touch_int;
                u32 motion_int;
                u32 als_prox_int;
                u32 temp_alert;
                u32 button_power_on;
                u32 button_vol_up;
                u32 button_vol_down;
                u32 button_slide_sw;
                u32 button_home;
                u32 pa6;
                u32 pe6;
                u32 pe7;
                u32 ph6;
                u32 pk0;
                u32 pk1;
                u32 pk2;
                u32 pk3;
                u32 pk4;
                u32 pk5;
                u32 pk6;
                u32 pk7;
                u32 pl0;
                u32 pl1;
                u32 pz0;
                u32 pz1;
                u32 pz2;
                u32 pz3;
                u32 pz4;
                u32 pz5;
            };
            static_assert(std::is_standard_layout_v<Registers>);
            static_assert(std::is_trivial_v<Registers>);

            enum Flags : u32 {
                PREEMP_ENABLED          = BIT(15),

                DRIVE_1X                = 0 << 13,
                DRIVE_2X                = 1 << 13,
                DRIVE_3X                = 2 << 13,
                DRIVE_4X                = 3 << 13,

                SCHMT_ENABLED           = BIT(12),
                OD_ENABLED              = BIT(11),
                IO_HV_ENABLED           = BIT(10),
                HSM_ENABLED             = BIT(9),
                LPDR_ENABLED            = BIT(8),
                LOCKED                  = BIT(7),
                INPUT                   = BIT(6),
                PARKED                  = BIT(5),
                TRISTATE                = BIT(4),

                PULL_NONE               = 0 << 2,
                PULL_DOWN               = 1 << 2,
                PULL_UP                 = 2 << 2,

                SELECT_FUNCTION0        = 0 << 0,
                SELECT_FUNCTION1        = 1 << 0,
                SELECT_FUNCTION2        = 2 << 0,
                SELECT_FUNCTION3        = 3 << 0,
            };

        private:
            // TODO friend
            volatile Registers *m_regs = nullptr;

        public:
            CONFIGURE_UART_DECL(1)
            CONFIGURE_UART_DECL(2)
            CONFIGURE_UART_DECL(3)
            CONFIGURE_UART_DECL(4)

            void ConfigureUart(u32 id)
            {
                switch (id) {
                    case 0: ConfigureUart1(); break;
                    case 1: ConfigureUart2(); break;
                    case 2: ConfigureUart3(); break;
                    case 3: ConfigureUart4(); break;
                    default: break;
                }
            }
    };
}

#undef CONFIGURE_UART_DECL