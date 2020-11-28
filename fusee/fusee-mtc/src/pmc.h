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
 
#ifndef FUSEE_PMC_H
#define FUSEE_PMC_H

#include <stdint.h>

#define PMC_BASE 0x7000E400
#define MAKE_PMC_REG(n) MAKE_REG32(PMC_BASE + n)

#define PMC_CONTROL_SDMMC1      (1 << 12)
#define PMC_CONTROL_SDMMC3      (1 << 13)
#define PMC_CONTROL_SDMMC4      (1 << 14)

#define APBDEV_PMC_CONTROL MAKE_PMC_REG(0x00)
#define APBDEV_PM_0 MAKE_PMC_REG(0x14)
#define APBDEV_PMC_DPD_ENABLE_0 MAKE_PMC_REG(0x24)
#define APBDEV_PMC_PWRGATE_TOGGLE_0 MAKE_PMC_REG(0x30)
#define APBDEV_PMC_PWRGATE_STATUS_0 MAKE_PMC_REG(0x38)
#define APBDEV_PMC_NO_IOPOWER_0 MAKE_PMC_REG(0x44)
#define APBDEV_PMC_SCRATCH0_0 MAKE_PMC_REG(0x50)
#define APBDEV_PMC_SCRATCH1_0 MAKE_PMC_REG(0x54)
#define APBDEV_PMC_SCRATCH20_0 MAKE_PMC_REG(0xA0)
#define APBDEV_PMC_PWR_DET_VAL_0 MAKE_PMC_REG(0xE4)
#define APBDEV_PMC_DDR_PWR_0 MAKE_PMC_REG(0xE8)
#define APBDEV_PMC_CRYPTO_OP_0 MAKE_PMC_REG(0xF4)
#define APBDEV_PMC_WAKE2_STATUS_0 MAKE_PMC_REG(0x168)
#define APBDEV_PMC_OSC_EDPD_OVER_0 MAKE_PMC_REG(0x1A4)
#define APBDEV_PMC_RST_STATUS_0 MAKE_PMC_REG(0x1B4)
#define APBDEV_PMC_IO_DPD_REQ_0 MAKE_PMC_REG(0x1B8)
#define APBDEV_PMC_IO_DPD2_REQ_0 MAKE_PMC_REG(0x1C0)
#define APBDEV_PMC_VDDP_SEL_0 MAKE_PMC_REG(0x1CC)
#define APBDEV_PMC_SCRATCH49_0 MAKE_PMC_REG(0x244)
#define APBDEV_PMC_TSC_MULT_0 MAKE_PMC_REG(0x2B4)
#define APBDEV_PMC_REG_SHORT_0 MAKE_PMC_REG(0x2CC)
#define APBDEV_PMC_WEAK_BIAS_0 MAKE_PMC_REG(0x2C8)
#define APBDEV_PMC_SECURE_SCRATCH21_0 MAKE_PMC_REG(0x334)
#define APBDEV_PMC_SECURE_SCRATCH32_0 MAKE_PMC_REG(0x360)
#define APBDEV_PMC_SECURE_SCRATCH49_0 MAKE_PMC_REG(0x3A4)
#define APBDEV_PMC_CNTRL2_0 MAKE_PMC_REG(0x440)
#define APBDEV_PMC_IO_DPD4_REQ_0 MAKE_PMC_REG(0x464)
#define APBDEV_PMC_UTMIP_PAD_CFG1_0 MAKE_PMC_REG(0x4C4)
#define APBDEV_PMC_UTMIP_PAD_CFG3_0 MAKE_PMC_REG(0x4CC)
#define APBDEV_PMC_DDR_CNTRL_0 MAKE_PMC_REG(0x4E4)
#define APBDEV_PMC_SCRATCH43_0 MAKE_PMC_REG(0x22C)
#define APBDEV_PMC_SCRATCH188_0 MAKE_PMC_REG(0x810)
#define APBDEV_PMC_SCRATCH190_0 MAKE_PMC_REG(0x818)
#define APBDEV_PMC_SCRATCH200_0 MAKE_PMC_REG(0x840)

#define APBDEV_PMC_SCRATCH45_0 MAKE_PMC_REG(0x234)
#define APBDEV_PMC_SCRATCH46_0 MAKE_PMC_REG(0x238)
#define APBDEV_PMC_SCRATCH33_0 MAKE_PMC_REG(0x120)
#define APBDEV_PMC_SCRATCH40_0 MAKE_PMC_REG(0x13C)

/* Power Management Controller (APBDEV_PMC_) regs */
typedef struct {
    uint32_t cntrl;                             /* _CNTRL_0,                        0x00 */
    uint32_t sec_disable;                       /* _SEC_DISABLE_0,                  0x04 */
    uint32_t pmc_swrst;                         /* _PMC_SWRST_0,                    0x08 */
    uint32_t wake_mask;                         /* _WAKE_MASK_0,                    0x0c */
    uint32_t wake_lvl;                          /* _WAKE_LVL_0,                     0x10 */
    uint32_t wake_status;                       /* _WAKE_STATUS_0,                  0x14 */
    uint32_t sw_wake_status;                    /* _SW_WAKE_STATUS_0,               0x18 */
    uint32_t dpd_pads_oride;                    /* _DPD_PADS_ORIDE_0,               0x1c */
    uint32_t dpd_sample;                        /* _DPD_SAMPLE_0,                   0x20 */
    uint32_t dpd_enable;                        /* _DPD_ENABLE_0,                   0x24 */
    uint32_t pwrgate_timer_off;                 /* _PWRGATE_TIMER_OFF_0,            0x28 */
    uint32_t clamp_status;                      /* _CLAMP_STATUS_0,                 0x2c */
    uint32_t pwrgate_toggle;                    /* _PWRGATE_TOGGLE_0,               0x30 */
    uint32_t remove_clamping;                   /* _REMOVE_CLAMPING_0,              0x34 */
    uint32_t pwrgate_status;                    /* _PWRGATE_STATUS_0,               0x38 */
    uint32_t pwrgood_timer;                     /* _PWRGOOD_TIMER_0,                0x3c */
    uint32_t blink_timer;                       /* _BLINK_TIMER_0,                  0x40 */
    uint32_t no_iopower;                        /* _NO_IOPOWER_0,                   0x44 */
    uint32_t pwr_det;                           /* _PWR_DET_0,                      0x48 */
    uint32_t pwr_det_latch;                     /* _PWR_DET_LATCH_0,                0x4c */
    uint32_t scratch0;                          /* _SCRATCH0_0,                     0x50 */
    uint32_t scratch1;                          /* _SCRATCH1_0,                     0x54 */
    uint32_t scratch2;                          /* _SCRATCH2_0,                     0x58 */
    uint32_t scratch3;                          /* _SCRATCH3_0,                     0x5c */
    uint32_t scratch4;                          /* _SCRATCH4_0,                     0x60 */
    uint32_t scratch5;                          /* _SCRATCH5_0,                     0x64 */
    uint32_t scratch6;                          /* _SCRATCH6_0,                     0x68 */
    uint32_t scratch7;                          /* _SCRATCH7_0,                     0x6c */
    uint32_t scratch8;                          /* _SCRATCH8_0,                     0x70 */
    uint32_t scratch9;                          /* _SCRATCH9_0,                     0x74 */
    uint32_t scratch10;                         /* _SCRATCH10_0,                    0x78 */
    uint32_t scratch11;                         /* _SCRATCH11_0,                    0x7c */
    uint32_t scratch12;                         /* _SCRATCH12_0,                    0x80 */
    uint32_t scratch13;                         /* _SCRATCH13_0,                    0x84 */
    uint32_t scratch14;                         /* _SCRATCH14_0,                    0x88 */
    uint32_t scratch15;                         /* _SCRATCH15_0,                    0x8c */
    uint32_t scratch16;                         /* _SCRATCH16_0,                    0x90 */
    uint32_t scratch17;                         /* _SCRATCH17_0,                    0x94 */
    uint32_t scratch18;                         /* _SCRATCH18_0,                    0x98 */
    uint32_t scratch19;                         /* _SCRATCH19_0,                    0x9c */
    uint32_t scratch20;                         /* _SCRATCH20_0,                    0xa0 */
    uint32_t scratch21;                         /* _SCRATCH21_0,                    0xa4 */
    uint32_t scratch22;                         /* _SCRATCH22_0,                    0xa8 */
    uint32_t scratch23;                         /* _SCRATCH23_0,                    0xac */
    uint32_t secure_scratch0;                   /* _SECURE_SCRATCH0_0,              0xb0 */
    uint32_t secure_scratch1;                   /* _SECURE_SCRATCH1_0,              0xb4 */
    uint32_t secure_scratch2;                   /* _SECURE_SCRATCH2_0,              0xb8 */
    uint32_t secure_scratch3;                   /* _SECURE_SCRATCH3_0,              0xbc */
    uint32_t secure_scratch4;                   /* _SECURE_SCRATCH4_0,              0xc0 */
    uint32_t secure_scratch5;                   /* _SECURE_SCRATCH5_0,              0xc4 */
    uint32_t cpupwrgood_timer;                  /* _CPUPWRGOOD_TIMER_0,             0xc8 */
    uint32_t cpupwroff_timer;                   /* _CPUPWROFF_TIMER_0,              0xcc */
    uint32_t pg_mask;                           /* _PG_MASK_0,                      0xd0 */
    uint32_t pg_mask_1;                         /* _PG_MASK_1_0,                    0xd4 */
    uint32_t auto_wake_lvl;                     /* _AUTO_WAKE_LVL_0,                0xd8 */
    uint32_t auto_wake_lvl_mask;                /* _AUTO_WAKE_LVL_MASK_0,           0xdc */
    uint32_t wake_delay;                        /* _WAKE_DELAY_0,                   0xe0 */
    uint32_t pwr_det_val;                       /* _PWR_DET_VAL_0,                  0xe4 */
    uint32_t ddr_pwr;                           /* _DDR_PWR_0,                      0xe8 */
    uint32_t usb_debounce_del;                  /* _USB_DEBOUNCE_DEL_0,             0xec */
    uint32_t usb_ao;                            /* _USB_AO_0,                       0xf0 */
    uint32_t crypto_op;                         /* _CRYPTO_OP_0,                    0xf4 */
    uint32_t pllp_wb0_override;                 /* _PLLP_WB0_OVERRIDE_0,            0xf8 */
    uint32_t scratch24;                         /* _SCRATCH24_0,                    0xfc */
    uint32_t scratch25;                         /* _SCRATCH25_0,                    0x100 */
    uint32_t scratch26;                         /* _SCRATCH26_0,                    0x104 */
    uint32_t scratch27;                         /* _SCRATCH27_0,                    0x108 */
    uint32_t scratch28;                         /* _SCRATCH28_0,                    0x10c */
    uint32_t scratch29;                         /* _SCRATCH29_0,                    0x110 */
    uint32_t scratch30;                         /* _SCRATCH30_0,                    0x114 */
    uint32_t scratch31;                         /* _SCRATCH31_0,                    0x118 */
    uint32_t scratch32;                         /* _SCRATCH32_0,                    0x11c */
    uint32_t scratch33;                         /* _SCRATCH33_0,                    0x120 */
    uint32_t scratch34;                         /* _SCRATCH34_0,                    0x124 */
    uint32_t scratch35;                         /* _SCRATCH35_0,                    0x128 */
    uint32_t scratch36;                         /* _SCRATCH36_0,                    0x12c */
    uint32_t scratch37;                         /* _SCRATCH37_0,                    0x130 */
    uint32_t scratch38;                         /* _SCRATCH38_0,                    0x134 */
    uint32_t scratch39;                         /* _SCRATCH39_0,                    0x138 */
    uint32_t scratch40;                         /* _SCRATCH40_0,                    0x13c */
    uint32_t scratch41;                         /* _SCRATCH41_0,                    0x140 */
    uint32_t scratch42;                         /* _SCRATCH42_0,                    0x144 */
    uint32_t bondout_mirror0;                   /* _BONDOUT_MIRROR0_0,              0x148 */
    uint32_t bondout_mirror1;                   /* _BONDOUT_MIRROR1_0,              0x14c */
    uint32_t bondout_mirror2;                   /* _BONDOUT_MIRROR2_0,              0x150 */
    uint32_t sys_33v_en;                        /* _SYS_33V_EN_0,                   0x154 */
    uint32_t bondout_mirror_access;             /* _BONDOUT_MIRROR_ACCESS_0,        0x158 */
    uint32_t gate;                              /* _GATE_0,                         0x15c */
    uint32_t wake2_mask;                        /* _WAKE2_MASK_0,                   0x160 */
    uint32_t wake2_lvl;                         /* _WAKE2_LVL_0,                    0x164 */
    uint32_t wake2_status;                      /* _WAKE2_STATUS_0,                 0x168 */
    uint32_t sw_wake2_status;                   /* _SW_WAKE2_STATUS_0,              0x16c */
    uint32_t auto_wake2_lvl_mask;               /* _AUTO_WAKE2_LVL_MASK_0,          0x170 */
    uint32_t pg_mask_2;                         /* _PG_MASK_2_0,                    0x174 */
    uint32_t pg_mask_ce1;                       /* _PG_MASK_CE1_0,                  0x178 */
    uint32_t pg_mask_ce2;                       /* _PG_MASK_CE2_0,                  0x17c */
    uint32_t pg_mask_ce3;                       /* _PG_MASK_CE3_0,                  0x180 */
    uint32_t pwrgate_timer_ce_0;                /* _PWRGATE_TIMER_CE_0_0,           0x184 */
    uint32_t pwrgate_timer_ce_1;                /* _PWRGATE_TIMER_CE_1_0,           0x188 */
    uint32_t pwrgate_timer_ce_2;                /* _PWRGATE_TIMER_CE_2_0,           0x18c */
    uint32_t pwrgate_timer_ce_3;                /* _PWRGATE_TIMER_CE_3_0,           0x190 */
    uint32_t pwrgate_timer_ce_4;                /* _PWRGATE_TIMER_CE_4_0,           0x194 */
    uint32_t pwrgate_timer_ce_5;                /* _PWRGATE_TIMER_CE_5_0,           0x198 */
    uint32_t pwrgate_timer_ce_6;                /* _PWRGATE_TIMER_CE_6_0,           0x19c */
    uint32_t pcx_edpd_cntrl;                    /* _PCX_EDPD_CNTRL_0,               0x1a0 */
    uint32_t osc_edpd_over;                     /* _OSC_EDPD_OVER_0,                0x1a4 */
    uint32_t clk_out_cntrl;                     /* _CLK_OUT_CNTRL_0,                0x1a8 */
    uint32_t sata_pwrgt;                        /* _SATA_PWRGT_0,                   0x1ac */
    uint32_t sensor_ctrl;                       /* _SENSOR_CTRL_0,                  0x1b0 */
    uint32_t rst_status;                        /* _RST_STATUS_0,                   0x1b4 */
    uint32_t io_dpd_req;                        /* _IO_DPD_REQ_0,                   0x1b8 */
    uint32_t io_dpd_status;                     /* _IO_DPD_STATUS_0,                0x1bc */
    uint32_t io_dpd2_req;                       /* _IO_DPD2_REQ_0,                  0x1c0 */
    uint32_t io_dpd2_status;                    /* _IO_DPD2_STATUS_0,               0x1c4 */
    uint32_t sel_dpd_tim;                       /* _SEL_DPD_TIM_0,                  0x1c8 */
    uint32_t vddp_sel;                          /* _VDDP_SEL_0,                     0x1cc */
    uint32_t ddr_cfg;                           /* _DDR_CFG_0,                      0x1d0 */
    uint32_t _0x1d4[2];
    uint32_t pllm_wb0_override_freq;            /* _PLLM_WB0_OVERRIDE_FREQ_0,       0x1dc */
    uint32_t _0x1e0;
    uint32_t pwrgate_timer_mult;                /* _PWRGATE_TIMER_MULT_0,           0x1e4 */
    uint32_t dsi_sel_dpd;                       /* _DSI_SEL_DPD_0,                  0x1e8 */
    uint32_t utmip_uhsic_triggers;              /* _UTMIP_UHSIC_TRIGGERS_0,         0x1ec */
    uint32_t utmip_uhsic_saved_state;           /* _UTMIP_UHSIC_SAVED_STATE_0,      0x1f0 */
    uint32_t _0x1f4;
    uint32_t utmip_term_pad_cfg;                /* _UTMIP_TERM_PAD_CFG_0,           0x1f8 */
    uint32_t utmip_uhsic_sleep_cfg;             /* _UTMIP_UHSIC_SLEEP_CFG_0,        0x1fc */
    uint32_t utmip_uhsic_sleepwalk_cfg;         /* _UTMIP_UHSIC_SLEEPWALK_CFG_0,    0x200 */
    uint32_t utmip_sleepwalk_p0;                /* _UTMIP_SLEEPWALK_P0_0,           0x204 */
    uint32_t utmip_sleepwalk_p1;                /* _UTMIP_SLEEPWALK_P1_0,           0x208 */
    uint32_t utmip_sleepwalk_p2;                /* _UTMIP_SLEEPWALK_P2_0,           0x20c */
    uint32_t uhsic_sleepwalk_p0;                /* _UHSIC_SLEEPWALK_P0_0,           0x210 */
    uint32_t utmip_uhsic_status;                /* _UTMIP_UHSIC_STATUS_0,           0x214 */
    uint32_t utmip_uhsic_fake;                  /* _UTMIP_UHSIC_FAKE_0,             0x218 */
    uint32_t bondout_mirror3;                   /* _BONDOUT_MIRROR3_0,              0x21c */
    uint32_t bondout_mirror4;                   /* _BONDOUT_MIRROR4_0,              0x220 */
    uint32_t secure_scratch6;                   /* _SECURE_SCRATCH6_0,              0x224 */
    uint32_t secure_scratch7;                   /* _SECURE_SCRATCH7_0,              0x228 */
    uint32_t scratch43;                         /* _SCRATCH43_0,                    0x22c */
    uint32_t scratch44;                         /* _SCRATCH44_0,                    0x230 */
    uint32_t scratch45;                         /* _SCRATCH45_0,                    0x234 */
    uint32_t scratch46;                         /* _SCRATCH46_0,                    0x238 */
    uint32_t scratch47;                         /* _SCRATCH47_0,                    0x23c */
    uint32_t scratch48;                         /* _SCRATCH48_0,                    0x240 */
    uint32_t scratch49;                         /* _SCRATCH49_0,                    0x244 */
    uint32_t scratch50;                         /* _SCRATCH50_0,                    0x248 */
    uint32_t scratch51;                         /* _SCRATCH51_0,                    0x24c */
    uint32_t scratch52;                         /* _SCRATCH52_0,                    0x250 */
    uint32_t scratch53;                         /* _SCRATCH53_0,                    0x254 */
    uint32_t scratch54;                         /* _SCRATCH54_0,                    0x258 */
    uint32_t scratch55;                         /* _SCRATCH55_0,                    0x25c */
    uint32_t scratch0_eco;                      /* _SCRATCH0_ECO_0,                 0x260 */
    uint32_t por_dpd_ctrl;                      /* _POR_DPD_CTRL_0,                 0x264 */
    uint32_t scratch2_eco;                      /* _SCRATCH2_ECO_0,                 0x268 */
    uint32_t utmip_uhsic_line_wakeup;           /* _UTMIP_UHSIC_LINE_WAKEUP_0,      0x26c */
    uint32_t utmip_bias_master_cntrl;           /* _UTMIP_BIAS_MASTER_CNTRL_0,      0x270 */
    uint32_t utmip_master_config;               /* _UTMIP_MASTER_CONFIG_0,          0x274 */
    uint32_t td_pwrgate_inter_part_timer;       /* _TD_PWRGATE_INTER_PART_TIMER_0,  0x278 */
    uint32_t utmip_uhsic2_triggers;             /* _UTMIP_UHSIC2_TRIGGERS_0,        0x27c */
    uint32_t utmip_uhsic2_saved_state;          /* _UTMIP_UHSIC2_SAVED_STATE_0,     0x280 */
    uint32_t utmip_uhsic2_sleep_cfg;            /* _UTMIP_UHSIC2_SLEEP_CFG_0,       0x284 */
    uint32_t utmip_uhsic2_sleepwalk_cfg;        /* _UTMIP_UHSIC2_SLEEPWALK_CFG_0,   0x288 */
    uint32_t uhsic2_sleepwalk_p1;               /* _UHSIC2_SLEEPWALK_P1_0,          0x28c */
    uint32_t utmip_uhsic2_status;               /* _UTMIP_UHSIC2_STATUS_0,          0x290 */
    uint32_t utmip_uhsic2_fake;                 /* _UTMIP_UHSIC2_FAKE_0,            0x294 */
    uint32_t utmip_uhsic2_line_wakeup;          /* _UTMIP_UHSIC2_LINE_WAKEUP_0,     0x298 */
    uint32_t utmip_master2_config;              /* _UTMIP_MASTER2_CONFIG_0,         0x29c */
    uint32_t utmip_uhsic_rpd_cfg;               /* _UTMIP_UHSIC_RPD_CFG_0,          0x2a0 */
    uint32_t pg_mask_ce0;                       /* _PG_MASK_CE0_0,                  0x2a4 */
    uint32_t pg_mask_3;                         /* _PG_MASK_3_0,                    0x2a8 */
    uint32_t pg_mask_4;                         /* _PG_MASK_4_0,                    0x2ac */
    uint32_t pllm_wb0_override2;                /* _PLLM_WB0_OVERRIDE2_0,           0x2b0 */
    uint32_t tsc_mult;                          /* _TSC_MULT_0,                     0x2b4 */
    uint32_t cpu_vsense_override;               /* _CPU_VSENSE_OVERRIDE_0,          0x2b8 */
    uint32_t glb_amap_cfg;                      /* _GLB_AMAP_CFG_0,                 0x2bc */
    uint32_t sticky_bits;                       /* _STICKY_BITS_0,                  0x2c0 */
    uint32_t sec_disable2;                      /* _SEC_DISABLE2_0,                 0x2c4 */
    uint32_t weak_bias;                         /* _WEAK_BIAS_0,                    0x2c8 */
    uint32_t reg_short;                         /* _REG_SHORT_0,                    0x2cc */
    uint32_t pg_mask_andor;                     /* _PG_MASK_ANDOR_0,                0x2d0 */
    uint32_t gpu_rg_cntrl;                      /* _GPU_RG_CNTRL_0,                 0x2d4 */
    uint32_t sec_disable3;                      /* _SEC_DISABLE3_0,                 0x2d8 */
    uint32_t pg_mask_5;                         /* _PG_MASK_5_0,                    0x2dc */
    uint32_t pg_mask_6;                         /* _PG_MASK_6_0,                    0x2e0 */
    uint32_t _0x2e4[7];
    uint32_t secure_scratch8;                   /* _SECURE_SCRATCH8_0,              0x300 */
    uint32_t secure_scratch9;                   /* _SECURE_SCRATCH9_0,              0x304 */
    uint32_t secure_scratch10;                  /* _SECURE_SCRATCH10_0,             0x308 */
    uint32_t secure_scratch11;                  /* _SECURE_SCRATCH11_0,             0x30c */
    uint32_t secure_scratch12;                  /* _SECURE_SCRATCH12_0,             0x310 */
    uint32_t secure_scratch13;                  /* _SECURE_SCRATCH13_0,             0x314 */
    uint32_t secure_scratch14;                  /* _SECURE_SCRATCH14_0,             0x318 */
    uint32_t secure_scratch15;                  /* _SECURE_SCRATCH15_0,             0x31c */
    uint32_t secure_scratch16;                  /* _SECURE_SCRATCH16_0,             0x320 */
    uint32_t secure_scratch17;                  /* _SECURE_SCRATCH17_0,             0x324 */
    uint32_t secure_scratch18;                  /* _SECURE_SCRATCH18_0,             0x328 */
    uint32_t secure_scratch19;                  /* _SECURE_SCRATCH19_0,             0x32c */
    uint32_t secure_scratch20;                  /* _SECURE_SCRATCH20_0,             0x330 */
    uint32_t secure_scratch21;                  /* _SECURE_SCRATCH21_0,             0x334 */
    uint32_t secure_scratch22;                  /* _SECURE_SCRATCH22_0,             0x338 */
    uint32_t secure_scratch23;                  /* _SECURE_SCRATCH23_0,             0x33c */
    uint32_t secure_scratch24;                  /* _SECURE_SCRATCH24_0,             0x340 */
    uint32_t secure_scratch25;                  /* _SECURE_SCRATCH25_0,             0x344 */
    uint32_t secure_scratch26;                  /* _SECURE_SCRATCH26_0,             0x348 */
    uint32_t secure_scratch27;                  /* _SECURE_SCRATCH27_0,             0x34c */
    uint32_t secure_scratch28;                  /* _SECURE_SCRATCH28_0,             0x350 */
    uint32_t secure_scratch29;                  /* _SECURE_SCRATCH29_0,             0x354 */
    uint32_t secure_scratch30;                  /* _SECURE_SCRATCH30_0,             0x358 */
    uint32_t secure_scratch31;                  /* _SECURE_SCRATCH31_0,             0x35c */
    uint32_t secure_scratch32;                  /* _SECURE_SCRATCH32_0,             0x360 */
    uint32_t secure_scratch33;                  /* _SECURE_SCRATCH33_0,             0x364 */
    uint32_t secure_scratch34;                  /* _SECURE_SCRATCH34_0,             0x368 */
    uint32_t secure_scratch35;                  /* _SECURE_SCRATCH35_0,             0x36c */
    uint32_t secure_scratch36;                  /* _SECURE_SCRATCH36_0,             0x370 */
    uint32_t secure_scratch37;                  /* _SECURE_SCRATCH37_0,             0x374 */
    uint32_t secure_scratch38;                  /* _SECURE_SCRATCH38_0,             0x378 */
    uint32_t secure_scratch39;                  /* _SECURE_SCRATCH39_0,             0x37c */
    uint32_t secure_scratch40;                  /* _SECURE_SCRATCH40_0,             0x380 */
    uint32_t secure_scratch41;                  /* _SECURE_SCRATCH41_0,             0x384 */
    uint32_t secure_scratch42;                  /* _SECURE_SCRATCH42_0,             0x388 */
    uint32_t secure_scratch43;                  /* _SECURE_SCRATCH43_0,             0x38c */
    uint32_t secure_scratch44;                  /* _SECURE_SCRATCH44_0,             0x390 */
    uint32_t secure_scratch45;                  /* _SECURE_SCRATCH45_0,             0x394 */
    uint32_t secure_scratch46;                  /* _SECURE_SCRATCH46_0,             0x398 */
    uint32_t secure_scratch47;                  /* _SECURE_SCRATCH47_0,             0x39c */
    uint32_t secure_scratch48;                  /* _SECURE_SCRATCH48_0,             0x3a0 */
    uint32_t secure_scratch49;                  /* _SECURE_SCRATCH49_0,             0x3a4 */
    uint32_t secure_scratch50;                  /* _SECURE_SCRATCH50_0,             0x3a8 */
    uint32_t secure_scratch51;                  /* _SECURE_SCRATCH51_0,             0x3ac */
    uint32_t secure_scratch52;                  /* _SECURE_SCRATCH52_0,             0x3b0 */
    uint32_t secure_scratch53;                  /* _SECURE_SCRATCH53_0,             0x3b4 */
    uint32_t secure_scratch54;                  /* _SECURE_SCRATCH54_0,             0x3b8 */
    uint32_t secure_scratch55;                  /* _SECURE_SCRATCH55_0,             0x3bc */
    uint32_t secure_scratch56;                  /* _SECURE_SCRATCH56_0,             0x3c0 */
    uint32_t secure_scratch57;                  /* _SECURE_SCRATCH57_0,             0x3c4 */
    uint32_t secure_scratch58;                  /* _SECURE_SCRATCH58_0,             0x3c8 */
    uint32_t secure_scratch59;                  /* _SECURE_SCRATCH59_0,             0x3cc */
    uint32_t secure_scratch60;                  /* _SECURE_SCRATCH60_0,             0x3d0 */
    uint32_t secure_scratch61;                  /* _SECURE_SCRATCH61_0,             0x3d4 */
    uint32_t secure_scratch62;                  /* _SECURE_SCRATCH62_0,             0x3d8 */
    uint32_t secure_scratch63;                  /* _SECURE_SCRATCH63_0,             0x3dc */
    uint32_t secure_scratch64;                  /* _SECURE_SCRATCH64_0,             0x3e0 */
    uint32_t secure_scratch65;                  /* _SECURE_SCRATCH65_0,             0x3e4 */
    uint32_t secure_scratch66;                  /* _SECURE_SCRATCH66_0,             0x3e8 */
    uint32_t secure_scratch67;                  /* _SECURE_SCRATCH67_0,             0x3ec */
    uint32_t secure_scratch68;                  /* _SECURE_SCRATCH68_0,             0x3f0 */
    uint32_t secure_scratch69;                  /* _SECURE_SCRATCH69_0,             0x3f4 */
    uint32_t secure_scratch70;                  /* _SECURE_SCRATCH70_0,             0x3f8 */
    uint32_t secure_scratch71;                  /* _SECURE_SCRATCH71_0,             0x3fc */
    uint32_t secure_scratch72;                  /* _SECURE_SCRATCH72_0,             0x400 */
    uint32_t secure_scratch73;                  /* _SECURE_SCRATCH73_0,             0x404 */
    uint32_t secure_scratch74;                  /* _SECURE_SCRATCH74_0,             0x408 */
    uint32_t secure_scratch75;                  /* _SECURE_SCRATCH75_0,             0x40c */
    uint32_t secure_scratch76;                  /* _SECURE_SCRATCH76_0,             0x410 */
    uint32_t secure_scratch77;                  /* _SECURE_SCRATCH77_0,             0x414 */
    uint32_t secure_scratch78;                  /* _SECURE_SCRATCH78_0,             0x418 */
    uint32_t secure_scratch79;                  /* _SECURE_SCRATCH79_0,             0x41c */
    uint32_t _0x420[8];
    uint32_t cntrl2;                            /* _CNTRL2_0,                       0x440 */
    uint32_t io_dpd_off_mask;                   /* _IO_DPD_OFF_MASK_0,              0x444 */
    uint32_t io_dpd2_off_mask;                  /* _IO_DPD2_OFF_MASK_0,             0x448 */
    uint32_t event_counter;                     /* _EVENT_COUNTER_0,                0x44c */
    uint32_t fuse_control;                      /* _FUSE_CONTROL_0,                 0x450 */
    uint32_t scratch1_eco;                      /* _SCRATCH1_ECO_0,                 0x454 */
    uint32_t _0x458;
    uint32_t io_dpd3_req;                       /* _IO_DPD3_REQ_0,                  0x45c */
    uint32_t io_dpd3_status;                    /* _IO_DPD3_STATUS_0,               0x460 */
    uint32_t io_dpd4_req;                       /* _IO_DPD4_REQ_0,                  0x464 */
    uint32_t io_dpd4_status;                    /* _IO_DPD4_STATUS_0,               0x468 */
    uint32_t _0x46c[2];
    uint32_t direct_thermtrip_cfg;              /* _DIRECT_THERMTRIP_CFG_0,         0x474 */
    uint32_t tsosc_delay;                       /* _TSOSC_DELAY_0,                  0x478 */
    uint32_t set_sw_clamp;                      /* _SET_SW_CLAMP_0,                 0x47c */
    uint32_t debug_authentication;              /* _DEBUG_AUTHENTICATION_0,         0x480 */
    uint32_t aotag_cfg;                         /* _AOTAG_CFG_0,                    0x484 */
    uint32_t aotag_thresh1_cfg;                 /* _AOTAG_THRESH1_CFG_0,            0x488 */
    uint32_t aotag_thresh2_cfg;                 /* _AOTAG_THRESH2_CFG_0,            0x48c */
    uint32_t aotag_thresh3_cfg;                 /* _AOTAG_THRESH3_CFG_0,            0x490 */
    uint32_t aotag_status;                      /* _AOTAG_STATUS_0,                 0x494 */
    uint32_t aotag_security;                    /* _AOTAG_SECURITY_0,               0x498 */
    uint32_t tsensor_config0;                   /* _TSENSOR_CONFIG0_0,              0x49c */
    uint32_t tsensor_config1;                   /* _TSENSOR_CONFIG1_0,              0x4a0 */
    uint32_t tsensor_config2;                   /* _TSENSOR_CONFIG2_0,              0x4a4 */
    uint32_t tsensor_status0;                   /* _TSENSOR_STATUS0_0,              0x4a8 */
    uint32_t tsensor_status1;                   /* _TSENSOR_STATUS1_0,              0x4ac */
    uint32_t tsensor_status2;                   /* _TSENSOR_STATUS2_0,              0x4b0 */
    uint32_t tsensor_pdiv;                      /* _TSENSOR_PDIV_0,                 0x4b4 */
    uint32_t aotag_intr_en;                     /* _AOTAG_INTR_EN_0,                0x4b8 */
    uint32_t aotag_intr_dis;                    /* _AOTAG_INTR_DIS_0,               0x4bc */
    uint32_t utmip_pad_cfg0;                    /* _UTMIP_PAD_CFG0_0,               0x4c0 */
    uint32_t utmip_pad_cfg1;                    /* _UTMIP_PAD_CFG1_0,               0x4c4 */
    uint32_t utmip_pad_cfg2;                    /* _UTMIP_PAD_CFG2_0,               0x4c8 */
    uint32_t utmip_pad_cfg3;                    /* _UTMIP_PAD_CFG3_0,               0x4cc */
    uint32_t utmip_uhsic_sleep_cfg1;            /* _UTMIP_UHSIC_SLEEP_CFG1_0,       0x4d0 */
    uint32_t cc4_hvc_control;                   /* _CC4_HVC_CONTROL_0,              0x4d4 */
    uint32_t wake_debounce_en;                  /* _WAKE_DEBOUNCE_EN_0,             0x4d8 */
    uint32_t ramdump_ctl_status;                /* _RAMDUMP_CTL_STATUS_0,           0x4dc */
    uint32_t utmip_sleepwalk_p3;                /* _UTMIP_SLEEPWALK_P3_0,           0x4e0 */
    uint32_t ddr_cntrl;                         /* _DDR_CNTRL_0,                    0x4e4 */
    uint32_t _0x4e8[50];
    uint32_t sec_disable4;                      /* _SEC_DISABLE4_0,                 0x5b0 */
    uint32_t sec_disable5;                      /* _SEC_DISABLE5_0,                 0x5b4 */
    uint32_t sec_disable6;                      /* _SEC_DISABLE6_0,                 0x5b8 */
    uint32_t sec_disable7;                      /* _SEC_DISABLE7_0,                 0x5bc */
    uint32_t sec_disable8;                      /* _SEC_DISABLE8_0,                 0x5c0 */
    uint32_t sec_disable9;                      /* _SEC_DISABLE9_0,                 0x5c4 */
    uint32_t sec_disable10;                     /* _SEC_DISABLE10_0,                0x5c8 */
    uint32_t _0x5cc[13];
    uint32_t scratch56;                         /* _SCRATCH56_0,                    0x600 */
    uint32_t scratch57;                         /* _SCRATCH57_0,                    0x604 */
    uint32_t scratch58;                         /* _SCRATCH58_0,                    0x608 */
    uint32_t scratch59;                         /* _SCRATCH59_0,                    0x60c */
    uint32_t scratch60;                         /* _SCRATCH60_0,                    0x610 */
    uint32_t scratch61;                         /* _SCRATCH61_0,                    0x614 */
    uint32_t scratch62;                         /* _SCRATCH62_0,                    0x618 */
    uint32_t scratch63;                         /* _SCRATCH63_0,                    0x61c */
    uint32_t scratch64;                         /* _SCRATCH64_0,                    0x620 */
    uint32_t scratch65;                         /* _SCRATCH65_0,                    0x624 */
    uint32_t scratch66;                         /* _SCRATCH66_0,                    0x628 */
    uint32_t scratch67;                         /* _SCRATCH67_0,                    0x62c */
    uint32_t scratch68;                         /* _SCRATCH68_0,                    0x630 */
    uint32_t scratch69;                         /* _SCRATCH69_0,                    0x634 */
    uint32_t scratch70;                         /* _SCRATCH70_0,                    0x638 */
    uint32_t scratch71;                         /* _SCRATCH71_0,                    0x63c */
    uint32_t scratch72;                         /* _SCRATCH72_0,                    0x640 */
    uint32_t scratch73;                         /* _SCRATCH73_0,                    0x644 */
    uint32_t scratch74;                         /* _SCRATCH74_0,                    0x648 */
    uint32_t scratch75;                         /* _SCRATCH75_0,                    0x64c */
    uint32_t scratch76;                         /* _SCRATCH76_0,                    0x650 */
    uint32_t scratch77;                         /* _SCRATCH77_0,                    0x654 */
    uint32_t scratch78;                         /* _SCRATCH78_0,                    0x658 */
    uint32_t scratch79;                         /* _SCRATCH79_0,                    0x65c */
    uint32_t scratch80;                         /* _SCRATCH80_0,                    0x660 */
    uint32_t scratch81;                         /* _SCRATCH81_0,                    0x664 */
    uint32_t scratch82;                         /* _SCRATCH82_0,                    0x668 */
    uint32_t scratch83;                         /* _SCRATCH83_0,                    0x66c */
    uint32_t scratch84;                         /* _SCRATCH84_0,                    0x670 */
    uint32_t scratch85;                         /* _SCRATCH85_0,                    0x674 */
    uint32_t scratch86;                         /* _SCRATCH86_0,                    0x678 */
    uint32_t scratch87;                         /* _SCRATCH87_0,                    0x67c */
    uint32_t scratch88;                         /* _SCRATCH88_0,                    0x680 */
    uint32_t scratch89;                         /* _SCRATCH89_0,                    0x684 */
    uint32_t scratch90;                         /* _SCRATCH90_0,                    0x688 */
    uint32_t scratch91;                         /* _SCRATCH91_0,                    0x68c */
    uint32_t scratch92;                         /* _SCRATCH92_0,                    0x690 */
    uint32_t scratch93;                         /* _SCRATCH93_0,                    0x694 */
    uint32_t scratch94;                         /* _SCRATCH94_0,                    0x698 */
    uint32_t scratch95;                         /* _SCRATCH95_0,                    0x69c */
    uint32_t scratch96;                         /* _SCRATCH96_0,                    0x6a0 */
    uint32_t scratch97;                         /* _SCRATCH97_0,                    0x6a4 */
    uint32_t scratch98;                         /* _SCRATCH98_0,                    0x6a8 */
    uint32_t scratch99;                         /* _SCRATCH99_0,                    0x6ac */
    uint32_t scratch100;                        /* _SCRATCH100_0,                   0x6b0 */
    uint32_t scratch101;                        /* _SCRATCH101_0,                   0x6b4 */
    uint32_t scratch102;                        /* _SCRATCH102_0,                   0x6b8 */
    uint32_t scratch103;                        /* _SCRATCH103_0,                   0x6bc */
    uint32_t scratch104;                        /* _SCRATCH104_0,                   0x6c0 */
    uint32_t scratch105;                        /* _SCRATCH105_0,                   0x6c4 */
    uint32_t scratch106;                        /* _SCRATCH106_0,                   0x6c8 */
    uint32_t scratch107;                        /* _SCRATCH107_0,                   0x6cc */
    uint32_t scratch108;                        /* _SCRATCH108_0,                   0x6d0 */
    uint32_t scratch109;                        /* _SCRATCH109_0,                   0x6d4 */
    uint32_t scratch110;                        /* _SCRATCH110_0,                   0x6d8 */
    uint32_t scratch111;                        /* _SCRATCH111_0,                   0x6dc */
    uint32_t scratch112;                        /* _SCRATCH112_0,                   0x6e0 */
    uint32_t scratch113;                        /* _SCRATCH113_0,                   0x6e4 */
    uint32_t scratch114;                        /* _SCRATCH114_0,                   0x6e8 */
    uint32_t scratch115;                        /* _SCRATCH115_0,                   0x6ec */
    uint32_t scratch116;                        /* _SCRATCH116_0,                   0x6f0 */
    uint32_t scratch117;                        /* _SCRATCH117_0,                   0x6f4 */
    uint32_t scratch118;                        /* _SCRATCH118_0,                   0x6f8 */
    uint32_t scratch119;                        /* _SCRATCH119_0,                   0x6fc */
    uint32_t scratch120;                        /* _SCRATCH120_0,                   0x700 */
    uint32_t scratch121;                        /* _SCRATCH121_0,                   0x704 */
    uint32_t scratch122;                        /* _SCRATCH122_0,                   0x708 */
    uint32_t scratch123;                        /* _SCRATCH123_0,                   0x70c */
    uint32_t scratch124;                        /* _SCRATCH124_0,                   0x710 */
    uint32_t scratch125;                        /* _SCRATCH125_0,                   0x714 */
    uint32_t scratch126;                        /* _SCRATCH126_0,                   0x718 */
    uint32_t scratch127;                        /* _SCRATCH127_0,                   0x71c */
    uint32_t scratch128;                        /* _SCRATCH128_0,                   0x720 */
    uint32_t scratch129;                        /* _SCRATCH129_0,                   0x724 */
    uint32_t scratch130;                        /* _SCRATCH130_0,                   0x728 */
    uint32_t scratch131;                        /* _SCRATCH131_0,                   0x72c */
    uint32_t scratch132;                        /* _SCRATCH132_0,                   0x730 */
    uint32_t scratch133;                        /* _SCRATCH133_0,                   0x734 */
    uint32_t scratch134;                        /* _SCRATCH134_0,                   0x738 */
    uint32_t scratch135;                        /* _SCRATCH135_0,                   0x73c */
    uint32_t scratch136;                        /* _SCRATCH136_0,                   0x740 */
    uint32_t scratch137;                        /* _SCRATCH137_0,                   0x744 */
    uint32_t scratch138;                        /* _SCRATCH138_0,                   0x748 */
    uint32_t scratch139;                        /* _SCRATCH139_0,                   0x74c */
    uint32_t scratch140;                        /* _SCRATCH140_0,                   0x750 */
    uint32_t scratch141;                        /* _SCRATCH141_0,                   0x754 */
    uint32_t scratch142;                        /* _SCRATCH142_0,                   0x758 */
    uint32_t scratch143;                        /* _SCRATCH143_0,                   0x75c */
    uint32_t scratch144;                        /* _SCRATCH144_0,                   0x760 */
    uint32_t scratch145;                        /* _SCRATCH145_0,                   0x764 */
    uint32_t scratch146;                        /* _SCRATCH146_0,                   0x768 */
    uint32_t scratch147;                        /* _SCRATCH147_0,                   0x76c */
    uint32_t scratch148;                        /* _SCRATCH148_0,                   0x770 */
    uint32_t scratch149;                        /* _SCRATCH149_0,                   0x774 */
    uint32_t scratch150;                        /* _SCRATCH150_0,                   0x778 */
    uint32_t scratch151;                        /* _SCRATCH151_0,                   0x77c */
    uint32_t scratch152;                        /* _SCRATCH152_0,                   0x780 */
    uint32_t scratch153;                        /* _SCRATCH153_0,                   0x784 */
    uint32_t scratch154;                        /* _SCRATCH154_0,                   0x788 */
    uint32_t scratch155;                        /* _SCRATCH155_0,                   0x78c */
    uint32_t scratch156;                        /* _SCRATCH156_0,                   0x790 */
    uint32_t scratch157;                        /* _SCRATCH157_0,                   0x794 */
    uint32_t scratch158;                        /* _SCRATCH158_0,                   0x798 */
    uint32_t scratch159;                        /* _SCRATCH159_0,                   0x79c */
    uint32_t scratch160;                        /* _SCRATCH160_0,                   0x7a0 */
    uint32_t scratch161;                        /* _SCRATCH161_0,                   0x7a4 */
    uint32_t scratch162;                        /* _SCRATCH162_0,                   0x7a8 */
    uint32_t scratch163;                        /* _SCRATCH163_0,                   0x7ac */
    uint32_t scratch164;                        /* _SCRATCH164_0,                   0x7b0 */
    uint32_t scratch165;                        /* _SCRATCH165_0,                   0x7b4 */
    uint32_t scratch166;                        /* _SCRATCH166_0,                   0x7b8 */
    uint32_t scratch167;                        /* _SCRATCH167_0,                   0x7bc */
    uint32_t scratch168;                        /* _SCRATCH168_0,                   0x7c0 */
    uint32_t scratch169;                        /* _SCRATCH169_0,                   0x7c4 */
    uint32_t scratch170;                        /* _SCRATCH170_0,                   0x7c8 */
    uint32_t scratch171;                        /* _SCRATCH171_0,                   0x7cc */
    uint32_t scratch172;                        /* _SCRATCH172_0,                   0x7d0 */
    uint32_t scratch173;                        /* _SCRATCH173_0,                   0x7d4 */
    uint32_t scratch174;                        /* _SCRATCH174_0,                   0x7d8 */
    uint32_t scratch175;                        /* _SCRATCH175_0,                   0x7dc */
    uint32_t scratch176;                        /* _SCRATCH176_0,                   0x7e0 */
    uint32_t scratch177;                        /* _SCRATCH177_0,                   0x7e4 */
    uint32_t scratch178;                        /* _SCRATCH178_0,                   0x7e8 */
    uint32_t scratch179;                        /* _SCRATCH179_0,                   0x7ec */
    uint32_t scratch180;                        /* _SCRATCH180_0,                   0x7f0 */
    uint32_t scratch181;                        /* _SCRATCH181_0,                   0x7f4 */
    uint32_t scratch182;                        /* _SCRATCH182_0,                   0x7f8 */
    uint32_t scratch183;                        /* _SCRATCH183_0,                   0x7fc */
    uint32_t scratch184;                        /* _SCRATCH184_0,                   0x800 */
    uint32_t scratch185;                        /* _SCRATCH185_0,                   0x804 */
    uint32_t scratch186;                        /* _SCRATCH186_0,                   0x808 */
    uint32_t scratch187;                        /* _SCRATCH187_0,                   0x80c */
    uint32_t scratch188;                        /* _SCRATCH188_0,                   0x810 */
    uint32_t scratch189;                        /* _SCRATCH189_0,                   0x814 */
    uint32_t scratch190;                        /* _SCRATCH190_0,                   0x818 */
    uint32_t scratch191;                        /* _SCRATCH191_0,                   0x81c */
    uint32_t scratch192;                        /* _SCRATCH192_0,                   0x820 */
    uint32_t scratch193;                        /* _SCRATCH193_0,                   0x824 */
    uint32_t scratch194;                        /* _SCRATCH194_0,                   0x828 */
    uint32_t scratch195;                        /* _SCRATCH195_0,                   0x82c */
    uint32_t scratch196;                        /* _SCRATCH196_0,                   0x830 */
    uint32_t scratch197;                        /* _SCRATCH197_0,                   0x834 */
    uint32_t scratch198;                        /* _SCRATCH198_0,                   0x838 */
    uint32_t scratch199;                        /* _SCRATCH199_0,                   0x83c */
    uint32_t scratch200;                        /* _SCRATCH200_0,                   0x840 */
    uint32_t scratch201;                        /* _SCRATCH201_0,                   0x844 */
    uint32_t scratch202;                        /* _SCRATCH202_0,                   0x848 */
    uint32_t scratch203;                        /* _SCRATCH203_0,                   0x84c */
    uint32_t scratch204;                        /* _SCRATCH204_0,                   0x850 */
    uint32_t scratch205;                        /* _SCRATCH205_0,                   0x854 */
    uint32_t scratch206;                        /* _SCRATCH206_0,                   0x858 */
    uint32_t scratch207;                        /* _SCRATCH207_0,                   0x85c */
    uint32_t scratch208;                        /* _SCRATCH208_0,                   0x860 */
    uint32_t scratch209;                        /* _SCRATCH209_0,                   0x864 */
    uint32_t scratch210;                        /* _SCRATCH210_0,                   0x868 */
    uint32_t scratch211;                        /* _SCRATCH211_0,                   0x86c */
    uint32_t scratch212;                        /* _SCRATCH212_0,                   0x870 */
    uint32_t scratch213;                        /* _SCRATCH213_0,                   0x874 */
    uint32_t scratch214;                        /* _SCRATCH214_0,                   0x878 */
    uint32_t scratch215;                        /* _SCRATCH215_0,                   0x87c */
    uint32_t scratch216;                        /* _SCRATCH216_0,                   0x880 */
    uint32_t scratch217;                        /* _SCRATCH217_0,                   0x884 */
    uint32_t scratch218;                        /* _SCRATCH218_0,                   0x888 */
    uint32_t scratch219;                        /* _SCRATCH219_0,                   0x88c */
    uint32_t scratch220;                        /* _SCRATCH220_0,                   0x890 */
    uint32_t scratch221;                        /* _SCRATCH221_0,                   0x894 */
    uint32_t scratch222;                        /* _SCRATCH222_0,                   0x898 */
    uint32_t scratch223;                        /* _SCRATCH223_0,                   0x89c */
    uint32_t scratch224;                        /* _SCRATCH224_0,                   0x8a0 */
    uint32_t scratch225;                        /* _SCRATCH225_0,                   0x8a4 */
    uint32_t scratch226;                        /* _SCRATCH226_0,                   0x8a8 */
    uint32_t scratch227;                        /* _SCRATCH227_0,                   0x8ac */
    uint32_t scratch228;                        /* _SCRATCH228_0,                   0x8b0 */
    uint32_t scratch229;                        /* _SCRATCH229_0,                   0x8b4 */
    uint32_t scratch230;                        /* _SCRATCH230_0,                   0x8b8 */
    uint32_t scratch231;                        /* _SCRATCH231_0,                   0x8bc */
    uint32_t scratch232;                        /* _SCRATCH232_0,                   0x8c0 */
    uint32_t scratch233;                        /* _SCRATCH233_0,                   0x8c4 */
    uint32_t scratch234;                        /* _SCRATCH234_0,                   0x8c8 */
    uint32_t scratch235;                        /* _SCRATCH235_0,                   0x8cc */
    uint32_t scratch236;                        /* _SCRATCH236_0,                   0x8d0 */
    uint32_t scratch237;                        /* _SCRATCH237_0,                   0x8d4 */
    uint32_t scratch238;                        /* _SCRATCH238_0,                   0x8d8 */
    uint32_t scratch239;                        /* _SCRATCH239_0,                   0x8dc */
    uint32_t scratch240;                        /* _SCRATCH240_0,                   0x8e0 */
    uint32_t scratch241;                        /* _SCRATCH241_0,                   0x8e4 */
    uint32_t scratch242;                        /* _SCRATCH242_0,                   0x8e8 */
    uint32_t scratch243;                        /* _SCRATCH243_0,                   0x8ec */
    uint32_t scratch244;                        /* _SCRATCH244_0,                   0x8f0 */
    uint32_t scratch245;                        /* _SCRATCH245_0,                   0x8f4 */
    uint32_t scratch246;                        /* _SCRATCH246_0,                   0x8f8 */
    uint32_t scratch247;                        /* _SCRATCH247_0,                   0x8fc */
    uint32_t scratch248;                        /* _SCRATCH248_0,                   0x900 */
    uint32_t scratch249;                        /* _SCRATCH249_0,                   0x904 */
    uint32_t scratch250;                        /* _SCRATCH250_0,                   0x908 */
    uint32_t scratch251;                        /* _SCRATCH251_0,                   0x90c */
    uint32_t scratch252;                        /* _SCRATCH252_0,                   0x910 */
    uint32_t scratch253;                        /* _SCRATCH253_0,                   0x914 */
    uint32_t scratch254;                        /* _SCRATCH254_0,                   0x918 */
    uint32_t scratch255;                        /* _SCRATCH255_0,                   0x91c */
    uint32_t scratch256;                        /* _SCRATCH256_0,                   0x920 */
    uint32_t scratch257;                        /* _SCRATCH257_0,                   0x924 */
    uint32_t scratch258;                        /* _SCRATCH258_0,                   0x928 */
    uint32_t scratch259;                        /* _SCRATCH259_0,                   0x92c */
    uint32_t scratch260;                        /* _SCRATCH260_0,                   0x930 */
    uint32_t scratch261;                        /* _SCRATCH261_0,                   0x934 */
    uint32_t scratch262;                        /* _SCRATCH262_0,                   0x938 */
    uint32_t scratch263;                        /* _SCRATCH263_0,                   0x93c */
    uint32_t scratch264;                        /* _SCRATCH264_0,                   0x940 */
    uint32_t scratch265;                        /* _SCRATCH265_0,                   0x944 */
    uint32_t scratch266;                        /* _SCRATCH266_0,                   0x948 */
    uint32_t scratch267;                        /* _SCRATCH267_0,                   0x94c */
    uint32_t scratch268;                        /* _SCRATCH268_0,                   0x950 */
    uint32_t scratch269;                        /* _SCRATCH269_0,                   0x954 */
    uint32_t scratch270;                        /* _SCRATCH270_0,                   0x958 */
    uint32_t scratch271;                        /* _SCRATCH271_0,                   0x95c */
    uint32_t scratch272;                        /* _SCRATCH272_0,                   0x960 */
    uint32_t scratch273;                        /* _SCRATCH273_0,                   0x964 */
    uint32_t scratch274;                        /* _SCRATCH274_0,                   0x968 */
    uint32_t scratch275;                        /* _SCRATCH275_0,                   0x96c */
    uint32_t scratch276;                        /* _SCRATCH276_0,                   0x970 */
    uint32_t scratch277;                        /* _SCRATCH277_0,                   0x974 */
    uint32_t scratch278;                        /* _SCRATCH278_0,                   0x978 */
    uint32_t scratch279;                        /* _SCRATCH279_0,                   0x97c */
    uint32_t scratch280;                        /* _SCRATCH280_0,                   0x980 */
    uint32_t scratch281;                        /* _SCRATCH281_0,                   0x984 */
    uint32_t scratch282;                        /* _SCRATCH282_0,                   0x988 */
    uint32_t scratch283;                        /* _SCRATCH283_0,                   0x98c */
    uint32_t scratch284;                        /* _SCRATCH284_0,                   0x990 */
    uint32_t scratch285;                        /* _SCRATCH285_0,                   0x994 */
    uint32_t scratch286;                        /* _SCRATCH286_0,                   0x998 */
    uint32_t scratch287;                        /* _SCRATCH287_0,                   0x99c */
    uint32_t scratch288;                        /* _SCRATCH288_0,                   0x9a0 */
    uint32_t scratch289;                        /* _SCRATCH289_0,                   0x9a4 */
    uint32_t scratch290;                        /* _SCRATCH290_0,                   0x9a8 */
    uint32_t scratch291;                        /* _SCRATCH291_0,                   0x9ac */
    uint32_t scratch292;                        /* _SCRATCH292_0,                   0x9b0 */
    uint32_t scratch293;                        /* _SCRATCH293_0,                   0x9b4 */
    uint32_t scratch294;                        /* _SCRATCH294_0,                   0x9b8 */
    uint32_t scratch295;                        /* _SCRATCH295_0,                   0x9bc */
    uint32_t scratch296;                        /* _SCRATCH296_0,                   0x9c0 */
    uint32_t scratch297;                        /* _SCRATCH297_0,                   0x9c4 */
    uint32_t scratch298;                        /* _SCRATCH298_0,                   0x9c8 */
    uint32_t scratch299;                        /* _SCRATCH299_0,                   0x9cc */
    uint32_t _0x9d0[50];
    uint32_t secure_scratch80;                  /* _SECURE_SCRATCH80_0,             0xa98 */
    uint32_t secure_scratch81;                  /* _SECURE_SCRATCH81_0,             0xa9c */
    uint32_t secure_scratch82;                  /* _SECURE_SCRATCH82_0,             0xaa0 */
    uint32_t secure_scratch83;                  /* _SECURE_SCRATCH83_0,             0xaa4 */
    uint32_t secure_scratch84;                  /* _SECURE_SCRATCH84_0,             0xaa8 */
    uint32_t secure_scratch85;                  /* _SECURE_SCRATCH85_0,             0xaac */
    uint32_t secure_scratch86;                  /* _SECURE_SCRATCH86_0,             0xab0 */
    uint32_t secure_scratch87;                  /* _SECURE_SCRATCH87_0,             0xab4 */
    uint32_t secure_scratch88;                  /* _SECURE_SCRATCH88_0,             0xab8 */
    uint32_t secure_scratch89;                  /* _SECURE_SCRATCH89_0,             0xabc */
    uint32_t secure_scratch90;                  /* _SECURE_SCRATCH90_0,             0xac0 */
    uint32_t secure_scratch91;                  /* _SECURE_SCRATCH91_0,             0xac4 */
    uint32_t secure_scratch92;                  /* _SECURE_SCRATCH92_0,             0xac8 */
    uint32_t secure_scratch93;                  /* _SECURE_SCRATCH93_0,             0xacc */
    uint32_t secure_scratch94;                  /* _SECURE_SCRATCH94_0,             0xad0 */
    uint32_t secure_scratch95;                  /* _SECURE_SCRATCH95_0,             0xad4 */
    uint32_t secure_scratch96;                  /* _SECURE_SCRATCH96_0,             0xad8 */
    uint32_t secure_scratch97;                  /* _SECURE_SCRATCH97_0,             0xadc */
    uint32_t secure_scratch98;                  /* _SECURE_SCRATCH98_0,             0xae0 */
    uint32_t secure_scratch99;                  /* _SECURE_SCRATCH99_0,             0xae4 */
    uint32_t secure_scratch100;                 /* _SECURE_SCRATCH100_0,            0xae8 */
    uint32_t secure_scratch101;                 /* _SECURE_SCRATCH101_0,            0xaec */
    uint32_t secure_scratch102;                 /* _SECURE_SCRATCH102_0,            0xaf0 */
    uint32_t secure_scratch103;                 /* _SECURE_SCRATCH103_0,            0xaf4 */
    uint32_t secure_scratch104;                 /* _SECURE_SCRATCH104_0,            0xaf8 */
    uint32_t secure_scratch105;                 /* _SECURE_SCRATCH105_0,            0xafc */
    uint32_t secure_scratch106;                 /* _SECURE_SCRATCH106_0,            0xb00 */
    uint32_t secure_scratch107;                 /* _SECURE_SCRATCH107_0,            0xb04 */
    uint32_t secure_scratch108;                 /* _SECURE_SCRATCH108_0,            0xb08 */
    uint32_t secure_scratch109;                 /* _SECURE_SCRATCH109_0,            0xb0c */
    uint32_t secure_scratch110;                 /* _SECURE_SCRATCH110_0,            0xb10 */
    uint32_t secure_scratch111;                 /* _SECURE_SCRATCH111_0,            0xb14 */
    uint32_t secure_scratch112;                 /* _SECURE_SCRATCH112_0,            0xb18 */
    uint32_t secure_scratch113;                 /* _SECURE_SCRATCH113_0,            0xb1c */
    uint32_t secure_scratch114;                 /* _SECURE_SCRATCH114_0,            0xb20 */
    uint32_t secure_scratch115;                 /* _SECURE_SCRATCH115_0,            0xb24 */
    uint32_t secure_scratch116;                 /* _SECURE_SCRATCH116_0,            0xb28 */
    uint32_t secure_scratch117;                 /* _SECURE_SCRATCH117_0,            0xb2c */
    uint32_t secure_scratch118;                 /* _SECURE_SCRATCH118_0,            0xb30 */
    uint32_t secure_scratch119;                 /* _SECURE_SCRATCH119_0,            0xb34 */
    uint32_t secure_scratch120;                 /* _SECURE_SCRATCH120_0,            0xb38 */
    uint32_t secure_scratch121;                 /* _SECURE_SCRATCH121_0,            0xb3c */
    uint32_t secure_scratch122;                 /* _SECURE_SCRATCH122_0,            0xb40 */
    uint32_t secure_scratch123;                 /* _SECURE_SCRATCH123_0,            0xb44 */
    uint32_t led_breathing_ctrl;                /* _LED_BREATHING_CTRL_0,           0xb48 */
    uint32_t led_breathing_counter0;            /* _LED_BREATHING_COUNTER0_0,       0xb4c */
    uint32_t led_breathing_counter1;            /* _LED_BREATHING_COUNTER1_0,       0xb50 */
    uint32_t led_breathing_counter2;            /* _LED_BREATHING_COUNTER2_0,       0xb54 */
    uint32_t led_breathing_counter3;            /* _LED_BREATHING_COUNTER3_0,       0xb58 */
    uint32_t led_breathing_status;              /* _LED_BREATHING_STATUS_0,         0xb5c */
    uint32_t _0xb60[2];
    uint32_t secure_scratch124;                 /* _SECURE_SCRATCH124_0,            0xb68 */
    uint32_t secure_scratch125;                 /* _SECURE_SCRATCH125_0,            0xb6c */
    uint32_t secure_scratch126;                 /* _SECURE_SCRATCH126_0,            0xb70 */
    uint32_t secure_scratch127;                 /* _SECURE_SCRATCH127_0,            0xb74 */
    uint32_t secure_scratch128;                 /* _SECURE_SCRATCH128_0,            0xb78 */
    uint32_t secure_scratch129;                 /* _SECURE_SCRATCH129_0,            0xb7c */
    uint32_t secure_scratch130;                 /* _SECURE_SCRATCH130_0,            0xb80 */
    uint32_t secure_scratch131;                 /* _SECURE_SCRATCH131_0,            0xb84 */
    uint32_t secure_scratch132;                 /* _SECURE_SCRATCH132_0,            0xb88 */
    uint32_t secure_scratch133;                 /* _SECURE_SCRATCH133_0,            0xb8c */
    uint32_t secure_scratch134;                 /* _SECURE_SCRATCH134_0,            0xb90 */
    uint32_t secure_scratch135;                 /* _SECURE_SCRATCH135_0,            0xb94 */
    uint32_t secure_scratch136;                 /* _SECURE_SCRATCH136_0,            0xb98 */
    uint32_t secure_scratch137;                 /* _SECURE_SCRATCH137_0,            0xb9c */
    uint32_t secure_scratch138;                 /* _SECURE_SCRATCH138_0,            0xba0 */
    uint32_t secure_scratch139;                 /* _SECURE_SCRATCH139_0,            0xba4 */
    uint32_t _0xba8[2];
    uint32_t sec_disable_ns;                    /* _SEC_DISABLE_NS_0,               0xbb0 */
    uint32_t sec_disable2_ns;                   /* _SEC_DISABLE2_NS_0,              0xbb4 */
    uint32_t sec_disable3_ns;                   /* _SEC_DISABLE3_NS_0,              0xbb8 */
    uint32_t sec_disable4_ns;                   /* _SEC_DISABLE4_NS_0,              0xbbc */
    uint32_t sec_disable5_ns;                   /* _SEC_DISABLE5_NS_0,              0xbc0 */
    uint32_t sec_disable6_ns;                   /* _SEC_DISABLE6_NS_0,              0xbc4 */
    uint32_t sec_disable7_ns;                   /* _SEC_DISABLE7_NS_0,              0xbc8 */
    uint32_t sec_disable8_ns;                   /* _SEC_DISABLE8_NS_0,              0xbcc */
    uint32_t sec_disable9_ns;                   /* _SEC_DISABLE9_NS_0,              0xbd0 */
    uint32_t sec_disable10_ns;                  /* _SEC_DISABLE10_NS_0,             0xbd4 */
    uint32_t _0xbd8[4];
    uint32_t tzram_pwr_cntrl;                   /* _TZRAM_PWR_CNTRL_0,              0xbe8 */
    uint32_t tzram_sec_disable;                 /* _TZRAM_SEC_DISABLE_0,            0xbec */
    uint32_t tzram_non_sec_disable;             /* _TZRAM_NON_SEC_DISABLE_0,        0xbf0 */
} tegra_pmc_t;

static inline volatile tegra_pmc_t *pmc_get_regs(void)
{
    return (volatile tegra_pmc_t *)PMC_BASE;
}

#endif
