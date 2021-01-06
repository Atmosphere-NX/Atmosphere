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
 
#ifndef FUSEE_CAR_H
#define FUSEE_CAR_H

#include <stdint.h>
#include <stdbool.h>

#define CAR_BASE 0x60006000
#define MAKE_CAR_REG(n) MAKE_REG32(CAR_BASE + n)

#define CLK_L_SDMMC1    (1 << 14)
#define CLK_L_SDMMC2    (1 << 9)
#define CLK_U_SDMMC3    (1 << 5)
#define CLK_L_SDMMC4    (1 << 15)

#define CLK_SOURCE_MASK             (0b111 << 29)
#define CLK_SOURCE_FIRST            (0b000 << 29)
#define CLK_DIVIDER_MASK            (0xff << 0)
#define CLK_DIVIDER_UNITY           (0x00 << 0)

#define NUM_CAR_BANKS 7

/* Clock and reset devices. */
typedef enum {
    CARDEVICE_BPMP      = ((0 << 5) | 0x1),
    CARDEVICE_UARTA     = ((0 << 5) | 0x6),
    CARDEVICE_UARTB     = ((0 << 5) | 0x7),
    CARDEVICE_I2C1      = ((0 << 5) | 0xC),
    CARDEVICE_USBD      = ((0 << 5) | 0x16),
    CARDEVICE_HOST1X    = ((0 << 5) | 0x1C),
    CARDEVICE_AHBDMA    = ((1 << 5) | 0x1),
    CARDEVICE_APBDMA    = ((1 << 5) | 0x2),
    CARDEVICE_KFUSE     = ((1 << 5) | 0x8),
    CARDEVICE_I2C5      = ((1 << 5) | 0xF),
    CARDEVICE_UARTC     = ((1 << 5) | 0x17),
    CARDEVICE_USB2      = ((1 << 5) | 0x1A),
    CARDEVICE_CORESIGHT = ((2 << 5) | 0x9),
    CARDEVICE_TSEC      = ((2 << 5) | 0x13),
    CARDEVICE_MSELECT   = ((3 << 5) | 0x8),
    CARDEVICE_ACTMON    = ((3 << 5) | 0x17),
    CARDEVICE_TZRAM     = ((3 << 5) | 0x1E),
    CARDEVICE_SE        = ((3 << 5) | 0x1F),
    CARDEVICE_CL_DVFS   = ((4 << 5) | 0x1B),
    CARDEVICE_SOR0      = ((5 << 5) | 0x16),
    CARDEVICE_SOR1      = ((5 << 5) | 0x17),
    CARDEVICE_SOR_SAFE  = ((6 << 5) | 0x1E),
} CarDevice;

/* Clock/Reset Controller (CLK_RST_CONTROLLER_) regs */
typedef struct {
    uint32_t rst_src;           /* _RST_SOURCE_0,   0x00 */

    /* _RST_DEVICES_L/H/U_0     0x4-0xc */
    uint32_t rst_dev_l;
    uint32_t rst_dev_h;
    uint32_t rst_dev_u;

    /* _CLK_OUT_ENB_L/H/U_0     0x10-0x18 */
    uint32_t clk_out_enb_l;
    uint32_t clk_out_enb_h;
    uint32_t clk_out_enb_u;

    uint32_t _0x1C;
    uint32_t cclk_brst_pol;     /* _CCLK_BURST_POLICY_0,    0x20 */
    uint32_t super_cclk_div;    /* _SUPER_CCLK_DIVIDER_0,   0x24 */
    uint32_t sclk_brst_pol;     /* _SCLK_BURST_POLICY_0,    0x28 */
    uint32_t super_sclk_div;    /* _SUPER_SCLK_DIVIDER_0,   0x2c */
    uint32_t clk_sys_rate;      /* _CLK_SYSTEM_RATE_0,      0x30 */
    uint32_t prog_dly_clk;      /* _PROG_DLY_CLK_0,         0x34 */
    uint32_t aud_sync_clk_rate; /* _AUDIO_SYNC_CLK_RATE_0,  0x38 */
    uint32_t _0x3C;
    uint32_t cop_clk_skip_plcy; /* _COP_CLK_SKIP_POLICY_0,  0x40 */
    uint32_t clk_mask_arm;      /* _CLK_MASK_ARM_0,         0x44 */
    uint32_t misc_clk_enb;      /* _MISC_CLK_ENB_0,         0x48 */
    uint32_t clk_cpu_cmplx;     /* _CLK_CPU_CMPLX_0,        0x4c */
    uint32_t osc_ctrl;          /* _OSC_CTRL_0,             0x50 */
    uint32_t pll_lfsr;          /* _PLL_LFSR_0,             0x54 */
    uint32_t osc_freq_det;      /* _OSC_FREQ_DET_0,         0x58 */
    uint32_t osc_freq_det_stat; /* _OSC_FREQ_DET_STATUS_0,  0x5c */
    uint32_t _0x60[2];
    uint32_t plle_ss_cntl;      /* _PLLE_SS_CNTL_0,         0x68 */
    uint32_t plle_misc1;        /* _PLLE_MISC1_0,           0x6c */
    uint32_t _0x70[4];

    /* PLLC 0x80-0x8c */
    uint32_t pllc_base;
    uint32_t pllc_out;
    uint32_t pllc_misc0;
    uint32_t pllc_misc1;

    /* PLLM 0x90-0x9c */
    uint32_t pllm_base;
    uint32_t pllm_out;
    uint32_t pllm_misc1;
    uint32_t pllm_misc2;

    /* PLLP 0xa0-0xac */
    uint32_t pllp_base;
    uint32_t pllp_outa;
    uint32_t pllp_outb;
    uint32_t pllp_misc;

    /* PLLA 0xb0-0xbc */
    uint32_t plla_base;
    uint32_t plla_out;
    uint32_t plla_misc0;
    uint32_t plla_misc1;

    /* PLLU 0xc0-0xcc */
    uint32_t pllu_base;
    uint32_t pllu_out;
    uint32_t pllu_misc1;
    uint32_t pllu_misc2;

    /* PLLD 0xd0-0xdc */
    uint32_t plld_base;
    uint32_t plld_out;
    uint32_t plld_misc1;
    uint32_t plld_misc2;

    /* PLLX 0xe0-0xe4 */
    uint32_t pllx_base;
    uint32_t pllx_misc;

    /* PLLE 0xe8-0xf4 */
    uint32_t plle_base;
    uint32_t plle_misc;
    uint32_t plle_ss_cntl1;
    uint32_t plle_ss_cntl2;

    uint32_t lvl2_clk_gate_ovra;            /* _LVL2_CLK_GATE_OVRA_0,   0xf8 */
    uint32_t lvl2_clk_gate_ovrb;            /* _LVL2_CLK_GATE_OVRB_0,   0xfc */

    uint32_t clk_source_i2s2;               /* _CLK_SOURCE_I2S2_0,      0x100 */
    uint32_t clk_source_i2s3;               /* _CLK_SOURCE_I2S3_0,      0x104 */
    uint32_t clk_source_spdif_out;          /* _CLK_SOURCE_SPDIF_OUT_0, 0x108 */
    uint32_t clk_source_spdif_in;           /* _CLK_SOURCE_SPDIF_IN_0,  0x10c */
    uint32_t clk_source_pwm;                /* _CLK_SOURCE_PWM_0,       0x110 */
    uint32_t _0x114;
    uint32_t clk_source_spi2;               /* _CLK_SOURCE_SPI2_0,      0x118 */
    uint32_t clk_source_spi3;               /* _CLK_SOURCE_SPI3_0,      0x11c */
    uint32_t _0x120;
    uint32_t clk_source_i2c1;               /* _CLK_SOURCE_I2C1_0,      0x124 */
    uint32_t clk_source_i2c5;               /* _CLK_SOURCE_I2C5_0,      0x128 */
    uint32_t _0x12c[2];
    uint32_t clk_source_spi1;               /* _CLK_SOURCE_SPI1_0,      0x134 */
    uint32_t clk_source_disp1;              /* _CLK_SOURCE_DISP1_0,     0x138 */
    uint32_t clk_source_disp2;              /* _CLK_SOURCE_DISP2_0,     0x13c */
    uint32_t _0x140;
    uint32_t clk_source_isp;                /* _CLK_SOURCE_ISP_0,       0x144 */
    uint32_t clk_source_vi;                 /* _CLK_SOURCE_VI_0,        0x148 */
    uint32_t _0x14c;
    uint32_t clk_source_sdmmc1;             /* _CLK_SOURCE_SDMMC1_0,    0x150 */
    uint32_t clk_source_sdmmc2;             /* _CLK_SOURCE_SDMMC2_0,    0x154 */
    uint32_t _0x158[3];
    uint32_t clk_source_sdmmc4;             /* _CLK_SOURCE_SDMMC4_0,    0x164 */
    uint32_t _0x168[4];
    uint32_t clk_source_uarta;              /* _CLK_SOURCE_UARTA_0,     0x178 */
    uint32_t clk_source_uartb;              /* _CLK_SOURCE_UARTB_0,     0x17c */
    uint32_t clk_source_host1x;             /* _CLK_SOURCE_HOST1X_0,    0x180 */
    uint32_t _0x184[5];
    uint32_t clk_source_i2c2;               /* _CLK_SOURCE_I2C2_0,      0x198 */
    uint32_t clk_source_emc;                /* _CLK_SOURCE_EMC_0,       0x19c */
    uint32_t clk_source_uartc;              /* _CLK_SOURCE_UARTC_0,     0x1a0 */
    uint32_t _0x1a4;
    uint32_t clk_source_vi_sensor;          /* _CLK_SOURCE_VI_SENSOR_0, 0x1a8 */
    uint32_t _0x1ac[2];
    uint32_t clk_source_spi4;               /* _CLK_SOURCE_SPI4_0,      0x1b4 */
    uint32_t clk_source_i2c3;               /* _CLK_SOURCE_I2C3_0,      0x1b8 */
    uint32_t clk_source_sdmmc3;             /* _CLK_SOURCE_SDMMC3_0,    0x1bc */
    uint32_t clk_source_uartd;              /* _CLK_SOURCE_UARTD_0,     0x1c0 */
    uint32_t _0x1c4[2];
    uint32_t clk_source_owr;                /* _CLK_SOURCE_OWR_0,       0x1cc */
    uint32_t _0x1d0;
    uint32_t clk_source_csite;              /* _CLK_SOURCE_CSITE_0,     0x1d4 */
    uint32_t clk_source_i2s1;               /* _CLK_SOURCE_I2S1_0,      0x1d8 */
    uint32_t clk_source_dtv;                /* _CLK_SOURCE_DTV_0,       0x1dc */
    uint32_t _0x1e0[5];
    uint32_t clk_source_tsec;               /* _CLK_SOURCE_TSEC_0,      0x1f4 */
    uint32_t _0x1f8;

    uint32_t clk_spare2;                    /* _CLK_SPARE2_0,       0x1fc */
    uint32_t _0x200[32];

    uint32_t clk_out_enb_x;                 /* _CLK_OUT_ENB_X_0,    0x280 */
    uint32_t clk_enb_x_set;                 /* _CLK_ENB_X_SET_0,    0x284 */
    uint32_t clk_enb_x_clr;                 /* _CLK_ENB_X_CLR_0,    0x288 */

    uint32_t rst_devices_x;                 /* _RST_DEVICES_X_0,    0x28c */
    uint32_t rst_dev_x_set;                 /* _RST_DEV_X_SET_0,    0x290 */
    uint32_t rst_dev_x_clr;                 /* _RST_DEV_X_CLR_0,    0x294 */

    uint32_t clk_out_enb_y;                 /* _CLK_OUT_ENB_Y_0,    0x298 */
    uint32_t clk_enb_y_set;                 /* _CLK_ENB_Y_SET_0,    0x29c */
    uint32_t clk_enb_y_clr;                 /* _CLK_ENB_Y_CLR_0,    0x2a0 */

    uint32_t rst_devices_y;                 /* _RST_DEVICES_Y_0,    0x2a4 */
    uint32_t rst_dev_y_set;                 /* _RST_DEV_Y_SET_0,    0x2a8 */
    uint32_t rst_dev_y_clr;                 /* _RST_DEV_Y_CLR_0,    0x2ac */

    uint32_t _0x2b0[17];
    uint32_t dfll_base;                     /* _DFLL_BASE_0,        0x2f4 */
    uint32_t _0x2f8[2];

    /* _RST_DEV_L/H/U_SET_0 0x300-0x314 */
    uint32_t rst_dev_l_set;
    uint32_t rst_dev_l_clr;
    uint32_t rst_dev_h_set;
    uint32_t rst_dev_h_clr;
    uint32_t rst_dev_u_set;
    uint32_t rst_dev_u_clr;

    uint32_t _0x318[2];

    /* _CLK_ENB_L/H/U_CLR_0 0x320-0x334 */
    uint32_t clk_enb_l_set;
    uint32_t clk_enb_l_clr;
    uint32_t clk_enb_h_set;
    uint32_t clk_enb_h_clr;
    uint32_t clk_enb_u_set;
    uint32_t clk_enb_u_clr;

    uint32_t _0x338;
    uint32_t ccplex_pg_sm_ovrd;         /* _CCPLEX_PG_SM_OVRD_0,    0x33c */
    uint32_t rst_cpu_cmplx_set;         /* _RST_CPU_CMPLX_SET_0,    0x340 */
    uint32_t rst_cpu_cmplx_clr;         /* _RST_CPU_CMPLX_CLR_0,    0x344 */

    /* Additional (T30) registers */
    uint32_t clk_cpu_cmplx_set;         /* _CLK_CPU_CMPLX_SET_0,    0x348 */
    uint32_t clk_cpu_cmplx_clr;         /* _CLK_CPU_CMPLX_SET_0,    0x34c */

    uint32_t _0x350[2];
    uint32_t rst_dev_v;                 /* _RST_DEVICES_V_0,        0x358 */
    uint32_t rst_dev_w;                 /* _RST_DEVICES_W_0,        0x35c */
    uint32_t clk_out_enb_v;             /* _CLK_OUT_ENB_V_0,        0x360 */
    uint32_t clk_out_enb_w;             /* _CLK_OUT_ENB_W_0,        0x364 */
    uint32_t cclkg_brst_pol;            /* _CCLKG_BURST_POLICY_0,   0x368 */
    uint32_t super_cclkg_div;           /* _SUPER_CCLKG_DIVIDER_0,  0x36c */
    uint32_t cclklp_brst_pol;           /* _CCLKLP_BURST_POLICY_0,  0x370 */
    uint32_t super_cclkp_div;           /* _SUPER_CCLKLP_DIVIDER_0, 0x374 */
    uint32_t clk_cpug_cmplx;            /* _CLK_CPUG_CMPLX_0,       0x378 */
    uint32_t clk_cpulp_cmplx;           /* _CLK_CPULP_CMPLX_0,      0x37c */
    uint32_t cpu_softrst_ctrl;          /* _CPU_SOFTRST_CTRL_0,     0x380 */
    uint32_t cpu_softrst_ctrl1;         /* _CPU_SOFTRST_CTRL1_0,    0x384 */
    uint32_t cpu_softrst_ctrl2;         /* _CPU_SOFTRST_CTRL2_0,    0x388 */
    uint32_t _0x38c[5];
    uint32_t lvl2_clk_gate_ovrc;        /* _LVL2_CLK_GATE_OVRC,     0x3a0 */
    uint32_t lvl2_clk_gate_ovrd;        /* _LVL2_CLK_GATE_OVRD,     0x3a4 */
    uint32_t _0x3a8[2];

    uint32_t _0x3b0;
    uint32_t clk_source_mselect;        /* _CLK_SOURCE_MSELECT_0,       0x3b4 */
    uint32_t clk_source_tsensor;        /* _CLK_SOURCE_TSENSOR_0,       0x3b8 */
    uint32_t clk_source_i2s4;           /* _CLK_SOURCE_I2S4_0,          0x3bc */
    uint32_t clk_source_i2s5;           /* _CLK_SOURCE_I2S5_0,          0x3c0 */
    uint32_t clk_source_i2c4;           /* _CLK_SOURCE_I2C4_0,          0x3c4 */
    uint32_t _0x3c8[2];
    uint32_t clk_source_ahub;           /* _CLK_SOURCE_AHUB_0,          0x3d0 */
    uint32_t _0x3d4[4];
    uint32_t clk_source_hda2codec_2x;   /* _CLK_SOURCE_HDA2CODEC_2X_0,  0x3e4 */
    uint32_t clk_source_actmon;         /* _CLK_SOURCE_ACTMON_0,        0x3e8 */
    uint32_t clk_source_extperiph1;     /* _CLK_SOURCE_EXTPERIPH1_0,    0x3ec */
    uint32_t clk_source_extperiph2;     /* _CLK_SOURCE_EXTPERIPH2_0,    0x3f0 */
    uint32_t clk_source_extperiph3;     /* _CLK_SOURCE_EXTPERIPH3_0,    0x3f4 */
    uint32_t _0x3f8;
    uint32_t clk_source_i2c_slow;       /* _CLK_SOURCE_I2C_SLOW_0,      0x3fc */
    uint32_t clk_source_sys;            /* _CLK_SOURCE_SYS_0,           0x400 */
    uint32_t clk_source_ispb;           /* _CLK_SOURCE_ISPB_0,          0x404 */
    uint32_t _0x408[2];
    uint32_t clk_source_sor1;           /* _CLK_SOURCE_SOR1_0,          0x410 */
    uint32_t clk_source_sor0;           /* _CLK_SOURCE_SOR0_0,          0x414 */
    uint32_t _0x418[2];
    uint32_t clk_source_sata_oob;       /* _CLK_SOURCE_SATA_OOB_0,      0x420 */
    uint32_t clk_source_sata;           /* _CLK_SOURCE_SATA_0,          0x424 */
    uint32_t clk_source_hda;            /* _CLK_SOURCE_HDA_0,           0x428 */
    uint32_t clk_source_se;             /* _CLK_SOURCE_SE_0,            0x42c */

    /* _RST_DEV_V/W_SET_0 0x430-0x43c */
    uint32_t rst_dev_v_set;
    uint32_t rst_dev_v_clr;
    uint32_t rst_dev_w_set;
    uint32_t rst_dev_w_clr;

    /* _CLK_ENB_V/W_CLR_0 0x440-0x44c */
    uint32_t clk_enb_v_set;
    uint32_t clk_enb_v_clr;
    uint32_t clk_enb_w_set;
    uint32_t clk_enb_w_clr;

    /* Additional (T114+) registers */
    uint32_t rst_cpug_cmplx_set;    /* _RST_CPUG_CMPLX_SET_0,   0x450 */
    uint32_t rst_cpug_cmplx_clr;    /* _RST_CPUG_CMPLX_CLR_0,   0x454 */
    uint32_t rst_cpulp_cmplx_set;   /* _RST_CPULP_CMPLX_SET_0,  0x458 */
    uint32_t rst_cpulp_cmplx_clr;   /* _RST_CPULP_CMPLX_CLR_0,  0x45c */
    uint32_t clk_cpug_cmplx_set;    /* _CLK_CPUG_CMPLX_SET_0,   0x460 */
    uint32_t clk_cpug_cmplx_clr;    /* _CLK_CPUG_CMPLX_CLR_0,   0x464 */
    uint32_t clk_cpulp_cmplx_set;   /* _CLK_CPULP_CMPLX_SET_0,  0x468 */
    uint32_t clk_cpulp_cmplx_clr;   /* _CLK_CPULP_CMPLX_CLR_0,  0x46c */
    uint32_t cpu_cmplx_status;      /* _CPU_CMPLX_STATUS_0,     0x470 */
    uint32_t _0x474;
    uint32_t intstatus;             /* _INTSTATUS_0,            0x478 */
    uint32_t intmask;               /* _INTMASK_0,              0x47c */
    uint32_t utmip_pll_cfg0;        /* _UTMIP_PLL_CFG0_0,       0x480 */
    uint32_t utmip_pll_cfg1;        /* _UTMIP_PLL_CFG1_0,       0x484 */
    uint32_t utmip_pll_cfg2;        /* _UTMIP_PLL_CFG2_0,       0x488 */

    uint32_t plle_aux;              /* _PLLE_AUX_0,             0x48c */
    uint32_t sata_pll_cfg0;         /* _SATA_PLL_CFG0_0,        0x490 */
    uint32_t sata_pll_cfg1;         /* _SATA_PLL_CFG1_0,        0x494 */
    uint32_t pcie_pll_cfg0;         /* _PCIE_PLL_CFG0_0,        0x498 */

    uint32_t prog_audio_dly_clk;    /* _PROG_AUDIO_DLY_CLK_0,   0x49c */
    uint32_t audio_sync_clk_i2s0;   /* _AUDIO_SYNC_CLK_I2S0_0,  0x4a0 */
    uint32_t audio_sync_clk_i2s1;   /* _AUDIO_SYNC_CLK_I2S1_0,  0x4a4 */
    uint32_t audio_sync_clk_i2s2;   /* _AUDIO_SYNC_CLK_I2S2_0,  0x4a8 */
    uint32_t audio_sync_clk_i2s3;   /* _AUDIO_SYNC_CLK_I2S3_0,  0x4ac */
    uint32_t audio_sync_clk_i2s4;   /* _AUDIO_SYNC_CLK_I2S4_0,  0x4b0 */
    uint32_t audio_sync_clk_spdif;  /* _AUDIO_SYNC_CLK_SPDIF_0, 0x4b4 */

    uint32_t plld2_base;            /* _PLLD2_BASE_0, 0x4b8 */
    uint32_t plld2_misc;            /* _PLLD2_MISC_0, 0x4bc */
    uint32_t utmip_pll_cfg3;        /* _UTMIP_PLL_CFG3_0, 0x4c0 */
    uint32_t pllrefe_base;          /* _PLLREFE_BASE_0, 0x4c4 */
    uint32_t pllrefe_misc;          /* _PLLREFE_MISC_0, 0x4c8 */
    uint32_t pllrefe_out;           /* _PLLREFE_OUT_0, 0x4cc */
    uint32_t cpu_finetrim_byp;      /* _CPU_FINETRIM_BYP_0, 0x4d0 */
    uint32_t cpu_finetrim_select;   /* _CPU_FINETRIM_SELECT_0, 0x4d4 */
    uint32_t cpu_finetrim_dr;       /* _CPU_FINETRIM_DR_0, 0x4d8 */
    uint32_t cpu_finetrim_df;       /* _CPU_FINETRIM_DF_0, 0x4dc */
    uint32_t cpu_finetrim_f;        /* _CPU_FINETRIM_F_0, 0x4e0 */
    uint32_t cpu_finetrim_r;        /* _CPU_FINETRIM_R_0, 0x4e4 */
    uint32_t pllc2_base;            /* _PLLC2_BASE_0, 0x4e8 */
    uint32_t pllc2_misc0;           /* _PLLC2_MISC_0_0, 0x4ec */
    uint32_t pllc2_misc1;           /* _PLLC2_MISC_1_0, 0x4f0 */
    uint32_t pllc2_misc2;           /* _PLLC2_MISC_2_0, 0x4f4 */
    uint32_t pllc2_misc3;           /* _PLLC2_MISC_3_0, 0x4f8 */
    uint32_t pllc3_base;            /* _PLLC3_BASE_0, 0x4fc */
    uint32_t pllc3_misc0;           /* _PLLC3_MISC_0_0, 0x500 */
    uint32_t pllc3_misc1;           /* _PLLC3_MISC_1_0, 0x504 */
    uint32_t pllc3_misc2;           /* _PLLC3_MISC_2_0, 0x508 */
    uint32_t pllc3_misc3;           /* _PLLC3_MISC_3_0, 0x50c */
    uint32_t pllx_misc1;            /* _PLLX_MISC_1_0, 0x510 */
    uint32_t pllx_misc2;            /* _PLLX_MISC_2_0, 0x514 */
    uint32_t pllx_misc3;            /* _PLLX_MISC_3_0, 0x518 */
    uint32_t xusbio_pll_cfg0;       /* _XUSBIO_PLL_CFG0_0, 0x51c */
    uint32_t xusbio_pll_cfg1;       /* _XUSBIO_PLL_CFG0_1, 0x520 */
    uint32_t plle_aux1;             /* _PLLE_AUX1_0, 0x524 */
    uint32_t pllp_reshift;          /* _PLLP_RESHIFT_0, 0x528 */
    uint32_t utmipll_hw_pwrdn_cfg0; /* _UTMIPLL_HW_PWRDN_CFG0_0, 0x52c */
    uint32_t pllu_hw_pwrdn_cfg0;    /* _PLLU_HW_PWRDN_CFG0_0, 0x530 */
    uint32_t xusb_pll_cfg0;         /* _XUSB_PLL_CFG0_0, 0x534 */
    uint32_t _0x538;
    uint32_t clk_cpu_misc;          /* _CLK_CPU_MISC_0, 0x53c */
    uint32_t clk_cpug_misc;         /* _CLK_CPUG_MISC_0, 0x540 */
    uint32_t clk_cpulp_misc;        /* _CLK_CPULP_MISC_0, 0x544 */
    uint32_t pllx_hw_ctrl_cfg;      /* _PLLX_HW_CTRL_CFG_0, 0x548 */
    uint32_t pllx_sw_ramp_cfg;      /* _PLLX_SW_RAMP_CFG_0, 0x54c */
    uint32_t pllx_hw_ctrl_status;   /* _PLLX_HW_CTRL_STATUS_0, 0x550 */
    uint32_t lvl2_clk_gate_ovre;    /* _LVL2_CLK_GATE_OVRE, 0x554 */
    uint32_t super_gr3d_clk_div;    /* _SUPER_GR3D_CLK_DIVIDER_0, 0x558 */
    uint32_t spare_reg0;            /* _SPARE_REG0_0, 0x55c */
    uint32_t audio_sync_clk_dmic1;  /* _AUDIO_SYNC_CLK_DMIC1_0, 0x560 */
    uint32_t audio_sync_clk_dmic2;  /* _AUDIO_SYNC_CLK_DMIC2_0, 0x564 */

    uint32_t _0x568[2];
    uint32_t plld2_ss_cfg;          /* _PLLD2_SS_CFG,           0x570 */
    uint32_t plld2_ss_ctrl1;        /* _PLLD2_SS_CTRL1_0,       0x574 */
    uint32_t plld2_ss_ctrl2;        /* _PLLD2_SS_CTRL2_0,       0x578 */
    uint32_t _0x57c[5];

    uint32_t plldp_base;            /* _PLLDP_BASE, 0x590*/
    uint32_t plldp_misc;            /* _PLLDP_MISC, 0x594 */
    uint32_t plldp_ss_cfg;          /* _PLLDP_SS_CFG, 0x598 */
    uint32_t plldp_ss_ctrl1;        /* _PLLDP_SS_CTRL1_0, 0x59c */
    uint32_t plldp_ss_ctrl2;        /* _PLLDP_SS_CTRL2_0, 0x5a0 */
    uint32_t pllc4_base;            /* _PLLC4_BASE_0, 0x5a4 */
    uint32_t pllc4_misc;            /* _PLLC4_MISC_0, 0x5a8 */
    uint32_t _0x5ac[6];
    uint32_t clk_spare0;            /* _CLK_SPARE0_0, 0x5c4 */
    uint32_t clk_spare1;            /* _CLK_SPARE1_0, 0x5c8 */
    uint32_t gpu_isob_ctrl;         /* _GPU_ISOB_CTRL_0, 0x5cc */
    uint32_t pllc_misc2;            /* _PLLC_MISC_2_0, 0x5d0 */
    uint32_t pllc_misc3;            /* _PLLC_MISC_3_0, 0x5d4 */
    uint32_t plla_misc2;            /* _PLLA_MISC2_0, 0x5d8 */
    uint32_t _0x5dc[2];
    uint32_t pllc4_out;             /* _PLLC4_OUT_0, 0x5e4 */
    uint32_t pllmb_base;            /* _PLLMB_BASE_0, 0x5e8 */
    uint32_t pllmb_misc1;           /* _PLLMB_MISC1_0, 0x5ec */
    uint32_t pllx_misc4;            /* _PLLX_MISC_4_0, 0x5f0 */
    uint32_t pllx_misc5;            /* _PLLX_MISC_5_0, 0x5f4 */
    uint32_t _0x5f8[2];

    uint32_t clk_source_xusb_core_host;             /* _CLK_SOURCE_XUSB_CORE_HOST_0,    0x600 */
    uint32_t clk_source_xusb_falcon;                /* _CLK_SOURCE_XUSB_FALCON_0,    0x604 */
    uint32_t clk_source_xusb_fs;                    /* _CLK_SOURCE_XUSB_FS_0,    0x608 */
    uint32_t clk_source_xusb_core_dev;              /* _CLK_SOURCE_XUSB_CORE_DEV_0,    0x60c */
    uint32_t clk_source_xusb_ss;                    /* _CLK_SOURCE_XUSB_SS_0,    0x610 */
    uint32_t clk_source_cilab;                      /* _CLK_SOURCE_CILAB_0,    0x614 */
    uint32_t clk_source_cilcd;                      /* _CLK_SOURCE_CILCD_0,    0x618 */
    uint32_t clk_source_cilef;                      /* _CLK_SOURCE_CILEF_0,    0x61c */
    uint32_t clk_source_dsia_lp;                    /* _CLK_SOURCE_DSIA_LP_0,    0x620 */
    uint32_t clk_source_dsib_lp;                    /* _CLK_SOURCE_DSIB_LP_0,    0x624 */
    uint32_t clk_source_entropy;                    /* _CLK_SOURCE_ENTROPY_0,    0x628 */
    uint32_t clk_source_dvfs_ref;                   /* _CLK_SOURCE_DVFS_REF_0,    0x62c */
    uint32_t clk_source_dvfs_soc;                   /* _CLK_SOURCE_DVFS_SOC_0,    0x630 */
    uint32_t _0x634[3];
    uint32_t clk_source_emc_latency;                /* _CLK_SOURCE_EMC_LATENCY_0,    0x640 */
    uint32_t clk_source_soc_therm;                  /* _CLK_SOURCE_SOC_THERM_0,    0x644 */
    uint32_t _0x648;
    uint32_t clk_source_dmic1;                      /* _CLK_SOURCE_DMIC1_0,    0x64c */
    uint32_t clk_source_dmic2;                      /* _CLK_SOURCE_DMIC2_0,    0x650 */
    uint32_t _0x654;
    uint32_t clk_source_vi_sensor2;                 /* _CLK_SOURCE_VI_SENSOR2_0,    0x658 */
    uint32_t clk_source_i2c6;                       /* _CLK_SOURCE_I2C6_0,    0x65c */
    uint32_t clk_source_mipibif;                    /* _CLK_SOURCE_MIPIBIF_0,    0x660 */
    uint32_t clk_source_emc_dll;                    /* _CLK_SOURCE_EMC_DLL_0,    0x664 */
    uint32_t _0x668;
    uint32_t clk_source_uart_fst_mipi_cal;          /* _CLK_SOURCE_UART_FST_MIPI_CAL_0,    0x66c */
    uint32_t _0x670[2];
    uint32_t clk_source_vic;                        /* _CLK_SOURCE_VIC_0,    0x678 */

    uint32_t pllp_outc;                             /* _PLLP_OUTC_0, 0x67c */
    uint32_t pllp_misc1;                            /* _PLLP_MISC1_0, 0x680 */
    uint32_t _0x684[2];
    uint32_t emc_div_clk_shaper_ctrl;               /* _EMC_DIV_CLK_SHAPER_CTRL_0, 0x68c */
    uint32_t emc_pllc_shaper_ctrl;                  /* _EMC_PLLC_SHAPER_CTRL_0, 0x690 */

    uint32_t clk_source_sdmmc_legacy_tm;            /* _CLK_SOURCE_SDMMC_LEGACY_TM_0,    0x694 */
    uint32_t clk_source_nvdec;                      /* _CLK_SOURCE_NVDEC_0,    0x698 */
    uint32_t clk_source_nvjpg;                      /* _CLK_SOURCE_NVJPG_0,    0x69c */
    uint32_t clk_source_nvenc;                      /* _CLK_SOURCE_NVENC_0,    0x6a0 */

    uint32_t plla1_base;                            /* _PLLA1_BASE_0, 0x6a4 */
    uint32_t plla1_misc0;                           /* _PLLA1_MISC_0_0, 0x6a8 */
    uint32_t plla1_misc1;                           /* _PLLA1_MISC_1_0, 0x6ac */
    uint32_t plla1_misc2;                           /* _PLLA1_MISC_2_0, 0x6b0 */
    uint32_t plla1_misc3;                           /* _PLLA1_MISC_3_0, 0x6b4 */
    uint32_t audio_sync_clk_dmic3;                  /* _AUDIO_SYNC_CLK_DMIC3_0, 0x6b8 */

    uint32_t clk_source_dmic3;                      /* _CLK_SOURCE_DMIC3_0,    0x6bc */
    uint32_t clk_source_ape;                        /* _CLK_SOURCE_APE_0,    0x6c0 */
    uint32_t clk_source_qspi;                       /* _CLK_SOURCE_QSPI_0,    0x6c4 */
    uint32_t clk_source_vi_i2c;                     /* _CLK_SOURCE_VI_I2C_0,    0x6c8 */
    uint32_t clk_source_usb2_hsic_trk;              /* _CLK_SOURCE_USB2_HSIC_TRK_0,    0x6cc */
    uint32_t clk_source_pex_sata_usb_rx_byp;        /* _CLK_SOURCE_PEX_SATA_USB_RX_BYP_0,    0x6d0 */
    uint32_t clk_source_maud;                       /* _CLK_SOURCE_MAUD_0,    0x6d4 */
    uint32_t clk_source_tsecb;                      /* _CLK_SOURCE_TSECB_0,    0x6d8 */

    uint32_t clk_cpug_misc1;                        /* _CLK_CPUG_MISC1_0, 0x6dc */
    uint32_t aclk_burst_policy;                     /* _ACLK_BURST_POLICY_0, 0x6e0 */
    uint32_t super_aclk_divider;                    /* _SUPER_ACLK_DIVIDER_0, 0x6e4 */

    uint32_t nvenc_super_clk_divider;               /* _NVENC_SUPER_CLK_DIVIDER_0, 0x6e8 */
    uint32_t vi_super_clk_divider;                  /* _VI_SUPER_CLK_DIVIDER_0, 0x6ec */
    uint32_t vic_super_clk_divider;                 /* _VIC_SUPER_CLK_DIVIDER_0, 0x6f0 */
    uint32_t nvdec_super_clk_divider;               /* _NVDEC_SUPER_CLK_DIVIDER_0, 0x6f4 */
    uint32_t isp_super_clk_divider;                 /* _ISP_SUPER_CLK_DIVIDER_0, 0x6f8 */
    uint32_t ispb_super_clk_divider;                /* _ISPB_SUPER_CLK_DIVIDER_0, 0x6fc */
    uint32_t nvjpg_super_clk_divider;               /* _NVJPG_SUPER_CLK_DIVIDER_0, 0x700 */
    uint32_t se_super_clk_divider;                  /* _SE_SUPER_CLK_DIVIDER_0, 0x704 */
    uint32_t tsec_super_clk_divider;                /* _TSEC_SUPER_CLK_DIVIDER_0, 0x708 */
    uint32_t tsecb_super_clk_divider;               /* _TSECB_SUPER_CLK_DIVIDER_0, 0x70c */

    uint32_t clk_source_uartape;                    /* _CLK_SOURCE_UARTAPE_0, 0x710 */
    uint32_t clk_cpug_misc2;                        /* _CLK_CPUG_MISC2_0, 0x714 */
    uint32_t clk_source_dbgapb;                     /* _CLK_SOURCE_DBGAPB_0, 0x718 */
    uint32_t clk_ccplex_cc4_ret_clk_enb;            /* _CLK_CCPLEX_CC4_RET_CLK_ENB_0, 0x71c */
    uint32_t actmon_cpu_clk;                        /* _ACTMON_CPU_CLK_0, 0x720 */
    uint32_t clk_source_emc_safe;                   /* _CLK_SOURCE_EMC_SAFE_0, 0x724 */
    uint32_t sdmmc2_pllc4_out0_shaper_ctrl;         /* _SDMMC2_PLLC4_OUT0_SHAPER_CTRL_0, 0x728 */
    uint32_t sdmmc2_pllc4_out1_shaper_ctrl;         /* _SDMMC2_PLLC4_OUT1_SHAPER_CTRL_0, 0x72c */
    uint32_t sdmmc2_pllc4_out2_shaper_ctrl;         /* _SDMMC2_PLLC4_OUT2_SHAPER_CTRL_0, 0x730 */
    uint32_t sdmmc2_div_clk_shaper_ctrl;            /* _SDMMC2_DIV_CLK_SHAPER_CTRL_0, 0x734 */
    uint32_t sdmmc4_pllc4_out0_shaper_ctrl;         /* _SDMMC4_PLLC4_OUT0_SHAPER_CTRL_0, 0x738 */
    uint32_t sdmmc4_pllc4_out1_shaper_ctrl;         /* _SDMMC4_PLLC4_OUT1_SHAPER_CTRL_0, 0x73c */
    uint32_t sdmmc4_pllc4_out2_shaper_ctrl;         /* _SDMMC4_PLLC4_OUT2_SHAPER_CTRL_0, 0x740 */
    uint32_t sdmmc4_div_clk_shaper_ctrl;            /* _SDMMC4_DIV_CLK_SHAPER_CTRL_0, 0x744 */    
} tegra_car_t;

static inline volatile tegra_car_t *car_get_regs(void) {
    return (volatile tegra_car_t *)CAR_BASE;
}

void clk_enable(CarDevice dev);
void clk_disable(CarDevice dev);
void rst_enable(CarDevice dev);
void rst_disable(CarDevice dev);

void clkrst_enable(CarDevice dev);
void clkrst_disable(CarDevice dev);
void clkrst_reboot(CarDevice dev);

void clkrst_enable_fuse_regs(bool enable);

#endif
