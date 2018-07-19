#ifndef FUSEE_CAR_H
#define FUSEE_CAR_H

#define CLK_SOURCE_SDMMC1   20
#define CLK_SOURCE_SDMMC2   21
#define CLK_SOURCE_SDMMC3   47
#define CLK_SOURCE_SDMMC4   25
#define CLK_SOURCE_SDMMC_LEGACY   0

#define CLK_L_SDMMC1    (1 << 14)
#define CLK_L_SDMMC2    (1 << 9)
#define CLK_U_SDMMC3    (1 << 5)
#define CLK_L_SDMMC4    (1 << 15)

#define TEGRA_CLK_PLLS              6    /* Number of normal PLLs */
#define TEGRA_CLK_SIMPLE_PLLS       3    /* Number of simple PLLs */
#define TEGRA_CLK_SOURCES           64   /* Number of ppl clock sources L/H/U */
#define TEGRA_CLK_SOURCES_VW        32   /* Number of ppl clock sources V/W */
#define TEGRA_CLK_SOURCES_X         32   /* Number of ppl clock sources X */
#define TEGRA_CLK_SOURCES_Y         18   /* Number of ppl clock sources Y */

#define CLK_SOURCE_MASK             (0b111 << 29)
#define CLK_SOURCE_FIRST            (0b000 << 29)
#define CLK_DIVIDER_MASK            (0xff << 0)
#define CLK_DIVIDER_UNITY           (0x00 << 0)

#define CAR_CONTROL_SDMMC1          (1 << 14)
#define CAR_CONTROL_SDMMC4          (1 << 15)
#define CAR_CONTROL_SDMMC_LEGACY    (1 << 1)

/* PLL registers - there are several PLLs in the clock controller */
typedef struct {
    uint32_t pll_base;      /* the control register */
    
    /* pll_out[0] is output A control, pll_out[1] is output B control */
    uint32_t pll_out[2];
    uint32_t pll_misc;      /* other misc things */
} clk_pll_t;

/* PLL registers - there are several PLLs in the clock controller */
typedef struct {
    uint32_t pll_base;      /* the control register */
    uint32_t pll_misc;      /* other misc things */
} clk_pll_simple_t;

typedef struct {
    uint32_t pllm_base;     /* the control register */
    uint32_t pllm_out;      /* output control */
    uint32_t pllm_misc1;    /* misc1 */
    uint32_t pllm_misc2;    /* misc2 */
} clk_pllm_t;

/* Clock/Reset Controller (CLK_RST_CONTROLLER_) regs */
typedef struct {
    uint32_t rst_src;           /* _RST_SOURCE_0,0x00 */

    uint32_t rst_dev_l;
    uint32_t rst_dev_h;
    uint32_t rst_dev_u;

    uint32_t clk_out_enb_l;
    uint32_t clk_out_enb_h;
    uint32_t clk_out_enb_u;

    uint32_t reserved0;         /* reserved_0,      0x1C */
    uint32_t cclk_brst_pol;     /* _CCLK_BURST_POLICY_0, 0x20 */
    uint32_t super_cclk_div;    /* _SUPER_CCLK_DIVIDER_0,0x24 */
    uint32_t sclk_brst_pol;     /* _SCLK_BURST_POLICY_0, 0x28 */
    uint32_t super_sclk_div;    /* _SUPER_SCLK_DIVIDER_0,0x2C */
    uint32_t clk_sys_rate;      /* _CLK_SYSTEM_RATE_0,  0x30 */
    uint32_t prog_dly_clk;      /* _PROG_DLY_CLK_0, 0x34 */
    uint32_t aud_sync_clk_rate; /* _AUDIO_SYNC_CLK_RATE_0,0x38 */
    uint32_t reserved1;         /* reserved_1,      0x3C */
    uint32_t cop_clk_skip_plcy; /* _COP_CLK_SKIP_POLICY_0,0x40 */
    uint32_t clk_mask_arm;      /* _CLK_MASK_ARM_0, 0x44 */
    uint32_t misc_clk_enb;      /* _MISC_CLK_ENB_0, 0x48 */
    uint32_t clk_cpu_cmplx;     /* _CLK_CPU_CMPLX_0,    0x4C */
    uint32_t osc_ctrl;          /* _OSC_CTRL_0,     0x50 */
    uint32_t pll_lfsr;          /* _PLL_LFSR_0,     0x54 */
    uint32_t osc_freq_det;      /* _OSC_FREQ_DET_0, 0x58 */
    uint32_t osc_freq_det_stat; /* _OSC_FREQ_DET_STATUS_0,0x5C */
    uint32_t reserved2[8];      /* reserved_2[8],   0x60-7C */

    clk_pll_t pll[TEGRA_CLK_PLLS]; /* PLLs from 0x80 to 0xdc */

    /* PLLs from 0xe0 to 0xf4    */
    clk_pll_simple_t pll_simple[TEGRA_CLK_SIMPLE_PLLS];

    uint32_t reserved10;        /* _reserved_10,    0xF8 */
    uint32_t reserved11;        /* _reserved_11,    0xFC */

    uint32_t clk_src[TEGRA_CLK_SOURCES]; /*_I2S1_0...   0x100-1fc */

    uint32_t reserved20[32];    /* _reserved_20,    0x200-27c */

    uint32_t clk_out_enb_x;     /* _CLK_OUT_ENB_X_0,    0x280 */
    uint32_t clk_enb_x_set;     /* _CLK_ENB_X_SET_0,    0x284 */
    uint32_t clk_enb_x_clr;     /* _CLK_ENB_X_CLR_0,    0x288 */

    uint32_t rst_devices_x;     /* _RST_DEVICES_X_0,    0x28c */
    uint32_t rst_dev_x_set;     /* _RST_DEV_X_SET_0,    0x290 */
    uint32_t rst_dev_x_clr;     /* _RST_DEV_X_CLR_0,    0x294 */

    uint32_t clk_out_enb_y;     /* _CLK_OUT_ENB_Y_0,    0x298 */
    uint32_t clk_enb_y_set;     /* _CLK_ENB_Y_SET_0,    0x29c */
    uint32_t clk_enb_y_clr;     /* _CLK_ENB_Y_CLR_0,    0x2a0 */

    uint32_t rst_devices_y;     /* _RST_DEVICES_Y_0,    0x2a4 */
    uint32_t rst_dev_y_set;     /* _RST_DEV_Y_SET_0,    0x2a8 */
    uint32_t rst_dev_y_clr;     /* _RST_DEV_Y_CLR_0,    0x2ac */

    uint32_t reserved21[17];    /* _reserved_21,    0x2b0-2f0 */

    uint32_t dfll_base;     /* _DFLL_BASE_0,    0x2f4 */

    uint32_t reserved22[2];     /* _reserved_22,    0x2f8-2fc */

    /* _RST_DEV_L/H/U_SET_0 0x300 ~ 0x314 */
    uint32_t rst_dev_l_set;
    uint32_t rst_dev_l_clr;
    uint32_t rst_dev_h_set;
    uint32_t rst_dev_h_clr;
    uint32_t rst_dev_u_set;
    uint32_t rst_dev_u_clr;

    uint32_t reserved30[2];     /* _reserved_30,    0x318, 0x31c */

    /* _CLK_ENB_L/H/U_CLR_0 0x320 ~ 0x334 */
    uint32_t clk_enb_l_set;
    uint32_t clk_enb_l_clr;
    uint32_t clk_enb_h_set;
    uint32_t clk_enb_h_clr;
    uint32_t clk_enb_u_set;
    uint32_t clk_enb_u_clr;

    uint32_t reserved31[2];     /* _reserved_31,    0x338, 0x33c */

    uint32_t cpu_cmplx_set;     /* _RST_CPU_CMPLX_SET_0,    0x340 */
    uint32_t cpu_cmplx_clr;     /* _RST_CPU_CMPLX_CLR_0,    0x344 */

    /* Additional (T30) registers */
    uint32_t clk_cpu_cmplx_set; /* _CLK_CPU_CMPLX_SET_0,    0x348 */
    uint32_t clk_cpu_cmplx_clr; /* _CLK_CPU_CMPLX_SET_0,    0x34c */

    uint32_t reserved32[2];     /* _reserved_32,      0x350,0x354 */

    uint32_t rst_dev_v;         /* _RST_DEVICES_V/W_0 */
    uint32_t rst_dev_w;         /* _RST_DEVICES_V/W_0 */

    uint32_t clk_out_enb_v;     /* _CLK_OUT_ENB_V/W_0 */
    uint32_t clk_out_enb_w;     /* _CLK_OUT_ENB_V/W_0 */
    uint32_t cclkg_brst_pol;    /* _CCLKG_BURST_POLICY_0,   0x368 */
    uint32_t super_cclkg_div;   /* _SUPER_CCLKG_DIVIDER_0,  0x36C */
    uint32_t cclklp_brst_pol;   /* _CCLKLP_BURST_POLICY_0,  0x370 */
    uint32_t super_cclkp_div;   /* _SUPER_CCLKLP_DIVIDER_0, 0x374 */
    uint32_t clk_cpug_cmplx;    /* _CLK_CPUG_CMPLX_0,       0x378 */
    uint32_t clk_cpulp_cmplx;   /* _CLK_CPULP_CMPLX_0,      0x37C */
    uint32_t cpu_softrst_ctrl;  /* _CPU_SOFTRST_CTRL_0,     0x380 */
    uint32_t cpu_softrst_ctrl1; /* _CPU_SOFTRST_CTRL1_0,    0x384 */
    uint32_t cpu_softrst_ctrl2; /* _CPU_SOFTRST_CTRL2_0,    0x388 */
    uint32_t reserved33[9];     /* _reserved_33,        0x38c-3ac */
    uint32_t clk_src_v;         /* 0x3B0-0x42C */
    uint32_t clk_src_w;         /* 0x3B0-0x42C */

    /* _RST_DEV_V/W_SET_0 0x430 ~ 0x43c */
    uint32_t rst_dev_v_set;
    uint32_t rst_dev_v_clr;
    uint32_t rst_dev_w_set;
    uint32_t rst_dev_w_clr;

    /* _CLK_ENB_V/W_CLR_0 0x440 ~ 0x44c */
    uint32_t rst_clk_v_set;
    uint32_t rst_clk_v_clr;
    uint32_t rst_clk_w_set;
    uint32_t rst_clk_w_clr;

    /* Additional (T114+) registers */
    uint32_t rst_cpug_cmplx_set;    /* _RST_CPUG_CMPLX_SET_0,  0x450 */
    uint32_t rst_cpug_cmplx_clr;    /* _RST_CPUG_CMPLX_CLR_0,  0x454 */
    uint32_t rst_cpulp_cmplx_set;   /* _RST_CPULP_CMPLX_SET_0, 0x458 */
    uint32_t rst_cpulp_cmplx_clr;   /* _RST_CPULP_CMPLX_CLR_0, 0x45C */
    uint32_t clk_cpug_cmplx_set;    /* _CLK_CPUG_CMPLX_SET_0,  0x460 */
    uint32_t clk_cpug_cmplx_clr;    /* _CLK_CPUG_CMPLX_CLR_0,  0x464 */
    uint32_t clk_cpulp_cmplx_set;   /* _CLK_CPULP_CMPLX_SET_0, 0x468 */
    uint32_t clk_cpulp_cmplx_clr;   /* _CLK_CPULP_CMPLX_CLR_0, 0x46C */
    uint32_t cpu_cmplx_status;      /* _CPU_CMPLX_STATUS_0,    0x470 */
    uint32_t reserved40[1];         /* _reserved_40,        0x474 */
    uint32_t intstatus;             /* __INTSTATUS_0,       0x478 */
    uint32_t intmask;               /* __INTMASK_0,         0x47C */
    uint32_t utmip_pll_cfg0;        /* _UTMIP_PLL_CFG0_0,   0x480 */
    uint32_t utmip_pll_cfg1;        /* _UTMIP_PLL_CFG1_0,   0x484 */
    uint32_t utmip_pll_cfg2;        /* _UTMIP_PLL_CFG2_0,   0x488 */

    uint32_t plle_aux;              /* _PLLE_AUX_0,     0x48C */
    uint32_t sata_pll_cfg0;         /* _SATA_PLL_CFG0_0,    0x490 */
    uint32_t sata_pll_cfg1;         /* _SATA_PLL_CFG1_0,    0x494 */
    uint32_t pcie_pll_cfg0;         /* _PCIE_PLL_CFG0_0,    0x498 */

    uint32_t prog_audio_dly_clk;    /* _PROG_AUDIO_DLY_CLK_0, 0x49C */
    uint32_t audio_sync_clk_i2s0;   /* _AUDIO_SYNC_CLK_I2S0_0, 0x4A0 */
    uint32_t audio_sync_clk_i2s1;   /* _AUDIO_SYNC_CLK_I2S1_0, 0x4A4 */
    uint32_t audio_sync_clk_i2s2;   /* _AUDIO_SYNC_CLK_I2S2_0, 0x4A8 */
    uint32_t audio_sync_clk_i2s3;   /* _AUDIO_SYNC_CLK_I2S3_0, 0x4AC */
    uint32_t audio_sync_clk_i2s4;   /* _AUDIO_SYNC_CLK_I2S4_0, 0x4B0 */
    uint32_t audio_sync_clk_spdif;  /* _AUDIO_SYNC_CLK_SPDIF_0, 0x4B4 */

    uint32_t plld2_base;            /* _PLLD2_BASE_0, 0x4B8 */
    uint32_t plld2_misc;            /* _PLLD2_MISC_0, 0x4BC */
    uint32_t utmip_pll_cfg3;        /* _UTMIP_PLL_CFG3_0, 0x4C0 */
    uint32_t pllrefe_base;          /* _PLLREFE_BASE_0, 0x4C4 */
    uint32_t pllrefe_misc;          /* _PLLREFE_MISC_0, 0x4C8 */
    uint32_t crs_reserved_50[7];    /* _reserved_50, 0x4CC-0x4E4 */
    uint32_t pllc2_base;            /* _PLLC2_BASE_0, 0x4E8 */
    uint32_t pllc2_misc0;           /* _PLLC2_MISC_0_0, 0x4EC */
    uint32_t pllc2_misc1;           /* _PLLC2_MISC_1_0, 0x4F0 */
    uint32_t pllc2_misc2;           /* _PLLC2_MISC_2_0, 0x4F4 */
    uint32_t pllc2_misc3;           /* _PLLC2_MISC_3_0, 0x4F8 */
    uint32_t pllc3_base;            /* _PLLC3_BASE_0, 0x4FC */
    uint32_t pllc3_misc0;           /* _PLLC3_MISC_0_0, 0x500 */
    uint32_t pllc3_misc1;           /* _PLLC3_MISC_1_0, 0x504 */
    uint32_t pllc3_misc2;           /* _PLLC3_MISC_2_0, 0x508 */
    uint32_t pllc3_misc3;           /* _PLLC3_MISC_3_0, 0x50C */
    uint32_t pllx_misc1;            /* _PLLX_MISC_1_0, 0x510 */
    uint32_t pllx_misc2;            /* _PLLX_MISC_2_0, 0x514 */
    uint32_t pllx_misc3;            /* _PLLX_MISC_3_0, 0x518 */
    uint32_t xusbio_pll_cfg0;       /* _XUSBIO_PLL_CFG0_0, 0x51C */
    uint32_t xusbio_pll_cfg1;       /* _XUSBIO_PLL_CFG0_1, 0x520 */
    uint32_t plle_aux1;             /* _PLLE_AUX1_0, 0x524 */
    uint32_t pllp_reshift;          /* _PLLP_RESHIFT_0, 0x528 */
    uint32_t utmipll_hw_pwrdn_cfg0; /* _UTMIPLL_HW_PWRDN_CFG0_0, 0x52C */
    uint32_t pllu_hw_pwrdn_cfg0;    /* _PLLU_HW_PWRDN_CFG0_0, 0x530 */
    uint32_t xusb_pll_cfg0;         /* _XUSB_PLL_CFG0_0, 0x534 */
    uint32_t reserved51[1];         /* _reserved_51, 0x538 */
    uint32_t clk_cpu_misc;          /* _CLK_CPU_MISC_0, 0x53C */
    uint32_t clk_cpug_misc;         /* _CLK_CPUG_MISC_0, 0x540 */
    uint32_t clk_cpulp_misc;        /* _CLK_CPULP_MISC_0, 0x544 */
    uint32_t pllx_hw_ctrl_cfg;      /* _PLLX_HW_CTRL_CFG_0, 0x548 */
    uint32_t pllx_sw_ramp_cfg;      /* _PLLX_SW_RAMP_CFG_0, 0x54C */
    uint32_t pllx_hw_ctrl_status;   /* _PLLX_HW_CTRL_STATUS_0, 0x550 */
    uint32_t reserved52[1];         /* _reserved_52, 0x554 */
    uint32_t super_gr3d_clk_div;    /* _SUPER_GR3D_CLK_DIVIDER_0, 0x558 */
    uint32_t spare_reg0;            /* _SPARE_REG0_0, 0x55C */
    uint32_t _rsv32[4];             /*                    0x560-0x56c */
    uint32_t plld2_ss_cfg;          /* _PLLD2_SS_CFG            0x570 */
    uint32_t _rsv32_1[7];           /*                      0x574-58c */
    clk_pll_simple_t plldp;         /* _PLLDP_BASE, 0x590 _PLLDP_MISC */
    uint32_t plldp_ss_cfg;          /* _PLLDP_SS_CFG, 0x598 */

    /* Tegra124+ - skip to 0x600 here for new CLK_SOURCE_ regs */
    uint32_t _rsrv32_2[25];                     /* _0x59C - 0x5FC */
    uint32_t clk_src_x[TEGRA_CLK_SOURCES_X];    /* XUSB, etc, 0x600-0x67C */

    /* Tegra210 - skip to 0x694 here for new CLK_SOURCE_ regs */
    uint32_t reserved61[5];                     /* _reserved_61, 0x680 - 0x690 */
    
    /*
     * NOTE: PLLA1 regs are in the middle of this Y region. Break this in
     * two later if PLLA1 is needed, but for now this is cleaner.
     */
    uint32_t clk_src_y[TEGRA_CLK_SOURCES_Y]; /* SPARE1, etc, 0x694-0x6D8 */
} tegra_car_t;

static inline volatile tegra_car_t *car_get_regs(void)
{
    return (volatile tegra_car_t *)0x60006000;
}

#endif
