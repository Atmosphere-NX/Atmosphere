/*
 * Copyright (c) Atmosphère-NX
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
#include <exosphere.hpp>
#include "fusee_mtc_timing_table_common.hpp"

namespace ams::nxboot::erista {

    #define FOREACH_BURST_REG(HANDLER)                                                                     \
        HANDLER(EMC, EMC_RC, emc_rc)                                                                       \
        HANDLER(EMC, EMC_RFC, emc_rfc)                                                                     \
        HANDLER(EMC, EMC_RFCPB, emc_rfcpb)                                                                 \
        HANDLER(EMC, EMC_REFCTRL2, emc_refctrl2)                                                           \
        HANDLER(EMC, EMC_RFC_SLR, emc_rfc_slr)                                                             \
        HANDLER(EMC, EMC_RAS, emc_ras)                                                                     \
        HANDLER(EMC, EMC_RP, emc_rp)                                                                       \
        HANDLER(EMC, EMC_R2W, emc_r2w)                                                                     \
        HANDLER(EMC, EMC_W2R, emc_w2r)                                                                     \
        HANDLER(EMC, EMC_R2P, emc_r2p)                                                                     \
        HANDLER(EMC, EMC_W2P, emc_w2p)                                                                     \
        HANDLER(EMC, EMC_R2R, emc_r2r)                                                                     \
        HANDLER(EMC, EMC_TPPD, emc_tppd)                                                                   \
        HANDLER(EMC, EMC_CCDMW, emc_ccdmw)                                                                 \
        HANDLER(EMC, EMC_RD_RCD, emc_rd_rcd)                                                               \
        HANDLER(EMC, EMC_WR_RCD, emc_wr_rcd)                                                               \
        HANDLER(EMC, EMC_RRD, emc_rrd)                                                                     \
        HANDLER(EMC, EMC_REXT, emc_rext)                                                                   \
        HANDLER(EMC, EMC_WEXT, emc_wext)                                                                   \
        HANDLER(EMC, EMC_WDV_CHK, emc_wdv_chk)                                                             \
        HANDLER(EMC, EMC_WDV, emc_wdv)                                                                     \
        HANDLER(EMC, EMC_WSV, emc_wsv)                                                                     \
        HANDLER(EMC, EMC_WEV, emc_wev)                                                                     \
        HANDLER(EMC, EMC_WDV_MASK, emc_wdv_mask)                                                           \
        HANDLER(EMC, EMC_WS_DURATION, emc_ws_duration)                                                     \
        HANDLER(EMC, EMC_WE_DURATION, emc_we_duration)                                                     \
        HANDLER(EMC, EMC_QUSE, emc_quse)                                                                   \
        HANDLER(EMC, EMC_QUSE_WIDTH, emc_quse_width)                                                       \
        HANDLER(EMC, EMC_IBDLY, emc_ibdly)                                                                 \
        HANDLER(EMC, EMC_OBDLY, emc_obdly)                                                                 \
        HANDLER(EMC, EMC_EINPUT, emc_einput)                                                               \
        HANDLER(EMC, EMC_MRW6, emc_mrw6)                                                                   \
        HANDLER(EMC, EMC_EINPUT_DURATION, emc_einput_duration)                                             \
        HANDLER(EMC, EMC_PUTERM_EXTRA, emc_puterm_extra)                                                   \
        HANDLER(EMC, EMC_PUTERM_WIDTH, emc_puterm_width)                                                   \
        HANDLER(EMC, EMC_QRST, emc_qrst)                                                                   \
        HANDLER(EMC, EMC_QSAFE, emc_qsafe)                                                                 \
        HANDLER(EMC, EMC_RDV, emc_rdv)                                                                     \
        HANDLER(EMC, EMC_RDV_MASK, emc_rdv_mask)                                                           \
        HANDLER(EMC, EMC_RDV_EARLY, emc_rdv_early)                                                         \
        HANDLER(EMC, EMC_RDV_EARLY_MASK, emc_rdv_early_mask)                                               \
        HANDLER(EMC, EMC_REFRESH, emc_refresh)                                                             \
        HANDLER(EMC, EMC_BURST_REFRESH_NUM, emc_burst_refresh_num)                                         \
        HANDLER(EMC, EMC_PRE_REFRESH_REQ_CNT, emc_pre_refresh_req_cnt)                                     \
        HANDLER(EMC, EMC_PDEX2WR, emc_pdex2wr)                                                             \
        HANDLER(EMC, EMC_PDEX2RD, emc_pdex2rd)                                                             \
        HANDLER(EMC, EMC_PCHG2PDEN, emc_pchg2pden)                                                         \
        HANDLER(EMC, EMC_ACT2PDEN, emc_act2pden)                                                           \
        HANDLER(EMC, EMC_AR2PDEN, emc_ar2pden)                                                             \
        HANDLER(EMC, EMC_RW2PDEN, emc_rw2pden)                                                             \
        HANDLER(EMC, EMC_CKE2PDEN, emc_cke2pden)                                                           \
        HANDLER(EMC, EMC_PDEX2CKE, emc_pdex2cke)                                                           \
        HANDLER(EMC, EMC_PDEX2MRR, emc_pdex2mrr)                                                           \
        HANDLER(EMC, EMC_TXSR, emc_txsr)                                                                   \
        HANDLER(EMC, EMC_TXSRDLL, emc_txsrdll)                                                             \
        HANDLER(EMC, EMC_TCKE, emc_tcke)                                                                   \
        HANDLER(EMC, EMC_TCKESR, emc_tckesr)                                                               \
        HANDLER(EMC, EMC_TPD, emc_tpd)                                                                     \
        HANDLER(EMC, EMC_TFAW, emc_tfaw)                                                                   \
        HANDLER(EMC, EMC_TRPAB, emc_trpab)                                                                 \
        HANDLER(EMC, EMC_TCLKSTABLE, emc_tclkstable)                                                       \
        HANDLER(EMC, EMC_TCLKSTOP, emc_tclkstop)                                                           \
        HANDLER(EMC, EMC_MRW7, emc_mrw7)                                                                   \
        HANDLER(EMC, EMC_TREFBW, emc_trefbw)                                                               \
        HANDLER(EMC, EMC_ODT_WRITE, emc_odt_write)                                                         \
        HANDLER(EMC, EMC_FBIO_CFG5, emc_fbio_cfg5)                                                         \
        HANDLER(EMC, EMC_FBIO_CFG7, emc_fbio_cfg7)                                                         \
        HANDLER(EMC, EMC_CFG_DIG_DLL, emc_cfg_dig_dll)                                                     \
        HANDLER(EMC, EMC_CFG_DIG_DLL_PERIOD, emc_cfg_dig_dll_period)                                       \
        HANDLER(EMC, EMC_PMACRO_IB_RXRT, emc_pmacro_ib_rxrt)                                               \
        HANDLER(EMC, EMC_CFG_PIPE_1, emc_cfg_pipe_1)                                                       \
        HANDLER(EMC, EMC_CFG_PIPE_2, emc_cfg_pipe_2)                                                       \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK0_4, emc_pmacro_quse_ddll_rank0_4)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK0_5, emc_pmacro_quse_ddll_rank0_5)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK1_4, emc_pmacro_quse_ddll_rank1_4)                           \
        HANDLER(EMC, EMC_PMACRO_QUSE_DDLL_RANK1_5, emc_pmacro_quse_ddll_rank1_5)                           \
        HANDLER(EMC, EMC_MRW8, emc_mrw8)                                                                   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_4, emc_pmacro_ob_ddll_long_dq_rank1_4)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_5, emc_pmacro_ob_ddll_long_dq_rank1_5)               \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_0, emc_pmacro_ob_ddll_long_dqs_rank0_0)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_1, emc_pmacro_ob_ddll_long_dqs_rank0_1)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_2, emc_pmacro_ob_ddll_long_dqs_rank0_2)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_3, emc_pmacro_ob_ddll_long_dqs_rank0_3)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_4, emc_pmacro_ob_ddll_long_dqs_rank0_4)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_5, emc_pmacro_ob_ddll_long_dqs_rank0_5)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_0, emc_pmacro_ob_ddll_long_dqs_rank1_0)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_1, emc_pmacro_ob_ddll_long_dqs_rank1_1)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_2, emc_pmacro_ob_ddll_long_dqs_rank1_2)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_3, emc_pmacro_ob_ddll_long_dqs_rank1_3)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_4, emc_pmacro_ob_ddll_long_dqs_rank1_4)             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_5, emc_pmacro_ob_ddll_long_dqs_rank1_5)             \
        HANDLER(EMC, EMC_PMACRO_DDLL_LONG_CMD_0, emc_pmacro_ddll_long_cmd_0)                               \
        HANDLER(EMC, EMC_PMACRO_DDLL_LONG_CMD_1, emc_pmacro_ddll_long_cmd_1)                               \
        HANDLER(EMC, EMC_PMACRO_DDLL_LONG_CMD_2, emc_pmacro_ddll_long_cmd_2)                               \
        HANDLER(EMC, EMC_PMACRO_DDLL_LONG_CMD_3, emc_pmacro_ddll_long_cmd_3)                               \
        HANDLER(EMC, EMC_PMACRO_DDLL_LONG_CMD_4, emc_pmacro_ddll_long_cmd_4)                               \
        HANDLER(EMC, EMC_PMACRO_DDLL_SHORT_CMD_0, emc_pmacro_ddll_short_cmd_0)                             \
        HANDLER(EMC, EMC_PMACRO_DDLL_SHORT_CMD_1, emc_pmacro_ddll_short_cmd_1)                             \
        HANDLER(EMC, EMC_PMACRO_DDLL_SHORT_CMD_2, emc_pmacro_ddll_short_cmd_2)                             \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_3, emc_pmacro_ob_ddll_short_dq_rank0_byte0_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_3, emc_pmacro_ob_ddll_short_dq_rank0_byte1_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_3, emc_pmacro_ob_ddll_short_dq_rank0_byte2_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_3, emc_pmacro_ob_ddll_short_dq_rank0_byte3_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_3, emc_pmacro_ob_ddll_short_dq_rank0_byte4_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_3, emc_pmacro_ob_ddll_short_dq_rank0_byte5_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_3, emc_pmacro_ob_ddll_short_dq_rank0_byte6_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_3, emc_pmacro_ob_ddll_short_dq_rank0_byte7_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_3, emc_pmacro_ob_ddll_short_dq_rank0_cmd0_3)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_3, emc_pmacro_ob_ddll_short_dq_rank0_cmd1_3)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_3, emc_pmacro_ob_ddll_short_dq_rank0_cmd2_3)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_3, emc_pmacro_ob_ddll_short_dq_rank0_cmd3_3)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_3, emc_pmacro_ob_ddll_short_dq_rank1_byte0_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_3, emc_pmacro_ob_ddll_short_dq_rank1_byte1_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_3, emc_pmacro_ob_ddll_short_dq_rank1_byte2_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_3, emc_pmacro_ob_ddll_short_dq_rank1_byte3_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_3, emc_pmacro_ob_ddll_short_dq_rank1_byte4_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_3, emc_pmacro_ob_ddll_short_dq_rank1_byte5_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_3, emc_pmacro_ob_ddll_short_dq_rank1_byte6_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_3, emc_pmacro_ob_ddll_short_dq_rank1_byte7_3) \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_0, emc_pmacro_ob_ddll_short_dq_rank1_cmd0_0)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_1, emc_pmacro_ob_ddll_short_dq_rank1_cmd0_1)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_2, emc_pmacro_ob_ddll_short_dq_rank1_cmd0_2)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_3, emc_pmacro_ob_ddll_short_dq_rank1_cmd0_3)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_0, emc_pmacro_ob_ddll_short_dq_rank1_cmd1_0)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_1, emc_pmacro_ob_ddll_short_dq_rank1_cmd1_1)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_2, emc_pmacro_ob_ddll_short_dq_rank1_cmd1_2)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_3, emc_pmacro_ob_ddll_short_dq_rank1_cmd1_3)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_0, emc_pmacro_ob_ddll_short_dq_rank1_cmd2_0)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_1, emc_pmacro_ob_ddll_short_dq_rank1_cmd2_1)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_2, emc_pmacro_ob_ddll_short_dq_rank1_cmd2_2)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_3, emc_pmacro_ob_ddll_short_dq_rank1_cmd2_3)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_0, emc_pmacro_ob_ddll_short_dq_rank1_cmd3_0)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_1, emc_pmacro_ob_ddll_short_dq_rank1_cmd3_1)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_2, emc_pmacro_ob_ddll_short_dq_rank1_cmd3_2)   \
        HANDLER(EMC, EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_3, emc_pmacro_ob_ddll_short_dq_rank1_cmd3_3)   \
        HANDLER(EMC, EMC_TXDSRVTTGEN, emc_txdsrvttgen)                                                     \
        HANDLER(EMC, EMC_FDPD_CTRL_DQ, emc_fdpd_ctrl_dq)                                                   \
        HANDLER(EMC, EMC_FDPD_CTRL_CMD, emc_fdpd_ctrl_cmd)                                                 \
        HANDLER(EMC, EMC_FBIO_SPARE, emc_fbio_spare)                                                       \
        HANDLER(EMC, EMC_ZCAL_INTERVAL, emc_zcal_interval)                                                 \
        HANDLER(EMC, EMC_ZCAL_WAIT_CNT, emc_zcal_wait_cnt)                                                 \
        HANDLER(EMC, EMC_MRS_WAIT_CNT, emc_mrs_wait_cnt)                                                   \
        HANDLER(EMC, EMC_MRS_WAIT_CNT2, emc_mrs_wait_cnt2)                                                 \
        HANDLER(EMC, EMC_AUTO_CAL_CHANNEL, emc_auto_cal_channel)                                           \
        HANDLER(EMC, EMC_DLL_CFG_0, emc_dll_cfg_0)                                                         \
        HANDLER(EMC, EMC_DLL_CFG_1, emc_dll_cfg_1)                                                         \
        HANDLER(EMC, EMC_PMACRO_AUTOCAL_CFG_COMMON, emc_pmacro_autocal_cfg_common)                         \
        HANDLER(EMC, EMC_PMACRO_ZCTRL, emc_pmacro_zctrl)                                                   \
        HANDLER(EMC, EMC_CFG, emc_cfg)                                                                     \
        HANDLER(EMC, EMC_CFG_PIPE, emc_cfg_pipe)                                                           \
        HANDLER(EMC, EMC_DYN_SELF_REF_CONTROL, emc_dyn_self_ref_control)                                   \
        HANDLER(EMC, EMC_QPOP, emc_qpop)                                                                   \
        HANDLER(EMC, EMC_DQS_BRLSHFT_0, emc_dqs_brlshft_0)                                                 \
        HANDLER(EMC, EMC_DQS_BRLSHFT_1, emc_dqs_brlshft_1)                                                 \
        HANDLER(EMC, EMC_CMD_BRLSHFT_2, emc_cmd_brlshft_2)                                                 \
        HANDLER(EMC, EMC_CMD_BRLSHFT_3, emc_cmd_brlshft_3)                                                 \
        HANDLER(EMC, EMC_PMACRO_PAD_CFG_CTRL, emc_pmacro_pad_cfg_ctrl)                                     \
        HANDLER(EMC, EMC_PMACRO_DATA_PAD_RX_CTRL, emc_pmacro_data_pad_rx_ctrl)                             \
        HANDLER(EMC, EMC_PMACRO_CMD_PAD_RX_CTRL, emc_pmacro_cmd_pad_rx_ctrl)                               \
        HANDLER(EMC, EMC_PMACRO_DATA_RX_TERM_MODE, emc_pmacro_data_rx_term_mode)                           \
        HANDLER(EMC, EMC_PMACRO_CMD_RX_TERM_MODE, emc_pmacro_cmd_rx_term_mode)                             \
        HANDLER(EMC, EMC_PMACRO_CMD_PAD_TX_CTRL, emc_pmacro_cmd_pad_tx_ctrl)                               \
        HANDLER(EMC, EMC_PMACRO_DATA_PAD_TX_CTRL, emc_pmacro_data_pad_tx_ctrl)                             \
        HANDLER(EMC, EMC_PMACRO_COMMON_PAD_TX_CTRL, emc_pmacro_common_pad_tx_ctrl)                         \
        HANDLER(EMC, EMC_PMACRO_VTTGEN_CTRL_0, emc_pmacro_vttgen_ctrl_0)                                   \
        HANDLER(EMC, EMC_PMACRO_VTTGEN_CTRL_1, emc_pmacro_vttgen_ctrl_1)                                   \
        HANDLER(EMC, EMC_PMACRO_VTTGEN_CTRL_2, emc_pmacro_vttgen_ctrl_2)                                   \
        HANDLER(EMC, EMC_PMACRO_BRICK_CTRL_RFU1, emc_pmacro_brick_ctrl_rfu1)                               \
        HANDLER(EMC, EMC_PMACRO_CMD_BRICK_CTRL_FDPD, emc_pmacro_cmd_brick_ctrl_fdpd)                       \
        HANDLER(EMC, EMC_PMACRO_BRICK_CTRL_RFU2, emc_pmacro_brick_ctrl_rfu2)                               \
        HANDLER(EMC, EMC_PMACRO_DATA_BRICK_CTRL_FDPD, emc_pmacro_data_brick_ctrl_fdpd)                     \
        HANDLER(EMC, EMC_PMACRO_BG_BIAS_CTRL_0, emc_pmacro_bg_bias_ctrl_0)                                 \
        HANDLER(EMC, EMC_CFG_3, emc_cfg_3)                                                                 \
        HANDLER(EMC, EMC_PMACRO_TX_PWRD_0, emc_pmacro_tx_pwrd_0)                                           \
        HANDLER(EMC, EMC_PMACRO_TX_PWRD_1, emc_pmacro_tx_pwrd_1)                                           \
        HANDLER(EMC, EMC_PMACRO_TX_PWRD_2, emc_pmacro_tx_pwrd_2)                                           \
        HANDLER(EMC, EMC_PMACRO_TX_PWRD_3, emc_pmacro_tx_pwrd_3)                                           \
        HANDLER(EMC, EMC_PMACRO_TX_PWRD_4, emc_pmacro_tx_pwrd_4)                                           \
        HANDLER(EMC, EMC_PMACRO_TX_PWRD_5, emc_pmacro_tx_pwrd_5)                                           \
        HANDLER(EMC, EMC_CONFIG_SAMPLE_DELAY, emc_config_sample_delay)                                     \
        HANDLER(EMC, EMC_PMACRO_TX_SEL_CLK_SRC_0, emc_pmacro_tx_sel_clk_src_0)                             \
        HANDLER(EMC, EMC_PMACRO_TX_SEL_CLK_SRC_1, emc_pmacro_tx_sel_clk_src_1)                             \
        HANDLER(EMC, EMC_PMACRO_TX_SEL_CLK_SRC_2, emc_pmacro_tx_sel_clk_src_2)                             \
        HANDLER(EMC, EMC_PMACRO_TX_SEL_CLK_SRC_3, emc_pmacro_tx_sel_clk_src_3)                             \
        HANDLER(EMC, EMC_PMACRO_TX_SEL_CLK_SRC_4, emc_pmacro_tx_sel_clk_src_4)                             \
        HANDLER(EMC, EMC_PMACRO_TX_SEL_CLK_SRC_5, emc_pmacro_tx_sel_clk_src_5)                             \
        HANDLER(EMC, EMC_PMACRO_DDLL_BYPASS, emc_pmacro_ddll_bypass)                                       \
        HANDLER(EMC, EMC_PMACRO_DDLL_PWRD_0, emc_pmacro_ddll_pwrd_0)                                       \
        HANDLER(EMC, EMC_PMACRO_DDLL_PWRD_1, emc_pmacro_ddll_pwrd_1)                                       \
        HANDLER(EMC, EMC_PMACRO_DDLL_PWRD_2, emc_pmacro_ddll_pwrd_2)                                       \
        HANDLER(EMC, EMC_PMACRO_CMD_CTRL_0, emc_pmacro_cmd_ctrl_0)                                         \
        HANDLER(EMC, EMC_PMACRO_CMD_CTRL_1, emc_pmacro_cmd_ctrl_1)                                         \
        HANDLER(EMC, EMC_PMACRO_CMD_CTRL_2, emc_pmacro_cmd_ctrl_2)                                         \
        HANDLER(EMC, EMC_TR_TIMING_0, emc_tr_timing_0)                                                     \
        HANDLER(EMC, EMC_TR_DVFS, emc_tr_dvfs)                                                             \
        HANDLER(EMC, EMC_TR_CTRL_1, emc_tr_ctrl_1)                                                         \
        HANDLER(EMC, EMC_TR_RDV, emc_tr_rdv)                                                               \
        HANDLER(EMC, EMC_TR_QPOP, emc_tr_qpop)                                                             \
        HANDLER(EMC, EMC_TR_RDV_MASK, emc_tr_rdv_mask)                                                     \
        HANDLER(EMC, EMC_MRW14, emc_mrw14)                                                                 \
        HANDLER(EMC, EMC_TR_QSAFE, emc_tr_qsafe)                                                           \
        HANDLER(EMC, EMC_TR_QRST, emc_tr_qrst)                                                             \
        HANDLER(EMC, EMC_TRAINING_CTRL, emc_training_ctrl)                                                 \
        HANDLER(EMC, EMC_TRAINING_SETTLE, emc_training_settle)                                             \
        HANDLER(EMC, EMC_TRAINING_VREF_SETTLE, emc_training_vref_settle)                                   \
        HANDLER(EMC, EMC_TRAINING_CA_FINE_CTRL, emc_training_ca_fine_ctrl)                                 \
        HANDLER(EMC, EMC_TRAINING_CA_CTRL_MISC, emc_training_ca_ctrl_misc)                                 \
        HANDLER(EMC, EMC_TRAINING_CA_CTRL_MISC1, emc_training_ca_ctrl_misc1)                               \
        HANDLER(EMC, EMC_TRAINING_CA_VREF_CTRL, emc_training_ca_vref_ctrl)                                 \
        HANDLER(EMC, EMC_TRAINING_QUSE_CORS_CTRL, emc_training_quse_cors_ctrl)                             \
        HANDLER(EMC, EMC_TRAINING_QUSE_FINE_CTRL, emc_training_quse_fine_ctrl)                             \
        HANDLER(EMC, EMC_TRAINING_QUSE_CTRL_MISC, emc_training_quse_ctrl_misc)                             \
        HANDLER(EMC, EMC_TRAINING_QUSE_VREF_CTRL, emc_training_quse_vref_ctrl)                             \
        HANDLER(EMC, EMC_TRAINING_READ_FINE_CTRL, emc_training_read_fine_ctrl)                             \
        HANDLER(EMC, EMC_TRAINING_READ_CTRL_MISC, emc_training_read_ctrl_misc)                             \
        HANDLER(EMC, EMC_TRAINING_READ_VREF_CTRL, emc_training_read_vref_ctrl)                             \
        HANDLER(EMC, EMC_TRAINING_WRITE_FINE_CTRL, emc_training_write_fine_ctrl)                           \
        HANDLER(EMC, EMC_TRAINING_WRITE_CTRL_MISC, emc_training_write_ctrl_misc)                           \
        HANDLER(EMC, EMC_TRAINING_WRITE_VREF_CTRL, emc_training_write_vref_ctrl)                           \
        HANDLER(EMC, EMC_TRAINING_MPC, emc_training_mpc)                                                   \
        HANDLER(EMC, EMC_MRW15, emc_mrw15)

    #define DECLARE_STRUCT_MEMBER_HANDLER(BASE, REG, NAME) uint32_t NAME;

    #define DECLARE_ARRAY_AND_STRUCT_MEMBERS(NAME, FOREACH_HANDLER) \
        union { struct { FOREACH_HANDLER(DECLARE_STRUCT_MEMBER_HANDLER) } NAME; uint32_t NAME##_arr[sizeof(NAME) / sizeof(uint32_t)]; }

    struct EmcDvfsTimingTable {
        uint32_t rev;
        char dvfs_ver[60];
        uint32_t rate_khz;
        uint32_t min_volt;
        uint32_t gpu_min_volt;
        char clock_src[32];
        uint32_t clk_src_emc;
        uint32_t needs_training;
        uint32_t training_pattern;
        uint32_t trained;

        uint32_t periodic_training;
        uint32_t trained_dram_clktree_c0d0u0;
        uint32_t trained_dram_clktree_c0d0u1;
        uint32_t trained_dram_clktree_c0d1u0;
        uint32_t trained_dram_clktree_c0d1u1;
        uint32_t trained_dram_clktree_c1d0u0;
        uint32_t trained_dram_clktree_c1d0u1;
        uint32_t trained_dram_clktree_c1d1u0;
        uint32_t trained_dram_clktree_c1d1u1;
        uint32_t current_dram_clktree_c0d0u0;
        uint32_t current_dram_clktree_c0d0u1;
        uint32_t current_dram_clktree_c0d1u0;
        uint32_t current_dram_clktree_c0d1u1;
        uint32_t current_dram_clktree_c1d0u0;
        uint32_t current_dram_clktree_c1d0u1;
        uint32_t current_dram_clktree_c1d1u0;
        uint32_t current_dram_clktree_c1d1u1;
        uint32_t run_clocks;
        uint32_t tree_margin;

        uint32_t num_burst;
        uint32_t num_burst_per_ch;
        uint32_t num_trim;
        uint32_t num_trim_per_ch;
        uint32_t num_mc_regs;
        uint32_t num_up_down;
        uint32_t vref_num;
        uint32_t training_mod_num;
        uint32_t dram_timing_num;

        uint32_t ptfv_dqsosc_movavg_c0d0u0;
        uint32_t ptfv_dqsosc_movavg_c0d0u1;
        uint32_t ptfv_dqsosc_movavg_c0d1u0;
        uint32_t ptfv_dqsosc_movavg_c0d1u1;
        uint32_t ptfv_dqsosc_movavg_c1d0u0;
        uint32_t ptfv_dqsosc_movavg_c1d0u1;
        uint32_t ptfv_dqsosc_movavg_c1d1u0;
        uint32_t ptfv_dqsosc_movavg_c1d1u1;
        uint32_t ptfv_write_samples;
        uint32_t ptfv_dvfs_samples;
        uint32_t ptfv_movavg_weight;
        uint32_t ptfv_config_ctrl;

        DECLARE_ARRAY_AND_STRUCT_MEMBERS(burst_regs, FOREACH_BURST_REG);
        DECLARE_ARRAY_AND_STRUCT_MEMBERS(burst_perch_regs, FOREACH_PER_CHANNEL_BURST_REG);
        DECLARE_ARRAY_AND_STRUCT_MEMBERS(shadow_regs_ca_train, FOREACH_BURST_REG);
        DECLARE_ARRAY_AND_STRUCT_MEMBERS(shadow_regs_quse_train, FOREACH_BURST_REG);
        DECLARE_ARRAY_AND_STRUCT_MEMBERS(shadow_regs_rdwr_train, FOREACH_BURST_REG);
        DECLARE_ARRAY_AND_STRUCT_MEMBERS(trim_regs, FOREACH_TRIM_REG);
        DECLARE_ARRAY_AND_STRUCT_MEMBERS(trim_perch_regs, FOREACH_PER_CHANNEL_TRIM_REG);
        DECLARE_ARRAY_AND_STRUCT_MEMBERS(vref_perch_regs, FOREACH_PER_CHANNEL_VREF_REG);

        struct {
            uint32_t t_rp;
            uint32_t t_fc_lpddr4;
            uint32_t t_rfc;
            uint32_t t_pdex;
            uint32_t rl;
        } dram_timings;

        DECLARE_ARRAY_AND_STRUCT_MEMBERS(training_mod_regs, FOREACH_PER_CHANNEL_TRAINING_MOD_REG);

        uint32_t save_restore_mod_regs[12];

        DECLARE_ARRAY_AND_STRUCT_MEMBERS(burst_mc_regs, FOREACH_BURST_MC_REG);
        DECLARE_ARRAY_AND_STRUCT_MEMBERS(la_scale_regs, FOREACH_LA_SCALE_REG);

        uint32_t min_mrs_wait;
        uint32_t emc_mrw;
        uint32_t emc_mrw2;
        uint32_t emc_mrw3;
        uint32_t emc_mrw4;
        uint32_t emc_mrw9;
        uint32_t emc_mrs;
        uint32_t emc_emrs;
        uint32_t emc_emrs2;
        uint32_t emc_auto_cal_config;
        uint32_t emc_auto_cal_config2;
        uint32_t emc_auto_cal_config3;
        uint32_t emc_auto_cal_config4;
        uint32_t emc_auto_cal_config5;
        uint32_t emc_auto_cal_config6;
        uint32_t emc_auto_cal_config7;
        uint32_t emc_auto_cal_config8;
        uint32_t emc_cfg_2;
        uint32_t emc_sel_dpd_ctrl;
        uint32_t emc_fdpd_ctrl_cmd_no_ramp;
        uint32_t dll_clk_src;
        uint32_t clk_out_enb_x_0_clk_enb_emc_dll;
        uint32_t latency;
    };

    #undef DECLARE_STRUCT_MEMBER_HANDLER
    #undef DECLARE_ARRAY_AND_STRUCT_MEMBERS

    static_assert(sizeof(EmcDvfsTimingTable) == 0x1340);

}
