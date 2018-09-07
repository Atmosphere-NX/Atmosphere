/*
 * Copyright (c) 2018 naehrwert
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
 
#include "i2c.h"
#include "mc.h"
#include "emc.h"
#include "pmc.h"
#include "timers.h"
#include "sysreg.h"
#include "fuse.h"
#include "max77620.h"
#include "sdram_param_t210.h"
#include "car.h"

#define CONFIG_SDRAM_COMPRESS_CFG

#ifdef CONFIG_SDRAM_COMPRESS_CFG
#include "lib/lz.h"
#include "sdram_lz.inl"
#else
#include "sdram.inl"
#endif

static uint32_t _get_sdram_id()
{
    return ((fuse_get_reserved_odm(4) & 0x38) >> 3);
}

static void _sdram_config(const sdram_params_t *params)
{
    volatile tegra_car_t *car = car_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    
    pmc->io_dpd3_req = (((4 * params->emc_pmc_scratch1 >> 2) + 0x80000000) ^ 0xFFFF) & 0xC000FFFF;
    udelay(params->pmc_io_dpd3_req_wait);

    uint32_t req = (4 * params->emc_pmc_scratch2 >> 2) + 0x80000000;
    pmc->io_dpd4_req = (req >> 16 << 16) ^ 0x3FFF0000;
    
    udelay(params->pmc_io_dpd4_req_wait);
    pmc->io_dpd4_req = (req ^ 0xFFFF) & 0xC000FFFF;
    udelay(params->pmc_io_dpd4_req_wait);
    
    pmc->weak_bias = 0;
    udelay(1);

    car->pllm_misc1 = params->pllm_setup_control;
    car->pllm_misc2 = 0;
    car->pllm_base = ((params->pllm_feedback_divider << 8) | params->pllm_input_divider | 0x40000000 | ((params->pllm_post_divider & 0xFFFF) << 20));

    bool timeout = false;
    uint32_t wait_end = get_time_us() + 300;
    
    while (!(car->pllm_base & 0x8000000) && !timeout)
    {
        if (get_time_us() >= wait_end)
            timeout = true;
    }
    
    if (!timeout) {
        udelay(10);
    }

    car->clk_source_emc = (((params->mc_emem_arb_misc0 >> 11) & 0x10000) | (params->emc_clock_source & 0xFFFEFFFF));
    
    if (params->emc_clock_source_dll)
        car->clk_source_emc_dll = params->emc_clock_source_dll;
    
    if (params->clear_clock2_mc1)
        car->clk_enb_w_clr = 0x40000000;
    
    car->clk_enb_h_set = 0x2000001;
    car->clk_enb_x_set = 0x4000;
    car->rst_dev_h_clr = 0x2000001;
    
    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_0) = params->emc_pmacro_vttgen_ctrl0;
    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_1) = params->emc_pmacro_vttgen_ctrl1;
    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_2) = params->emc_pmacro_vttgen_ctrl2;
    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    udelay(1);
    
    MAKE_EMC_REG(EMC_DBG) = (params->emc_dbg_write_mux << 1) | params->emc_dbg;
    
    if (params->emc_bct_spare2)
        *(volatile uint32_t *)params->emc_bct_spare2 = params->emc_bct_spare3;
    
    MAKE_EMC_REG(EMC_FBIO_CFG7) = params->emc_fbio_cfg7;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_0) = params->emc_cmd_mapping_cmd0_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_1) = params->emc_cmd_mapping_cmd0_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_2) = params->emc_cmd_mapping_cmd0_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_0) = params->emc_cmd_mapping_cmd1_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_1) = params->emc_cmd_mapping_cmd1_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_2) = params->emc_cmd_mapping_cmd1_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_0) = params->emc_cmd_mapping_cmd2_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_1) = params->emc_cmd_mapping_cmd2_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_2) = params->emc_cmd_mapping_cmd2_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_0) = params->emc_cmd_mapping_cmd3_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_1) = params->emc_cmd_mapping_cmd3_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_2) = params->emc_cmd_mapping_cmd3_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_BYTE) = params->emc_cmd_mapping_byte;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_0) = params->emc_pmacro_brick_mapping0;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_1) = params->emc_pmacro_brick_mapping1;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_2) = params->emc_pmacro_brick_mapping2;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU1) = ((params->emc_pmacro_brick_ctrl_rfu1 & 0x1120112) | 0x1EED1EED);
    MAKE_EMC_REG(EMC_CONFIG_SAMPLE_DELAY) = params->emc_config_sample_delay;
    MAKE_EMC_REG(EMC_FBIO_CFG8) = params->emc_fbio_cfg8;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE0) = params->emc_swizzle_rank0_byte0;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE1) = params->emc_swizzle_rank0_byte1;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE2) = params->emc_swizzle_rank0_byte2;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE3) = params->emc_swizzle_rank0_byte3;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE0) = params->emc_swizzle_rank1_byte0;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE1) = params->emc_swizzle_rank1_byte1;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE2) = params->emc_swizzle_rank1_byte2;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE3) = params->emc_swizzle_rank1_byte3;
    
    if (params->emc_bct_spare6)
        *(volatile uint32_t *)params->emc_bct_spare6 = params->emc_bct_spare7;
    
    MAKE_EMC_REG(EMC_XM2COMPPADCTRL) = params->emc_xm2_comp_pad_ctrl;
    MAKE_EMC_REG(EMC_XM2COMPPADCTRL2) = params->emc_xm2_comp_pad_ctrl2;
    MAKE_EMC_REG(EMC_XM2COMPPADCTRL3) = params->emc_xm2_comp_pad_ctrl3;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG2) = params->emc_auto_cal_config2;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG3) = params->emc_auto_cal_config3;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG4) = params->emc_auto_cal_config4;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG5) = params->emc_auto_cal_config5;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG6) = params->emc_auto_cal_config6;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG7) = params->emc_auto_cal_config7;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG8) = params->emc_auto_cal_config8;
    MAKE_EMC_REG(EMC_PMACRO_RX_TERM) = params->emc_pmacro_rx_term;
    MAKE_EMC_REG(EMC_PMACRO_DQ_TX_DRV) = params->emc_pmacro_dq_tx_drive;
    MAKE_EMC_REG(EMC_PMACRO_CA_TX_DRV) = params->emc_pmacro_ca_tx_drive;
    MAKE_EMC_REG(EMC_PMACRO_CMD_TX_DRV) = params->emc_pmacro_cmd_tx_drive;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_COMMON) = params->emc_pmacro_auto_cal_common;
    MAKE_EMC_REG(EMC_AUTO_CAL_CHANNEL) = params->emc_auto_cal_channel;
    MAKE_EMC_REG(EMC_PMACRO_ZCTRL) = params->emc_pmacro_zcrtl;
    MAKE_EMC_REG(EMC_DLL_CFG_0) = params->emc_dll_cfg0;
    MAKE_EMC_REG(EMC_DLL_CFG_1) = params->emc_dll_cfg1;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL_1) = params->emc_cfg_dig_dll_1;
    MAKE_EMC_REG(EMC_DATA_BRLSHFT_0) = params->emc_data_brlshft0;
    MAKE_EMC_REG(EMC_DATA_BRLSHFT_1) = params->emc_data_brlshft1;
    MAKE_EMC_REG(EMC_DQS_BRLSHFT_0) = params->emc_dqs_brlshft0;
    MAKE_EMC_REG(EMC_DQS_BRLSHFT_1) = params->emc_dqs_brlshft1;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_0) = params->emc_cmd_brlshft0;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_1) = params->emc_cmd_brlshft1;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_2) = params->emc_cmd_brlshft2;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_3) = params->emc_cmd_brlshft3;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_0) = params->emc_quse_brlshft0;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_1) = params->emc_quse_brlshft1;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_2) = params->emc_quse_brlshft2;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_3) = params->emc_quse_brlshft3;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU1) = ((params->emc_pmacro_brick_ctrl_rfu1 & 0x1BF01BF) | 0x1E401E40);
    MAKE_EMC_REG(EMC_PMACRO_PAD_CFG_CTRL) = params->emc_pmacro_pad_cfg_ctrl;
    MAKE_EMC_REG(EMC_PMACRO_CMD_BRICK_CTRL_FDPD) = params->emc_pmacro_cmd_brick_ctrl_fdpd;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU2) = (params->emc_pmacro_brick_ctrl_rfu2 & 0xFF7FFF7F);
    MAKE_EMC_REG(EMC_PMACRO_DATA_BRICK_CTRL_FDPD) = params->emc_pmacro_data_brick_ctrl_fdpd;
    MAKE_EMC_REG(EMC_PMACRO_BG_BIAS_CTRL_0) = params->emc_pmacro_bg_bias_ctrl0;
    MAKE_EMC_REG(EMC_PMACRO_DATA_PAD_RX_CTRL) = params->emc_pmacro_data_pad_rx_ctrl;
    MAKE_EMC_REG(EMC_PMACRO_CMD_PAD_RX_CTRL) = params->emc_pmacro_cmd_pad_rx_ctrl;
    MAKE_EMC_REG(EMC_PMACRO_DATA_PAD_TX_CTRL) = params->emc_pmacro_data_pad_tx_ctrl;
    MAKE_EMC_REG(EMC_PMACRO_DATA_RX_TERM_MODE) = params->emc_pmacro_data_rx_term_mode;
    MAKE_EMC_REG(EMC_PMACRO_CMD_RX_TERM_MODE) = params->emc_pmacro_cmd_rx_term_mode;
    MAKE_EMC_REG(EMC_PMACRO_CMD_PAD_TX_CTRL) = params->emc_pmacro_cmd_pad_tx_ctrl;
    MAKE_EMC_REG(EMC_CFG_3) = params->emc_cfg3;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_0) = params->emc_pmacro_tx_pwrd0;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_1) = params->emc_pmacro_tx_pwrd1;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_2) = params->emc_pmacro_tx_pwrd2;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_3) = params->emc_pmacro_tx_pwrd3;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_4) = params->emc_pmacro_tx_pwrd4;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_5) = params->emc_pmacro_tx_pwrd5;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_0) = params->emc_pmacro_tx_sel_clk_src0;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_1) = params->emc_pmacro_tx_sel_clk_src1;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_2) = params->emc_pmacro_tx_sel_clk_src2;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_3) = params->emc_pmacro_tx_sel_clk_src3;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_4) = params->emc_pmacro_tx_sel_clk_src4;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_5) = params->emc_pmacro_tx_sel_clk_src5;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_BYPASS) = params->emc_pmacro_ddll_bypass;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_0) = params->emc_pmacro_ddll_pwrd0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_1) = params->emc_pmacro_ddll_pwrd1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_2) = params->emc_pmacro_ddll_pwrd2;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_0) = params->emc_pmacro_cmd_ctrl0;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_1) = params->emc_pmacro_cmd_ctrl1;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_2) = params->emc_pmacro_cmd_ctrl2;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQ_0) = params->emc_pmacro_ib_vref_dq_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQ_1) = params->emc_pmacro_ib_vref_dq_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQS_0) = params->emc_pmacro_ib_vref_dqs_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQS_1) = params->emc_pmacro_ib_vref_dqs_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_RXRT) = params->emc_pmacro_ib_rxrt;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_0) = params->emc_pmacro_quse_ddll_rank0_0;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_1) = params->emc_pmacro_quse_ddll_rank0_1;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_2) = params->emc_pmacro_quse_ddll_rank0_2;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_3) = params->emc_pmacro_quse_ddll_rank0_3;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_4) = params->emc_pmacro_quse_ddll_rank0_4;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_5) = params->emc_pmacro_quse_ddll_rank0_5;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_0) = params->emc_pmacro_quse_ddll_rank1_0;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_1) = params->emc_pmacro_quse_ddll_rank1_1;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_2) = params->emc_pmacro_quse_ddll_rank1_2;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_3) = params->emc_pmacro_quse_ddll_rank1_3;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_4) = params->emc_pmacro_quse_ddll_rank1_4;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_5) = params->emc_pmacro_quse_ddll_rank1_5;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU1) = params->emc_pmacro_brick_ctrl_rfu1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0) = params->emc_pmacro_ob_ddll_long_dq_rank0_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1) = params->emc_pmacro_ob_ddll_long_dq_rank0_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2) = params->emc_pmacro_ob_ddll_long_dq_rank0_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3) = params->emc_pmacro_ob_ddll_long_dq_rank0_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4) = params->emc_pmacro_ob_ddll_long_dq_rank0_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5) = params->emc_pmacro_ob_ddll_long_dq_rank0_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0) = params->emc_pmacro_ob_ddll_long_dq_rank1_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1) = params->emc_pmacro_ob_ddll_long_dq_rank1_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2) = params->emc_pmacro_ob_ddll_long_dq_rank1_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3) = params->emc_pmacro_ob_ddll_long_dq_rank1_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_4) = params->emc_pmacro_ob_ddll_long_dq_rank1_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_5) = params->emc_pmacro_ob_ddll_long_dq_rank1_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_0) = params->emc_pmacro_ob_ddll_long_dqs_rank0_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_1) = params->emc_pmacro_ob_ddll_long_dqs_rank0_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_2) = params->emc_pmacro_ob_ddll_long_dqs_rank0_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_3) = params->emc_pmacro_ob_ddll_long_dqs_rank0_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_4) = params->emc_pmacro_ob_ddll_long_dqs_rank0_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_5) = params->emc_pmacro_ob_ddll_long_dqs_rank0_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_0) = params->emc_pmacro_ob_ddll_long_dqs_rank1_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_1) = params->emc_pmacro_ob_ddll_long_dqs_rank1_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_2) = params->emc_pmacro_ob_ddll_long_dqs_rank1_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_3) = params->emc_pmacro_ob_ddll_long_dqs_rank1_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_4) = params->emc_pmacro_ob_ddll_long_dqs_rank1_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_5) = params->emc_pmacro_ob_ddll_long_dqs_rank1_5;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0) = params->emc_pmacro_ib_ddll_long_dqs_rank0_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1) = params->emc_pmacro_ib_ddll_long_dqs_rank0_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2) = params->emc_pmacro_ib_ddll_long_dqs_rank0_2;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3) = params->emc_pmacro_ib_ddll_long_dqs_rank0_3;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0) = params->emc_pmacro_ib_ddll_long_dqs_rank1_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1) = params->emc_pmacro_ib_ddll_long_dqs_rank1_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2) = params->emc_pmacro_ib_ddll_long_dqs_rank1_2;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3) = params->emc_pmacro_ib_ddll_long_dqs_rank1_3;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_0) = params->emc_pmacro_ddll_long_cmd_0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_1) = params->emc_pmacro_ddll_long_cmd_1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_2) = params->emc_pmacro_ddll_long_cmd_2;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_3) = params->emc_pmacro_ddll_long_cmd_3;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_4) = params->emc_pmacro_ddll_long_cmd_4;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_0) = params->emc_pmacro_ddll_short_cmd_0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_1) = params->emc_pmacro_ddll_short_cmd_1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_2) = params->emc_pmacro_ddll_short_cmd_2;
    MAKE_EMC_REG(EMC_PMACRO_COMMON_PAD_TX_CTRL) = ((params->emc_pmacro_common_pad_tx_ctrl & 1) | 0xE);
    
    if (params->emc_bct_spare4)
        *(volatile uint32_t *)params->emc_bct_spare4 = params->emc_bct_spare5;
    
    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    
    MAKE_MC_REG(MC_VIDEO_PROTECT_BOM) = params->mc_video_protect_bom;
    MAKE_MC_REG(MC_VIDEO_PROTECT_BOM_ADR_HI) = params->mc_video_protect_bom_adr_hi;
    MAKE_MC_REG(MC_VIDEO_PROTECT_SIZE_MB) = params->mc_video_protect_size_mb;
    MAKE_MC_REG(MC_VIDEO_PROTECT_VPR_OVERRIDE) = params->mc_video_protect_vpr_override;
    MAKE_MC_REG(MC_VIDEO_PROTECT_VPR_OVERRIDE1) = params->mc_video_protect_vpr_override1;
    MAKE_MC_REG(MC_VIDEO_PROTECT_GPU_OVERRIDE_0) = params->mc_video_protect_gpu_override0;
    MAKE_MC_REG(MC_VIDEO_PROTECT_GPU_OVERRIDE_1) = params->mc_video_protect_gpu_override1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG) = params->mc_emem_adr_cfg;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_DEV0) = params->mc_emem_adr_cfg_dev0;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_DEV1) = params->mc_emem_adr_cfg_dev1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_CHANNEL_MASK) = params->mc_emem_adr_cfg_channel_mask;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_0) = params->mc_emem_adr_cfg_bank_mask0;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_1) = params->mc_emem_adr_cfg_bank_mask1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_2) = params->mc_emem_adr_cfg_bank_mask2;
    MAKE_MC_REG(MC_EMEM_CFG) = params->mc_emem_cfg;
    MAKE_MC_REG(MC_SEC_CARVEOUT_BOM) = params->mc_sec_carveout_bom;
    MAKE_MC_REG(MC_SEC_CARVEOUT_ADR_HI) = params->mc_sec_carveout_adr_hi;
    MAKE_MC_REG(MC_SEC_CARVEOUT_SIZE_MB) = params->mc_sec_carveout_size_mb;
    MAKE_MC_REG(MC_MTS_CARVEOUT_BOM) = params->mc_mts_carveout_bom;
    MAKE_MC_REG(MC_MTS_CARVEOUT_ADR_HI) = params->mc_mts_carveout_adr_hi;
    MAKE_MC_REG(MC_MTS_CARVEOUT_SIZE_MB) = params->mc_mts_carveout_size_mb;
    MAKE_MC_REG(MC_EMEM_ARB_CFG) = params->mc_emem_arb_cfg;
    MAKE_MC_REG(MC_EMEM_ARB_OUTSTANDING_REQ) = params->mc_emem_arb_outstanding_req;
    MAKE_MC_REG(MC_EMEM_ARB_REFPB_HP_CTRL) = params->emc_emem_arb_refpb_hp_ctrl;
    MAKE_MC_REG(MC_EMEM_ARB_REFPB_BANK_CTRL) = params->emc_emem_arb_refpb_bank_ctrl;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RCD) = params->mc_emem_arb_timing_rcd;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RP) = params->mc_emem_arb_timing_rp;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RC) = params->mc_emem_arb_timing_rc;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RAS) = params->mc_emem_arb_timing_ras;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_FAW) = params->mc_emem_arb_timing_faw;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RRD) = params->mc_emem_arb_timing_rrd;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RAP2PRE) = params->mc_emem_arb_timing_rap2pre;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_WAP2PRE) = params->mc_emem_arb_timing_wap2pre;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_R2R) = params->mc_emem_arb_timing_r2r;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_W2W) = params->mc_emem_arb_timing_w2w;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_CCDMW) = params->mc_emem_arb_timing_ccdmw;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_R2W) = params->mc_emem_arb_timing_r2w;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_W2R) = params->mc_emem_arb_timing_w2r;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RFCPB) = params->mc_emem_arb_timing_rfcpb;
    MAKE_MC_REG(MC_EMEM_ARB_DA_TURNS) = params->mc_emem_arb_da_turns;
    MAKE_MC_REG(MC_EMEM_ARB_DA_COVERS) = params->mc_emem_arb_da_covers;
    MAKE_MC_REG(MC_EMEM_ARB_MISC0) = params->mc_emem_arb_misc0;
    MAKE_MC_REG(MC_EMEM_ARB_MISC1) = params->mc_emem_arb_misc1;
    MAKE_MC_REG(MC_EMEM_ARB_MISC2) = params->mc_emem_arb_misc2;
    MAKE_MC_REG(MC_EMEM_ARB_RING1_THROTTLE) = params->mc_emem_arb_ring1_throttle;
    MAKE_MC_REG(MC_EMEM_ARB_OVERRIDE) = params->mc_emem_arb_override;
    MAKE_MC_REG(MC_EMEM_ARB_OVERRIDE_1) = params->mc_emem_arb_override1;
    MAKE_MC_REG(MC_EMEM_ARB_RSV) = params->mc_emem_arb_rsv;
    MAKE_MC_REG(MC_DA_CONFIG0) = params->mc_da_cfg0;
    MAKE_MC_REG(MC_TIMING_CONTROL) = 1;
    MAKE_MC_REG(MC_CLKEN_OVERRIDE) = params->mc_clken_override;
    MAKE_MC_REG(MC_STAT_CONTROL) = params->mc_stat_control;
    
    MAKE_EMC_REG(EMC_ADR_CFG) = params->emc_adr_cfg;
    MAKE_EMC_REG(EMC_CLKEN_OVERRIDE) = params->emc_clken_override;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_0) = params->emc_pmacro_auto_cal_cfg0;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_1) = params->emc_pmacro_auto_cal_cfg1;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_2) = params->emc_pmacro_auto_cal_cfg2;
    MAKE_EMC_REG(EMC_AUTO_CAL_VREF_SEL_0) = params->emc_auto_cal_vref_sel0;
    MAKE_EMC_REG(EMC_AUTO_CAL_VREF_SEL_1) = params->emc_auto_cal_vref_sel1;
    MAKE_EMC_REG(EMC_AUTO_CAL_INTERVAL) = params->emc_auto_cal_interval;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG) = params->emc_auto_cal_config;
    udelay(params->emc_auto_cal_wait);
    
    if (params->emc_bct_spare8)
        *(volatile uint32_t *)params->emc_bct_spare8 = params->emc_bct_spare9;
    
    MAKE_EMC_REG(EMC_CFG_2) = params->emc_cfg2;
    MAKE_EMC_REG(EMC_CFG_PIPE) = params->emc_cfg_pipe;
    MAKE_EMC_REG(EMC_CFG_PIPE_1) = params->emc_cfg_pipe1;
    MAKE_EMC_REG(EMC_CFG_PIPE_2) = params->emc_cfg_pipe2;
    MAKE_EMC_REG(EMC_CMDQ) = params->emc_cmd_q;
    MAKE_EMC_REG(EMC_MC2EMCQ) = params->emc_mc2emc_q;
    MAKE_EMC_REG(EMC_MRS_WAIT_CNT) = params->emc_mrs_wait_cnt;
    MAKE_EMC_REG(EMC_MRS_WAIT_CNT2) = params->emc_mrs_wait_cnt2;
    MAKE_EMC_REG(EMC_FBIO_CFG5) = params->emc_fbio_cfg5;
    MAKE_EMC_REG(EMC_RC) = params->emc_rc;
    MAKE_EMC_REG(EMC_RFC) = params->emc_rfc;
    MAKE_EMC_REG(EMC_RFCPB) = params->emc_rfc_pb;
    MAKE_EMC_REG(EMC_REFCTRL2) = params->emc_ref_ctrl2;
    MAKE_EMC_REG(EMC_RFC_SLR) = params->emc_rfc_slr;
    MAKE_EMC_REG(EMC_RAS) = params->emc_ras;
    MAKE_EMC_REG(EMC_RP) = params->emc_rp;
    MAKE_EMC_REG(EMC_TPPD) = params->emc_tppd;
    MAKE_EMC_REG(EMC_R2R) = params->emc_r2r;
    MAKE_EMC_REG(EMC_W2W) = params->emc_w2w;
    MAKE_EMC_REG(EMC_R2W) = params->emc_r2w;
    MAKE_EMC_REG(EMC_W2R) = params->emc_w2r;
    MAKE_EMC_REG(EMC_R2P) = params->emc_r2p;
    MAKE_EMC_REG(EMC_W2P) = params->emc_w2p;
    MAKE_EMC_REG(EMC_CCDMW) = params->emc_ccdmw;
    MAKE_EMC_REG(EMC_RD_RCD) = params->emc_rd_rcd;
    MAKE_EMC_REG(EMC_WR_RCD) = params->emc_wr_rcd;
    MAKE_EMC_REG(EMC_RRD) = params->emc_rrd;
    MAKE_EMC_REG(EMC_REXT) = params->emc_rext;
    MAKE_EMC_REG(EMC_WEXT) = params->emc_wext;
    MAKE_EMC_REG(EMC_WDV) = params->emc_wdv;
    MAKE_EMC_REG(EMC_WDV_CHK) = params->emc_wdv_chk;
    MAKE_EMC_REG(EMC_WSV) = params->emc_wsv;
    MAKE_EMC_REG(EMC_WEV) = params->emc_wev;
    MAKE_EMC_REG(EMC_WDV_MASK) = params->emc_wdv_mask;
    MAKE_EMC_REG(EMC_WS_DURATION) = params->emc_ws_duration;
    MAKE_EMC_REG(EMC_WE_DURATION) = params->emc_we_duration;
    MAKE_EMC_REG(EMC_QUSE) = params->emc_quse;
    MAKE_EMC_REG(EMC_QUSE_WIDTH) = params->emc_quse_width;
    MAKE_EMC_REG(EMC_IBDLY) = params->emc_ibdly;
    MAKE_EMC_REG(EMC_OBDLY) = params->emc_obdly;
    MAKE_EMC_REG(EMC_EINPUT) = params->emc_einput;
    MAKE_EMC_REG(EMC_EINPUT_DURATION) = params->emc_einput_duration;
    MAKE_EMC_REG(EMC_PUTERM_EXTRA) = params->emc_puterm_extra;
    MAKE_EMC_REG(EMC_PUTERM_WIDTH) = params->emc_puterm_width;
    MAKE_EMC_REG(EMC_PMACRO_COMMON_PAD_TX_CTRL) = params->emc_pmacro_common_pad_tx_ctrl;
    MAKE_EMC_REG(EMC_DBG) = params->emc_dbg;
    MAKE_EMC_REG(EMC_QRST) = params->emc_qrst;
    MAKE_EMC_REG(EMC_ISSUE_QRST) = 0;
    MAKE_EMC_REG(EMC_QSAFE) = params->emc_qsafe;
    MAKE_EMC_REG(EMC_RDV) = params->emc_rdv;
    MAKE_EMC_REG(EMC_RDV_MASK) = params->emc_rdv_mask;
    MAKE_EMC_REG(EMC_RDV_EARLY) = params->emc_rdv_early;
    MAKE_EMC_REG(EMC_RDV_EARLY_MASK) = params->emc_rdv_early_mask;
    MAKE_EMC_REG(EMC_QPOP) = params->emc_qpop;
    MAKE_EMC_REG(EMC_REFRESH) = params->emc_refresh;
    MAKE_EMC_REG(EMC_BURST_REFRESH_NUM) = params->emc_burst_refresh_num;
    MAKE_EMC_REG(EMC_PRE_REFRESH_REQ_CNT) = params->emc_prerefresh_req_cnt;
    MAKE_EMC_REG(EMC_PDEX2WR) = params->emc_pdex2wr;
    MAKE_EMC_REG(EMC_PDEX2RD) = params->emc_pdex2rd;
    MAKE_EMC_REG(EMC_PCHG2PDEN) = params->emc_pchg2pden;
    MAKE_EMC_REG(EMC_ACT2PDEN) = params->emc_act2pden;
    MAKE_EMC_REG(EMC_AR2PDEN) = params->emc_ar2pden;
    MAKE_EMC_REG(EMC_RW2PDEN) = params->emc_rw2pden;
    MAKE_EMC_REG(EMC_CKE2PDEN) = params->emc_cke2pden;
    MAKE_EMC_REG(EMC_PDEX2CKE) = params->emc_pdex2che;
    MAKE_EMC_REG(EMC_PDEX2MRR) = params->emc_pdex2mrr;
    MAKE_EMC_REG(EMC_TXSR) = params->emc_txsr;
    MAKE_EMC_REG(EMC_TXSRDLL) = params->emc_txsr_dll;
    MAKE_EMC_REG(EMC_TCKE) = params->emc_tcke;
    MAKE_EMC_REG(EMC_TCKESR) = params->emc_tckesr;
    MAKE_EMC_REG(EMC_TPD) = params->emc_tpd;
    MAKE_EMC_REG(EMC_TFAW) = params->emc_tfaw;
    MAKE_EMC_REG(EMC_TRPAB) = params->emc_trpab;
    MAKE_EMC_REG(EMC_TCLKSTABLE) = params->emc_tclkstable;
    MAKE_EMC_REG(EMC_TCLKSTOP) = params->emc_tclkstop;
    MAKE_EMC_REG(EMC_TREFBW) = params->emc_trefbw;
    MAKE_EMC_REG(EMC_ODT_WRITE) = params->emc_odt_write;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL) = params->emc_cfg_dig_dll;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL_PERIOD) = params->emc_cfg_dig_dll_period;
    MAKE_EMC_REG(EMC_FBIO_SPARE) = params->emc_fbio_spare & 0xFFFFFFFD;
    MAKE_EMC_REG(EMC_CFG_RSV) = params->emc_cfg_rsv;
    MAKE_EMC_REG(EMC_PMC_SCRATCH1) = params->emc_pmc_scratch1;
    MAKE_EMC_REG(EMC_PMC_SCRATCH2) = params->emc_pmc_scratch2;
    MAKE_EMC_REG(EMC_PMC_SCRATCH3) = params->emc_pmc_scratch3;
    MAKE_EMC_REG(EMC_ACPD_CONTROL) = params->emc_acpd_control;
    MAKE_EMC_REG(EMC_TXDSRVTTGEN) = params->emc_txdsrvttgen;
    MAKE_EMC_REG(EMC_CFG) = (params->emc_cfg & 0xE) | 0x3C00000;
    
    if (params->boot_rom_patch_control & 0x80000000)
    {
        *(volatile uint32_t *)(4 * (params->boot_rom_patch_control + 0x1C000000)) = params->boot_rom_patch_data;
        MAKE_MC_REG(MC_TIMING_CONTROL) = 1;
    }
    
    pmc->io_dpd3_req = (((4 * params->emc_pmc_scratch1 >> 2) + 0x40000000) & 0xCFFF0000);
    udelay(params->pmc_io_dpd3_req_wait);
    
    if (!params->emc_auto_cal_interval)
        MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG) = (params->emc_auto_cal_config | 0x200);
    
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU2) = params->emc_pmacro_brick_ctrl_rfu2;
    
    if (params->emc_zcal_warm_cold_boot_enables & 1)
    {
        if (params->memory_type == 2)
            MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = (8 * params->emc_zcal_wait_cnt);
        
        if (params->memory_type == 3)
        {
            MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt;
            MAKE_EMC_REG(EMC_ZCAL_MRW_CMD) = params->emc_zcal_mrw_cmd;
        }
    }
    
    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    udelay(params->emc_timing_control_wait);
    
    pmc->ddr_cntrl &= 0xFFF8007F;
    udelay(params->pmc_ddr_ctrl_wait);
    
    if (params->memory_type == 2)
    {
        MAKE_EMC_REG(EMC_PIN) = ((params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12));
        udelay(params->emc_pin_extra_wait + 200);
        MAKE_EMC_REG(EMC_PIN) = (((params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12)) + 256);
        udelay(params->emc_pin_extra_wait + 500);
    }
    
    if (params->memory_type == 3)
    {
        MAKE_EMC_REG(EMC_PIN) = ((params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12));
        udelay(params->emc_pin_extra_wait + 200);
        MAKE_EMC_REG(EMC_PIN) = (((params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12)) + 256);
        udelay(params->emc_pin_extra_wait + 2000);
    }
    
    MAKE_EMC_REG(EMC_PIN) = (((params->emc_pin_gpio_enable << 16) | (params->emc_pin_gpio << 12)) + 0x101);
    udelay(params->emc_pin_program_wait);
    
    if (params->memory_type != 3)
        MAKE_EMC_REG(EMC_NOP) = ((params->emc_dev_select << 30) + 1);
    
    if (params->memory_type == 1)
        udelay(params->emc_pin_extra_wait + 200);
    
    if (params->memory_type == 3)
    {
        if (params->emc_bct_spare10)
            *(volatile uint32_t *)params->emc_bct_spare10 = params->emc_bct_spare11;
        
        MAKE_EMC_REG(EMC_MRW2) = params->emc_mrw2;
        MAKE_EMC_REG(EMC_MRW) = params->emc_mrw1;
        MAKE_EMC_REG(EMC_MRW3) = params->emc_mrw3;
        MAKE_EMC_REG(EMC_MRW4) = params->emc_mrw4;
        MAKE_EMC_REG(EMC_MRW6) = params->emc_mrw6;
        MAKE_EMC_REG(EMC_MRW14) = params->emc_mrw14;
        MAKE_EMC_REG(EMC_MRW8) = params->emc_mrw8;
        MAKE_EMC_REG(EMC_MRW12) = params->emc_mrw12;
        MAKE_EMC_REG(EMC_MRW9) = params->emc_mrw9;
        MAKE_EMC_REG(EMC_MRW13) = params->emc_mrw13;
        
        if (params->emc_zcal_warm_cold_boot_enables & 1)
        {
            MAKE_EMC_REG(EMC_ZQ_CAL) = params->emc_zcal_init_dev0;
            udelay(params->emc_zcal_init_wait);
            MAKE_EMC_REG(EMC_ZQ_CAL) = (params->emc_zcal_init_dev0 ^ 3);
            
            if (!(params->emc_dev_select & 2))
            {
                MAKE_EMC_REG(EMC_ZQ_CAL) = params->emc_zcal_init_dev1;
                udelay(params->emc_zcal_init_wait);
                MAKE_EMC_REG(EMC_ZQ_CAL) = (params->emc_zcal_init_dev1 ^ 3);
            }
        }
    }
    
    pmc->ddr_cfg = params->pmc_ddr_cfg;
    if ((params->memory_type - 1) <= 2)
    {
        MAKE_EMC_REG(EMC_ZCAL_INTERVAL) = params->emc_zcal_interval;
        MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = params->emc_zcal_wait_cnt;
        MAKE_EMC_REG(EMC_ZCAL_MRW_CMD) = params->emc_zcal_mrw_cmd;
    }
    
    if (params->emc_bct_spare12)
        *(volatile uint32_t *)params->emc_bct_spare12 = params->emc_bct_spare13;
    
    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    
    if (params->emc_extra_refresh_num)
        MAKE_EMC_REG(EMC_REF) = (((1 << params->emc_extra_refresh_num << 8) - 0xFD) | (params->emc_pin_gpio << 30));
    
    MAKE_EMC_REG(EMC_REFCTRL) = (params->emc_dev_select | 0x80000000);
    MAKE_EMC_REG(EMC_DYN_SELF_REF_CONTROL) = params->emc_dyn_self_ref_control;
    MAKE_EMC_REG(EMC_CFG_UPDATE) = params->emc_cfg_update;
    MAKE_EMC_REG(EMC_CFG) = params->emc_cfg;
    MAKE_EMC_REG(EMC_FDPD_CTRL_DQ) = params->emc_fdpd_ctrl_dq;
    MAKE_EMC_REG(EMC_FDPD_CTRL_CMD) = params->emc_fdpd_ctrl_cmd;
    MAKE_EMC_REG(EMC_SEL_DPD_CTRL) = params->emc_sel_dpd_ctrl;
    MAKE_EMC_REG(EMC_FBIO_SPARE) = (params->emc_fbio_spare | 2);
    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    MAKE_EMC_REG(EMC_CFG_PIPE_CLK) = params->emc_cfg_pipe_clk;
    MAKE_EMC_REG(EMC_FDPD_CTRL_CMD_NO_RAMP) = params->emc_fdpd_ctrl_cmd_no_ramp;
    
    AHB_ARBITRATION_XBAR_CTRL_0 = ((AHB_ARBITRATION_XBAR_CTRL_0 & 0xFFFEFFFF) | ((params->ahb_arbitration_xbar_ctrl_meminit_done & 0xFFFF) << 16));
    
    MAKE_MC_REG(MC_VIDEO_PROTECT_REG_CTRL) = params->mc_video_protect_write_access;
    MAKE_MC_REG(MC_SEC_CARVEOUT_REG_CTRL) = params->mc_sec_carveout_protect_write_access;
    MAKE_MC_REG(MC_MTS_CARVEOUT_REG_CTRL) = params->mc_mts_carveout_reg_ctrl;
    MAKE_MC_REG(MC_EMEM_CFG_ACCESS_CTRL) = 1;   /* Disable write access to a bunch of MC registers. */
}

const void *sdram_get_params()
{
    /* TODO: sdram_id should be in [0, 7]. */

#ifdef CONFIG_SDRAM_COMPRESS_CFG
    uint8_t *buf = (uint8_t *)0x40030000;
    LZ_Uncompress(_dram_cfg_lz, buf, sizeof(_dram_cfg_lz));
    return (const void *)&buf[sizeof(sdram_params_t) * _get_sdram_id()];
#else
    return _dram_cfgs[_get_sdram_id()];
#endif
}

void sdram_init()
{
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    
    /* TODO: sdram_id should be in [0,4]. */
    const sdram_params_t *params = (const sdram_params_t *)sdram_get_params();

    uint8_t val = 5;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD_CFG2, &val, 1);
    val = 40;   /* 40 = (1000 * 1100 - 600000) / 12500 -> 1.1V */
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD1, &val, 1);

    pmc->vddp_sel = params->pmc_vddp_sel;
    udelay(params->pmc_vddp_sel_wait);
    
    pmc->ddr_pwr = pmc->ddr_pwr;
    pmc->no_iopower = params->pmc_no_io_power;
    pmc->reg_short = params->pmc_reg_short;
    pmc->ddr_cntrl = params->pmc_ddr_ctrl;

    if (params->emc_bct_spare0)
        *(volatile uint32_t *)params->emc_bct_spare0 = params->emc_bct_spare1;

    _sdram_config(params);
}
