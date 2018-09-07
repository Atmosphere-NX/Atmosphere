/*
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

typedef struct {
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
    uint32_t _reserved0;
    uint32_t pllm_wb0_ovrride_frq;
    uint32_t test_pwrgate;
    uint32_t pwrgate_timer_mult;
    uint32_t dsi_sel_dpd;
    uint32_t utmip_uhsic_triggers;
    uint32_t utmip_uhsic_saved_st;
    uint32_t utmip_pad_cfg;
    uint32_t utmip_term_pad_cfg;
    uint32_t utmip_uhsic_sleep_cfg;
    uint32_t utmip_uhsic_sleepwalk_cfg;
    uint32_t utmip_sleepwalk_p[3];
    uint32_t uhsic_sleepwalk_p0;
    uint32_t utmip_uhsic_status;
    uint32_t utmip_uhsic_fake;
    uint32_t bo_mirror3[2];
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
    uint32_t utmip_uhsic_line_wakeup;
    uint32_t utmip_bias_master_cntrl;
    uint32_t utmip_master_config;
    uint32_t td_pwrgate_inter_part_timer;
    uint32_t utmip_uhsic2_triggers;
    uint32_t utmip_uhsic2_saved_state;
    uint32_t utmip_uhsic2_sleep_cfg;
    uint32_t utmip_uhsic2_sleepwalk_cfg;
    uint32_t uhsic2_sleepwalk_p1;
    uint32_t utmip_uhsic2_status;
    uint32_t utmip_uhsic2_fake;
    uint32_t utmip_uhsic2_line_wakeup;
    uint32_t utmip_master2_config;
    uint32_t utmip_uhsic_rpd_cfg;
    uint32_t pg_mask_ce0;
    uint32_t pg_mask3[2];
    uint32_t pllm_wb0_override2;
    uint32_t tsc_mult;
    uint32_t cpu_vsense_override;
    uint32_t glb_amap_cfg;
    uint32_t sticky_bits;
    uint32_t sec_disable2;
    uint32_t weak_bias;
    uint32_t reg_short;
    uint32_t pg_mask_andor;
    uint32_t _reserved1[11];
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
    uint32_t secure_scratch36;
    uint32_t secure_scratch37;
    uint32_t secure_scratch38;
    uint32_t secure_scratch39;
    uint32_t secure_scratch40;
    uint32_t secure_scratch41;
    uint32_t secure_scratch42;
    uint32_t secure_scratch43;
    uint32_t secure_scratch44;
    uint32_t secure_scratch45;
    uint32_t secure_scratch46;
    uint32_t secure_scratch47;
    uint32_t secure_scratch48;
    uint32_t secure_scratch49;
    uint32_t secure_scratch50;
    uint32_t secure_scratch51;
    uint32_t secure_scratch52;
    uint32_t secure_scratch53;
    uint32_t secure_scratch54;
    uint32_t secure_scratch55;
    uint32_t secure_scratch56;
    uint32_t secure_scratch57;
    uint32_t secure_scratch58;
    uint32_t secure_scratch59;
    uint32_t secure_scratch60;
    uint32_t secure_scratch61;
    uint32_t secure_scratch62;
    uint32_t secure_scratch63;
    uint32_t secure_scratch64;
    uint32_t secure_scratch65;
    uint32_t secure_scratch66;
    uint32_t secure_scratch67;
    uint32_t secure_scratch68;
    uint32_t secure_scratch69;
    uint32_t secure_scratch70;
    uint32_t secure_scratch71;
    uint32_t secure_scratch72;
    uint32_t secure_scratch73;
    uint32_t secure_scratch74;
    uint32_t secure_scratch75;
    uint32_t secure_scratch76;
    uint32_t secure_scratch77;
    uint32_t secure_scratch78;
    uint32_t secure_scratch79;
    uint32_t _reserved2[8];
    uint32_t cntrl2;
    uint32_t _reserved3[2];
    uint32_t event_counter;
    uint32_t fuse_control;
    uint32_t scratch1_eco;
    uint32_t _reserved4;
    uint32_t io_dpd3_req;
    uint32_t io_dpd3_status;
    uint32_t io_dpd4_req;
    uint32_t io_dpd4_status;
    uint32_t _reserved5[30];
    uint32_t ddr_cntrl;
    uint32_t _reserved6[70];
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
    uint32_t scratch120;
    uint32_t scratch121;
    uint32_t scratch122;
    uint32_t scratch123;
    uint32_t scratch124;
    uint32_t scratch125;
    uint32_t scratch126;
    uint32_t scratch127;
    uint32_t scratch128;
    uint32_t scratch129;
    uint32_t scratch130;
    uint32_t scratch131;
    uint32_t scratch132;
    uint32_t scratch133;
    uint32_t scratch134;
    uint32_t scratch135;
    uint32_t scratch136;
    uint32_t scratch137;
    uint32_t scratch138;
    uint32_t scratch139;
    uint32_t scratch140;
    uint32_t scratch141;
    uint32_t scratch142;
    uint32_t scratch143;
    uint32_t scratch144;
    uint32_t scratch145;
    uint32_t scratch146;
    uint32_t scratch147;
    uint32_t scratch148;
    uint32_t scratch149;
    uint32_t scratch150;
    uint32_t scratch151;
    uint32_t scratch152;
    uint32_t scratch153;
    uint32_t scratch154;
    uint32_t scratch155;
    uint32_t scratch156;
    uint32_t scratch157;
    uint32_t scratch158;
    uint32_t scratch159;
    uint32_t scratch160;
    uint32_t scratch161;
    uint32_t scratch162;
    uint32_t scratch163;
    uint32_t scratch164;
    uint32_t scratch165;
    uint32_t scratch166;
    uint32_t scratch167;
    uint32_t scratch168;
    uint32_t scratch169;
    uint32_t scratch170;
    uint32_t scratch171;
    uint32_t scratch172;
    uint32_t scratch173;
    uint32_t scratch174;
    uint32_t scratch175;
    uint32_t scratch176;
    uint32_t scratch177;
    uint32_t scratch178;
    uint32_t scratch179;
    uint32_t scratch180;
    uint32_t scratch181;
    uint32_t scratch182;
    uint32_t scratch183;
    uint32_t scratch184;
    uint32_t scratch185;
    uint32_t scratch186;
    uint32_t scratch187;
    uint32_t scratch188;
    uint32_t scratch189;
    uint32_t scratch190;
    uint32_t scratch191;
    uint32_t scratch192;
    uint32_t scratch193;
    uint32_t scratch194;
    uint32_t scratch195;
    uint32_t scratch196;
    uint32_t scratch197;
    uint32_t scratch198;
    uint32_t scratch199;
    uint32_t scratch200;
    uint32_t scratch201;
    uint32_t scratch202;
    uint32_t scratch203;
    uint32_t scratch204;
    uint32_t scratch205;
    uint32_t scratch206;
    uint32_t scratch207;
    uint32_t scratch208;
    uint32_t scratch209;
    uint32_t scratch210;
    uint32_t scratch211;
    uint32_t scratch212;
    uint32_t scratch213;
    uint32_t scratch214;
    uint32_t scratch215;
    uint32_t scratch216;
    uint32_t scratch217;
    uint32_t scratch218;
    uint32_t scratch219;
    uint32_t scratch220;
    uint32_t scratch221;
    uint32_t scratch222;
    uint32_t scratch223;
    uint32_t scratch224;
    uint32_t scratch225;
    uint32_t scratch226;
    uint32_t scratch227;
    uint32_t scratch228;
    uint32_t scratch229;
    uint32_t scratch230;
    uint32_t scratch231;
    uint32_t scratch232;
    uint32_t scratch233;
    uint32_t scratch234;
    uint32_t scratch235;
    uint32_t scratch236;
    uint32_t scratch237;
    uint32_t scratch238;
    uint32_t scratch239;
    uint32_t scratch240;
    uint32_t scratch241;
    uint32_t scratch242;
    uint32_t scratch243;
    uint32_t scratch244;
    uint32_t scratch245;
    uint32_t scratch246;
    uint32_t scratch247;
    uint32_t scratch248;
    uint32_t scratch249;
    uint32_t scratch250;
    uint32_t scratch251;
    uint32_t scratch252;
    uint32_t scratch253;
    uint32_t scratch254;
    uint32_t scratch255;
    uint32_t scratch256;
    uint32_t scratch257;
    uint32_t scratch258;
    uint32_t scratch259;
    uint32_t scratch260;
    uint32_t scratch261;
    uint32_t scratch262;
    uint32_t scratch263;
    uint32_t scratch264;
    uint32_t scratch265;
    uint32_t scratch266;
    uint32_t scratch267;
    uint32_t scratch268;
    uint32_t scratch269;
    uint32_t scratch270;
    uint32_t scratch271;
    uint32_t scratch272;
    uint32_t scratch273;
    uint32_t scratch274;
    uint32_t scratch275;
    uint32_t scratch276;
    uint32_t scratch277;
    uint32_t scratch278;
    uint32_t scratch279;
    uint32_t scratch280;
    uint32_t scratch281;
    uint32_t scratch282;
    uint32_t scratch283;
    uint32_t scratch284;
    uint32_t scratch285;
    uint32_t scratch286;
    uint32_t scratch287;
    uint32_t scratch288;
    uint32_t scratch289;
    uint32_t scratch290;
    uint32_t scratch291;
    uint32_t scratch292;
    uint32_t scratch293;
    uint32_t scratch294;
    uint32_t scratch295;
    uint32_t scratch296;
    uint32_t scratch297;
    uint32_t scratch298;
    uint32_t scratch299;
    uint32_t _reserved7[50];
    uint32_t secure_scratch80;
    uint32_t secure_scratch81;
    uint32_t secure_scratch82;
    uint32_t secure_scratch83;
    uint32_t secure_scratch84;
    uint32_t secure_scratch85;
    uint32_t secure_scratch86;
    uint32_t secure_scratch87;
    uint32_t secure_scratch88;
    uint32_t secure_scratch89;
    uint32_t secure_scratch90;
    uint32_t secure_scratch91;
    uint32_t secure_scratch92;
    uint32_t secure_scratch93;
    uint32_t secure_scratch94;
    uint32_t secure_scratch95;
    uint32_t secure_scratch96;
    uint32_t secure_scratch97;
    uint32_t secure_scratch98;
    uint32_t secure_scratch99;
    uint32_t secure_scratch100;
    uint32_t secure_scratch101;
    uint32_t secure_scratch102;
    uint32_t secure_scratch103;
    uint32_t secure_scratch104;
    uint32_t secure_scratch105;
    uint32_t secure_scratch106;
    uint32_t secure_scratch107;
    uint32_t secure_scratch108;
    uint32_t secure_scratch109;
    uint32_t secure_scratch110;
    uint32_t secure_scratch111;
    uint32_t secure_scratch112;
    uint32_t secure_scratch113;
    uint32_t secure_scratch114;
    uint32_t secure_scratch115;
    uint32_t secure_scratch116;
    uint32_t secure_scratch117;
    uint32_t secure_scratch118;
    uint32_t secure_scratch119;
} tegra_pmc_t;

static inline volatile tegra_pmc_t *pmc_get_regs(void)
{
    return (volatile tegra_pmc_t *)PMC_BASE;
}

#endif
