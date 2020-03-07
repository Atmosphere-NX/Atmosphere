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

namespace ams::hvisor::drivers::tegra::t210 {

    class Car final {
        private:
            struct Registers {
                u32 rst_src;                                // _RST_SOURCE_0,                           0x000

                // _RST_DEVICES_L/H/U_0 0x4-0xc
                u32 rst_dev_l;
                u32 rst_dev_h;
                u32 rst_dev_u;

                // _CLK_OUT_ENB_L/H/U_0 0x10-0x18
                u32 clk_out_enb_l;
                u32 clk_out_enb_h;
                u32 clk_out_enb_u;

                u32 _0x1C;
                u32 cclk_brst_pol;                          // _CCLK_BURST_POLICY_0,                    0x020
                u32 super_cclk_div;                         // _SUPER_CCLK_DIVIDER_0,                   0x024
                u32 sclk_brst_pol;                          // _SCLK_BURST_POLICY_0,                    0x028
                u32 super_sclk_div;                         // _SUPER_SCLK_DIVIDER_0,                   0x02c
                u32 clk_sys_rate;                           // _CLK_SYSTEM_RATE_0,                      0x030
                u32 prog_dly_clk;                           // _PROG_DLY_CLK_0,                         0x034
                u32 aud_sync_clk_rate;                      // _AUDIO_SYNC_CLK_RATE_0,                  0x038
                u32 _0x3C;
                u32 cop_clk_skip_plcy;                      // _COP_CLK_SKIP_POLICY_0,                  0x040
                u32 clk_mask_arm;                           // _CLK_MASK_ARM_0,                         0x044
                u32 misc_clk_enb;                           // _MISC_CLK_ENB_0,                         0x048
                u32 clk_cpu_cmplx;                          // _CLK_CPU_CMPLX_0,                        0x04c
                u32 osc_ctrl;                               // _OSC_CTRL_0,                             0x050
                u32 pll_lfsr;                               // _PLL_LFSR_0,                             0x054
                u32 osc_freq_det;                           // _OSC_FREQ_DET_0,                         0x058
                u32 osc_freq_det_stat;                      // _OSC_FREQ_DET_STATUS_0,                  0x05c
                u32 _0x60[2];
                u32 plle_ss_cntl;                           // _PLLE_SS_CNTL_0,                         0x068
                u32 plle_misc1;                             // _PLLE_MISC1_0,                           0x06c
                u32 _0x70[4];

                // PLLC 0x80-0x8c
                u32 pllc_base;
                u32 pllc_out;
                u32 pllc_misc0;
                u32 pllc_misc1;

                // PLLM 0x90-0x9c
                u32 pllm_base;
                u32 pllm_out;
                u32 pllm_misc1;
                u32 pllm_misc2;

                // PLLP 0xa0-0xac
                u32 pllp_base;
                u32 pllp_outa;
                u32 pllp_outb;
                u32 pllp_misc;

                // PLLA 0xb0-0xbc
                u32 plla_base;
                u32 plla_out;
                u32 plla_misc0;
                u32 plla_misc1;

                // PLLU 0xc0-0xcc
                u32 pllu_base;
                u32 pllu_out;
                u32 pllu_misc1;
                u32 pllu_misc2;

                // PLLD 0xd0-0xdc
                u32 plld_base;
                u32 plld_out;
                u32 plld_misc1;
                u32 plld_misc2;

                // PLLX 0xe0-0xe4
                u32 pllx_base;
                u32 pllx_misc;

                // PLLE 0xe8-0xf4
                u32 plle_base;
                u32 plle_misc;
                u32 plle_ss_cntl1;
                u32 plle_ss_cntl2;

                u32 lvl2_clk_gate_ovra;                     // _LVL2_CLK_GATE_OVRA_0,                   0x0f8
                u32 lvl2_clk_gate_ovrb;                     // _LVL2_CLK_GATE_OVRB_0,                   0x0fc

                u32 clk_source_i2s2;                        // _CLK_SOURCE_I2S2_0,                      0x100
                u32 clk_source_i2s3;                        // _CLK_SOURCE_I2S3_0,                      0x104
                u32 clk_source_spdif_out;                   // _CLK_SOURCE_SPDIF_OUT_0,                 0x108
                u32 clk_source_spdif_in;                    // _CLK_SOURCE_SPDIF_IN_0,                  0x10c
                u32 clk_source_pwm;                         // _CLK_SOURCE_PWM_0,                       0x110
                u32 _0x114;
                u32 clk_source_spi2;                        // _CLK_SOURCE_SPI2_0,                      0x118
                u32 clk_source_spi3;                        // _CLK_SOURCE_SPI3_0,                      0x11c
                u32 _0x120;
                u32 clk_source_i2c1;                        // _CLK_SOURCE_I2C1_0,                      0x124
                u32 clk_source_i2c5;                        // _CLK_SOURCE_I2C5_0,                      0x128
                u32 _0x12c[2];
                u32 clk_source_spi1;                        // _CLK_SOURCE_SPI1_0,                      0x134
                u32 clk_source_disp1;                       // _CLK_SOURCE_DISP1_0,                     0x138
                u32 clk_source_disp2;                       // _CLK_SOURCE_DISP2_0,                     0x13c
                u32 _0x140;
                u32 clk_source_isp;                         // _CLK_SOURCE_ISP_0,                       0x144
                u32 clk_source_vi;                          // _CLK_SOURCE_VI_0,                        0x148
                u32 _0x14c;
                u32 clk_source_sdmmc1;                      // _CLK_SOURCE_SDMMC1_0,                    0x150
                u32 clk_source_sdmmc2;                      // _CLK_SOURCE_SDMMC2_0,                    0x154
                u32 _0x158[3];
                u32 clk_source_sdmmc4;                      // _CLK_SOURCE_SDMMC4_0,                    0x164
                u32 _0x168[4];
                u32 clk_source_uarta;                       // _CLK_SOURCE_UARTA_0,                     0x178
                u32 clk_source_uartb;                       // _CLK_SOURCE_UARTB_0,                     0x17c
                u32 clk_source_host1x;                      // _CLK_SOURCE_HOST1X_0,                    0x180
                u32 _0x184[5];
                u32 clk_source_i2c2;                        // _CLK_SOURCE_I2C2_0,                      0x198
                u32 clk_source_emc;                         // _CLK_SOURCE_EMC_0,                       0x19c
                u32 clk_source_uartc;                       // _CLK_SOURCE_UARTC_0,                     0x1a0
                u32 _0x1a4;
                u32 clk_source_vi_sensor;                   // _CLK_SOURCE_VI_SENSOR_0,                 0x1a8
                u32 _0x1ac[2];
                u32 clk_source_spi4;                        // _CLK_SOURCE_SPI4_0,                      0x1b4
                u32 clk_source_i2c3;                        // _CLK_SOURCE_I2C3_0,                      0x1b8
                u32 clk_source_sdmmc3;                      // _CLK_SOURCE_SDMMC3_0,                    0x1bc
                u32 clk_source_uartd;                       // _CLK_SOURCE_UARTD_0,                     0x1c0
                u32 _0x1c4[2];
                u32 clk_source_owr;                         // _CLK_SOURCE_OWR_0,                       0x1cc
                u32 _0x1d0;
                u32 clk_source_csite;                       // _CLK_SOURCE_CSITE_0,                     0x1d4
                u32 clk_source_i2s1;                        // _CLK_SOURCE_I2S1_0,                      0x1d8
                u32 clk_source_dtv;                         // _CLK_SOURCE_DTV_0,                       0x1dc
                u32 _0x1e0[5];
                u32 clk_source_tsec;                        // _CLK_SOURCE_TSEC_0,                      0x1f4
                u32 _0x1f8;

                u32 clk_spare2;                             // _CLK_SPARE2_0,                           0x1fc
                u32 _0x200[32];

                u32 clk_out_enb_x;                          // _CLK_OUT_ENB_X_0,                        0x280
                u32 clk_enb_x_set;                          // _CLK_ENB_X_SET_0,                        0x284
                u32 clk_enb_x_clr;                          // _CLK_ENB_X_CLR_0,                        0x288

                u32 rst_devices_x;                          // _RST_DEVICES_X_0,                        0x28c
                u32 rst_dev_x_set;                          // _RST_DEV_X_SET_0,                        0x290
                u32 rst_dev_x_clr;                          // _RST_DEV_X_CLR_0,                        0x294

                u32 clk_out_enb_y;                          // _CLK_OUT_ENB_Y_0,                        0x298
                u32 clk_enb_y_set;                          // _CLK_ENB_Y_SET_0,                        0x29c
                u32 clk_enb_y_clr;                          // _CLK_ENB_Y_CLR_0,                        0x2a0

                u32 rst_devices_y;                          // _RST_DEVICES_Y_0,                        0x2a4
                u32 rst_dev_y_set;                          // _RST_DEV_Y_SET_0,                        0x2a8
                u32 rst_dev_y_clr;                          // _RST_DEV_Y_CLR_0,                        0x2ac

                u32 _0x2b0[17];
                u32 dfll_base;                              // _DFLL_BASE_0,                            0x2f4
                u32 _0x2f8[2];

                // _RST_DEV_L/H/U_SET_0 0x300-0x314
                u32 rst_dev_l_set;
                u32 rst_dev_l_clr;
                u32 rst_dev_h_set;
                u32 rst_dev_h_clr;
                u32 rst_dev_u_set;
                u32 rst_dev_u_clr;

                u32 _0x318[2];

                // _CLK_ENB_L/H/U_CLR_0 0x320-0x334
                u32 clk_enb_l_set;
                u32 clk_enb_l_clr;
                u32 clk_enb_h_set;
                u32 clk_enb_h_clr;
                u32 clk_enb_u_set;
                u32 clk_enb_u_clr;

                u32 _0x338;
                u32 ccplex_pg_sm_ovrd;                      // _CCPLEX_PG_SM_OVRD_0,                    0x33c
                u32 rst_cpu_cmplx_set;                      // _RST_CPU_CMPLX_SET_0,                    0x340
                u32 rst_cpu_cmplx_clr;                      // _RST_CPU_CMPLX_CLR_0,                    0x344

                // Additional (T30) registers
                u32 clk_cpu_cmplx_set;                      // _CLK_CPU_CMPLX_SET_0,                    0x348
                u32 clk_cpu_cmplx_clr;                      // _CLK_CPU_CMPLX_SET_0,                    0x34c

                u32 _0x350[2];
                u32 rst_dev_v;                              // _RST_DEVICES_V_0,                        0x358
                u32 rst_dev_w;                              // _RST_DEVICES_W_0,                        0x35c
                u32 clk_out_enb_v;                          // _CLK_OUT_ENB_V_0,                        0x360
                u32 clk_out_enb_w;                          // _CLK_OUT_ENB_W_0,                        0x364
                u32 cclkg_brst_pol;                         // _CCLKG_BURST_POLICY_0,                   0x368
                u32 super_cclkg_div;                        // _SUPER_CCLKG_DIVIDER_0,                  0x36c
                u32 cclklp_brst_pol;                        // _CCLKLP_BURST_POLICY_0,                  0x370
                u32 super_cclkp_div;                        // _SUPER_CCLKLP_DIVIDER_0,                 0x374
                u32 clk_cpug_cmplx;                         // _CLK_CPUG_CMPLX_0,                       0x378
                u32 clk_cpulp_cmplx;                        // _CLK_CPULP_CMPLX_0,                      0x37c
                u32 cpu_softrst_ctrl;                       // _CPU_SOFTRST_CTRL_0,                     0x380
                u32 cpu_softrst_ctrl1;                      // _CPU_SOFTRST_CTRL1_0,                    0x384
                u32 cpu_softrst_ctrl2;                      // _CPU_SOFTRST_CTRL2_0,                    0x388
                u32 _0x38c[5];
                u32 lvl2_clk_gate_ovrc;                     // _LVL2_CLK_GATE_OVRC,                     0x3a0
                u32 lvl2_clk_gate_ovrd;                     // _LVL2_CLK_GATE_OVRD,                     0x3a4
                u32 _0x3a8[2];

                u32 _0x3b0;
                u32 clk_source_mselect;                     // _CLK_SOURCE_MSELECT_0,                   0x3b4
                u32 clk_source_tsensor;                     // _CLK_SOURCE_TSENSOR_0,                   0x3b8
                u32 clk_source_i2s4;                        // _CLK_SOURCE_I2S4_0,                      0x3bc
                u32 clk_source_i2s5;                        // _CLK_SOURCE_I2S5_0,                      0x3c0
                u32 clk_source_i2c4;                        // _CLK_SOURCE_I2C4_0,                      0x3c4
                u32 _0x3c8[2];
                u32 clk_source_ahub;                        // _CLK_SOURCE_AHUB_0,                      0x3d0
                u32 _0x3d4[4];
                u32 clk_source_hda2codec_2x;                // _CLK_SOURCE_HDA2CODEC_2X_0,              0x3e4
                u32 clk_source_actmon;                      // _CLK_SOURCE_ACTMON_0,                    0x3e8
                u32 clk_source_extperiph1;                  // _CLK_SOURCE_EXTPERIPH1_0,                0x3ec
                u32 clk_source_extperiph2;                  // _CLK_SOURCE_EXTPERIPH2_0,                0x3f0
                u32 clk_source_extperiph3;                  // _CLK_SOURCE_EXTPERIPH3_0,                0x3f4
                u32 _0x3f8;
                u32 clk_source_i2c_slow;                    // _CLK_SOURCE_I2C_SLOW_0,                  0x3fc
                u32 clk_source_sys;                         // _CLK_SOURCE_SYS_0,                       0x400
                u32 clk_source_ispb;                        // _CLK_SOURCE_ISPB_0,                      0x404
                u32 _0x408[2];
                u32 clk_source_sor1;                        // _CLK_SOURCE_SOR1_0,                      0x410
                u32 clk_source_sor0;                        // _CLK_SOURCE_SOR0_0,                      0x414
                u32 _0x418[2];
                u32 clk_source_sata_oob;                    // _CLK_SOURCE_SATA_OOB_0,                  0x420
                u32 clk_source_sata;                        // _CLK_SOURCE_SATA_0,                      0x424
                u32 clk_source_hda;                         // _CLK_SOURCE_HDA_0,                       0x428
                u32 _0x42c;

                // _RST_DEV_V/W_SET_0 0x430-0x43c
                u32 rst_dev_v_set;
                u32 rst_dev_v_clr;
                u32 rst_dev_w_set;
                u32 rst_dev_w_clr;

                // _CLK_ENB_V/W_CLR_0 0x440-0x44c
                u32 clk_enb_v_set;
                u32 clk_enb_v_clr;
                u32 clk_enb_w_set;
                u32 clk_enb_w_clr;

                // Additional (T114+) registers
                u32 rst_cpug_cmplx_set;                     // _RST_CPUG_CMPLX_SET_0,                   0x450
                u32 rst_cpug_cmplx_clr;                     // _RST_CPUG_CMPLX_CLR_0,                   0x454
                u32 rst_cpulp_cmplx_set;                    // _RST_CPULP_CMPLX_SET_0,                  0x458
                u32 rst_cpulp_cmplx_clr;                    // _RST_CPULP_CMPLX_CLR_0,                  0x45c
                u32 clk_cpug_cmplx_set;                     // _CLK_CPUG_CMPLX_SET_0,                   0x460
                u32 clk_cpug_cmplx_clr;                     // _CLK_CPUG_CMPLX_CLR_0,                   0x464
                u32 clk_cpulp_cmplx_set;                    // _CLK_CPULP_CMPLX_SET_0,                  0x468
                u32 clk_cpulp_cmplx_clr;                    // _CLK_CPULP_CMPLX_CLR_0,                  0x46c
                u32 cpu_cmplx_status;                       // _CPU_CMPLX_STATUS_0,                     0x470
                u32 _0x474;                                 
                u32 intstatus;                              // _INTSTATUS_0,                            0x478
                u32 intmask;                                // _INTMASK_0,                              0x47c
                u32 utmip_pll_cfg0;                         // _UTMIP_PLL_CFG0_0,                       0x480
                u32 utmip_pll_cfg1;                         // _UTMIP_PLL_CFG1_0,                       0x484
                u32 utmip_pll_cfg2;                         // _UTMIP_PLL_CFG2_0,                       0x488

                u32 plle_aux;                               // _PLLE_AUX_0,                             0x48c
                u32 sata_pll_cfg0;                          // _SATA_PLL_CFG0_0,                        0x490
                u32 sata_pll_cfg1;                          // _SATA_PLL_CFG1_0,                        0x494
                u32 pcie_pll_cfg0;                          // _PCIE_PLL_CFG0_0,                        0x498

                u32 prog_audio_dly_clk;                     // _PROG_AUDIO_DLY_CLK_0,                   0x49c
                u32 audio_sync_clk_i2s0;                    // _AUDIO_SYNC_CLK_I2S0_0,                  0x4a0
                u32 audio_sync_clk_i2s1;                    // _AUDIO_SYNC_CLK_I2S1_0,                  0x4a4
                u32 audio_sync_clk_i2s2;                    // _AUDIO_SYNC_CLK_I2S2_0,                  0x4a8
                u32 audio_sync_clk_i2s3;                    // _AUDIO_SYNC_CLK_I2S3_0,                  0x4ac
                u32 audio_sync_clk_i2s4;                    // _AUDIO_SYNC_CLK_I2S4_0,                  0x4b0
                u32 audio_sync_clk_spdif;                   // _AUDIO_SYNC_CLK_SPDIF_0,                 0x4b4

                u32 plld2_base;                             // _PLLD2_BASE_0,                           0x4b8
                u32 plld2_misc;                             // _PLLD2_MISC_0,                           0x4bc
                u32 utmip_pll_cfg3;                         // _UTMIP_PLL_CFG3_0,                       0x4c0
                u32 pllrefe_base;                           // _PLLREFE_BASE_0,                         0x4c4
                u32 pllrefe_misc;                           // _PLLREFE_MISC_0,                         0x4c8
                u32 pllrefe_out;                            // _PLLREFE_OUT_0,                          0x4cc
                u32 cpu_finetrim_byp;                       // _CPU_FINETRIM_BYP_0,                     0x4d0
                u32 cpu_finetrim_select;                    // _CPU_FINETRIM_SELECT_0,                  0x4d4
                u32 cpu_finetrim_dr;                        // _CPU_FINETRIM_DR_0,                      0x4d8
                u32 cpu_finetrim_df;                        // _CPU_FINETRIM_DF_0,                      0x4dc
                u32 cpu_finetrim_f;                         // _CPU_FINETRIM_F_0,                       0x4e0
                u32 cpu_finetrim_r;                         // _CPU_FINETRIM_R_0,                       0x4e4
                u32 pllc2_base;                             // _PLLC2_BASE_0,                           0x4e8
                u32 pllc2_misc0;                            // _PLLC2_MISC_0_0,                         0x4ec
                u32 pllc2_misc1;                            // _PLLC2_MISC_1_0,                         0x4f0
                u32 pllc2_misc2;                            // _PLLC2_MISC_2_0,                         0x4f4
                u32 pllc2_misc3;                            // _PLLC2_MISC_3_0,                         0x4f8
                u32 pllc3_base;                             // _PLLC3_BASE_0,                           0x4fc
                u32 pllc3_misc0;                            // _PLLC3_MISC_0_0,                         0x500
                u32 pllc3_misc1;                            // _PLLC3_MISC_1_0,                         0x504
                u32 pllc3_misc2;                            // _PLLC3_MISC_2_0,                         0x508
                u32 pllc3_misc3;                            // _PLLC3_MISC_3_0,                         0x50c
                u32 pllx_misc1;                             // _PLLX_MISC_1_0,                          0x510
                u32 pllx_misc2;                             // _PLLX_MISC_2_0,                          0x514
                u32 pllx_misc3;                             // _PLLX_MISC_3_0,                          0x518
                u32 xusbio_pll_cfg0;                        // _XUSBIO_PLL_CFG0_0,                      0x51c
                u32 xusbio_pll_cfg1;                        // _XUSBIO_PLL_CFG0_1,                      0x520
                u32 plle_aux1;                              // _PLLE_AUX1_0,                            0x524
                u32 pllp_reshift;                           // _PLLP_RESHIFT_0,                         0x528
                u32 utmipll_hw_pwrdn_cfg0;                  // _UTMIPLL_HW_PWRDN_CFG0_0,                0x52c
                u32 pllu_hw_pwrdn_cfg0;                     // _PLLU_HW_PWRDN_CFG0_0,                   0x530
                u32 xusb_pll_cfg0;                          // _XUSB_PLL_CFG0_0,                        0x534
                u32 _0x538;
                u32 clk_cpu_misc;                           // _CLK_CPU_MISC_0,                         0x53c
                u32 clk_cpug_misc;                          // _CLK_CPUG_MISC_0,                        0x540
                u32 clk_cpulp_misc;                         // _CLK_CPULP_MISC_0,                       0x544
                u32 pllx_hw_ctrl_cfg;                       // _PLLX_HW_CTRL_CFG_0,                     0x548
                u32 pllx_sw_ramp_cfg;                       // _PLLX_SW_RAMP_CFG_0,                     0x54c
                u32 pllx_hw_ctrl_status;                    // _PLLX_HW_CTRL_STATUS_0,                  0x550
                u32 lvl2_clk_gate_ovre;                     // _LVL2_CLK_GATE_OVRE,                     0x554
                u32 super_gr3d_clk_div;                     // _SUPER_GR3D_CLK_DIVIDER_0,               0x558
                u32 spare_reg0;                             // _SPARE_REG0_0,                           0x55c
                u32 audio_sync_clk_dmic1;                   // _AUDIO_SYNC_CLK_DMIC1_0,                 0x560
                u32 audio_sync_clk_dmic2;                   // _AUDIO_SYNC_CLK_DMIC2_0,                 0x564

                u32 _0x568[2];
                u32 plld2_ss_cfg;                           // _PLLD2_SS_CFG,                           0x570
                u32 plld2_ss_ctrl1;                         // _PLLD2_SS_CTRL1_0,                       0x574
                u32 plld2_ss_ctrl2;                         // _PLLD2_SS_CTRL2_0,                       0x578
                u32 _0x57c[5];

                u32 plldp_base;                             // _PLLDP_BASE,                             0x590
                u32 plldp_misc;                             // _PLLDP_MISC,                             0x594
                u32 plldp_ss_cfg;                           // _PLLDP_SS_CFG,                           0x598
                u32 plldp_ss_ctrl1;                         // _PLLDP_SS_CTRL1_0,                       0x59c
                u32 plldp_ss_ctrl2;                         // _PLLDP_SS_CTRL2_0,                       0x5a0
                u32 pllc4_base;                             // _PLLC4_BASE_0,                           0x5a4
                u32 pllc4_misc;                             // _PLLC4_MISC_0,                           0x5a8
                u32 _0x5ac[6];
                u32 clk_spare0;                             // _CLK_SPARE0_0,                           0x5c4
                u32 clk_spare1;                             // _CLK_SPARE1_0,                           0x5c8
                u32 gpu_isob_ctrl;                          // _GPU_ISOB_CTRL_0,                        0x5cc
                u32 pllc_misc2;                             // _PLLC_MISC_2_0,                          0x5d0
                u32 pllc_misc3;                             // _PLLC_MISC_3_0,                          0x5d4
                u32 plla_misc2;                             // _PLLA_MISC2_0,                           0x5d8
                u32 _0x5dc[2];
                u32 pllc4_out;                              // _PLLC4_OUT_0,                            0x5e4
                u32 pllmb_base;                             // _PLLMB_BASE_0,                           0x5e8
                u32 pllmb_misc1;                            // _PLLMB_MISC1_0,                          0x5ec
                u32 pllx_misc4;                             // _PLLX_MISC_4_0,                          0x5f0
                u32 pllx_misc5;                             // _PLLX_MISC_5_0,                          0x5f4
                u32 _0x5f8[2];

                u32 clk_source_xusb_core_host;              // _CLK_SOURCE_XUSB_CORE_HOST_0,            0x600
                u32 clk_source_xusb_falcon;                 // _CLK_SOURCE_XUSB_FALCON_0,               0x604
                u32 clk_source_xusb_fs;                     // _CLK_SOURCE_XUSB_FS_0,                   0x608
                u32 clk_source_xusb_core_dev;               // _CLK_SOURCE_XUSB_CORE_DEV_0,             0x60c
                u32 clk_source_xusb_ss;                     // _CLK_SOURCE_XUSB_SS_0,                   0x610
                u32 clk_source_cilab;                       // _CLK_SOURCE_CILAB_0,                     0x614
                u32 clk_source_cilcd;                       // _CLK_SOURCE_CILCD_0,                     0x618
                u32 clk_source_cilef;                       // _CLK_SOURCE_CILEF_0,                     0x61c
                u32 clk_source_dsia_lp;                     // _CLK_SOURCE_DSIA_LP_0,                   0x620
                u32 clk_source_dsib_lp;                     // _CLK_SOURCE_DSIB_LP_0,                   0x624
                u32 clk_source_entropy;                     // _CLK_SOURCE_ENTROPY_0,                   0x628
                u32 clk_source_dvfs_ref;                    // _CLK_SOURCE_DVFS_REF_0,                  0x62c
                u32 clk_source_dvfs_soc;                    // _CLK_SOURCE_DVFS_SOC_0,                  0x630
                u32 _0x634[3];
                u32 clk_source_emc_latency;                 // _CLK_SOURCE_EMC_LATENCY_0,               0x640
                u32 clk_source_soc_therm;                   // _CLK_SOURCE_SOC_THERM_0,                 0x644
                u32 _0x648;
                u32 clk_source_dmic1;                       // _CLK_SOURCE_DMIC1_0,                     0x64c
                u32 clk_source_dmic2;                       // _CLK_SOURCE_DMIC2_0,                     0x650
                u32 _0x654;
                u32 clk_source_vi_sensor2;                  // _CLK_SOURCE_VI_SENSOR2_0,                0x658
                u32 clk_source_i2c6;                        // _CLK_SOURCE_I2C6_0,                      0x65c
                u32 clk_source_mipibif;                     // _CLK_SOURCE_MIPIBIF_0,                   0x660
                u32 clk_source_emc_dll;                     // _CLK_SOURCE_EMC_DLL_0,                   0x664
                u32 _0x668;
                u32 clk_source_uart_fst_mipi_cal;           // _CLK_SOURCE_UART_FST_MIPI_CAL_0,         0x66c
                u32 _0x670[2];
                u32 clk_source_vic;                         // _CLK_SOURCE_VIC_0,                       0x678

                u32 pllp_outc;                              // _PLLP_OUTC_0,                            0x67c
                u32 pllp_misc1;                             // _PLLP_MISC1_0,                           0x680
                u32 _0x684[2];
                u32 emc_div_clk_shaper_ctrl;                // _EMC_DIV_CLK_SHAPER_CTRL_0,              0x68c
                u32 emc_pllc_shaper_ctrl;                   // _EMC_PLLC_SHAPER_CTRL_0,                 0x690

                u32 clk_source_sdmmc_legacy_tm;             // _CLK_SOURCE_SDMMC_LEGACY_TM_0,           0x694
                u32 clk_source_nvdec;                       // _CLK_SOURCE_NVDEC_0,                     0x698
                u32 clk_source_nvjpg;                       // _CLK_SOURCE_NVJPG_0,                     0x69c
                u32 clk_source_nvenc;                       // _CLK_SOURCE_NVENC_0,                     0x6a0

                u32 plla1_base;                             // _PLLA1_BASE_0,                           0x6a4
                u32 plla1_misc0;                            // _PLLA1_MISC_0_0,                         0x6a8
                u32 plla1_misc1;                            // _PLLA1_MISC_1_0,                         0x6ac
                u32 plla1_misc2;                            // _PLLA1_MISC_2_0,                         0x6b0
                u32 plla1_misc3;                            // _PLLA1_MISC_3_0,                         0x6b4
                u32 audio_sync_clk_dmic3;                   // _AUDIO_SYNC_CLK_DMIC3_0,                 0x6b8

                u32 clk_source_dmic3;                       // _CLK_SOURCE_DMIC3_0,                     0x6bc
                u32 clk_source_ape;                         // _CLK_SOURCE_APE_0,                       0x6c0
                u32 clk_source_qspi;                        // _CLK_SOURCE_QSPI_0,                      0x6c4
                u32 clk_source_vi_i2c;                      // _CLK_SOURCE_VI_I2C_0,                    0x6c8
                u32 clk_source_usb2_hsic_trk;               // _CLK_SOURCE_USB2_HSIC_TRK_0,             0x6cc
                u32 clk_source_pex_sata_usb_rx_byp;         // _CLK_SOURCE_PEX_SATA_USB_RX_BYP_0,       0x6d0
                u32 clk_source_maud;                        // _CLK_SOURCE_MAUD_0,                      0x6d4
                u32 clk_source_tsecb;                       // _CLK_SOURCE_TSECB_0,                     0x6d8

                u32 clk_cpug_misc1;                         // _CLK_CPUG_MISC1_0, 0x6dc
                u32 aclk_burst_policy;                      // _ACLK_BURST_POLICY_0,                    0x6e0
                u32 super_aclk_divider;                     // _SUPER_ACLK_DIVIDER_0,                   0x6e4

                u32 nvenc_super_clk_divider;                // _NVENC_SUPER_CLK_DIVIDER_0,              0x6e8
                u32 vi_super_clk_divider;                   // _VI_SUPER_CLK_DIVIDER_0,                 0x6ec
                u32 vic_super_clk_divider;                  // _VIC_SUPER_CLK_DIVIDER_0,                0x6f0
                u32 nvdec_super_clk_divider;                // _NVDEC_SUPER_CLK_DIVIDER_0,              0x6f4
                u32 isp_super_clk_divider;                  // _ISP_SUPER_CLK_DIVIDER_0,                0x6f8
                u32 ispb_super_clk_divider;                 // _ISPB_SUPER_CLK_DIVIDER_0,               0x6fc
                u32 nvjpg_super_clk_divider;                // _NVJPG_SUPER_CLK_DIVIDER_0,              0x700
                u32 se_super_clk_divider;                   // _SE_SUPER_CLK_DIVIDER_0,                 0x704
                u32 tsec_super_clk_divider;                 // _TSEC_SUPER_CLK_DIVIDER_0,               0x708
                u32 tsecb_super_clk_divider;                // _TSECB_SUPER_CLK_DIVIDER_0,              0x70c

                u32 clk_source_uartape;                     // _CLK_SOURCE_UARTAPE_0,                   0x710
                u32 clk_cpug_misc2;                         // _CLK_CPUG_MISC2_0,                       0x714
                u32 clk_source_dbgapb;                      // _CLK_SOURCE_DBGAPB_0,                    0x718
                u32 clk_ccplex_cc4_ret_clk_enb;             // _CLK_CCPLEX_CC4_RET_CLK_ENB_0,           0x71c
                u32 actmon_cpu_clk;                         // _ACTMON_CPU_CLK_0,                       0x720
                u32 clk_source_emc_safe;                    // _CLK_SOURCE_EMC_SAFE_0,                  0x724
                u32 sdmmc2_pllc4_out0_shaper_ctrl;          // _SDMMC2_PLLC4_OUT0_SHAPER_CTRL_0,        0x728
                u32 sdmmc2_pllc4_out1_shaper_ctrl;          // _SDMMC2_PLLC4_OUT1_SHAPER_CTRL_0,        0x72c
                u32 sdmmc2_pllc4_out2_shaper_ctrl;          // _SDMMC2_PLLC4_OUT2_SHAPER_CTRL_0,        0x730
                u32 sdmmc2_div_clk_shaper_ctrl;             // _SDMMC2_DIV_CLK_SHAPER_CTRL_0,           0x734
                u32 sdmmc4_pllc4_out0_shaper_ctrl;          // _SDMMC4_PLLC4_OUT0_SHAPER_CTRL_0,        0x738
                u32 sdmmc4_pllc4_out1_shaper_ctrl;          // _SDMMC4_PLLC4_OUT1_SHAPER_CTRL_0,        0x73c
                u32 sdmmc4_pllc4_out2_shaper_ctrl;          // _SDMMC4_PLLC4_OUT2_SHAPER_CTRL_0,        0x740
                u32 sdmmc4_div_clk_shaper_ctrl;             // _SDMMC4_DIV_CLK_SHAPER_CTRL_0,           0x744
            };
            static_assert(std::is_standard_layout_v<Registers>);
            static_assert(std::is_trivial_v<Registers>);

        private:
            static constexpr u32 clkRegOffsets[] = { 0x010, 0x014, 0x018, 0x360, 0x364, 0x280, 0x298 };
            static constexpr u32 rstRegOffsets[] = { 0x004, 0x008, 0x00C, 0x358, 0x35C, 0x28C, 0x2A4 };

        private:
            volatile Registers *m_regs = nullptr;
            // TODO friend

        private:
            vu32 *RegisterAt(u32 offset) const
            {
                return reinterpret_cast<vu32 *>(reinterpret_cast<uintptr_t>(m_regs) + offset);
            }

        public:
            union Device {
                struct {
                    u32 bank        : 3;
                    u32 bitPos      : 5;
                    u32 regOffset   : 12;
                    u32 value       : 3;
                    u32             : 0;
                    u32 divisor     : 16;
                };
                u64 raw;
            };
            static_assert(std::is_standard_layout_v<Device>);
            static_assert(std::is_trivial_v<Device>);
            static_assert(sizeof(Device) == 8);

            static constexpr Device uartA = {{
                .bank = 5,
                .bitPos = 6,
                .regOffset = 0x178,
                .value = 0,
                .divisor = 0,
            }};
            static constexpr Device uartB = {{
                .bank = 0,
                .bitPos = 7,
                .regOffset = 0x17C,
                .value = 0,
                .divisor = 0,
            }};
            static constexpr Device uartC = {{
                .bank = 1,
                .bitPos = 23,
                .regOffset = 0x1A0,
                .value = 0,
                .divisor = 0,
            }};
            static constexpr Device uartD = {{
                .bank = 2,
                .bitPos = 1,
                .regOffset = 0x1C0,
                .value = 0,
                .divisor = 0,
            }};
            static constexpr Device i2c1 = {{
                .bank = 0,
                .bitPos = 12,
                .regOffset = 0x124,
                .value = 6,
                .divisor = 0,
            }};
            static constexpr Device i2c5 = {{
                .bank = 1,
                .bitPos = 15,
                .regOffset = 0x128,
                .value = 6,
                .divisor = 0,
            }};
            static constexpr Device tzram = {{
                .bank = 3,
                .bitPos = 30,
            }};
            static constexpr Device se = {{
                .bank = 3,
                .bitPos = 31,
                .regOffset = 0x42C,
                .value = 0,
                .divisor = 0,
            }};
            static constexpr Device host1x = {{
                .bank = 0,
                .bitPos = 28,
                .regOffset = 0x180,
                .value = 4,
                .divisor = 3,
            }};
            static constexpr Device tsec = {{
                .bank = 2,
                .bitPos = 19,
                .regOffset = 0x1F4,
                .value = 0,
                .divisor = 2,
            }};
            static constexpr Device sorSafe = {{
                .bank = 6,
                .bitPos = 30,
            }};
            static constexpr Device sor0 = {{
                .bank = 5,
                .bitPos = 22,
            }};
            static constexpr Device sor1 = {{
                .bank = 5,
                .bitPos = 30,
                .regOffset = 0x410,
                .value = 0,
                .divisor = 2,
            }};
            static constexpr Device kfuse = {{
                .bank = 1,
                .bitPos = 8,
            }};
            static constexpr Device clDvfs = {{
                .bank = 4,
                .bitPos = 27,
            }};
            static constexpr Device bpmp = {{
                .bank = 0,
                .bitPos = 1,
            }};
            static constexpr Device actmon = {{
                .bank = 3,
                .bitPos = 23,
                .regOffset = 0x3E8,
                .value = 6,
                .divisor = 0,
            }};
            static constexpr Device coresight = {{
                .bank = 2,
                .bitPos = 9,
                .regOffset = 0x1D4,
                .value = 0,
                .divisor = 4,
            }};

        public:
            void EnableClock(Device dev) const
            {
                // Configure default PLL and divisor
                if (dev.regOffset != 0) {
                    *RegisterAt(dev.regOffset) = dev.value << 29 | dev.divisor;
                }

                // Enable the clock
                *RegisterAt(clkRegOffsets[dev.bank]) |= BIT(dev.bitPos);
            }

            void DisableClock(Device dev) const
            {
                *RegisterAt(clkRegOffsets[dev.bank]) &= ~BIT(dev.bitPos);
            }

            void EnableReset(Device dev) const
            {
                *RegisterAt(rstRegOffsets[dev.bank]) |= BIT(dev.bitPos);
            }

            void DisableReset(Device dev) const
            {
                *RegisterAt(rstRegOffsets[dev.bank]) &= ~BIT(dev.bitPos);
            }

            void Enable(Device dev) const
            {
                EnableClock(dev);
                DisableReset(dev);
            }

            void Disable(Device dev) const
            {
                EnableReset(dev);
                DisableClock(dev);
            }

            void Reboot(Device dev) const {
                Disable(dev);
                // KFUSE needs a workaround
                /*if (dev.raw == kfuse.raw) {
                    EnableClock(dev);
                    // Wait 100us
                    DisableReset(dev);
                    // Wait 200us
                }*/
                Enable(dev);
            }

            void SetFuseRegsEnabled(bool enabled) const
            {
                u32 mask = enabled ? BIT(28) : 0;
                m_regs->misc_clk_enb = (m_regs->misc_clk_enb & ~BIT(28)) | mask;
            }
    };

}
