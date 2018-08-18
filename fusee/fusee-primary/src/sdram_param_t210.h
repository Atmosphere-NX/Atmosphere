/*
 * Copyright (c) 2015, NVIDIA CORPORATION.  All rights reserved.
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
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 */

/**
 * Defines the SDRAM parameter structure.
 *
 * Note that PLLM is used by EMC.
 */

#ifndef _SDRAM_PARAM_T210_H_
#define _SDRAM_PARAM_T210_H_

#include <stdint.h>

#define MEMORY_TYPE_NONE 0
#define MEMORY_TYPE_DDR 0
#define MEMORY_TYPE_LPDDR 0
#define MEMORY_TYPE_DDR2 0
#define MEMORY_TYPE_LPDDR2 1
#define MEMORY_TYPE_DDR3 2
#define MEMORY_TYPE_LPDDR4 3

/**
 * Defines the SDRAM parameter structure
 */
typedef struct _sdram_params
{
    /* Specifies the type of memory device */
    uint32_t memory_type;

    /* MC/EMC clock source configuration */

    /* Specifies the M value for PllM */
    uint32_t pllm_input_divider;
    /* Specifies the N value for PllM */
    uint32_t pllm_feedback_divider;
    /* Specifies the time to wait for PLLM to lock (in microseconds) */
    uint32_t pllm_stable_time;
    /* Specifies misc. control bits */
    uint32_t pllm_setup_control;
    /* Specifies the P value for PLLM */
    uint32_t pllm_post_divider;
    /* Specifies value for Charge Pump Gain Control */
    uint32_t pllm_kcp;
    /* Specifies VCO gain */
    uint32_t pllm_kvco;
    /* Spare BCT param */
    uint32_t emc_bct_spare0;
    /* Spare BCT param */
    uint32_t emc_bct_spare1;
    /* Spare BCT param */
    uint32_t emc_bct_spare2;
    /* Spare BCT param */
    uint32_t emc_bct_spare3;
    /* Spare BCT param */
    uint32_t emc_bct_spare4;
    /* Spare BCT param */
    uint32_t emc_bct_spare5;
    /* Spare BCT param */
    uint32_t emc_bct_spare6;
    /* Spare BCT param */
    uint32_t emc_bct_spare7;
    /* Spare BCT param */
    uint32_t emc_bct_spare8;
    /* Spare BCT param */
    uint32_t emc_bct_spare9;
    /* Spare BCT param */
    uint32_t emc_bct_spare10;
    /* Spare BCT param */
    uint32_t emc_bct_spare11;
    /* Spare BCT param */
    uint32_t emc_bct_spare12;
    /* Spare BCT param */
    uint32_t emc_bct_spare13;

    /* Defines EMC_2X_CLK_SRC, EMC_2X_CLK_DIVISOR, EMC_INVERT_DCD */
    uint32_t emc_clock_source;
    uint32_t emc_clock_source_dll;

    /* Defines possible override for PLLLM_MISC2 */
    uint32_t clk_rst_pllm_misc20_override;
    /* enables override for PLLLM_MISC2 */
    uint32_t clk_rst_pllm_misc20_override_enable;
    /* defines CLK_ENB_MC1 in register clk_rst_controller_clk_enb_w_clr */
    uint32_t clear_clock2_mc1;

    /* Auto-calibration of EMC pads */

    /* Specifies the value for EMC_AUTO_CAL_INTERVAL */
    uint32_t emc_auto_cal_interval;
    /*
     * Specifies the value for EMC_AUTO_CAL_CONFIG
     * Note: Trigger bits are set by the SDRAM code.
     */
    uint32_t emc_auto_cal_config;

    /* Specifies the value for EMC_AUTO_CAL_CONFIG2 */
    uint32_t emc_auto_cal_config2;

    /* Specifies the value for EMC_AUTO_CAL_CONFIG3 */
    uint32_t emc_auto_cal_config3;

    uint32_t emc_auto_cal_config4;
    uint32_t emc_auto_cal_config5;
    uint32_t emc_auto_cal_config6;
    uint32_t emc_auto_cal_config7;
    uint32_t emc_auto_cal_config8;
    /* Specifies the value for EMC_AUTO_CAL_VREF_SEL_0 */
    uint32_t emc_auto_cal_vref_sel0;
    uint32_t emc_auto_cal_vref_sel1;

    /* Specifies the value for EMC_AUTO_CAL_CHANNEL */
    uint32_t emc_auto_cal_channel;

    /* Specifies the value for EMC_PMACRO_AUTOCAL_CFG_0 */
    uint32_t emc_pmacro_auto_cal_cfg0;
    uint32_t emc_pmacro_auto_cal_cfg1;
    uint32_t emc_pmacro_auto_cal_cfg2;

    uint32_t emc_pmacro_rx_term;
    uint32_t emc_pmacro_dq_tx_drive;
    uint32_t emc_pmacro_ca_tx_drive;
    uint32_t emc_pmacro_cmd_tx_drive;
    uint32_t emc_pmacro_auto_cal_common;
    uint32_t emc_pmacro_zcrtl;

    /*
     * Specifies the time for the calibration
     * to stabilize (in microseconds)
     */
    uint32_t emc_auto_cal_wait;

    uint32_t emc_xm2_comp_pad_ctrl;
    uint32_t emc_xm2_comp_pad_ctrl2;
    uint32_t emc_xm2_comp_pad_ctrl3;

    /*
     * DRAM size information
     * Specifies the value for EMC_ADR_CFG
     */
    uint32_t emc_adr_cfg;

    /*
     * Specifies the time to wait after asserting pin
     * CKE (in microseconds)
     */
    uint32_t emc_pin_program_wait;
    /* Specifies the extra delay before/after pin RESET/CKE command */
    uint32_t emc_pin_extra_wait;

    uint32_t emc_pin_gpio_enable;
    uint32_t emc_pin_gpio;

    /*
     * Specifies the extra delay after the first writing
     * of EMC_TIMING_CONTROL
     */
    uint32_t emc_timing_control_wait;

    /* Timing parameters required for the SDRAM */

    /* Specifies the value for EMC_RC */
    uint32_t emc_rc;
    /* Specifies the value for EMC_RFC */
    uint32_t emc_rfc;

    uint32_t emc_rfc_pb;
    uint32_t emc_ref_ctrl2;

    /* Specifies the value for EMC_RFC_SLR */
    uint32_t emc_rfc_slr;
    /* Specifies the value for EMC_RAS */
    uint32_t emc_ras;
    /* Specifies the value for EMC_RP */
    uint32_t emc_rp;
    /* Specifies the value for EMC_R2R */
    uint32_t emc_r2r;
    /* Specifies the value for EMC_W2W */
    uint32_t emc_w2w;
    /* Specifies the value for EMC_R2W */
    uint32_t emc_r2w;
    /* Specifies the value for EMC_W2R */
    uint32_t emc_w2r;
    /* Specifies the value for EMC_R2P */
    uint32_t emc_r2p;
    /* Specifies the value for EMC_W2P */
    uint32_t emc_w2p;
    /* Specifies the value for EMC_RD_RCD */

    uint32_t emc_tppd;
    uint32_t emc_ccdmw;

    uint32_t emc_rd_rcd;
    /* Specifies the value for EMC_WR_RCD */
    uint32_t emc_wr_rcd;
    /* Specifies the value for EMC_RRD */
    uint32_t emc_rrd;
    /* Specifies the value for EMC_REXT */
    uint32_t emc_rext;
    /* Specifies the value for EMC_WEXT */
    uint32_t emc_wext;
    /* Specifies the value for EMC_WDV */
    uint32_t emc_wdv;

    uint32_t emc_wdv_chk;
    uint32_t emc_wsv;
    uint32_t emc_wev;

    /* Specifies the value for EMC_WDV_MASK */
    uint32_t emc_wdv_mask;

    uint32_t emc_ws_duration;
    uint32_t emc_we_duration;

    /* Specifies the value for EMC_QUSE */
    uint32_t emc_quse;
    /* Specifies the value for EMC_QUSE_WIDTH */
    uint32_t emc_quse_width;
    /* Specifies the value for EMC_IBDLY */
    uint32_t emc_ibdly;

    uint32_t emc_obdly;

    /* Specifies the value for EMC_EINPUT */
    uint32_t emc_einput;
    /* Specifies the value for EMC_EINPUT_DURATION */
    uint32_t emc_einput_duration;
    /* Specifies the value for EMC_PUTERM_EXTRA */
    uint32_t emc_puterm_extra;
    /* Specifies the value for EMC_PUTERM_WIDTH */
    uint32_t emc_puterm_width;

    uint32_t emc_qrst;
    uint32_t emc_qsafe;
    uint32_t emc_rdv;
    uint32_t emc_rdv_mask;

    uint32_t emc_rdv_early;
    uint32_t emc_rdv_early_mask;

    /* Specifies the value for EMC_QPOP */
    uint32_t emc_qpop;

    /* Specifies the value for EMC_REFRESH */
    uint32_t emc_refresh;
    /* Specifies the value for EMC_BURST_REFRESH_NUM */
    uint32_t emc_burst_refresh_num;
    /* Specifies the value for EMC_PRE_REFRESH_REQ_CNT */
    uint32_t emc_prerefresh_req_cnt;
    /* Specifies the value for EMC_PDEX2WR */
    uint32_t emc_pdex2wr;
    /* Specifies the value for EMC_PDEX2RD */
    uint32_t emc_pdex2rd;
    /* Specifies the value for EMC_PCHG2PDEN */
    uint32_t emc_pchg2pden;
    /* Specifies the value for EMC_ACT2PDEN */
    uint32_t emc_act2pden;
    /* Specifies the value for EMC_AR2PDEN */
    uint32_t emc_ar2pden;
    /* Specifies the value for EMC_RW2PDEN */
    uint32_t emc_rw2pden;

    uint32_t emc_cke2pden;
    uint32_t emc_pdex2che;
    uint32_t emc_pdex2mrr;

    /* Specifies the value for EMC_TXSR */
    uint32_t emc_txsr;
    /* Specifies the value for EMC_TXSRDLL */
    uint32_t emc_txsr_dll;
    /* Specifies the value for EMC_TCKE */
    uint32_t emc_tcke;
    /* Specifies the value for EMC_TCKESR */
    uint32_t emc_tckesr;
    /* Specifies the value for EMC_TPD */
    uint32_t emc_tpd;
    /* Specifies the value for EMC_TFAW */
    uint32_t emc_tfaw;
    /* Specifies the value for EMC_TRPAB */
    uint32_t emc_trpab;
    /* Specifies the value for EMC_TCLKSTABLE */
    uint32_t emc_tclkstable;
    /* Specifies the value for EMC_TCLKSTOP */
    uint32_t emc_tclkstop;
    /* Specifies the value for EMC_TREFBW */
    uint32_t emc_trefbw;

    /* FBIO configuration values */

    /* Specifies the value for EMC_FBIO_CFG5 */
    uint32_t emc_fbio_cfg5;
    /* Specifies the value for EMC_FBIO_CFG7 */
    uint32_t emc_fbio_cfg7;
    uint32_t emc_fbio_cfg8;

    /* Command mapping for CMD brick 0 */
    uint32_t emc_cmd_mapping_cmd0_0;
    uint32_t emc_cmd_mapping_cmd0_1;
    uint32_t emc_cmd_mapping_cmd0_2;
    uint32_t emc_cmd_mapping_cmd1_0;
    uint32_t emc_cmd_mapping_cmd1_1;
    uint32_t emc_cmd_mapping_cmd1_2;
    uint32_t emc_cmd_mapping_cmd2_0;
    uint32_t emc_cmd_mapping_cmd2_1;
    uint32_t emc_cmd_mapping_cmd2_2;
    uint32_t emc_cmd_mapping_cmd3_0;
    uint32_t emc_cmd_mapping_cmd3_1;
    uint32_t emc_cmd_mapping_cmd3_2;
    uint32_t emc_cmd_mapping_byte;

    /* Specifies the value for EMC_FBIO_SPARE */
    uint32_t emc_fbio_spare;

    /* Specifies the value for EMC_CFG_RSV */
    uint32_t emc_cfg_rsv;

    /* MRS command values */

    /* Specifies the value for EMC_MRS */
    uint32_t emc_mrs;
    /* Specifies the MP0 command to initialize mode registers */
    uint32_t emc_emrs;
    /* Specifies the MP2 command to initialize mode registers */
    uint32_t emc_emrs2;
    /* Specifies the MP3 command to initialize mode registers */
    uint32_t emc_emrs3;
    /* Specifies the programming to LPDDR2 Mode Register 1 at cold boot */
    uint32_t emc_mrw1;
    /* Specifies the programming to LPDDR2 Mode Register 2 at cold boot */
    uint32_t emc_mrw2;
    /* Specifies the programming to LPDDR2 Mode Register 3 at cold boot */
    uint32_t emc_mrw3;
    /* Specifies the programming to LPDDR2 Mode Register 11 at cold boot */
    uint32_t emc_mrw4;

    /* Specifies the programming to LPDDR4 Mode Register 3 at cold boot */
    uint32_t emc_mrw6;
    /* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
    uint32_t emc_mrw8;
    /* Specifies the programming to LPDDR4 Mode Register 11 at cold boot */
    uint32_t emc_mrw9;
    /* Specifies the programming to LPDDR4 Mode Register 12 at cold boot */
    uint32_t emc_mrw10;
    /* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
    uint32_t emc_mrw12;
    /* Specifies the programming to LPDDR4 Mode Register 14 at cold boot */
    uint32_t emc_mrw13;
    /* Specifies the programming to LPDDR4 Mode Register 22 at cold boot */
    uint32_t emc_mrw14;

    /*
     * Specifies the programming to extra LPDDR2 Mode Register
     * at cold boot
     */
    uint32_t emc_mrw_extra;
    /*
     * Specifies the programming to extra LPDDR2 Mode Register
     * at warm boot
     */
    uint32_t emc_warm_boot_mrw_extra;
    /*
     * Specify the enable of extra Mode Register programming at
     * warm boot
     */
    uint32_t emc_warm_boot_extramode_reg_write_enable;
    /*
     * Specify the enable of extra Mode Register programming at
     * cold boot
     */
    uint32_t emc_extramode_reg_write_enable;

    /* Specifies the EMC_MRW reset command value */
    uint32_t emc_mrw_reset_command;
    /* Specifies the EMC Reset wait time (in microseconds) */
    uint32_t emc_mrw_reset_ninit_wait;
    /* Specifies the value for EMC_MRS_WAIT_CNT */
    uint32_t emc_mrs_wait_cnt;
    /* Specifies the value for EMC_MRS_WAIT_CNT2 */
    uint32_t emc_mrs_wait_cnt2;

    /* EMC miscellaneous configurations */

    /* Specifies the value for EMC_CFG */
    uint32_t emc_cfg;
    /* Specifies the value for EMC_CFG_2 */
    uint32_t emc_cfg2;
    /* Specifies the pipe bypass controls */
    uint32_t emc_cfg_pipe;

    uint32_t emc_cfg_pipe_clk;
    uint32_t emc_fdpd_ctrl_cmd_no_ramp;
    uint32_t emc_cfg_update;

    /* Specifies the value for EMC_DBG */
    uint32_t emc_dbg;

    uint32_t emc_dbg_write_mux;

    /* Specifies the value for EMC_CMDQ */
    uint32_t emc_cmd_q;
    /* Specifies the value for EMC_MC2EMCQ */
    uint32_t emc_mc2emc_q;
    /* Specifies the value for EMC_DYN_SELF_REF_CONTROL */
    uint32_t emc_dyn_self_ref_control;

    /* Specifies the value for MEM_INIT_DONE */
    uint32_t ahb_arbitration_xbar_ctrl_meminit_done;

    /* Specifies the value for EMC_CFG_DIG_DLL */
    uint32_t emc_cfg_dig_dll;
    uint32_t emc_cfg_dig_dll_1;

    /* Specifies the value for EMC_CFG_DIG_DLL_PERIOD */
    uint32_t emc_cfg_dig_dll_period;
    /* Specifies the value of *DEV_SELECTN of various EMC registers */
    uint32_t emc_dev_select;

    /* Specifies the value for EMC_SEL_DPD_CTRL */
    uint32_t emc_sel_dpd_ctrl;

    /* Pads trimmer delays */
    uint32_t emc_fdpd_ctrl_dq;
    uint32_t emc_fdpd_ctrl_cmd;
    uint32_t emc_pmacro_ib_vref_dq_0;
    uint32_t emc_pmacro_ib_vref_dq_1;
    uint32_t emc_pmacro_ib_vref_dqs_0;
    uint32_t emc_pmacro_ib_vref_dqs_1;
    uint32_t emc_pmacro_ib_rxrt;
    uint32_t emc_cfg_pipe1;
    uint32_t emc_cfg_pipe2;

    /* Specifies the value for EMC_PMACRO_QUSE_DDLL_RANK0_0 */
    uint32_t emc_pmacro_quse_ddll_rank0_0;
    uint32_t emc_pmacro_quse_ddll_rank0_1;
    uint32_t emc_pmacro_quse_ddll_rank0_2;
    uint32_t emc_pmacro_quse_ddll_rank0_3;
    uint32_t emc_pmacro_quse_ddll_rank0_4;
    uint32_t emc_pmacro_quse_ddll_rank0_5;
    uint32_t emc_pmacro_quse_ddll_rank1_0;
    uint32_t emc_pmacro_quse_ddll_rank1_1;
    uint32_t emc_pmacro_quse_ddll_rank1_2;
    uint32_t emc_pmacro_quse_ddll_rank1_3;
    uint32_t emc_pmacro_quse_ddll_rank1_4;
    uint32_t emc_pmacro_quse_ddll_rank1_5;

    uint32_t emc_pmacro_ob_ddll_long_dq_rank0_0;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank0_1;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank0_2;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank0_3;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank0_4;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank0_5;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank1_0;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank1_1;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank1_2;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank1_3;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank1_4;
    uint32_t emc_pmacro_ob_ddll_long_dq_rank1_5;

    uint32_t emc_pmacro_ob_ddll_long_dqs_rank0_0;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank0_1;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank0_2;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank0_3;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank0_4;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank0_5;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank1_0;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank1_1;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank1_2;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank1_3;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank1_4;
    uint32_t emc_pmacro_ob_ddll_long_dqs_rank1_5;

    uint32_t emc_pmacro_ib_ddll_long_dqs_rank0_0;
    uint32_t emc_pmacro_ib_ddll_long_dqs_rank0_1;
    uint32_t emc_pmacro_ib_ddll_long_dqs_rank0_2;
    uint32_t emc_pmacro_ib_ddll_long_dqs_rank0_3;
    uint32_t emc_pmacro_ib_ddll_long_dqs_rank1_0;
    uint32_t emc_pmacro_ib_ddll_long_dqs_rank1_1;
    uint32_t emc_pmacro_ib_ddll_long_dqs_rank1_2;
    uint32_t emc_pmacro_ib_ddll_long_dqs_rank1_3;

    uint32_t emc_pmacro_ddll_long_cmd_0;
    uint32_t emc_pmacro_ddll_long_cmd_1;
    uint32_t emc_pmacro_ddll_long_cmd_2;
    uint32_t emc_pmacro_ddll_long_cmd_3;
    uint32_t emc_pmacro_ddll_long_cmd_4;
    uint32_t emc_pmacro_ddll_short_cmd_0;
    uint32_t emc_pmacro_ddll_short_cmd_1;
    uint32_t emc_pmacro_ddll_short_cmd_2;

    /*
     * Specifies the delay after asserting CKE pin during a WarmBoot0
     * sequence (in microseconds)
     */
    uint32_t warm_boot_wait;

    /* Specifies the value for EMC_ODT_WRITE */
    uint32_t emc_odt_write;

    /* Periodic ZQ calibration */

    /*
     * Specifies the value for EMC_ZCAL_INTERVAL
     * Value 0 disables ZQ calibration
     */
    uint32_t emc_zcal_interval;
    /* Specifies the value for EMC_ZCAL_WAIT_CNT */
    uint32_t emc_zcal_wait_cnt;
    /* Specifies the value for EMC_ZCAL_MRW_CMD */
    uint32_t emc_zcal_mrw_cmd;

    /* DRAM initialization sequence flow control */

    /* Specifies the MRS command value for resetting DLL */
    uint32_t emc_mrs_reset_dll;
    /* Specifies the command for ZQ initialization of device 0 */
    uint32_t emc_zcal_init_dev0;
    /* Specifies the command for ZQ initialization of device 1 */
    uint32_t emc_zcal_init_dev1;
    /*
     * Specifies the wait time after programming a ZQ initialization
     * command (in microseconds)
     */
    uint32_t emc_zcal_init_wait;
    /*
     * Specifies the enable for ZQ calibration at cold boot [bit 0]
     * and warm boot [bit 1]
     */
    uint32_t emc_zcal_warm_cold_boot_enables;

    /*
     * Specifies the MRW command to LPDDR2 for ZQ calibration
     * on warmboot
     */
    /* Is issued to both devices separately */
    uint32_t emc_mrw_lpddr2zcal_warm_boot;
    /*
     * Specifies the ZQ command to DDR3 for ZQ calibration on warmboot
     * Is issued to both devices separately
     */
    uint32_t emc_zqcal_ddr3_warm_boot;

    uint32_t emc_zqcal_lpddr4_warm_boot;

    /*
     * Specifies the wait time for ZQ calibration on warmboot
     * (in microseconds)
     */
    uint32_t emc_zcal_warm_boot_wait;
    /*
     * Specifies the enable for DRAM Mode Register programming
     * at warm boot
     */
    uint32_t emc_mrs_warm_boot_enable;
    /*
     * Specifies the wait time after sending an MRS DLL reset command
     * in microseconds)
     */
    uint32_t emc_mrs_reset_dll_wait;
    /* Specifies the extra MRS command to initialize mode registers */
    uint32_t emc_mrs_extra;
    /* Specifies the extra MRS command at warm boot */
    uint32_t emc_warm_boot_mrs_extra;
    /* Specifies the EMRS command to enable the DDR2 DLL */
    uint32_t emc_emrs_ddr2_dll_enable;
    /* Specifies the MRS command to reset the DDR2 DLL */
    uint32_t emc_mrs_ddr2_dll_reset;
    /* Specifies the EMRS command to set OCD calibration */
    uint32_t emc_emrs_ddr2_ocd_calib;
    /*
     * Specifies the wait between initializing DDR and setting OCD
     * calibration (in microseconds)
     */
    uint32_t emc_ddr2_wait;
    /* Specifies the value for EMC_CLKEN_OVERRIDE */
    uint32_t emc_clken_override;
    /*
     * Specifies LOG2 of the extra refresh numbers after booting
     * Program 0 to disable
     */
    uint32_t emc_extra_refresh_num;
    /* Specifies the master override for all EMC clocks */
    uint32_t emc_clken_override_allwarm_boot;
    /* Specifies the master override for all MC clocks */
    uint32_t mc_clken_override_allwarm_boot;
    /* Specifies digital dll period, choosing between 4 to 64 ms */
    uint32_t emc_cfg_dig_dll_period_warm_boot;

    /* Pad controls */

    /* Specifies the value for PMC_VDDP_SEL */
    uint32_t pmc_vddp_sel;
    /* Specifies the wait time after programming PMC_VDDP_SEL */
    uint32_t pmc_vddp_sel_wait;
    /* Specifies the value for PMC_DDR_PWR */
    uint32_t pmc_ddr_pwr;
    /* Specifies the value for PMC_DDR_CFG */
    uint32_t pmc_ddr_cfg;
    /* Specifies the value for PMC_IO_DPD3_REQ */
    uint32_t pmc_io_dpd3_req;
    /* Specifies the wait time after programming PMC_IO_DPD3_REQ */
    uint32_t pmc_io_dpd3_req_wait;

    uint32_t pmc_io_dpd4_req_wait;

    /* Specifies the value for PMC_REG_SHORT */
    uint32_t pmc_reg_short;
    /* Specifies the value for PMC_NO_IOPOWER */
    uint32_t pmc_no_io_power;

    uint32_t pmc_ddr_ctrl_wait;
    uint32_t pmc_ddr_ctrl;

    /* Specifies the value for EMC_ACPD_CONTROL */
    uint32_t emc_acpd_control;

    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE0 */
    uint32_t emc_swizzle_rank0_byte0;
    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE1 */
    uint32_t emc_swizzle_rank0_byte1;
    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE2 */
    uint32_t emc_swizzle_rank0_byte2;
    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE3 */
    uint32_t emc_swizzle_rank0_byte3;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE0 */
    uint32_t emc_swizzle_rank1_byte0;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE1 */
    uint32_t emc_swizzle_rank1_byte1;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE2 */
    uint32_t emc_swizzle_rank1_byte2;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE3 */
    uint32_t emc_swizzle_rank1_byte3;

    /* Specifies the value for EMC_TXDSRVTTGEN */
    uint32_t emc_txdsrvttgen;

    /* Specifies the value for EMC_DATA_BRLSHFT_0 */
    uint32_t emc_data_brlshft0;
    uint32_t emc_data_brlshft1;

    uint32_t emc_dqs_brlshft0;
    uint32_t emc_dqs_brlshft1;

    uint32_t emc_cmd_brlshft0;
    uint32_t emc_cmd_brlshft1;
    uint32_t emc_cmd_brlshft2;
    uint32_t emc_cmd_brlshft3;

    uint32_t emc_quse_brlshft0;
    uint32_t emc_quse_brlshft1;
    uint32_t emc_quse_brlshft2;
    uint32_t emc_quse_brlshft3;

    uint32_t emc_dll_cfg0;
    uint32_t emc_dll_cfg1;

    uint32_t emc_pmc_scratch1;
    uint32_t emc_pmc_scratch2;
    uint32_t emc_pmc_scratch3;

    uint32_t emc_pmacro_pad_cfg_ctrl;

    uint32_t emc_pmacro_vttgen_ctrl0;
    uint32_t emc_pmacro_vttgen_ctrl1;
    uint32_t emc_pmacro_vttgen_ctrl2;

    uint32_t emc_pmacro_brick_ctrl_rfu1;
    uint32_t emc_pmacro_cmd_brick_ctrl_fdpd;
    uint32_t emc_pmacro_brick_ctrl_rfu2;
    uint32_t emc_pmacro_data_brick_ctrl_fdpd;
    uint32_t emc_pmacro_bg_bias_ctrl0;
    uint32_t emc_pmacro_data_pad_rx_ctrl;
    uint32_t emc_pmacro_cmd_pad_rx_ctrl;
    uint32_t emc_pmacro_data_rx_term_mode;
    uint32_t emc_pmacro_cmd_rx_term_mode;
    uint32_t emc_pmacro_data_pad_tx_ctrl;
    uint32_t emc_pmacro_common_pad_tx_ctrl;
    uint32_t emc_pmacro_cmd_pad_tx_ctrl;
    uint32_t emc_cfg3;

    uint32_t emc_pmacro_tx_pwrd0;
    uint32_t emc_pmacro_tx_pwrd1;
    uint32_t emc_pmacro_tx_pwrd2;
    uint32_t emc_pmacro_tx_pwrd3;
    uint32_t emc_pmacro_tx_pwrd4;
    uint32_t emc_pmacro_tx_pwrd5;

    uint32_t emc_config_sample_delay;

    uint32_t emc_pmacro_brick_mapping0;
    uint32_t emc_pmacro_brick_mapping1;
    uint32_t emc_pmacro_brick_mapping2;

    uint32_t emc_pmacro_tx_sel_clk_src0;
    uint32_t emc_pmacro_tx_sel_clk_src1;
    uint32_t emc_pmacro_tx_sel_clk_src2;
    uint32_t emc_pmacro_tx_sel_clk_src3;
    uint32_t emc_pmacro_tx_sel_clk_src4;
    uint32_t emc_pmacro_tx_sel_clk_src5;

    uint32_t emc_pmacro_ddll_bypass;

    uint32_t emc_pmacro_ddll_pwrd0;
    uint32_t emc_pmacro_ddll_pwrd1;
    uint32_t emc_pmacro_ddll_pwrd2;

    uint32_t emc_pmacro_cmd_ctrl0;
    uint32_t emc_pmacro_cmd_ctrl1;
    uint32_t emc_pmacro_cmd_ctrl2;

    /* DRAM size information */

    /* Specifies the value for MC_EMEM_ADR_CFG */
    uint32_t mc_emem_adr_cfg;
    /* Specifies the value for MC_EMEM_ADR_CFG_DEV0 */
    uint32_t mc_emem_adr_cfg_dev0;
    /* Specifies the value for MC_EMEM_ADR_CFG_DEV1 */
    uint32_t mc_emem_adr_cfg_dev1;

    uint32_t mc_emem_adr_cfg_channel_mask;

    /* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG0 */
    uint32_t mc_emem_adr_cfg_bank_mask0;
    /* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG1 */
    uint32_t mc_emem_adr_cfg_bank_mask1;
    /* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG2 */
    uint32_t mc_emem_adr_cfg_bank_mask2;

    /*
     * Specifies the value for MC_EMEM_CFG which holds the external memory
     * size (in KBytes)
     */
    uint32_t mc_emem_cfg;

    /* MC arbitration configuration */

    /* Specifies the value for MC_EMEM_ARB_CFG */
    uint32_t mc_emem_arb_cfg;
    /* Specifies the value for MC_EMEM_ARB_OUTSTANDING_REQ */
    uint32_t mc_emem_arb_outstanding_req;

    uint32_t emc_emem_arb_refpb_hp_ctrl;
    uint32_t emc_emem_arb_refpb_bank_ctrl;

    /* Specifies the value for MC_EMEM_ARB_TIMING_RCD */
    uint32_t mc_emem_arb_timing_rcd;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RP */
    uint32_t mc_emem_arb_timing_rp;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RC */
    uint32_t mc_emem_arb_timing_rc;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RAS */
    uint32_t mc_emem_arb_timing_ras;
    /* Specifies the value for MC_EMEM_ARB_TIMING_FAW */
    uint32_t mc_emem_arb_timing_faw;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RRD */
    uint32_t mc_emem_arb_timing_rrd;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RAP2PRE */
    uint32_t mc_emem_arb_timing_rap2pre;
    /* Specifies the value for MC_EMEM_ARB_TIMING_WAP2PRE */
    uint32_t mc_emem_arb_timing_wap2pre;
    /* Specifies the value for MC_EMEM_ARB_TIMING_R2R */
    uint32_t mc_emem_arb_timing_r2r;
    /* Specifies the value for MC_EMEM_ARB_TIMING_W2W */
    uint32_t mc_emem_arb_timing_w2w;
    /* Specifies the value for MC_EMEM_ARB_TIMING_R2W */
    uint32_t mc_emem_arb_timing_r2w;
    /* Specifies the value for MC_EMEM_ARB_TIMING_W2R */
    uint32_t mc_emem_arb_timing_w2r;

    uint32_t mc_emem_arb_timing_rfcpb;

    /* Specifies the value for MC_EMEM_ARB_DA_TURNS */
    uint32_t mc_emem_arb_da_turns;
    /* Specifies the value for MC_EMEM_ARB_DA_COVERS */
    uint32_t mc_emem_arb_da_covers;
    /* Specifies the value for MC_EMEM_ARB_MISC0 */
    uint32_t mc_emem_arb_misc0;
    /* Specifies the value for MC_EMEM_ARB_MISC1 */
    uint32_t mc_emem_arb_misc1;
    uint32_t mc_emem_arb_misc2;

    /* Specifies the value for MC_EMEM_ARB_RING1_THROTTLE */
    uint32_t mc_emem_arb_ring1_throttle;
    /* Specifies the value for MC_EMEM_ARB_OVERRIDE */
    uint32_t mc_emem_arb_override;
    /* Specifies the value for MC_EMEM_ARB_OVERRIDE_1 */
    uint32_t mc_emem_arb_override1;
    /* Specifies the value for MC_EMEM_ARB_RSV */
    uint32_t mc_emem_arb_rsv;

    uint32_t mc_da_cfg0;
    uint32_t mc_emem_arb_timing_ccdmw;

    /* Specifies the value for MC_CLKEN_OVERRIDE */
    uint32_t mc_clken_override;

    /* Specifies the value for MC_STAT_CONTROL */
    uint32_t mc_stat_control;
    /* Specifies the value for MC_VIDEO_PROTECT_BOM */
    uint32_t mc_video_protect_bom;
    /* Specifies the value for MC_VIDEO_PROTECT_BOM_ADR_HI */
    uint32_t mc_video_protect_bom_adr_hi;
    /* Specifies the value for MC_VIDEO_PROTECT_SIZE_MB */
    uint32_t mc_video_protect_size_mb;
    /* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE */
    uint32_t mc_video_protect_vpr_override;
    /* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE1 */
    uint32_t mc_video_protect_vpr_override1;
    /* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_0 */
    uint32_t mc_video_protect_gpu_override0;
    /* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_1 */
    uint32_t mc_video_protect_gpu_override1;
    /* Specifies the value for MC_SEC_CARVEOUT_BOM */
    uint32_t mc_sec_carveout_bom;
    /* Specifies the value for MC_SEC_CARVEOUT_ADR_HI */
    uint32_t mc_sec_carveout_adr_hi;
    /* Specifies the value for MC_SEC_CARVEOUT_SIZE_MB */
    uint32_t mc_sec_carveout_size_mb;
    /* Specifies the value for MC_VIDEO_PROTECT_REG_CTRL.VIDEO_PROTECT_WRITE_ACCESS */
    uint32_t mc_video_protect_write_access;
    /* Specifies the value for MC_SEC_CARVEOUT_REG_CTRL.SEC_CARVEOUT_WRITE_ACCESS */
    uint32_t mc_sec_carveout_protect_write_access;

    uint32_t mc_generalized_carveout1_bom;
    uint32_t mc_generalized_carveout1_bom_hi;
    uint32_t mc_generalized_carveout1_size_128kb;
    uint32_t mc_generalized_carveout1_access0;
    uint32_t mc_generalized_carveout1_access1;
    uint32_t mc_generalized_carveout1_access2;
    uint32_t mc_generalized_carveout1_access3;
    uint32_t mc_generalized_carveout1_access4;
    uint32_t mc_generalized_carveout1_force_internal_access0;
    uint32_t mc_generalized_carveout1_force_internal_access1;
    uint32_t mc_generalized_carveout1_force_internal_access2;
    uint32_t mc_generalized_carveout1_force_internal_access3;
    uint32_t mc_generalized_carveout1_force_internal_access4;
    uint32_t mc_generalized_carveout1_cfg0;

    uint32_t mc_generalized_carveout2_bom;
    uint32_t mc_generalized_carveout2_bom_hi;
    uint32_t mc_generalized_carveout2_size_128kb;
    uint32_t mc_generalized_carveout2_access0;
    uint32_t mc_generalized_carveout2_access1;
    uint32_t mc_generalized_carveout2_access2;
    uint32_t mc_generalized_carveout2_access3;
    uint32_t mc_generalized_carveout2_access4;
    uint32_t mc_generalized_carveout2_force_internal_access0;
    uint32_t mc_generalized_carveout2_force_internal_access1;
    uint32_t mc_generalized_carveout2_force_internal_access2;
    uint32_t mc_generalized_carveout2_force_internal_access3;
    uint32_t mc_generalized_carveout2_force_internal_access4;
    uint32_t mc_generalized_carveout2_cfg0;

    uint32_t mc_generalized_carveout3_bom;
    uint32_t mc_generalized_carveout3_bom_hi;
    uint32_t mc_generalized_carveout3_size_128kb;
    uint32_t mc_generalized_carveout3_access0;
    uint32_t mc_generalized_carveout3_access1;
    uint32_t mc_generalized_carveout3_access2;
    uint32_t mc_generalized_carveout3_access3;
    uint32_t mc_generalized_carveout3_access4;
    uint32_t mc_generalized_carveout3_force_internal_access0;
    uint32_t mc_generalized_carveout3_force_internal_access1;
    uint32_t mc_generalized_carveout3_force_internal_access2;
    uint32_t mc_generalized_carveout3_force_internal_access3;
    uint32_t mc_generalized_carveout3_force_internal_access4;
    uint32_t mc_generalized_carveout3_cfg0;

    uint32_t mc_generalized_carveout4_bom;
    uint32_t mc_generalized_carveout4_bom_hi;
    uint32_t mc_generalized_carveout4_size_128kb;
    uint32_t mc_generalized_carveout4_access0;
    uint32_t mc_generalized_carveout4_access1;
    uint32_t mc_generalized_carveout4_access2;
    uint32_t mc_generalized_carveout4_access3;
    uint32_t mc_generalized_carveout4_access4;
    uint32_t mc_generalized_carveout4_force_internal_access0;
    uint32_t mc_generalized_carveout4_force_internal_access1;
    uint32_t mc_generalized_carveout4_force_internal_access2;
    uint32_t mc_generalized_carveout4_force_internal_access3;
    uint32_t mc_generalized_carveout4_force_internal_access4;
    uint32_t mc_generalized_carveout4_cfg0;

    uint32_t mc_generalized_carveout5_bom;
    uint32_t mc_generalized_carveout5_bom_hi;
    uint32_t mc_generalized_carveout5_size_128kb;
    uint32_t mc_generalized_carveout5_access0;
    uint32_t mc_generalized_carveout5_access1;
    uint32_t mc_generalized_carveout5_access2;
    uint32_t mc_generalized_carveout5_access3;
    uint32_t mc_generalized_carveout5_access4;
    uint32_t mc_generalized_carveout5_force_internal_access0;
    uint32_t mc_generalized_carveout5_force_internal_access1;
    uint32_t mc_generalized_carveout5_force_internal_access2;
    uint32_t mc_generalized_carveout5_force_internal_access3;
    uint32_t mc_generalized_carveout5_force_internal_access4;
    uint32_t mc_generalized_carveout5_cfg0;

    /* Specifies enable for CA training */
    uint32_t emc_ca_training_enable;
    /* Set if bit 6 select is greater than bit 7 select; uses aremc.spec packet SWIZZLE_BIT6_GT_BIT7 */
    uint32_t swizzle_rank_byte_encode;
    /* Specifies enable and offset for patched boot rom write */
    uint32_t boot_rom_patch_control;
    /* Specifies data for patched boot rom write */
    uint32_t boot_rom_patch_data;

    /* Specifies the value for MC_MTS_CARVEOUT_BOM */
    uint32_t mc_mts_carveout_bom;
    /* Specifies the value for MC_MTS_CARVEOUT_ADR_HI */
    uint32_t mc_mts_carveout_adr_hi;
    /* Specifies the value for MC_MTS_CARVEOUT_SIZE_MB */
    uint32_t mc_mts_carveout_size_mb;
    /* Specifies the value for MC_MTS_CARVEOUT_REG_CTRL */
    uint32_t mc_mts_carveout_reg_ctrl;
} sdram_params_t;

#endif
