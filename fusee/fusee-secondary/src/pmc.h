#ifndef __FUSEE_PMC_H__
#define __FUSEE_PMC_H__

#include "utils.h"

#define PMC_BASE 0x7000E400


/* TODO: get rid of these defines; use the struct instead */
#define APBDEV_PMC_CONTROL MAKE_REG32(PMC_BASE + 0x00)

#define APBDEV_PMC_DPD_ENABLE_0 MAKE_REG32(PMC_BASE + 0x24)

#define APBDEV_PMC_PWRGATE_TOGGLE_0 MAKE_REG32(PMC_BASE + 0x30)
#define APBDEV_PMC_PWRGATE_STATUS_0 MAKE_REG32(PMC_BASE + 0x38)

#define APBDEV_PMC_SCRATCH0_0 MAKE_REG32(PMC_BASE + 0x50)

#define APBDEV_PMC_CRYPTO_OP_0 MAKE_REG32(PMC_BASE + 0xF4)

#define APBDEV_PM_0 MAKE_REG32(PMC_BASE + 0x14)
#define APBDEV_PMC_WAKE2_STATUS_0 MAKE_REG32(PMC_BASE + 0x168)
#define APBDEV_PMC_CNTRL2_0 MAKE_REG32(PMC_BASE + 0x440)

#define APBDEV_PMC_SCRATCH43_0 MAKE_REG32(PMC_BASE + 0x22C)

#define APBDEV_PMC_SCRATCH200_0 MAKE_REG32(PMC_BASE + 0x840)

/**
 * Definitions of the Tegra PMC.
 */
struct PACKED tegra_pmc {
    uint32_t cntrl;
    uint32_t sec_disable;
    uint32_t pmc_swrst;
    uint32_t wake_mask;
    uint32_t wake_lvl;
    uint32_t wake_status;
    uint32_t sw_wake_status;
    uint32_t dpd_pads_oride;
    uint32_t dpd_sample;
    uint32_t dpd_enable;
    uint32_t pwrgate_timer_off;
    uint32_t clamp_status;
    uint32_t pwrgate_toggle;
    uint32_t remove_clamping;
    uint32_t pwrgate_status;
    uint32_t pwrgood_timer;
    uint32_t blink_timer;
    uint32_t no_iopower;
    uint32_t pwr_det;
    uint32_t pwr_det_latch;

    uint32_t scratch0;
    uint32_t scratch1;
    uint32_t scratch2;
    uint32_t scratch3;
    uint32_t scratch4;
    uint32_t scratch5;
    uint32_t scratch6;
    uint32_t scratch7;
    uint32_t scratch8;
    uint32_t scratch9;
    uint32_t scratch10;
    uint32_t scratch11;
    uint32_t scratch12;
    uint32_t scratch13;
    uint32_t scratch14;
    uint32_t scratch15;
    uint32_t scratch16;
    uint32_t scratch17;
    uint32_t scratch18;
    uint32_t scratch19;
    uint32_t scratch20;
    uint32_t scratch21;
    uint32_t scratch22;
    uint32_t scratch23;

    uint32_t secure_scratch0;
    uint32_t secure_scratch1;
    uint32_t secure_scratch2;
    uint32_t secure_scratch3;
    uint32_t secure_scratch4;
    uint32_t secure_scratch5;

    uint32_t cpupwrgood_timer;
    uint32_t cpupwroff_timer;
    uint32_t pg_mask;
    uint32_t pg_mask_1;
    uint32_t auto_wake_lvl;
    uint32_t auto_wake_lvl_mask;
    uint32_t wake_delay;
    uint32_t pwr_det_val;
    uint32_t ddr_pwr;
    uint32_t usb_debounce_del;
    uint32_t usb_ao;
    uint32_t crypto_op;
    uint32_t pllp_wb0_override;

    uint32_t scratch24;
    uint32_t scratch25;
    uint32_t scratch26;
    uint32_t scratch27;
    uint32_t scratch28;
    uint32_t scratch29;
    uint32_t scratch30;
    uint32_t scratch31;
    uint32_t scratch32;
    uint32_t scratch33;
    uint32_t scratch34;
    uint32_t scratch35;
    uint32_t scratch36;
    uint32_t scratch37;
    uint32_t scratch38;
    uint32_t scratch39;
    uint32_t scratch40;
    uint32_t scratch41;
    uint32_t scratch42;

    uint32_t bo_mirror0;
    uint32_t bo_mirror1;
    uint32_t bo_mirror2;
    uint32_t sys_33v_en;
    uint32_t bo_mirror_access;
    uint32_t gate;
    uint32_t wake2_mask;
    uint32_t wake2_lvl;
    uint32_t wake2_stat;
    uint32_t sw_wake2_stat;
    uint32_t auto_wake2_lvl_mask;
    uint32_t pg_mask2;
    uint32_t pg_mask_ce1;
    uint32_t pg_mask_ce2;
    uint32_t pg_mask_ce3;
    uint32_t pwrgate_timer_ce0;
    uint32_t pwrgate_timer_ce1;
    uint32_t pwrgate_timer_ce2;
    uint32_t pwrgate_timer_ce3;
    uint32_t pwrgate_timer_ce4;
    uint32_t pwrgate_timer_ce5;
    uint32_t pwrgate_timer_ce6;
    uint32_t pcx_edpd_cntrl;
    uint32_t osc_edpd_over;
    uint32_t clk_out_cntrl;
    uint32_t sata_pwrgate;
    uint32_t sensor_ctrl;
    uint32_t reset_status;
    uint32_t io_dpd_req;
    uint32_t io_dpd_stat;
    uint32_t io_dpd2_req;
    uint32_t io_dpd2_stat;
    uint32_t sel_dpd_tim;
    uint32_t vddp_sel;

    uint32_t ddr_cfg;
    uint32_t e_no_vttgen;
    uint32_t reserved0;
    uint32_t pllm_wb0_ovrride_frq;
    uint32_t test_pwrgate;
    uint32_t pwrgate_timer_mult;
    uint32_t dsi_sel_dpd;
    uint32_t utmip_uhsic_triggers;
    uint32_t utmip_uhsic_saved_st;
    uint32_t utmip_pad_cfg;
    uint32_t utmip_term_pad_cfg;
    uint32_t utmip_uhsic_sleep_cfg;

    uint32_t todo_0[9];
    uint32_t secure_scratch6;
    uint32_t secure_scratch7;
    uint32_t scratch43;
    uint32_t scratch44;
    uint32_t scratch45;
    uint32_t scratch46;
    uint32_t scratch47;
    uint32_t scratch48;
    uint32_t scratch49;
    uint32_t scratch50;
    uint32_t scratch51;
    uint32_t scratch52;
    uint32_t scratch53;
    uint32_t scratch54;
    uint32_t scratch55;
    uint32_t scratch0_eco;
    uint32_t por_dpd_ctrl;
    uint32_t scratch2_eco;
    uint32_t todo_1[17];
    uint32_t pllm_wb0_override2;
    uint32_t tsc_mult;
    uint32_t cpu_vsense_override;
    uint32_t glb_amap_cfg;
    uint32_t sticky_bits;
    uint32_t sec_disable2;
    uint32_t weak_bias;
    uint32_t todo_3[13];
    uint32_t secure_scratch8;
    uint32_t secure_scratch9;
    uint32_t secure_scratch10;
    uint32_t secure_scratch11;
    uint32_t secure_scratch12;
    uint32_t secure_scratch13;
    uint32_t secure_scratch14;
    uint32_t secure_scratch15;
    uint32_t secure_scratch16;
    uint32_t secure_scratch17;
    uint32_t secure_scratch18;
    uint32_t secure_scratch19;
    uint32_t secure_scratch20;
    uint32_t secure_scratch21;
    uint32_t secure_scratch22;
    uint32_t secure_scratch23;
    uint32_t secure_scratch24;
    uint32_t secure_scratch25;
    uint32_t secure_scratch26;
    uint32_t secure_scratch27;
    uint32_t secure_scratch28;
    uint32_t secure_scratch29;
    uint32_t secure_scratch30;
    uint32_t secure_scratch31;
    uint32_t secure_scratch32;
    uint32_t secure_scratch33;
    uint32_t secure_scratch34;
    uint32_t secure_scratch35;

    uint32_t reserved1[52];
    uint32_t cntrl2;
    uint32_t reserved2[6];
    uint32_t io_dpd3_req;
    uint32_t io_dpd3_stat;
    uint32_t strap_opt_a;
    uint32_t reserved3[102];

    uint32_t scratch56;
    uint32_t scratch57;
    uint32_t scratch58;
    uint32_t scratch59;
    uint32_t scratch60;
    uint32_t scratch61;
    uint32_t scratch62;
    uint32_t scratch63;
    uint32_t scratch64;
    uint32_t scratch65;
    uint32_t scratch66;
    uint32_t scratch67;
    uint32_t scratch68;
    uint32_t scratch69;
    uint32_t scratch70;
    uint32_t scratch71;
    uint32_t scratch72;
    uint32_t scratch73;
    uint32_t scratch74;
    uint32_t scratch75;
    uint32_t scratch76;
    uint32_t scratch77;
    uint32_t scratch78;
    uint32_t scratch79;
    uint32_t scratch80;
    uint32_t scratch81;
    uint32_t scratch82;
    uint32_t scratch83;
    uint32_t scratch84;
    uint32_t scratch85;
    uint32_t scratch86;
    uint32_t scratch87;
    uint32_t scratch88;
    uint32_t scratch89;
    uint32_t scratch90;
    uint32_t scratch91;
    uint32_t scratch92;
    uint32_t scratch93;
    uint32_t scratch94;
    uint32_t scratch95;
    uint32_t scratch96;
    uint32_t scratch97;
    uint32_t scratch98;
    uint32_t scratch99;
    uint32_t scratch100;
    uint32_t scratch101;
    uint32_t scratch102;
    uint32_t scratch103;
    uint32_t scratch104;
    uint32_t scratch105;
    uint32_t scratch106;
    uint32_t scratch107;
    uint32_t scratch108;
    uint32_t scratch109;
    uint32_t scratch110;
    uint32_t scratch111;
    uint32_t scratch112;
    uint32_t scratch113;
    uint32_t scratch114;
    uint32_t scratch115;
    uint32_t scratch116;
    uint32_t scratch117;
    uint32_t scratch118;
    uint32_t scratch119;
    uint32_t scratch1_eco;
};

enum tegra_pmc_masks {
    /* NO_IOPOWER, power detect, ect. */
    PMC_CONTROL_SDMMC1 = (1 << 12),
    PMC_CONTROL_SDMMC3 = (1 << 13),
    PMC_CONTROL_SDMMC4 = (1 << 14),
};


/**
 * Utility function that grabs the Tegra pinmux registers.
 */
static inline struct tegra_pmc *pmc_get_regs(void)
{
    return (struct tegra_pmc *)0x7000e400;
}


#endif
