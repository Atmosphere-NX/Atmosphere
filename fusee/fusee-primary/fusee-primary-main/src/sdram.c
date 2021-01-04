/*
 * Copyright (c) 2018 naehrwert
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

#include "i2c.h"
#include "mc.h"
#include "emc.h"
#include "pmc.h"
#include "timers.h"
#include "sysreg.h"
#include "fuse.h"
#include "max77620.h"
#include "sdram_params.h"
#include "car.h"

#define CONFIG_SDRAM_COMPRESS

#ifdef CONFIG_SDRAM_COMPRESS
#include "../../../fusee/common/lz.h"
#include "sdram_lz.inl"
#else
#include "sdram.inl"
#endif

/* Determine the current SoC for Mariko specific code. */
static bool is_soc_mariko() {
    return (fuse_get_soc_type() == 1);
}

static void sdram_config_erista(const sdram_params_erista_t *params) {
    volatile tegra_car_t *car = car_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();

    pmc->io_dpd3_req = (((4 * params->EmcPmcScratch1 >> 2) + 0x80000000) ^ 0xFFFF) & 0xC000FFFF;
    udelay(params->PmcIoDpd3ReqWait);
    uint32_t req = (4 * params->EmcPmcScratch2 >> 2) + 0x80000000;
    pmc->io_dpd4_req = (req >> 16 << 16) ^ 0x3FFF0000;
    udelay(params->PmcIoDpd4ReqWait);
    pmc->io_dpd4_req = (req ^ 0xFFFF) & 0xC000FFFF;
    udelay(params->PmcIoDpd4ReqWait);
    pmc->weak_bias = 0;
    udelay(1);

    car->pllm_misc1 = params->PllMSetupControl;
    car->pllm_misc2 = 0;
    car->pllm_base = ((params->PllMFeedbackDivider << 8) | params->PllMInputDivider | 0x40000000 | ((params->PllMPostDivider & 0xFFFF) << 20));

    bool timeout = false;
    uint32_t wait_end = get_time_us() + 300;

    while (!(car->pllm_base & 0x8000000) && !timeout) {
        if (get_time_us() >= wait_end)
            timeout = true;
    }

    if (!timeout) {
        udelay(10);
    }

    car->clk_source_emc = (((params->McEmemArbMisc0 >> 11) & 0x10000) | (params->EmcClockSource & 0xFFFEFFFF));

    if (params->EmcClockSourceDll) {
        car->clk_source_emc_dll = params->EmcClockSourceDll;
    }
    if (params->ClearClk2Mc1) {
        car->clk_enb_w_clr = 0x40000000;
    }

    car->clk_enb_h_set = 0x2000001;
    car->clk_enb_x_set = 0x4000;
    car->rst_dev_h_clr = 0x2000001;

    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_0) = params->EmcPmacroVttgenCtrl0;
    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_1) = params->EmcPmacroVttgenCtrl1;
    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_2) = params->EmcPmacroVttgenCtrl2;
    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    udelay(1);

    MAKE_EMC_REG(EMC_DBG) = (params->EmcDbgWriteMux << 1) | params->EmcDbg;

    if (params->EmcBctSpare2) {
        *(volatile uint32_t *)params->EmcBctSpare2 = params->EmcBctSpare3;
    }

    MAKE_EMC_REG(EMC_FBIO_CFG7) = params->EmcFbioCfg7;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_0) = params->EmcCmdMappingCmd0_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_1) = params->EmcCmdMappingCmd0_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_2) = params->EmcCmdMappingCmd0_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_0) = params->EmcCmdMappingCmd1_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_1) = params->EmcCmdMappingCmd1_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_2) = params->EmcCmdMappingCmd1_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_0) = params->EmcCmdMappingCmd2_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_1) = params->EmcCmdMappingCmd2_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_2) = params->EmcCmdMappingCmd2_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_0) = params->EmcCmdMappingCmd3_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_1) = params->EmcCmdMappingCmd3_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_2) = params->EmcCmdMappingCmd3_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_BYTE) = params->EmcCmdMappingByte;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_0) = params->EmcPmacroBrickMapping0;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_1) = params->EmcPmacroBrickMapping1;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_2) = params->EmcPmacroBrickMapping2;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU1) = ((params->EmcPmacroBrickCtrlRfu1 & 0x1120112) | 0x1EED1EED);
    MAKE_EMC_REG(EMC_CONFIG_SAMPLE_DELAY) = params->EmcConfigSampleDelay;
    MAKE_EMC_REG(EMC_FBIO_CFG8) = params->EmcFbioCfg8;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE0) = params->EmcSwizzleRank0Byte0;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE1) = params->EmcSwizzleRank0Byte1;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE2) = params->EmcSwizzleRank0Byte2;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE3) = params->EmcSwizzleRank0Byte3;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE0) = params->EmcSwizzleRank1Byte0;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE1) = params->EmcSwizzleRank1Byte1;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE2) = params->EmcSwizzleRank1Byte2;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE3) = params->EmcSwizzleRank1Byte3;

    if (params->EmcBctSpare6) {
        *(volatile uint32_t *)params->EmcBctSpare6 = params->EmcBctSpare7;
    }

    MAKE_EMC_REG(EMC_XM2COMPPADCTRL) = params->EmcXm2CompPadCtrl;
    MAKE_EMC_REG(EMC_XM2COMPPADCTRL2) = params->EmcXm2CompPadCtrl2;
    MAKE_EMC_REG(EMC_XM2COMPPADCTRL3) = params->EmcXm2CompPadCtrl3;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG2) = params->EmcAutoCalConfig2;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG3) = params->EmcAutoCalConfig3;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG4) = params->EmcAutoCalConfig4;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG5) = params->EmcAutoCalConfig5;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG6) = params->EmcAutoCalConfig6;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG7) = params->EmcAutoCalConfig7;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG8) = params->EmcAutoCalConfig8;
    MAKE_EMC_REG(EMC_PMACRO_RX_TERM) = params->EmcPmacroRxTerm;
    MAKE_EMC_REG(EMC_PMACRO_DQ_TX_DRV) = params->EmcPmacroDqTxDrv;
    MAKE_EMC_REG(EMC_PMACRO_CA_TX_DRV) = params->EmcPmacroCaTxDrv;
    MAKE_EMC_REG(EMC_PMACRO_CMD_TX_DRV) = params->EmcPmacroCmdTxDrv;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_COMMON) = params->EmcPmacroAutocalCfgCommon;
    MAKE_EMC_REG(EMC_AUTO_CAL_CHANNEL) = params->EmcAutoCalChannel;
    MAKE_EMC_REG(EMC_PMACRO_ZCTRL) = params->EmcPmacroZctrl;
    MAKE_EMC_REG(EMC_DLL_CFG_0) = params->EmcDllCfg0;
    MAKE_EMC_REG(EMC_DLL_CFG_1) = params->EmcDllCfg1;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL_1) = params->EmcCfgDigDll_1;
    MAKE_EMC_REG(EMC_DATA_BRLSHFT_0) = params->EmcDataBrlshft0;
    MAKE_EMC_REG(EMC_DATA_BRLSHFT_1) = params->EmcDataBrlshft1;
    MAKE_EMC_REG(EMC_DQS_BRLSHFT_0) = params->EmcDqsBrlshft0;
    MAKE_EMC_REG(EMC_DQS_BRLSHFT_1) = params->EmcDqsBrlshft1;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_0) = params->EmcCmdBrlshft0;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_1) = params->EmcCmdBrlshft1;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_2) = params->EmcCmdBrlshft2;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_3) = params->EmcCmdBrlshft3;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_0) = params->EmcQuseBrlshft0;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_1) = params->EmcQuseBrlshft1;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_2) = params->EmcQuseBrlshft2;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_3) = params->EmcQuseBrlshft3;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU1) = ((params->EmcPmacroBrickCtrlRfu1 & 0x1BF01BF) | 0x1E401E40);
    MAKE_EMC_REG(EMC_PMACRO_PAD_CFG_CTRL) = params->EmcPmacroPadCfgCtrl;
    MAKE_EMC_REG(EMC_PMACRO_CMD_BRICK_CTRL_FDPD) = params->EmcPmacroCmdBrickCtrlFdpd;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU2) = (params->EmcPmacroBrickCtrlRfu2 & 0xFF7FFF7F);
    MAKE_EMC_REG(EMC_PMACRO_DATA_BRICK_CTRL_FDPD) = params->EmcPmacroDataBrickCtrlFdpd;
    MAKE_EMC_REG(EMC_PMACRO_BG_BIAS_CTRL_0) = params->EmcPmacroBgBiasCtrl0;
    MAKE_EMC_REG(EMC_PMACRO_DATA_PAD_RX_CTRL) = params->EmcPmacroDataPadRxCtrl;
    MAKE_EMC_REG(EMC_PMACRO_CMD_PAD_RX_CTRL) = params->EmcPmacroCmdPadRxCtrl;
    MAKE_EMC_REG(EMC_PMACRO_DATA_PAD_TX_CTRL) = params->EmcPmacroDataPadTxCtrl;
    MAKE_EMC_REG(EMC_PMACRO_DATA_RX_TERM_MODE) = params->EmcPmacroDataRxTermMode;
    MAKE_EMC_REG(EMC_PMACRO_CMD_RX_TERM_MODE) = params->EmcPmacroCmdRxTermMode;
    MAKE_EMC_REG(EMC_PMACRO_CMD_PAD_TX_CTRL) = params->EmcPmacroCmdPadTxCtrl;
    MAKE_EMC_REG(EMC_CFG_3) = params->EmcCfg3;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_0) = params->EmcPmacroTxPwrd0;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_1) = params->EmcPmacroTxPwrd1;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_2) = params->EmcPmacroTxPwrd2;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_3) = params->EmcPmacroTxPwrd3;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_4) = params->EmcPmacroTxPwrd4;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_5) = params->EmcPmacroTxPwrd5;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_0) = params->EmcPmacroTxSelClkSrc0;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_1) = params->EmcPmacroTxSelClkSrc1;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_2) = params->EmcPmacroTxSelClkSrc2;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_3) = params->EmcPmacroTxSelClkSrc3;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_4) = params->EmcPmacroTxSelClkSrc4;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_5) = params->EmcPmacroTxSelClkSrc5;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_BYPASS) = params->EmcPmacroDdllBypass;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_0) = params->EmcPmacroDdllPwrd0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_1) = params->EmcPmacroDdllPwrd1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_2) = params->EmcPmacroDdllPwrd2;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_0) = params->EmcPmacroCmdCtrl0;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_1) = params->EmcPmacroCmdCtrl1;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_2) = params->EmcPmacroCmdCtrl2;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQ_0) = params->EmcPmacroIbVrefDq_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQ_1) = params->EmcPmacroIbVrefDq_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQS_0) = params->EmcPmacroIbVrefDqs_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQS_1) = params->EmcPmacroIbVrefDqs_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_RXRT) = params->EmcPmacroIbRxrt;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_0) = params->EmcPmacroQuseDdllRank0_0;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_1) = params->EmcPmacroQuseDdllRank0_1;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_2) = params->EmcPmacroQuseDdllRank0_2;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_3) = params->EmcPmacroQuseDdllRank0_3;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_4) = params->EmcPmacroQuseDdllRank0_4;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_5) = params->EmcPmacroQuseDdllRank0_5;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_0) = params->EmcPmacroQuseDdllRank1_0;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_1) = params->EmcPmacroQuseDdllRank1_1;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_2) = params->EmcPmacroQuseDdllRank1_2;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_3) = params->EmcPmacroQuseDdllRank1_3;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_4) = params->EmcPmacroQuseDdllRank1_4;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_5) = params->EmcPmacroQuseDdllRank1_5;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU1) = params->EmcPmacroBrickCtrlRfu1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0) = params->EmcPmacroObDdllLongDqRank0_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1) = params->EmcPmacroObDdllLongDqRank0_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2) = params->EmcPmacroObDdllLongDqRank0_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3) = params->EmcPmacroObDdllLongDqRank0_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4) = params->EmcPmacroObDdllLongDqRank0_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5) = params->EmcPmacroObDdllLongDqRank0_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0) = params->EmcPmacroObDdllLongDqRank1_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1) = params->EmcPmacroObDdllLongDqRank1_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2) = params->EmcPmacroObDdllLongDqRank1_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3) = params->EmcPmacroObDdllLongDqRank1_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_4) = params->EmcPmacroObDdllLongDqRank1_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_5) = params->EmcPmacroObDdllLongDqRank1_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_0) = params->EmcPmacroObDdllLongDqsRank0_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_1) = params->EmcPmacroObDdllLongDqsRank0_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_2) = params->EmcPmacroObDdllLongDqsRank0_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_3) = params->EmcPmacroObDdllLongDqsRank0_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_4) = params->EmcPmacroObDdllLongDqsRank0_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_5) = params->EmcPmacroObDdllLongDqsRank0_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_0) = params->EmcPmacroObDdllLongDqsRank1_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_1) = params->EmcPmacroObDdllLongDqsRank1_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_2) = params->EmcPmacroObDdllLongDqsRank1_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_3) = params->EmcPmacroObDdllLongDqsRank1_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_4) = params->EmcPmacroObDdllLongDqsRank1_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_5) = params->EmcPmacroObDdllLongDqsRank1_5;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0) = params->EmcPmacroIbDdllLongDqsRank0_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1) = params->EmcPmacroIbDdllLongDqsRank0_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2) = params->EmcPmacroIbDdllLongDqsRank0_2;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3) = params->EmcPmacroIbDdllLongDqsRank0_3;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0) = params->EmcPmacroIbDdllLongDqsRank1_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1) = params->EmcPmacroIbDdllLongDqsRank1_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2) = params->EmcPmacroIbDdllLongDqsRank1_2;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3) = params->EmcPmacroIbDdllLongDqsRank1_3;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_0) = params->EmcPmacroDdllLongCmd_0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_1) = params->EmcPmacroDdllLongCmd_1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_2) = params->EmcPmacroDdllLongCmd_2;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_3) = params->EmcPmacroDdllLongCmd_3;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_4) = params->EmcPmacroDdllLongCmd_4;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_0) = params->EmcPmacroDdllShortCmd_0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_1) = params->EmcPmacroDdllShortCmd_1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_2) = params->EmcPmacroDdllShortCmd_2;
    MAKE_EMC_REG(EMC_PMACRO_COMMON_PAD_TX_CTRL) = ((params->EmcPmacroCommonPadTxCtrl & 1) | 0xE);

    if (params->EmcBctSpare4) {
        *(volatile uint32_t *)params->EmcBctSpare4 = params->EmcBctSpare5;
    }

    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;

    MAKE_MC_REG(MC_VIDEO_PROTECT_BOM) = params->McVideoProtectBom;
    MAKE_MC_REG(MC_VIDEO_PROTECT_BOM_ADR_HI) = params->McVideoProtectBomAdrHi;
    MAKE_MC_REG(MC_VIDEO_PROTECT_SIZE_MB) = params->McVideoProtectSizeMb;
    MAKE_MC_REG(MC_VIDEO_PROTECT_VPR_OVERRIDE) = params->McVideoProtectVprOverride;
    MAKE_MC_REG(MC_VIDEO_PROTECT_VPR_OVERRIDE1) = params->McVideoProtectVprOverride1;
    MAKE_MC_REG(MC_VIDEO_PROTECT_GPU_OVERRIDE_0) = params->McVideoProtectGpuOverride0;
    MAKE_MC_REG(MC_VIDEO_PROTECT_GPU_OVERRIDE_1) = params->McVideoProtectGpuOverride1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG) = params->McEmemAdrCfg;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_DEV0) = params->McEmemAdrCfgDev0;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_DEV1) = params->McEmemAdrCfgDev1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_CHANNEL_MASK) = params->McEmemAdrCfgChannelMask;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_0) = params->McEmemAdrCfgBankMask0;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_1) = params->McEmemAdrCfgBankMask1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_2) = params->McEmemAdrCfgBankMask2;
    MAKE_MC_REG(MC_EMEM_CFG) = params->McEmemCfg;
    MAKE_MC_REG(MC_SEC_CARVEOUT_BOM) = params->McSecCarveoutBom;
    MAKE_MC_REG(MC_SEC_CARVEOUT_ADR_HI) = params->McSecCarveoutAdrHi;
    MAKE_MC_REG(MC_SEC_CARVEOUT_SIZE_MB) = params->McSecCarveoutSizeMb;
    MAKE_MC_REG(MC_MTS_CARVEOUT_BOM) = params->McMtsCarveoutBom;
    MAKE_MC_REG(MC_MTS_CARVEOUT_ADR_HI) = params->McMtsCarveoutAdrHi;
    MAKE_MC_REG(MC_MTS_CARVEOUT_SIZE_MB) = params->McMtsCarveoutSizeMb;
    MAKE_MC_REG(MC_EMEM_ARB_CFG) = params->McEmemArbCfg;
    MAKE_MC_REG(MC_EMEM_ARB_OUTSTANDING_REQ) = params->McEmemArbOutstandingReq;
    MAKE_MC_REG(MC_EMEM_ARB_REFPB_HP_CTRL) = params->McEmemArbRefpbHpCtrl;
    MAKE_MC_REG(MC_EMEM_ARB_REFPB_BANK_CTRL) = params->McEmemArbRefpbBankCtrl;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RCD) = params->McEmemArbTimingRcd;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RP) = params->McEmemArbTimingRp;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RC) = params->McEmemArbTimingRc;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RAS) = params->McEmemArbTimingRas;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_FAW) = params->McEmemArbTimingFaw;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RRD) = params->McEmemArbTimingRrd;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RAP2PRE) = params->McEmemArbTimingRap2Pre;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_WAP2PRE) = params->McEmemArbTimingWap2Pre;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_R2R) = params->McEmemArbTimingR2R;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_W2W) = params->McEmemArbTimingW2W;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_CCDMW) = params->McEmemArbTimingCcdmw;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_R2W) = params->McEmemArbTimingR2W;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_W2R) = params->McEmemArbTimingW2R;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RFCPB) = params->McEmemArbTimingRFCPB;
    MAKE_MC_REG(MC_EMEM_ARB_DA_TURNS) = params->McEmemArbDaTurns;
    MAKE_MC_REG(MC_EMEM_ARB_DA_COVERS) = params->McEmemArbDaCovers;
    MAKE_MC_REG(MC_EMEM_ARB_MISC0) = params->McEmemArbMisc0;
    MAKE_MC_REG(MC_EMEM_ARB_MISC1) = params->McEmemArbMisc1;
    MAKE_MC_REG(MC_EMEM_ARB_MISC2) = params->McEmemArbMisc2;
    MAKE_MC_REG(MC_EMEM_ARB_RING1_THROTTLE) = params->McEmemArbRing1Throttle;
    MAKE_MC_REG(MC_EMEM_ARB_OVERRIDE) = params->McEmemArbOverride;
    MAKE_MC_REG(MC_EMEM_ARB_OVERRIDE_1) = params->McEmemArbOverride1;
    MAKE_MC_REG(MC_EMEM_ARB_RSV) = params->McEmemArbRsv;
    MAKE_MC_REG(MC_DA_CONFIG0) = params->McDaCfg0;
    MAKE_MC_REG(MC_TIMING_CONTROL) = 1;
    MAKE_MC_REG(MC_CLKEN_OVERRIDE) = params->McClkenOverride;
    MAKE_MC_REG(MC_STAT_CONTROL) = params->McStatControl;

    MAKE_EMC_REG(EMC_ADR_CFG) = params->EmcAdrCfg;
    MAKE_EMC_REG(EMC_CLKEN_OVERRIDE) = params->EmcClkenOverride;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_0) = params->EmcPmacroAutocalCfg0;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_1) = params->EmcPmacroAutocalCfg1;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_2) = params->EmcPmacroAutocalCfg2;
    MAKE_EMC_REG(EMC_AUTO_CAL_VREF_SEL_0) = params->EmcAutoCalVrefSel0;
    MAKE_EMC_REG(EMC_AUTO_CAL_VREF_SEL_1) = params->EmcAutoCalVrefSel1;
    MAKE_EMC_REG(EMC_AUTO_CAL_INTERVAL) = params->EmcAutoCalInterval;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG) = params->EmcAutoCalConfig;
    udelay(params->EmcAutoCalWait);

    if (params->EmcBctSpare8) {
        *(volatile uint32_t *)params->EmcBctSpare8 = params->EmcBctSpare9;
    }

    MAKE_EMC_REG(EMC_CFG_2) = params->EmcCfg2;
    MAKE_EMC_REG(EMC_CFG_PIPE) = params->EmcCfgPipe;
    MAKE_EMC_REG(EMC_CFG_PIPE_1) = params->EmcCfgPipe1;
    MAKE_EMC_REG(EMC_CFG_PIPE_2) = params->EmcCfgPipe2;
    MAKE_EMC_REG(EMC_CMDQ) = params->EmcCmdQ;
    MAKE_EMC_REG(EMC_MC2EMCQ) = params->EmcMc2EmcQ;
    MAKE_EMC_REG(EMC_MRS_WAIT_CNT) = params->EmcMrsWaitCnt;
    MAKE_EMC_REG(EMC_MRS_WAIT_CNT2) = params->EmcMrsWaitCnt2;
    MAKE_EMC_REG(EMC_FBIO_CFG5) = params->EmcFbioCfg5;
    MAKE_EMC_REG(EMC_RC) = params->EmcRc;
    MAKE_EMC_REG(EMC_RFC) = params->EmcRfc;
    MAKE_EMC_REG(EMC_RFCPB) = params->EmcRfcPb;
    MAKE_EMC_REG(EMC_REFCTRL2) = params->EmcRefctrl2;
    MAKE_EMC_REG(EMC_RFC_SLR) = params->EmcRfcSlr;
    MAKE_EMC_REG(EMC_RAS) = params->EmcRas;
    MAKE_EMC_REG(EMC_RP) = params->EmcRp;
    MAKE_EMC_REG(EMC_TPPD) = params->EmcTppd;
    MAKE_EMC_REG(EMC_R2R) = params->EmcR2r;
    MAKE_EMC_REG(EMC_W2W) = params->EmcW2w;
    MAKE_EMC_REG(EMC_R2W) = params->EmcR2w;
    MAKE_EMC_REG(EMC_W2R) = params->EmcW2r;
    MAKE_EMC_REG(EMC_R2P) = params->EmcR2p;
    MAKE_EMC_REG(EMC_W2P) = params->EmcW2p;
    MAKE_EMC_REG(EMC_CCDMW) = params->EmcCcdmw;
    MAKE_EMC_REG(EMC_RD_RCD) = params->EmcRdRcd;
    MAKE_EMC_REG(EMC_WR_RCD) = params->EmcWrRcd;
    MAKE_EMC_REG(EMC_RRD) = params->EmcRrd;
    MAKE_EMC_REG(EMC_REXT) = params->EmcRext;
    MAKE_EMC_REG(EMC_WEXT) = params->EmcWext;
    MAKE_EMC_REG(EMC_WDV) = params->EmcWdv;
    MAKE_EMC_REG(EMC_WDV_CHK) = params->EmcWdvChk;
    MAKE_EMC_REG(EMC_WSV) = params->EmcWsv;
    MAKE_EMC_REG(EMC_WEV) = params->EmcWev;
    MAKE_EMC_REG(EMC_WDV_MASK) = params->EmcWdvMask;
    MAKE_EMC_REG(EMC_WS_DURATION) = params->EmcWsDuration;
    MAKE_EMC_REG(EMC_WE_DURATION) = params->EmcWeDuration;
    MAKE_EMC_REG(EMC_QUSE) = params->EmcQUse;
    MAKE_EMC_REG(EMC_QUSE_WIDTH) = params->EmcQuseWidth;
    MAKE_EMC_REG(EMC_IBDLY) = params->EmcIbdly;
    MAKE_EMC_REG(EMC_OBDLY) = params->EmcObdly;
    MAKE_EMC_REG(EMC_EINPUT) = params->EmcEInput;
    MAKE_EMC_REG(EMC_EINPUT_DURATION) = params->EmcEInputDuration;
    MAKE_EMC_REG(EMC_PUTERM_EXTRA) = params->EmcPutermExtra;
    MAKE_EMC_REG(EMC_PUTERM_WIDTH) = params->EmcPutermWidth;
    MAKE_EMC_REG(EMC_PMACRO_COMMON_PAD_TX_CTRL) = params->EmcPmacroCommonPadTxCtrl;
    MAKE_EMC_REG(EMC_DBG) = params->EmcDbg;
    MAKE_EMC_REG(EMC_QRST) = params->EmcQRst;
    MAKE_EMC_REG(EMC_ISSUE_QRST) = 0;
    MAKE_EMC_REG(EMC_QSAFE) = params->EmcQSafe;
    MAKE_EMC_REG(EMC_RDV) = params->EmcRdv;
    MAKE_EMC_REG(EMC_RDV_MASK) = params->EmcRdvMask;
    MAKE_EMC_REG(EMC_RDV_EARLY) = params->EmcRdvEarly;
    MAKE_EMC_REG(EMC_RDV_EARLY_MASK) = params->EmcRdvEarlyMask;
    MAKE_EMC_REG(EMC_QPOP) = params->EmcQpop;
    MAKE_EMC_REG(EMC_REFRESH) = params->EmcRefresh;
    MAKE_EMC_REG(EMC_BURST_REFRESH_NUM) = params->EmcBurstRefreshNum;
    MAKE_EMC_REG(EMC_PRE_REFRESH_REQ_CNT) = params->EmcPreRefreshReqCnt;
    MAKE_EMC_REG(EMC_PDEX2WR) = params->EmcPdEx2Wr;
    MAKE_EMC_REG(EMC_PDEX2RD) = params->EmcPdEx2Rd;
    MAKE_EMC_REG(EMC_PCHG2PDEN) = params->EmcPChg2Pden;
    MAKE_EMC_REG(EMC_ACT2PDEN) = params->EmcAct2Pden;
    MAKE_EMC_REG(EMC_AR2PDEN) = params->EmcAr2Pden;
    MAKE_EMC_REG(EMC_RW2PDEN) = params->EmcRw2Pden;
    MAKE_EMC_REG(EMC_CKE2PDEN) = params->EmcCke2Pden;
    MAKE_EMC_REG(EMC_PDEX2CKE) = params->EmcPdex2Cke;
    MAKE_EMC_REG(EMC_PDEX2MRR) = params->EmcPdex2Mrr;
    MAKE_EMC_REG(EMC_TXSR) = params->EmcTxsr;
    MAKE_EMC_REG(EMC_TXSRDLL) = params->EmcTxsrDll;
    MAKE_EMC_REG(EMC_TCKE) = params->EmcTcke;
    MAKE_EMC_REG(EMC_TCKESR) = params->EmcTckesr;
    MAKE_EMC_REG(EMC_TPD) = params->EmcTpd;
    MAKE_EMC_REG(EMC_TFAW) = params->EmcTfaw;
    MAKE_EMC_REG(EMC_TRPAB) = params->EmcTrpab;
    MAKE_EMC_REG(EMC_TCLKSTABLE) = params->EmcTClkStable;
    MAKE_EMC_REG(EMC_TCLKSTOP) = params->EmcTClkStop;
    MAKE_EMC_REG(EMC_TREFBW) = params->EmcTRefBw;
    MAKE_EMC_REG(EMC_ODT_WRITE) = params->EmcOdtWrite;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL) = params->EmcCfgDigDll;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL_PERIOD) = params->EmcCfgDigDllPeriod;
    MAKE_EMC_REG(EMC_FBIO_SPARE) = params->EmcFbioSpare & 0xFFFFFFFD;
    MAKE_EMC_REG(EMC_CFG_RSV) = params->EmcCfgRsv;
    MAKE_EMC_REG(EMC_PMC_SCRATCH1) = params->EmcPmcScratch1;
    MAKE_EMC_REG(EMC_PMC_SCRATCH2) = params->EmcPmcScratch2;
    MAKE_EMC_REG(EMC_PMC_SCRATCH3) = params->EmcPmcScratch3;
    MAKE_EMC_REG(EMC_ACPD_CONTROL) = params->EmcAcpdControl;
    MAKE_EMC_REG(EMC_TXDSRVTTGEN) = params->EmcTxdsrvttgen;
    MAKE_EMC_REG(EMC_CFG) = (params->EmcCfg & 0xE) | 0x3C00000;

    if (params->BootRomPatchControl & 0x80000000) {
        *(volatile uint32_t *)(4 * (params->BootRomPatchControl + 0x1C000000)) = params->BootRomPatchData;
        MAKE_MC_REG(MC_TIMING_CONTROL) = 1;
    }

    pmc->io_dpd3_req = (((4 * params->EmcPmcScratch1 >> 2) + 0x40000000) & 0xCFFF0000);
    udelay(params->PmcIoDpd3ReqWait);

    if (!params->EmcAutoCalInterval) {
        MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG) = (params->EmcAutoCalConfig | 0x200);
    }

    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU2) = params->EmcPmacroBrickCtrlRfu2;

    if (params->EmcZcalWarmColdBootEnables & 1) {
        if (params->MemoryType == NvBootMemoryType_Ddr3) {
            MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = (8 * params->EmcZcalWaitCnt);
        } else if (params->MemoryType == NvBootMemoryType_LpDdr4) {
            MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = params->EmcZcalWaitCnt;
            MAKE_EMC_REG(EMC_ZCAL_MRW_CMD) = params->EmcZcalMrwCmd;
        }
    }

    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    udelay(params->EmcTimingControlWait);

    pmc->ddr_cntrl &= 0xFFF8007F;
    udelay(params->PmcDdrCntrlWait);

    MAKE_EMC_REG(EMC_PIN) = (params->EmcPinGpioEn << 16) | (params->EmcPinGpio << 12);
    udelay(params->EmcPinExtraWait + 200);
    MAKE_EMC_REG(EMC_PIN) = ((params->EmcPinGpioEn << 16) | (params->EmcPinGpio << 12)) + 256;

    if (params->MemoryType == NvBootMemoryType_Ddr3) {
        udelay(params->EmcPinExtraWait + 500);
    } else if (params->MemoryType == NvBootMemoryType_LpDdr4) {
        udelay(params->EmcPinExtraWait + 2000);
    }

    MAKE_EMC_REG(EMC_PIN) = (((params->EmcPinGpioEn << 16) | (params->EmcPinGpio << 12)) + 0x101);
    udelay(params->EmcPinProgramWait);

    if (params->MemoryType != NvBootMemoryType_LpDdr4) {
        MAKE_EMC_REG(EMC_NOP) = (params->EmcDevSelect << 30) + 1;
        if (params->MemoryType == NvBootMemoryType_LpDdr2) {
            udelay(params->EmcPinExtraWait + 200);
        }
    } else {
        if (params->EmcBctSpare10) {
            *(volatile uint32_t *)params->EmcBctSpare10 = params->EmcBctSpare11;
        }

        MAKE_EMC_REG(EMC_MRW2) = params->EmcMrw2;
        MAKE_EMC_REG(EMC_MRW) = params->EmcMrw1;
        MAKE_EMC_REG(EMC_MRW3) = params->EmcMrw3;
        MAKE_EMC_REG(EMC_MRW4) = params->EmcMrw4;
        MAKE_EMC_REG(EMC_MRW6) = params->EmcMrw6;
        MAKE_EMC_REG(EMC_MRW14) = params->EmcMrw14;
        MAKE_EMC_REG(EMC_MRW8) = params->EmcMrw8;
        MAKE_EMC_REG(EMC_MRW12) = params->EmcMrw12;
        MAKE_EMC_REG(EMC_MRW9) = params->EmcMrw9;
        MAKE_EMC_REG(EMC_MRW13) = params->EmcMrw13;

        if (params->EmcZcalWarmColdBootEnables & 1) {
            MAKE_EMC_REG(EMC_ZQ_CAL) = params->EmcZcalInitDev0;
            udelay(params->EmcZcalInitWait);
            MAKE_EMC_REG(EMC_ZQ_CAL) = (params->EmcZcalInitDev0 ^ 3);

            if (!(params->EmcDevSelect & 2)) {
                MAKE_EMC_REG(EMC_ZQ_CAL) = params->EmcZcalInitDev1;
                udelay(params->EmcZcalInitWait);
                MAKE_EMC_REG(EMC_ZQ_CAL) = (params->EmcZcalInitDev1 ^ 3);
            }
        }
    }

    pmc->ddr_cfg = params->PmcDdrCfg;
    if ((params->MemoryType == NvBootMemoryType_LpDdr2)
        || (params->MemoryType == NvBootMemoryType_Ddr3)
        || (params->MemoryType == NvBootMemoryType_LpDdr4)) {
        MAKE_EMC_REG(EMC_ZCAL_INTERVAL) = params->EmcZcalInterval;
        MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = params->EmcZcalWaitCnt;
        MAKE_EMC_REG(EMC_ZCAL_MRW_CMD) = params->EmcZcalMrwCmd;
    }

    if (params->EmcBctSpare12) {
        *(volatile uint32_t *)params->EmcBctSpare12 = params->EmcBctSpare13;
    }

    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;

    if (params->EmcExtraRefreshNum) {
        MAKE_EMC_REG(EMC_REF) = (((1 << params->EmcExtraRefreshNum << 8) - 0xFD) | (params->EmcPinGpio << 30));
    }

    MAKE_EMC_REG(EMC_REFCTRL) = (params->EmcDevSelect | 0x80000000);
    MAKE_EMC_REG(EMC_DYN_SELF_REF_CONTROL) = params->EmcDynSelfRefControl;
    MAKE_EMC_REG(EMC_CFG_UPDATE) = params->EmcCfgUpdate;
    MAKE_EMC_REG(EMC_CFG) = params->EmcCfg;
    MAKE_EMC_REG(EMC_FDPD_CTRL_DQ) = params->EmcFdpdCtrlDq;
    MAKE_EMC_REG(EMC_FDPD_CTRL_CMD) = params->EmcFdpdCtrlCmd;
    MAKE_EMC_REG(EMC_SEL_DPD_CTRL) = params->EmcSelDpdCtrl;
    MAKE_EMC_REG(EMC_FBIO_SPARE) = (params->EmcFbioSpare | 2);
    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    MAKE_EMC_REG(EMC_CFG_PIPE_CLK) = params->EmcCfgPipeClk;
    MAKE_EMC_REG(EMC_FDPD_CTRL_CMD_NO_RAMP) = params->EmcFdpdCtrlCmdNoRamp;

    AHB_ARBITRATION_XBAR_CTRL_0 = ((AHB_ARBITRATION_XBAR_CTRL_0 & 0xFFFEFFFF) | ((params->AhbArbitrationXbarCtrlMemInitDone & 0xFFFF) << 16));

    MAKE_MC_REG(MC_VIDEO_PROTECT_REG_CTRL) = params->McVideoProtectWriteAccess;
    MAKE_MC_REG(MC_SEC_CARVEOUT_REG_CTRL) = params->McSecCarveoutProtectWriteAccess;
    MAKE_MC_REG(MC_MTS_CARVEOUT_REG_CTRL) = params->McMtsCarveoutRegCtrl;
    MAKE_MC_REG(MC_EMEM_CFG_ACCESS_CTRL) = 1;
}

static void sdram_config_mariko(const sdram_params_mariko_t *params) {
    volatile tegra_car_t *car = car_get_regs();
    volatile tegra_pmc_t *pmc = pmc_get_regs();

    if (params->EmcBctSpare0) {
        *(volatile uint32_t *)params->EmcBctSpare0 = params->EmcBctSpare1;
    }

    if (params->ClkRstControllerPllmMisc2OverrideEnable) {
        car->pllm_misc2 = params->ClkRstControllerPllmMisc2Override;
    }

    pmc->weak_bias = ((~params->EmcPmcScratch1 & 0x1000) << 19) | ((~params->EmcPmcScratch1 & 0xFFF) << 18) | ((~params->EmcPmcScratch1 & 0x8000) << 15);
    pmc->io_dpd3_req = (~params->EmcPmcScratch1 & 0x9FFF) + 0x80000000;
    udelay(params->PmcIoDpd3ReqWait);
    pmc->io_dpd4_req = (~params->EmcPmcScratch2 & 0x3FFF0000) | 0x80000000;
    udelay(params->PmcIoDpd4ReqWait);
    pmc->io_dpd4_req = (~params->EmcPmcScratch2 & 0x1FFF) | 0x80000000;
    udelay(1);

    MAKE_EMC_REG(EMC_FBIO_CFG7) = params->EmcFbioCfg7;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_0) = params->EmcCmdMappingCmd0_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_1) = params->EmcCmdMappingCmd0_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD0_2) = params->EmcCmdMappingCmd0_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_0) = params->EmcCmdMappingCmd1_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_1) = params->EmcCmdMappingCmd1_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD1_2) = params->EmcCmdMappingCmd1_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_0) = params->EmcCmdMappingCmd2_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_1) = params->EmcCmdMappingCmd2_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD2_2) = params->EmcCmdMappingCmd2_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_0) = params->EmcCmdMappingCmd3_0;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_1) = params->EmcCmdMappingCmd3_1;
    MAKE_EMC_REG(EMC_CMD_MAPPING_CMD3_2) = params->EmcCmdMappingCmd3_2;
    MAKE_EMC_REG(EMC_CMD_MAPPING_BYTE) = params->EmcCmdMappingByte;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_0) = params->EmcPmacroBrickMapping0;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_1) = params->EmcPmacroBrickMapping1;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_MAPPING_2) = params->EmcPmacroBrickMapping2;
    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_0) = params->EmcPmacroVttgenCtrl0;
    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_1) = params->EmcPmacroVttgenCtrl1;
    MAKE_EMC_REG(EMC_PMACRO_VTTGEN_CTRL_2) = params->EmcPmacroVttgenCtrl2;
    MAKE_EMC_REG(EMC_PMACRO_BG_BIAS_CTRL_0) = params->EmcPmacroBgBiasCtrl0;

    if (params->EmcBctSpareSecure0) {
        *(volatile uint32_t *)params->EmcBctSpareSecure0 = params->EmcBctSpareSecure1;
    }
    if (params->EmcBctSpareSecure2) {
        *(volatile uint32_t *)params->EmcBctSpareSecure2 = params->EmcBctSpareSecure3;
    }
    if (params->EmcBctSpareSecure4) {
        *(volatile uint32_t *)params->EmcBctSpareSecure4 = params->EmcBctSpareSecure5;
    }

    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    udelay(params->PmcVddpSelWait + 2);
    car->clk_source_emc = params->EmcClockSource;
    car->clk_source_emc_dll = params->EmcClockSourceDll;
    MAKE_EMC_REG(EMC_DBG) = params->EmcDbg | (2 * params->EmcDbgWriteMux);

    if (params->EmcBctSpare2) {
        *(volatile uint32_t *)params->EmcBctSpare2 = params->EmcBctSpare3;
    }

    MAKE_EMC_REG(EMC_CONFIG_SAMPLE_DELAY) = params->EmcConfigSampleDelay;
    MAKE_EMC_REG(EMC_FBIO_CFG8) = params->EmcFbioCfg8;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE0) = params->EmcSwizzleRank0Byte0;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE1) = params->EmcSwizzleRank0Byte1;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE2) = params->EmcSwizzleRank0Byte2;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK0_BYTE3) = params->EmcSwizzleRank0Byte3;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE0) = params->EmcSwizzleRank1Byte0;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE1) = params->EmcSwizzleRank1Byte1;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE2) = params->EmcSwizzleRank1Byte2;
    MAKE_EMC_REG(EMC_SWIZZLE_RANK1_BYTE3) = params->EmcSwizzleRank1Byte3;

    if (params->EmcBctSpare6) {
        *(volatile uint32_t *)params->EmcBctSpare6 = params->EmcBctSpare7;
    }

    MAKE_EMC_REG(EMC_XM2COMPPADCTRL) = params->EmcXm2CompPadCtrl;
    MAKE_EMC_REG(EMC_XM2COMPPADCTRL2) = params->EmcXm2CompPadCtrl2;
    MAKE_EMC_REG(EMC_XM2COMPPADCTRL3) = params->EmcXm2CompPadCtrl3;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG2) = params->EmcAutoCalConfig2;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG3) = params->EmcAutoCalConfig3;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG4) = params->EmcAutoCalConfig4;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG5) = params->EmcAutoCalConfig5;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG6) = params->EmcAutoCalConfig6;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG7) = params->EmcAutoCalConfig7;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG8) = params->EmcAutoCalConfig8;
    MAKE_EMC_REG(EMC_PMACRO_RX_TERM) = params->EmcPmacroRxTerm;
    MAKE_EMC_REG(EMC_PMACRO_DQ_TX_DRV) = params->EmcPmacroDqTxDrv;
    MAKE_EMC_REG(EMC_PMACRO_CA_TX_DRV) = params->EmcPmacroCaTxDrv;
    MAKE_EMC_REG(EMC_PMACRO_CMD_TX_DRV) = params->EmcPmacroCmdTxDrv;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_COMMON) = params->EmcPmacroAutocalCfgCommon;
    MAKE_EMC_REG(EMC_AUTO_CAL_CHANNEL) = params->EmcAutoCalChannel;
    MAKE_EMC_REG(EMC_PMACRO_ZCTRL) = params->EmcPmacroZctrl;
    MAKE_EMC_REG(EMC_DLL_CFG_0) = params->EmcPmacroDllCfg0;
    MAKE_EMC_REG(EMC_DLL_CFG_1) = params->EmcPmacroDllCfg1;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL_1) = params->EmcCfgDigDll_1;
    MAKE_EMC_REG(EMC_DATA_BRLSHFT_0) = params->EmcDataBrlshft0;
    MAKE_EMC_REG(EMC_DATA_BRLSHFT_1) = params->EmcDataBrlshft1;
    MAKE_EMC_REG(EMC_DQS_BRLSHFT_0) = params->EmcDqsBrlshft0;
    MAKE_EMC_REG(EMC_DQS_BRLSHFT_1) = params->EmcDqsBrlshft1;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_0) = params->EmcCmdBrlshft0;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_1) = params->EmcCmdBrlshft1;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_2) = params->EmcCmdBrlshft2;
    MAKE_EMC_REG(EMC_CMD_BRLSHFT_3) = params->EmcCmdBrlshft3;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_0) = params->EmcQuseBrlshft0;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_1) = params->EmcQuseBrlshft1;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_2) = params->EmcQuseBrlshft2;
    MAKE_EMC_REG(EMC_QUSE_BRLSHFT_3) = params->EmcQuseBrlshft3;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU1) = params->EmcPmacroBrickCtrlRfu1;
    MAKE_EMC_REG(EMC_PMACRO_PAD_CFG_CTRL) = params->EmcPmacroPadCfgCtrl;
    MAKE_EMC_REG(EMC_PMACRO_CMD_BRICK_CTRL_FDPD) = params->EmcPmacroCmdBrickCtrlFdpd;
    MAKE_EMC_REG(EMC_PMACRO_BRICK_CTRL_RFU2) = params->EmcPmacroBrickCtrlRfu2;
    MAKE_EMC_REG(EMC_PMACRO_DATA_BRICK_CTRL_FDPD) = params->EmcPmacroDataBrickCtrlFdpd;
    MAKE_EMC_REG(EMC_PMACRO_DATA_PAD_RX_CTRL) = params->EmcPmacroDataPadRxCtrl;
    MAKE_EMC_REG(EMC_PMACRO_CMD_PAD_RX_CTRL) = params->EmcPmacroCmdPadRxCtrl;
    MAKE_EMC_REG(EMC_PMACRO_DATA_PAD_TX_CTRL) = params->EmcPmacroDataPadTxCtrl;
    MAKE_EMC_REG(EMC_PMACRO_DATA_RX_TERM_MODE) = params->EmcPmacroDataRxTermMode;
    MAKE_EMC_REG(EMC_PMACRO_CMD_RX_TERM_MODE) = params->EmcPmacroCmdRxTermMode;
    MAKE_EMC_REG(EMC_PMACRO_CMD_PAD_TX_CTRL) = params->EmcPmacroCmdPadTxCtrl & 0xEFFFFFFF;
    MAKE_EMC_REG(EMC_CFG_3) = params->EmcCfg3;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_0) = params->EmcPmacroTxPwrd0;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_1) = params->EmcPmacroTxPwrd1;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_2) = params->EmcPmacroTxPwrd2;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_3) = params->EmcPmacroTxPwrd3;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_4) = params->EmcPmacroTxPwrd4;
    MAKE_EMC_REG(EMC_PMACRO_TX_PWRD_5) = params->EmcPmacroTxPwrd5;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_0) = params->EmcPmacroTxSelClkSrc0;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_1) = params->EmcPmacroTxSelClkSrc1;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_2) = params->EmcPmacroTxSelClkSrc2;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_3) = params->EmcPmacroTxSelClkSrc3;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_4) = params->EmcPmacroTxSelClkSrc4;
    MAKE_EMC_REG(EMC_PMACRO_TX_SEL_CLK_SRC_5) = params->EmcPmacroTxSelClkSrc5;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_FGCG_CTRL_0) = params->EmcPmacroPerbitFgcgCtrl0;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_FGCG_CTRL_1) = params->EmcPmacroPerbitFgcgCtrl1;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_FGCG_CTRL_2) = params->EmcPmacroPerbitFgcgCtrl2;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_FGCG_CTRL_3) = params->EmcPmacroPerbitFgcgCtrl3;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_FGCG_CTRL_4) = params->EmcPmacroPerbitFgcgCtrl4;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_FGCG_CTRL_5) = params->EmcPmacroPerbitFgcgCtrl5;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU_CTRL_0) = params->EmcPmacroPerbitRfuCtrl0;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU_CTRL_1) = params->EmcPmacroPerbitRfuCtrl1;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU_CTRL_2) = params->EmcPmacroPerbitRfuCtrl2;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU_CTRL_3) = params->EmcPmacroPerbitRfuCtrl3;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU_CTRL_4) = params->EmcPmacroPerbitRfuCtrl4;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU_CTRL_5) = params->EmcPmacroPerbitRfuCtrl5;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU1_CTRL_0) = params->EmcPmacroPerbitRfu1Ctrl0;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU1_CTRL_1) = params->EmcPmacroPerbitRfu1Ctrl1;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU1_CTRL_2) = params->EmcPmacroPerbitRfu1Ctrl2;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU1_CTRL_3) = params->EmcPmacroPerbitRfu1Ctrl3;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU1_CTRL_4) = params->EmcPmacroPerbitRfu1Ctrl4;
    MAKE_EMC_REG(EMC_PMACRO_PERBIT_RFU1_CTRL_5) = params->EmcPmacroPerbitRfu1Ctrl5;
    MAKE_EMC_REG(EMC_PMACRO_DATA_PI_CTRL) = params->EmcPmacroDataPiCtrl;
    MAKE_EMC_REG(EMC_PMACRO_CMD_PI_CTRL) = params->EmcPmacroCmdPiCtrl;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_BYPASS) = params->EmcPmacroDdllBypass;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_0) = params->EmcPmacroDdllPwrd0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_1) = params->EmcPmacroDdllPwrd1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PWRD_2) = params->EmcPmacroDdllPwrd2;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_0) = params->EmcPmacroCmdCtrl0;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_1) = params->EmcPmacroCmdCtrl1;
    MAKE_EMC_REG(EMC_PMACRO_CMD_CTRL_2) = params->EmcPmacroCmdCtrl2;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQ_0) = params->EmcPmacroIbVrefDq_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQ_1) = params->EmcPmacroIbVrefDq_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQS_0) = params->EmcPmacroIbVrefDqs_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_VREF_DQS_1) = params->EmcPmacroIbVrefDqs_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_RXRT) = params->EmcPmacroIbRxrt;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_0) = params->EmcPmacroQuseDdllRank0_0;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_1) = params->EmcPmacroQuseDdllRank0_1;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_2) = params->EmcPmacroQuseDdllRank0_2;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_3) = params->EmcPmacroQuseDdllRank0_3;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_4) = params->EmcPmacroQuseDdllRank0_4;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK0_5) = params->EmcPmacroQuseDdllRank0_5;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_0) = params->EmcPmacroQuseDdllRank1_0;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_1) = params->EmcPmacroQuseDdllRank1_1;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_2) = params->EmcPmacroQuseDdllRank1_2;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_3) = params->EmcPmacroQuseDdllRank1_3;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_4) = params->EmcPmacroQuseDdllRank1_4;
    MAKE_EMC_REG(EMC_PMACRO_QUSE_DDLL_RANK1_5) = params->EmcPmacroQuseDdllRank1_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0) = params->EmcPmacroObDdllLongDqRank0_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1) = params->EmcPmacroObDdllLongDqRank0_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2) = params->EmcPmacroObDdllLongDqRank0_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3) = params->EmcPmacroObDdllLongDqRank0_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4) = params->EmcPmacroObDdllLongDqRank0_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5) = params->EmcPmacroObDdllLongDqRank0_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0) = params->EmcPmacroObDdllLongDqRank1_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1) = params->EmcPmacroObDdllLongDqRank1_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2) = params->EmcPmacroObDdllLongDqRank1_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3) = params->EmcPmacroObDdllLongDqRank1_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_4) = params->EmcPmacroObDdllLongDqRank1_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_5) = params->EmcPmacroObDdllLongDqRank1_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_0) = params->EmcPmacroObDdllLongDqsRank0_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_1) = params->EmcPmacroObDdllLongDqsRank0_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_2) = params->EmcPmacroObDdllLongDqsRank0_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_3) = params->EmcPmacroObDdllLongDqsRank0_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_4) = params->EmcPmacroObDdllLongDqsRank0_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_5) = params->EmcPmacroObDdllLongDqsRank0_5;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_0) = params->EmcPmacroObDdllLongDqsRank1_0;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_1) = params->EmcPmacroObDdllLongDqsRank1_1;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_2) = params->EmcPmacroObDdllLongDqsRank1_2;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_3) = params->EmcPmacroObDdllLongDqsRank1_3;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_4) = params->EmcPmacroObDdllLongDqsRank1_4;
    MAKE_EMC_REG(EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_5) = params->EmcPmacroObDdllLongDqsRank1_5;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0) = params->EmcPmacroIbDdllLongDqsRank0_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1) = params->EmcPmacroIbDdllLongDqsRank0_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2) = params->EmcPmacroIbDdllLongDqsRank0_2;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3) = params->EmcPmacroIbDdllLongDqsRank0_3;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0) = params->EmcPmacroIbDdllLongDqsRank1_0;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1) = params->EmcPmacroIbDdllLongDqsRank1_1;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2) = params->EmcPmacroIbDdllLongDqsRank1_2;
    MAKE_EMC_REG(EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3) = params->EmcPmacroIbDdllLongDqsRank1_3;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_0) = params->EmcPmacroDdllLongCmd_0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_1) = params->EmcPmacroDdllLongCmd_1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_2) = params->EmcPmacroDdllLongCmd_2;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_3) = params->EmcPmacroDdllLongCmd_3;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_LONG_CMD_4) = params->EmcPmacroDdllLongCmd_4;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_0) = params->EmcPmacroDdllShortCmd_0;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_1) = params->EmcPmacroDdllShortCmd_1;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_SHORT_CMD_2) = params->EmcPmacroDdllShortCmd_2;
    MAKE_EMC_REG(EMC_PMACRO_DDLL_PERIODIC_OFFSET) = params->EmcPmacroDdllPeriodicOffset;

    if (params->EmcBctSpare4) {
        *(volatile uint32_t *)params->EmcBctSpare4 = params->EmcBctSpare5;
    }
    if (params->EmcBctSpareSecure6) {
        *(volatile uint32_t *)params->EmcBctSpareSecure6 = params->EmcBctSpareSecure7;
    }
    if (params->EmcBctSpareSecure8) {
        *(volatile uint32_t *)params->EmcBctSpareSecure8 = params->EmcBctSpareSecure9;
    }
    if (params->EmcBctSpareSecure10) {
        *(volatile uint32_t *)params->EmcBctSpareSecure10 = params->EmcBctSpareSecure11;
    }

    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;

    MAKE_MC_REG(MC_VIDEO_PROTECT_BOM) = params->McVideoProtectBom;
    MAKE_MC_REG(MC_VIDEO_PROTECT_BOM_ADR_HI) = params->McVideoProtectBomAdrHi;
    MAKE_MC_REG(MC_VIDEO_PROTECT_SIZE_MB) = params->McVideoProtectSizeMb;
    MAKE_MC_REG(MC_VIDEO_PROTECT_VPR_OVERRIDE) = params->McVideoProtectVprOverride;
    MAKE_MC_REG(MC_VIDEO_PROTECT_VPR_OVERRIDE1) = params->McVideoProtectVprOverride1;
    MAKE_MC_REG(MC_VIDEO_PROTECT_GPU_OVERRIDE_0) = params->McVideoProtectGpuOverride0;
    MAKE_MC_REG(MC_VIDEO_PROTECT_GPU_OVERRIDE_1) = params->McVideoProtectGpuOverride1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG) = params->McEmemAdrCfg;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_DEV0) = params->McEmemAdrCfgDev0;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_DEV1) = params->McEmemAdrCfgDev1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_CHANNEL_MASK) = params->McEmemAdrCfgChannelMask;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_0) = params->McEmemAdrCfgBankMask0;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_1) = params->McEmemAdrCfgBankMask1;
    MAKE_MC_REG(MC_EMEM_ADR_CFG_BANK_MASK_2) = params->McEmemAdrCfgBankMask2;
    MAKE_MC_REG(MC_EMEM_CFG) = params->McEmemCfg;
    MAKE_MC_REG(MC_SEC_CARVEOUT_BOM) = params->McSecCarveoutBom;
    MAKE_MC_REG(MC_SEC_CARVEOUT_ADR_HI) = params->McSecCarveoutAdrHi;
    MAKE_MC_REG(MC_SEC_CARVEOUT_SIZE_MB) = params->McSecCarveoutSizeMb;
    MAKE_MC_REG(MC_MTS_CARVEOUT_BOM) = params->McMtsCarveoutBom;
    MAKE_MC_REG(MC_MTS_CARVEOUT_ADR_HI) = params->McMtsCarveoutAdrHi;
    MAKE_MC_REG(MC_MTS_CARVEOUT_SIZE_MB) = params->McMtsCarveoutSizeMb;
    MAKE_MC_REG(MC_EMEM_ARB_CFG) = params->McEmemArbCfg;
    MAKE_MC_REG(MC_EMEM_ARB_OUTSTANDING_REQ) = params->McEmemArbOutstandingReq;
    MAKE_MC_REG(MC_EMEM_ARB_REFPB_HP_CTRL) = params->McEmemArbRefpbHpCtrl;
    MAKE_MC_REG(MC_EMEM_ARB_REFPB_BANK_CTRL) = params->McEmemArbRefpbBankCtrl;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RCD) = params->McEmemArbTimingRcd;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RP) = params->McEmemArbTimingRp;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RC) = params->McEmemArbTimingRc;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RAS) = params->McEmemArbTimingRas;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_FAW) = params->McEmemArbTimingFaw;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RRD) = params->McEmemArbTimingRrd;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RAP2PRE) = params->McEmemArbTimingRap2Pre;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_WAP2PRE) = params->McEmemArbTimingWap2Pre;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_R2R) = params->McEmemArbTimingR2R;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_W2W) = params->McEmemArbTimingW2W;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_CCDMW) = params->McEmemArbTimingCcdmw;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_R2W) = params->McEmemArbTimingR2W;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_W2R) = params->McEmemArbTimingW2R;
    MAKE_MC_REG(MC_EMEM_ARB_TIMING_RFCPB) = params->McEmemArbTimingRFCPB;
    MAKE_MC_REG(MC_EMEM_ARB_DA_TURNS) = params->McEmemArbDaTurns;
    MAKE_MC_REG(MC_EMEM_ARB_DA_COVERS) = params->McEmemArbDaCovers;
    MAKE_MC_REG(MC_EMEM_ARB_MISC0) = params->McEmemArbMisc0;
    MAKE_MC_REG(MC_EMEM_ARB_MISC1) = params->McEmemArbMisc1;
    MAKE_MC_REG(MC_EMEM_ARB_MISC2) = params->McEmemArbMisc2;
    MAKE_MC_REG(MC_EMEM_ARB_RING1_THROTTLE) = params->McEmemArbRing1Throttle;
    MAKE_MC_REG(MC_EMEM_ARB_OVERRIDE) = params->McEmemArbOverride;
    MAKE_MC_REG(MC_EMEM_ARB_OVERRIDE_1) = params->McEmemArbOverride1;
    MAKE_MC_REG(MC_EMEM_ARB_RSV) = params->McEmemArbRsv;
    MAKE_MC_REG(MC_DA_CONFIG0) = params->McDaCfg0;
    MAKE_MC_REG(MC_TIMING_CONTROL) = 1;
    MAKE_MC_REG(MC_CLKEN_OVERRIDE) = params->McClkenOverride;
    MAKE_MC_REG(MC_STAT_CONTROL) = params->McStatControl;

    MAKE_EMC_REG(EMC_ADR_CFG) = params->EmcAdrCfg;
    MAKE_EMC_REG(EMC_CLKEN_OVERRIDE) = params->EmcClkenOverride;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_0) = params->EmcPmacroAutocalCfg0;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_1) = params->EmcPmacroAutocalCfg1;
    MAKE_EMC_REG(EMC_PMACRO_AUTOCAL_CFG_2) = params->EmcPmacroAutocalCfg2;
    MAKE_EMC_REG(EMC_AUTO_CAL_VREF_SEL_0) = params->EmcAutoCalVrefSel0;
    MAKE_EMC_REG(EMC_AUTO_CAL_VREF_SEL_1) = params->EmcAutoCalVrefSel1;
    MAKE_EMC_REG(EMC_AUTO_CAL_INTERVAL) = params->EmcAutoCalInterval;
    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG) = params->EmcAutoCalConfig;
    udelay(params->EmcAutoCalWait);

    if (params->EmcBctSpare8) {
        *(volatile uint32_t *)params->EmcBctSpare8 = params->EmcBctSpare9;
    }

    MAKE_EMC_REG(EMC_AUTO_CAL_CONFIG9) = params->EmcAutoCalConfig9;
    MAKE_EMC_REG(EMC_CFG_2) = params->EmcCfg2;
    MAKE_EMC_REG(EMC_CFG_PIPE) = params->EmcCfgPipe;
    MAKE_EMC_REG(EMC_CFG_PIPE_1) = params->EmcCfgPipe1;
    MAKE_EMC_REG(EMC_CFG_PIPE_2) = params->EmcCfgPipe2;
    MAKE_EMC_REG(EMC_CMDQ) = params->EmcCmdQ;
    MAKE_EMC_REG(EMC_MC2EMCQ) = params->EmcMc2EmcQ;
    MAKE_EMC_REG(EMC_MRS_WAIT_CNT) = params->EmcMrsWaitCnt;
    MAKE_EMC_REG(EMC_MRS_WAIT_CNT2) = params->EmcMrsWaitCnt2;
    MAKE_EMC_REG(EMC_FBIO_CFG5) = params->EmcFbioCfg5;
    MAKE_EMC_REG(EMC_RC) = params->EmcRc;
    MAKE_EMC_REG(EMC_RFC) = params->EmcRfc;
    MAKE_EMC_REG(EMC_RFCPB) = params->EmcRfcPb;
    MAKE_EMC_REG(EMC_REFCTRL2) = params->EmcRefctrl2;
    MAKE_EMC_REG(EMC_RFC_SLR) = params->EmcRfcSlr;
    MAKE_EMC_REG(EMC_RAS) = params->EmcRas;
    MAKE_EMC_REG(EMC_RP) = params->EmcRp;
    MAKE_EMC_REG(EMC_TPPD) = params->EmcTppd;
    MAKE_EMC_REG(EMC_TRTM) = params->EmcTrtm;
    MAKE_EMC_REG(EMC_TWTM) = params->EmcTwtm;
    MAKE_EMC_REG(EMC_TRATM) = params->EmcTratm;
    MAKE_EMC_REG(EMC_TWATM) = params->EmcTwatm;
    MAKE_EMC_REG(EMC_TR2REF) = params->EmcTr2ref;
    MAKE_EMC_REG(EMC_R2R) = params->EmcR2r;
    MAKE_EMC_REG(EMC_W2W) = params->EmcW2w;
    MAKE_EMC_REG(EMC_R2W) = params->EmcR2w;
    MAKE_EMC_REG(EMC_W2R) = params->EmcW2r;
    MAKE_EMC_REG(EMC_R2P) = params->EmcR2p;
    MAKE_EMC_REG(EMC_W2P) = params->EmcW2p;
    MAKE_EMC_REG(EMC_CCDMW) = params->EmcCcdmw;
    MAKE_EMC_REG(EMC_RD_RCD) = params->EmcRdRcd;
    MAKE_EMC_REG(EMC_WR_RCD) = params->EmcWrRcd;
    MAKE_EMC_REG(EMC_RRD) = params->EmcRrd;
    MAKE_EMC_REG(EMC_REXT) = params->EmcRext;
    MAKE_EMC_REG(EMC_WEXT) = params->EmcWext;
    MAKE_EMC_REG(EMC_WDV) = params->EmcWdv;
    MAKE_EMC_REG(EMC_WDV_CHK) = params->EmcWdvChk;
    MAKE_EMC_REG(EMC_WSV) = params->EmcWsv;
    MAKE_EMC_REG(EMC_WEV) = params->EmcWev;
    MAKE_EMC_REG(EMC_WDV_MASK) = params->EmcWdvMask;
    MAKE_EMC_REG(EMC_WS_DURATION) = params->EmcWsDuration;
    MAKE_EMC_REG(EMC_WE_DURATION) = params->EmcWeDuration;
    MAKE_EMC_REG(EMC_QUSE) = params->EmcQUse;
    MAKE_EMC_REG(EMC_QUSE_WIDTH) = params->EmcQuseWidth;
    MAKE_EMC_REG(EMC_IBDLY) = params->EmcIbdly;
    MAKE_EMC_REG(EMC_OBDLY) = params->EmcObdly;
    MAKE_EMC_REG(EMC_EINPUT) = params->EmcEInput;
    MAKE_EMC_REG(EMC_EINPUT_DURATION) = params->EmcEInputDuration;
    MAKE_EMC_REG(EMC_PUTERM_EXTRA) = params->EmcPutermExtra;
    MAKE_EMC_REG(EMC_PUTERM_WIDTH) = params->EmcPutermWidth;
    MAKE_EMC_REG(EMC_DBG) = params->EmcDbg;
    MAKE_EMC_REG(EMC_QRST) = params->EmcQRst;
    MAKE_EMC_REG(EMC_ISSUE_QRST) = 0;
    MAKE_EMC_REG(EMC_QSAFE) = params->EmcQSafe;
    MAKE_EMC_REG(EMC_RDV) = params->EmcRdv;
    MAKE_EMC_REG(EMC_RDV_MASK) = params->EmcRdvMask;
    MAKE_EMC_REG(EMC_RDV_EARLY) = params->EmcRdvEarly;
    MAKE_EMC_REG(EMC_RDV_EARLY_MASK) = params->EmcRdvEarlyMask;
    MAKE_EMC_REG(EMC_QPOP) = params->EmcQpop;
    MAKE_EMC_REG(EMC_REFRESH) = params->EmcRefresh;
    MAKE_EMC_REG(EMC_BURST_REFRESH_NUM) = params->EmcBurstRefreshNum;
    MAKE_EMC_REG(EMC_PRE_REFRESH_REQ_CNT) = params->EmcPreRefreshReqCnt;
    MAKE_EMC_REG(EMC_PDEX2WR) = params->EmcPdEx2Wr;
    MAKE_EMC_REG(EMC_PDEX2RD) = params->EmcPdEx2Rd;
    MAKE_EMC_REG(EMC_PCHG2PDEN) = params->EmcPChg2Pden;
    MAKE_EMC_REG(EMC_ACT2PDEN) = params->EmcAct2Pden;
    MAKE_EMC_REG(EMC_AR2PDEN) = params->EmcAr2Pden;
    MAKE_EMC_REG(EMC_RW2PDEN) = params->EmcRw2Pden;
    MAKE_EMC_REG(EMC_CKE2PDEN) = params->EmcCke2Pden;
    MAKE_EMC_REG(EMC_PDEX2CKE) = params->EmcPdex2Cke;
    MAKE_EMC_REG(EMC_PDEX2MRR) = params->EmcPdex2Mrr;
    MAKE_EMC_REG(EMC_TXSR) = params->EmcTxsr;
    MAKE_EMC_REG(EMC_TXSRDLL) = params->EmcTxsrDll;
    MAKE_EMC_REG(EMC_TCKE) = params->EmcTcke;
    MAKE_EMC_REG(EMC_TCKESR) = params->EmcTckesr;
    MAKE_EMC_REG(EMC_TPD) = params->EmcTpd;
    MAKE_EMC_REG(EMC_TFAW) = params->EmcTfaw;
    MAKE_EMC_REG(EMC_TRPAB) = params->EmcTrpab;
    MAKE_EMC_REG(EMC_TCLKSTABLE) = params->EmcTClkStable;
    MAKE_EMC_REG(EMC_TCLKSTOP) = params->EmcTClkStop;
    MAKE_EMC_REG(EMC_TREFBW) = params->EmcTRefBw;
    MAKE_EMC_REG(EMC_ODT_WRITE) = params->EmcOdtWrite;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL) = params->EmcCfgDigDll;
    MAKE_EMC_REG(EMC_CFG_DIG_DLL_PERIOD) = params->EmcCfgDigDllPeriod;
    MAKE_EMC_REG(EMC_FBIO_SPARE) = params->EmcFbioSpare & 0xFFFFFFFD;
    MAKE_EMC_REG(EMC_CFG_RSV) = params->EmcCfgRsv;
    MAKE_EMC_REG(EMC_PMC_SCRATCH1) = params->EmcPmcScratch1;
    MAKE_EMC_REG(EMC_PMC_SCRATCH2) = params->EmcPmcScratch2;
    MAKE_EMC_REG(EMC_PMC_SCRATCH3) = params->EmcPmcScratch3;
    MAKE_EMC_REG(EMC_ACPD_CONTROL) = params->EmcAcpdControl;
    MAKE_EMC_REG(EMC_TXDSRVTTGEN) = params->EmcTxdsrvttgen;
    MAKE_EMC_REG(EMC_PMACRO_DSR_VTTGEN_CTRL_0) = params->EmcPmacroDsrVttgenCtrl0;
    MAKE_EMC_REG(EMC_CFG) = ((((((params->EmcCfg & 4) | 0x3C00000) & 0xFFFFFFF7) | (params->EmcCfg & 8)) & 0xFFFFFFFD) | (params->EmcCfg & 2));

    if (params->BootRomPatchControl) {
        *(volatile uint32_t *)params->BootRomPatchControl = params->BootRomPatchData;
        MAKE_MC_REG(MC_TIMING_CONTROL) = 1;
    }

    if (params->EmcBctSpareSecure12) {
        *(volatile uint32_t *)params->EmcBctSpareSecure12 = params->EmcBctSpareSecure13;
    }
    if (params->EmcBctSpareSecure14) {
        *(volatile uint32_t *)params->EmcBctSpareSecure14 = params->EmcBctSpareSecure15;
    }
    if (params->EmcBctSpareSecure16) {
        *(volatile uint32_t *)params->EmcBctSpareSecure16 = params->EmcBctSpareSecure17;
    }

    pmc->io_dpd3_req = ((4 * params->EmcPmcScratch1 >> 2) + 0x40000000) & 0xCFFF0000;
    udelay(params->PmcIoDpd3ReqWait);
    MAKE_EMC_REG(EMC_PMACRO_CMD_PAD_TX_CTRL) = params->EmcPmacroCmdPadTxCtrl;

    if (params->EmcZcalWarmColdBootEnables & 1) {
        if (params->MemoryType == NvBootMemoryType_Ddr3) {
            MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = 8 * params->EmcZcalWaitCnt;
        } else if (params->MemoryType == NvBootMemoryType_LpDdr4) {
            MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = params->EmcZcalWaitCnt;
            MAKE_EMC_REG(EMC_ZCAL_MRW_CMD) = params->EmcZcalMrwCmd;
        }
    }

    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    udelay(params->EmcTimingControlWait);

    pmc->ddr_cntrl &= 0xFF78007F;
    udelay(params->PmcDdrCntrlWait);

    MAKE_EMC_REG(EMC_PIN) = (params->EmcPinGpioEn << 16) | (params->EmcPinGpio << 12);
    udelay(params->EmcPinExtraWait + 200);
    MAKE_EMC_REG(EMC_PIN) = ((params->EmcPinGpioEn << 16) | (params->EmcPinGpio << 12)) + 256;

    if (params->MemoryType == NvBootMemoryType_Ddr3) {
        udelay(params->EmcPinExtraWait + 500);
    } else if (params->MemoryType == NvBootMemoryType_LpDdr4) {
        udelay(params->EmcPinExtraWait + 2000);
    }

    MAKE_EMC_REG(EMC_PIN) = ((params->EmcPinGpioEn << 16) | (params->EmcPinGpio << 12)) + 257;
    udelay(params->EmcPinProgramWait);

    if (params->MemoryType != NvBootMemoryType_LpDdr4) {
        MAKE_EMC_REG(EMC_NOP) = (params->EmcDevSelect << 30) + 1;
        if (params->MemoryType == NvBootMemoryType_LpDdr2) {
            udelay(params->EmcPinExtraWait + 200);
        }
    } else {
        if (params->EmcBctSpare10) {
            *(volatile uint32_t *)params->EmcBctSpare10 = params->EmcBctSpare11;
        }

        MAKE_EMC_REG(EMC_MRW2) = params->EmcMrw2;
        MAKE_EMC_REG(EMC_MRW) = params->EmcMrw1;
        MAKE_EMC_REG(EMC_MRW3) = params->EmcMrw3;
        MAKE_EMC_REG(EMC_MRW4) = params->EmcMrw4;
        MAKE_EMC_REG(EMC_MRW6) = params->EmcMrw6;
        MAKE_EMC_REG(EMC_MRW14) = params->EmcMrw14;
        MAKE_EMC_REG(EMC_MRW8) = params->EmcMrw8;
        MAKE_EMC_REG(EMC_MRW12) = params->EmcMrw12;
        MAKE_EMC_REG(EMC_MRW9) = params->EmcMrw9;
        MAKE_EMC_REG(EMC_MRW13) = params->EmcMrw13;

        if (params->EmcZcalWarmColdBootEnables & 1) {
            MAKE_EMC_REG(EMC_ZQ_CAL) = params->EmcZcalInitDev0;
            udelay(params->EmcZcalInitWait);
            MAKE_EMC_REG(EMC_ZQ_CAL) = params->EmcZcalInitDev0 ^ 3;

            if (!(params->EmcDevSelect & 2)) {
                MAKE_EMC_REG(EMC_ZQ_CAL) = params->EmcZcalInitDev1;
                udelay(params->EmcZcalInitWait);
                MAKE_EMC_REG(EMC_ZQ_CAL) = params->EmcZcalInitDev1 ^ 3;
            }
        }
    }

    if (params->EmcBctSpareSecure18) {
        *(volatile uint32_t *)params->EmcBctSpareSecure18 = params->EmcBctSpareSecure19;
    }
    if (params->EmcBctSpareSecure20) {
        *(volatile uint32_t *)params->EmcBctSpareSecure20 = params->EmcBctSpareSecure21;
    }
    if (params->EmcBctSpareSecure22) {
        *(volatile uint32_t *)params->EmcBctSpareSecure22 = params->EmcBctSpareSecure23;
    }

    pmc->ddr_cfg = params->PmcDdrCfg;
    if ((params->MemoryType == NvBootMemoryType_LpDdr2)
        || (params->MemoryType == NvBootMemoryType_Ddr3)
        || (params->MemoryType == NvBootMemoryType_LpDdr4)) {
        MAKE_EMC_REG(EMC_ZCAL_INTERVAL) = params->EmcZcalInterval;
        MAKE_EMC_REG(EMC_ZCAL_WAIT_CNT) = params->EmcZcalWaitCnt;
        MAKE_EMC_REG(EMC_ZCAL_MRW_CMD) = params->EmcZcalMrwCmd;
    }

    if (params->EmcBctSpare12) {
        *(volatile uint32_t *)params->EmcBctSpare12 = params->EmcBctSpare13;
    }

    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;

    if (params->EmcExtraRefreshNum) {
        MAKE_EMC_REG(EMC_REF) = ((1 << params->EmcExtraRefreshNum << 8) - 253) | (params->EmcDevSelect << 30);
    }

    MAKE_EMC_REG(EMC_REFCTRL) = params->EmcDevSelect | 0x80000000;
    MAKE_EMC_REG(EMC_DYN_SELF_REF_CONTROL) = params->EmcDynSelfRefControl;
    MAKE_EMC_REG(EMC_CFG) = params->EmcCfg;
    MAKE_EMC_REG(EMC_FDPD_CTRL_DQ) = params->EmcFdpdCtrlDq;
    MAKE_EMC_REG(EMC_FDPD_CTRL_CMD) = params->EmcFdpdCtrlCmd;
    MAKE_EMC_REG(EMC_SEL_DPD_CTRL) = params->EmcSelDpdCtrl;
    MAKE_EMC_REG(EMC_FBIO_SPARE) = params->EmcFbioSpare | 2;
    MAKE_EMC_REG(EMC_TIMING_CONTROL) = 1;
    MAKE_EMC_REG(EMC_CFG_UPDATE) = params->EmcCfgUpdate;
    MAKE_EMC_REG(EMC_CFG_PIPE_CLK) = params->EmcCfgPipeClk;
    MAKE_EMC_REG(EMC_FDPD_CTRL_CMD_NO_RAMP) = params->EmcFdpdCtrlCmdNoRamp;
    MAKE_MC_REG(MC_UNTRANSLATED_REGION_CHECK) = params->McUntranslatedRegionCheck;
    MAKE_MC_REG(MC_VIDEO_PROTECT_REG_CTRL) = params->McVideoProtectWriteAccess;
    MAKE_MC_REG(MC_SEC_CARVEOUT_REG_CTRL) = params->McSecCarveoutProtectWriteAccess;
    MAKE_MC_REG(MC_MTS_CARVEOUT_REG_CTRL) = params->McMtsCarveoutRegCtrl;
    MAKE_MC_REG(MC_EMEM_CFG_ACCESS_CTRL) = 1;

    AHB_ARBITRATION_XBAR_CTRL_0 = ((AHB_ARBITRATION_XBAR_CTRL_0 & 0xFFFEFFFF) | ((params->AhbArbitrationXbarCtrlMemInitDone & 0xFFFF) << 16));
}

static const void *sdram_get_params_erista(uint32_t dram_id) {
    uint32_t sdram_params_index = sdram_params_index_table_erista[dram_id];
#ifdef CONFIG_SDRAM_COMPRESS
    uint8_t *buf = (uint8_t *)0x40030000;
    LZ_Uncompress(sdram_params_erista_lz, buf, sizeof(sdram_params_erista_lz));
    return (const void *)&buf[sizeof(sdram_params_erista_t) * sdram_params_index];
#else
    return sdram_params_erista[sdram_params_index];
#endif
}

static const void *sdram_get_params_mariko(uint32_t dram_id) {
    uint32_t sdram_params_index = sdram_params_index_table_mariko[dram_id];
#ifdef CONFIG_SDRAM_COMPRESS
    uint8_t *buf = (uint8_t *)0x40030000;
    LZ_Uncompress(sdram_params_mariko_lz, buf, sizeof(sdram_params_mariko_lz));
    return (const void *)&buf[sizeof(sdram_params_mariko_t) * sdram_params_index];
#else
    return sdram_params_mariko[sdram_params_index];
#endif
}

static void sdram_init_erista(void) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    const sdram_params_erista_t *params = (const sdram_params_erista_t *)sdram_get_params_erista(fuse_get_dram_id());

    /* Enable VddMemory. */
    uint8_t val = 5;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD_CFG2, &val, 1);
    val = 40;   /* 40 = (1000 * 1100 - 600000) / 12500 -> 1.1V */
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD1, &val, 1);

    pmc->vddp_sel = params->PmcVddpSel;
    udelay(params->PmcVddpSelWait);
    pmc->ddr_pwr = pmc->ddr_pwr;
    pmc->no_iopower = params->PmcNoIoPower;
    pmc->reg_short = params->PmcRegShort;
    pmc->ddr_cntrl = params->PmcDdrCntrl;

    if (params->EmcBctSpare0) {
        *(volatile uint32_t *)params->EmcBctSpare0 = params->EmcBctSpare1;
    }

    sdram_config_erista(params);
}

static void sdram_init_mariko(void) {
    volatile tegra_pmc_t *pmc = pmc_get_regs();
    const sdram_params_mariko_t *params = (const sdram_params_mariko_t *)sdram_get_params_mariko(fuse_get_dram_id());

    /* Enable VddMemory. */
    uint8_t val = 5;
    i2c_send(I2C_5, MAX77620_PWR_I2C_ADDR, MAX77620_REG_SD_CFG2, &val, 1);

    pmc->vddp_sel = params->PmcVddpSel;
    udelay(params->PmcVddpSelWait);
    pmc->no_iopower = params->PmcNoIoPower;
    pmc->reg_short = params->PmcRegShort;
    pmc->ddr_cntrl = params->PmcDdrCntrl;

    if (params->EmcBctSpare0) {
        *(volatile uint32_t *)params->EmcBctSpare0 = params->EmcBctSpare1;
    }

    sdram_config_mariko(params);
}

static void sdram_save_params_erista(const void *save_params) {
    const sdram_params_erista_t *params = (const sdram_params_erista_t *)save_params;
    volatile tegra_pmc_t *pmc = pmc_get_regs();

#define pack(src, src_bits, dst, dst_bits) { \
    uint32_t mask = 0xffffffff >> (31 - ((1 ? src_bits) - (0 ? src_bits))); \
    dst &= ~(mask << (0 ? dst_bits)); \
    dst |= ((src >> (0 ? src_bits)) & mask) << (0 ? dst_bits); \
}

#define s(param, src_bits, pmcreg, dst_bits) \
    pack(params->param, src_bits, pmc->pmcreg, dst_bits)

#define c(value, pmcreg, dst_bits) \
    pack(value, (1 ? dst_bits) - (0 ? dst_bits) : 0, pmc->pmcreg, dst_bits)

/* 32 bits version of s macro */
#define s32(param, pmcreg) pmc->pmcreg = params->param

/* 32 bits version c macro */
#define c32(value, pmcreg) pmc->pmcreg = value

    /* [1.0.0-3.0.2] Patch carveout parameters. */
    /*
    *(volatile uint32_t *)params->McGeneralizedCarveout1Bom = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1BomHi = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1Size128kb = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1Access0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1Access1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1Access2 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1Access3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1Access4 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1ForceInternalAccess0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1ForceInternalAccess1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1ForceInternalAccess2 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1ForceInternalAccess3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1ForceInternalAccess4 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout1Cfg0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2Bom = 0x80020000;
    *(volatile uint32_t *)params->McGeneralizedCarveout2BomHi = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2Size128kb = 2;
    *(volatile uint32_t *)params->McGeneralizedCarveout2Access0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2Access1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2Access2 = 0x3000000;
    *(volatile uint32_t *)params->McGeneralizedCarveout2Access3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2Access4 = 0x300;
    *(volatile uint32_t *)params->McGeneralizedCarveout2ForceInternalAccess0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2ForceInternalAccess1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2ForceInternalAccess2 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2ForceInternalAccess3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2ForceInternalAccess4 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout2Cfg0 = 0x440167E;
    *(volatile uint32_t *)params->McGeneralizedCarveout3Bom = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3BomHi = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3Size128kb = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3Access0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3Access1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3Access2 = 0x3000000;
    *(volatile uint32_t *)params->McGeneralizedCarveout3Access3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3Access4 = 0x300;
    *(volatile uint32_t *)params->McGeneralizedCarveout3ForceInternalAccess0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3ForceInternalAccess1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3ForceInternalAccess2 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3ForceInternalAccess3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3ForceInternalAccess4 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout3Cfg0 = 0x4401E7E;
    *(volatile uint32_t *)params->McGeneralizedCarveout4Bom = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4BomHi = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4Size128kb = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4Access0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4Access1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4Access2 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4Access3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4Access4 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4ForceInternalAccess0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4ForceInternalAccess1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4ForceInternalAccess2 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4ForceInternalAccess3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4ForceInternalAccess4 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout4Cfg0 = 0x8F;
    *(volatile uint32_t *)params->McGeneralizedCarveout5Bom = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5BomHi = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5Size128kb = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5Access0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5Access1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5Access2 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5Access3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5Access4 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5ForceInternalAccess0 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5ForceInternalAccess1 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5ForceInternalAccess2 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5ForceInternalAccess3 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5ForceInternalAccess4 = 0;
    *(volatile uint32_t *)params->McGeneralizedCarveout5Cfg0 = 0x8F;
    */

    /* [4.0.0+] Patch carveout parameters. */
    *(volatile uint32_t *)&params->McGeneralizedCarveout1Cfg0 = 0;
    *(volatile uint32_t *)&params->McGeneralizedCarveout2Cfg0 = 0;
    *(volatile uint32_t *)&params->McGeneralizedCarveout3Cfg0 = 0;
    *(volatile uint32_t *)&params->McGeneralizedCarveout4Cfg0 = 0;
    *(volatile uint32_t *)&params->McGeneralizedCarveout5Cfg0 = 0;

    /* Patch SDRAM parameters. */
    uint32_t t0 = params->EmcSwizzleRank0Byte0 << 5 >> 29 > params->EmcSwizzleRank0Byte0 << 1 >> 29;
    uint32_t t1 = (t0 & 0xFFFFFFEF) | ((params->EmcSwizzleRank1Byte0 << 5 >> 29 > params->EmcSwizzleRank1Byte0 << 1 >> 29) << 4);
    uint32_t t2 = (t1 & 0xFFFFFFFD) | ((params->EmcSwizzleRank0Byte1 << 5 >> 29 > params->EmcSwizzleRank0Byte1 << 1 >> 29) << 1);
    uint32_t t3 = (t2 & 0xFFFFFFDF) | ((params->EmcSwizzleRank1Byte1 << 5 >> 29 > params->EmcSwizzleRank1Byte1 << 1 >> 29) << 5);
    uint32_t t4 = (t3 & 0xFFFFFFFB) | ((params->EmcSwizzleRank0Byte2 << 5 >> 29 > params->EmcSwizzleRank0Byte2 << 1 >> 29) << 2);
    uint32_t t5 = (t4 & 0xFFFFFFBF) | ((params->EmcSwizzleRank1Byte2 << 5 >> 29 > params->EmcSwizzleRank1Byte2 << 1 >> 29) << 6);
    uint32_t t6 = (t5 & 0xFFFFFFF7) | ((params->EmcSwizzleRank0Byte3 << 5 >> 29 > params->EmcSwizzleRank0Byte3 << 1 >> 29) << 3);
    uint32_t t7 = (t6 & 0xFFFFFF7F) | ((params->EmcSwizzleRank1Byte3 << 5 >> 29 > params->EmcSwizzleRank1Byte3 << 1 >> 29) << 7);
    *(volatile uint32_t *)&params->SwizzleRankByteEncode = t7;
    *(volatile uint32_t *)&params->EmcBctSpare2 = 0x40000DD8;
    *(volatile uint32_t *)&params->EmcBctSpare3 = t7;

    s(EmcClockSource, 7:0, scratch6, 15:8);
    s(EmcClockSourceDll, 7:0, scratch6, 23:16);
    s(EmcClockSource, 31:29, scratch6, 26:24);
    s(EmcClockSourceDll, 31:29, scratch6, 29:27);
    s(EmcClockSourceDll, 11:10, scratch6, 31:30);
    s(ClkRstControllerPllmMisc2Override, 9:8, scratch7, 1:0);
    s(ClkRstControllerPllmMisc2Override, 2:1, scratch7, 3:2);
    s(EmcZqCalLpDdr4WarmBoot, 31:30, scratch7, 5:4);
    s(EmcClockSource, 15:15, scratch7, 6:6);
    s(EmcClockSource, 26:26, scratch7, 7:7);
    s(EmcClockSource, 20:20, scratch7, 8:8);
    s(EmcClockSource, 19:19, scratch7, 9:9);
    s(ClkRstControllerPllmMisc2Override, 13:13, scratch7, 10:10);
    s(ClkRstControllerPllmMisc2Override, 12:12, scratch7, 11:11);
    s(ClkRstControllerPllmMisc2Override, 11:11, scratch7, 12:12);
    s(ClkRstControllerPllmMisc2Override, 10:10, scratch7, 13:13);
    s(ClkRstControllerPllmMisc2Override, 5:5, scratch7, 14:14);
    s(ClkRstControllerPllmMisc2Override, 4:4, scratch7, 15:15);
    s(ClkRstControllerPllmMisc2Override, 3:3, scratch7, 16:16);
    s(ClkRstControllerPllmMisc2Override, 0:0, scratch7, 17:17);
    s(EmcZqCalLpDdr4WarmBoot, 1:0, scratch7, 19:18);
    s(EmcZqCalLpDdr4WarmBoot, 4:4, scratch7, 20:20);
    s(EmcOdtWrite, 5:0, scratch7, 26:21);
    s(EmcOdtWrite, 11:8, scratch7, 30:27);
    s(EmcOdtWrite, 31:31, scratch7, 31:31);
    s(EmcFdpdCtrlCmdNoRamp, 0:0, scratch13, 30:30);
    s(EmcCfgPipeClk, 0:0, scratch13, 31:31);
    s(McEmemArbMisc2, 0:0, scratch14, 30:30);
    s(McDaCfg0, 0:0, scratch14, 31:31);
    s(EmcQRst, 6:0, scratch15, 26:20);
    s(EmcQRst, 20:16, scratch15, 31:27);
    s(EmcPmacroCmdTxDrv, 5:0, scratch16, 25:20);
    s(EmcPmacroCmdTxDrv, 13:8, scratch16, 31:26);
    s(EmcPmacroAutocalCfg0, 2:0, scratch17, 22:20);
    s(EmcPmacroAutocalCfg0, 10:8, scratch17, 25:23);
    s(EmcPmacroAutocalCfg0, 18:16, scratch17, 28:26);
    s(EmcPmacroAutocalCfg0, 26:24, scratch17, 31:29);
    s(EmcPmacroAutocalCfg1, 2:0, scratch18, 22:20);
    s(EmcPmacroAutocalCfg1, 10:8, scratch18, 25:23);
    s(EmcPmacroAutocalCfg1, 18:16, scratch18, 28:26);
    s(EmcPmacroAutocalCfg1, 26:24, scratch18, 31:29);
    s(EmcPmacroAutocalCfg2, 2:0, scratch19, 22:20);
    s(EmcPmacroAutocalCfg2, 10:8, scratch19, 25:23);
    s(EmcPmacroAutocalCfg2, 18:16, scratch19, 28:26);
    s(EmcPmacroAutocalCfg2, 26:24, scratch19, 31:29);
    s32(EmcCfgRsv,scratch22);
    s32(EmcAutoCalConfig, scratch23);
    s32(EmcAutoCalVrefSel0, scratch24);
    s32(EmcPmacroBrickCtrlRfu1, scratch25);
    s32(EmcPmacroBrickCtrlRfu2, scratch26);
    s32(EmcPmcScratch1, scratch27);
    s32(EmcPmcScratch2, scratch28);
    s32(EmcPmcScratch3, scratch29);
    s32(McEmemArbDaTurns, scratch30);
    s(EmcFbioSpare, 31:24, scratch58, 7:0);
    s(EmcFbioSpare, 23:16, scratch58, 15:8);
    s(EmcFbioSpare, 15:8, scratch58, 23:16);
    s(EmcFbioSpare, 7:2, scratch58, 29:24);
    s(EmcFbioSpare, 0:0, scratch58, 30:30);
    s(EmcDllCfg0, 29:0, scratch59, 29:0);
    s(EmcPmacroDdllBypass, 11:0, scratch60, 11:0);
    s(EmcPmacroDdllBypass, 27:13, scratch60, 26:12);
    s(EmcPmacroDdllBypass, 31:29, scratch60, 29:27);
    s(McEmemArbMisc0, 14:0, scratch61, 14:0);
    s(McEmemArbMisc0, 30:16, scratch61, 29:15);
    s(EmcFdpdCtrlCmd, 16:0, scratch62, 16:0);
    s(EmcFdpdCtrlCmd, 31:20, scratch62, 28:17);
    s(EmcAutoCalConfig2, 27:0, scratch63, 27:0);
    s(EmcBurstRefreshNum, 3:0, scratch63, 31:28);
    s(EmcPmacroZctrl, 27:0, scratch64, 27:0);
    s(EmcTppd, 3:0, scratch64, 31:28);
    s(EmcCfgDigDll, 10:0, scratch65, 10:0);
    s(EmcCfgDigDll, 25:12, scratch65, 24:11);
    s(EmcCfgDigDll, 27:27, scratch65, 25:25);
    s(EmcCfgDigDll, 31:30, scratch65, 27:26);
    s(EmcR2r, 3:0, scratch65, 31:28);
    s(EmcFdpdCtrlDq, 16:0, scratch66, 16:0);
    s(EmcFdpdCtrlDq, 28:20, scratch66, 25:17);
    s(EmcFdpdCtrlDq, 31:30, scratch66, 27:26);
    s(EmcW2w, 3:0, scratch66, 31:28);
    s(EmcPmacroTxPwrd4, 13:0, scratch67, 13:0);
    s(EmcPmacroTxPwrd4, 29:16, scratch67, 27:14);
    s(EmcPmacroCommonPadTxCtrl, 3:0, scratch67, 31:28);
    s(EmcPmacroTxPwrd5, 13:0, scratch68, 13:0);
    s(EmcPmacroTxPwrd5, 29:16, scratch68, 27:14);
    s(EmcPmacroDdllPwrd0, 4:0, scratch69, 4:0);
    s(EmcPmacroDdllPwrd0, 12:6, scratch69, 11:5);
    s(EmcPmacroDdllPwrd0, 20:14, scratch69, 18:12);
    s(EmcPmacroDdllPwrd0, 28:22, scratch69, 25:19);
    s(EmcPmacroDdllPwrd0, 31:30, scratch69, 27:26);
    s(EmcCfg, 4:4, scratch69, 31:31);
    s(EmcPmacroDdllPwrd1, 4:0, scratch70, 4:0);
    s(EmcPmacroDdllPwrd1, 12:6, scratch70, 11:5);
    s(EmcPmacroDdllPwrd1, 20:14, scratch70, 18:12);
    s(EmcPmacroDdllPwrd1, 28:22, scratch70, 25:19);
    s(EmcPmacroDdllPwrd1, 31:30, scratch70, 27:26);
    s(EmcCfg, 5:5, scratch70, 31:31);
    s(EmcPmacroDdllPwrd2, 4:0, scratch71, 4:0);
    s(EmcPmacroDdllPwrd2, 12:6, scratch71, 11:5);
    s(EmcPmacroDdllPwrd2, 20:14, scratch71, 18:12);
    s(EmcPmacroDdllPwrd2, 28:22, scratch71, 25:19);
    s(EmcPmacroDdllPwrd2, 31:30, scratch71, 27:26);
    s(EmcFbioCfg5, 23:20, scratch71, 31:28);
    s(EmcPmacroIbVrefDq_0, 6:0, scratch72, 6:0);
    s(EmcPmacroIbVrefDq_0, 14:8, scratch72, 13:7);
    s(EmcPmacroIbVrefDq_0, 22:16, scratch72, 20:14);
    s(EmcPmacroIbVrefDq_0, 30:24, scratch72, 27:21);
    s(EmcFbioCfg5, 15:13, scratch72, 30:28);
    s(EmcCfg, 6:6, scratch72, 31:31);
    s(EmcPmacroIbVrefDq_1, 6:0, scratch73, 6:0);
    s(EmcPmacroIbVrefDq_1, 14:8, scratch73, 13:7);
    s(EmcPmacroIbVrefDq_1, 22:16, scratch73, 20:14);
    s(EmcPmacroIbVrefDq_1, 30:24, scratch73, 27:21);
    s(EmcCfg2, 5:3, scratch73, 30:28);
    s(EmcCfg, 7:7, scratch73, 31:31);
    s(EmcPmacroIbVrefDqs_0, 6:0, scratch74, 6:0);
    s(EmcPmacroIbVrefDqs_0, 14:8, scratch74, 13:7);
    s(EmcPmacroIbVrefDqs_0, 22:16, scratch74, 20:14);
    s(EmcPmacroIbVrefDqs_0, 30:24, scratch74, 27:21);
    s(EmcCfg, 17:16, scratch74, 29:28);
    s(EmcFbioCfg5, 1:0, scratch74, 31:30);
    s(EmcPmacroIbVrefDqs_1, 6:0, scratch75, 6:0);
    s(EmcPmacroIbVrefDqs_1, 14:8, scratch75, 13:7);
    s(EmcPmacroIbVrefDqs_1, 22:16, scratch75, 20:14);
    s(EmcPmacroIbVrefDqs_1, 30:24, scratch75, 27:21);
    s(EmcFbioCfg5, 3:2, scratch75, 29:28);
    s(EmcCfg2, 27:26, scratch75, 31:30);
    s(EmcPmacroDdllShortCmd_0, 6:0, scratch76, 6:0);
    s(EmcPmacroDdllShortCmd_0, 14:8, scratch76, 13:7);
    s(EmcPmacroDdllShortCmd_0, 22:16, scratch76, 20:14);
    s(EmcPmacroDdllShortCmd_0, 30:24, scratch76, 27:21);
    s(EmcPmacroCmdPadTxCtrl, 3:2, scratch76, 29:28);
    s(EmcPmacroCmdPadTxCtrl, 7:6, scratch76, 31:30);
    s(EmcPmacroDdllShortCmd_1, 6:0, scratch77, 6:0);
    s(EmcPmacroDdllShortCmd_1, 14:8, scratch77, 13:7);
    s(EmcPmacroDdllShortCmd_1, 22:16, scratch77, 20:14);
    s(EmcPmacroDdllShortCmd_1, 30:24, scratch77, 27:21);
    s(EmcPmacroCmdPadTxCtrl, 11:10, scratch77, 29:28);
    s(EmcPmacroCmdPadTxCtrl, 15:14, scratch77, 31:30);
    s(EmcAutoCalChannel, 5:0, scratch78, 5:0);
    s(EmcAutoCalChannel, 11:8, scratch78, 9:6);
    s(EmcAutoCalChannel, 27:16, scratch78, 21:10);
    s(EmcAutoCalChannel, 31:29, scratch78, 24:22);
    s(EmcConfigSampleDelay, 6:0, scratch78, 31:25);
    s(EmcPmacroRxTerm, 5:0, scratch79, 5:0);
    s(EmcPmacroRxTerm, 13:8, scratch79, 11:6);
    s(EmcPmacroRxTerm, 21:16, scratch79, 17:12);
    s(EmcPmacroRxTerm, 29:24, scratch79, 23:18);
    s(EmcRc, 7:0, scratch79, 31:24);
    s(EmcPmacroDqTxDrv, 5:0, scratch80, 5:0);
    s(EmcPmacroDqTxDrv, 13:8, scratch80, 11:6);
    s(EmcPmacroDqTxDrv, 21:16, scratch80, 17:12);
    s(EmcPmacroDqTxDrv, 29:24, scratch80, 23:18);
    s(EmcSelDpdCtrl, 5:2, scratch80, 27:24);
    s(EmcSelDpdCtrl, 8:8, scratch80, 28:28);
    s(EmcSelDpdCtrl, 18:16, scratch80, 31:29);
    s(EmcPmacroCaTxDrv, 5:0, scratch81, 5:0);
    s(EmcPmacroCaTxDrv, 13:8, scratch81, 11:6);
    s(EmcPmacroCaTxDrv, 21:16, scratch81, 17:12);
    s(EmcPmacroCaTxDrv, 29:24, scratch81, 23:18);
    s(EmcObdly, 5:0, scratch81, 29:24);
    s(EmcObdly, 29:28, scratch81, 31:30);
    s(EmcZcalInterval, 23:10, scratch82, 13:0);
    s(EmcZcalInterval, 9:0, scratch82, 23:14);
    s(EmcPmacroCmdRxTermMode, 1:0, scratch82, 25:24);
    s(EmcPmacroCmdRxTermMode, 5:4, scratch82, 27:26);
    s(EmcPmacroCmdRxTermMode, 9:8, scratch82, 29:28);
    s(EmcPmacroCmdRxTermMode, 13:12, scratch82, 31:30);
    s(EmcDataBrlshft0, 23:0, scratch83, 23:0);
    s(EmcPmacroDataRxTermMode, 1:0, scratch83, 25:24);
    s(EmcPmacroDataRxTermMode, 5:4, scratch83, 27:26);
    s(EmcPmacroDataRxTermMode, 9:8, scratch83, 29:28);
    s(EmcPmacroDataRxTermMode, 13:12, scratch83, 31:30);
    s(EmcDataBrlshft1, 23:0, scratch84, 23:0);
    s(McEmemArbTimingRc, 7:0, scratch84, 31:24);
    s(EmcDqsBrlshft0, 23:0, scratch85, 23:0);
    s(McEmemArbRsv, 7:0, scratch85, 31:24);
    s(EmcDqsBrlshft1, 23:0, scratch86, 23:0);
    s(EmcCfgPipe2, 11:0, scratch87, 11:0);
    s(EmcCfgPipe2, 27:16, scratch87, 23:12);
    s(EmcCfgPipe1, 11:0, scratch88, 11:0);
    s(EmcCfgPipe1, 27:16, scratch88, 23:12);
    s(EmcPmacroCmdCtrl0, 5:0, scratch89, 5:0);
    s(EmcPmacroCmdCtrl0, 13:8, scratch89, 11:6);
    s(EmcPmacroCmdCtrl0, 21:16, scratch89, 17:12);
    s(EmcPmacroCmdCtrl0, 29:24, scratch89, 23:18);
    s(EmcPmacroCmdCtrl1, 5:0, scratch90, 5:0);
    s(EmcPmacroCmdCtrl1, 13:8, scratch90, 11:6);
    s(EmcPmacroCmdCtrl1, 21:16, scratch90, 17:12);
    s(EmcPmacroCmdCtrl1, 29:24, scratch90, 23:18);
    s(EmcRas, 6:0, scratch90, 30:24);
    s(EmcCfg, 8:8, scratch90, 31:31);
    s(EmcPmacroVttgenCtrl2, 23:0, scratch91, 23:0);
    s(EmcW2p, 6:0, scratch91, 30:24);
    s(EmcCfg, 9:9, scratch91, 31:31);
    s(EmcPmacroCmdPadRxCtrl, 2:0, scratch92, 2:0);
    s(EmcPmacroCmdPadRxCtrl, 5:4, scratch92, 4:3);
    s(EmcPmacroCmdPadRxCtrl, 10:8, scratch92, 7:5);
    s(EmcPmacroCmdPadRxCtrl, 22:12, scratch92, 18:8);
    s(EmcPmacroCmdPadRxCtrl, 28:24, scratch92, 23:19);
    s(EmcQSafe, 6:0, scratch92, 30:24);
    s(EmcCfg, 18:18, scratch92, 31:31);
    s(EmcPmacroDataPadRxCtrl, 2:0, scratch93, 2:0);
    s(EmcPmacroDataPadRxCtrl, 5:4, scratch93, 4:3);
    s(EmcPmacroDataPadRxCtrl, 10:8, scratch93, 7:5);
    s(EmcPmacroDataPadRxCtrl, 22:12, scratch93, 18:8);
    s(EmcPmacroDataPadRxCtrl, 28:24, scratch93, 23:19);
    s(EmcRdv, 6:0, scratch93, 30:24);
    s(EmcCfg, 21:21, scratch93, 31:31);
    s(McEmemArbDaCovers, 23:0, scratch94, 23:0);
    s(EmcRw2Pden, 6:0, scratch94, 30:24);
    s(EmcCfg, 22:22, scratch94, 31:31);
    s(EmcPmacroCmdCtrl2, 5:0, scratch95, 5:0);
    s(EmcPmacroCmdCtrl2, 13:9, scratch95, 10:6);
    s(EmcPmacroCmdCtrl2, 21:16, scratch95, 16:11);
    s(EmcPmacroCmdCtrl2, 29:24, scratch95, 22:17);
    s(EmcRfcPb, 8:0, scratch95, 31:23);
    s(EmcPmacroQuseDdllRank0_0, 10:0, scratch96, 10:0);
    s(EmcPmacroQuseDdllRank0_0, 26:16, scratch96, 21:11);
    s(EmcCfgUpdate, 2:0, scratch96, 24:22);
    s(EmcCfgUpdate, 10:8, scratch96, 27:25);
    s(EmcCfgUpdate, 31:28, scratch96, 31:28);
    s(EmcPmacroQuseDdllRank0_1, 10:0, scratch97, 10:0);
    s(EmcPmacroQuseDdllRank0_1, 26:16, scratch97, 21:11);
    s(EmcRfc, 9:0, scratch97, 31:22);
    s(EmcPmacroQuseDdllRank0_2, 10:0, scratch98, 10:0);
    s(EmcPmacroQuseDdllRank0_2, 26:16, scratch98, 21:11);
    s(EmcTxsr, 9:0, scratch98, 31:22);
    s(EmcPmacroQuseDdllRank0_3, 10:0, scratch99, 10:0);
    s(EmcPmacroQuseDdllRank0_3, 26:16, scratch99, 21:11);
    s(EmcMc2EmcQ, 2:0, scratch99, 24:22);
    s(EmcMc2EmcQ, 10:8, scratch99, 27:25);
    s(EmcMc2EmcQ, 27:24, scratch99, 31:28);
    s(EmcPmacroQuseDdllRank0_4, 10:0, scratch100, 10:0);
    s(EmcPmacroQuseDdllRank0_4, 26:16, scratch100, 21:11);
    s(McEmemArbRing1Throttle, 4:0, scratch100, 26:22);
    s(McEmemArbRing1Throttle, 20:16, scratch100, 31:27);
    s(EmcPmacroQuseDdllRank0_5, 10:0, scratch101, 10:0);
    s(EmcPmacroQuseDdllRank0_5, 26:16, scratch101, 21:11);
    s(EmcPmacroQuseDdllRank1_0, 10:0, scratch102, 10:0);
    s(EmcPmacroQuseDdllRank1_0, 26:16, scratch102, 21:11);
    s(EmcAr2Pden, 8:0, scratch102, 30:22);
    s(EmcCfg, 23:23, scratch102, 31:31);
    s(EmcPmacroQuseDdllRank1_1, 10:0, scratch103, 10:0);
    s(EmcPmacroQuseDdllRank1_1, 26:16, scratch103, 21:11);
    s(EmcRfcSlr, 8:0, scratch103, 30:22);
    s(EmcCfg, 24:24, scratch103, 31:31);
    s(EmcPmacroQuseDdllRank1_2, 10:0, scratch104, 10:0);
    s(EmcPmacroQuseDdllRank1_2, 26:16, scratch104, 21:11);
    s(EmcIbdly, 6:0, scratch104, 28:22);
    s(EmcIbdly, 29:28, scratch104, 30:29);
    s(EmcCfg, 25:25, scratch104, 31:31);
    s(EmcPmacroQuseDdllRank1_3, 10:0, scratch105, 10:0);
    s(EmcPmacroQuseDdllRank1_3, 26:16, scratch105, 21:11);
    s(McEmemArbTimingRFCPB, 8:0, scratch105, 30:22);
    s(EmcCfg, 26:26, scratch105, 31:31);
    s(EmcPmacroQuseDdllRank1_4, 10:0, scratch106, 10:0);
    s(EmcPmacroQuseDdllRank1_4, 26:16, scratch106, 21:11);
    s(EmcTfaw, 6:0, scratch106, 28:22);
    s(EmcPmacroDataPadTxCtrl, 3:2, scratch106, 30:29);
    s(EmcCfg, 28:28, scratch106, 31:31);
    s(EmcPmacroQuseDdllRank1_5, 10:0, scratch107, 10:0);
    s(EmcPmacroQuseDdllRank1_5, 26:16, scratch107, 21:11);
    s(EmcTClkStable, 6:0, scratch107, 28:22);
    s(EmcPmacroDataPadTxCtrl, 7:6, scratch107, 30:29);
    s(EmcCfg, 29:29, scratch107, 31:31);
    s(EmcPmacroObDdllLongDqRank0_0, 10:0, scratch108, 10:0);
    s(EmcPmacroObDdllLongDqRank0_0, 26:16, scratch108, 21:11);
    s(EmcPdex2Mrr, 6:0, scratch108, 28:22);
    s(EmcPmacroDataPadTxCtrl, 11:10, scratch108, 30:29);
    s(EmcCfg, 30:30, scratch108, 31:31);
    s(EmcPmacroObDdllLongDqRank0_1, 10:0, scratch109, 10:0);
    s(EmcPmacroObDdllLongDqRank0_1, 26:16, scratch109, 21:11);
    s(EmcRdvMask, 6:0, scratch109, 28:22);
    s(EmcPmacroDataPadTxCtrl, 15:14, scratch109, 30:29);
    s(EmcCfg, 31:31, scratch109, 31:31);
    s(EmcPmacroObDdllLongDqRank0_2, 10:0, scratch110, 10:0);
    s(EmcPmacroObDdllLongDqRank0_2, 26:16, scratch110, 21:11);
    s(EmcRdvEarlyMask, 6:0, scratch110, 28:22);
    s(EmcFbioCfg5, 4:4, scratch110, 29:29);
    s(EmcFbioCfg5, 8:8, scratch110, 30:30);
    s(EmcFbioCfg5, 10:10, scratch110, 31:31);
    s(EmcPmacroObDdllLongDqRank0_3, 10:0, scratch111, 10:0);
    s(EmcPmacroObDdllLongDqRank0_3, 26:16, scratch111, 21:11);
    s(EmcRdvEarly, 6:0, scratch111, 28:22);
    s(EmcFbioCfg5, 12:12, scratch111, 29:29);
    s(EmcFbioCfg5, 25:24, scratch111, 31:30);
    s(EmcPmacroObDdllLongDqRank0_4, 10:0, scratch112, 10:0);
    s(EmcPmacroObDdllLongDqRank0_4, 26:16, scratch112, 21:11);
    s(EmcPmacroDdllShortCmd_2, 6:0, scratch112, 28:22);
    s(EmcFbioCfg5, 28:26, scratch112, 31:29);
    s(EmcPmacroObDdllLongDqRank0_5, 10:0, scratch113, 10:0);
    s(EmcPmacroObDdllLongDqRank0_5, 26:16, scratch113, 21:11);
    s(McEmemArbTimingRp, 6:0, scratch113, 28:22);
    s(EmcFbioCfg5, 31:30, scratch113, 30:29);
    s(EmcCfg2, 0:0, scratch113, 31:31);
    s(EmcPmacroObDdllLongDqRank1_0, 10:0, scratch114, 10:0);
    s(EmcPmacroObDdllLongDqRank1_0, 26:16, scratch114, 21:11);
    s(McEmemArbTimingRas, 6:0, scratch114, 28:22);
    s(EmcCfg2, 2:1, scratch114, 30:29);
    s(EmcCfg2, 7:7, scratch114, 31:31);
    s(EmcPmacroObDdllLongDqRank1_1, 10:0, scratch115, 10:0);
    s(EmcPmacroObDdllLongDqRank1_1, 26:16, scratch115, 21:11);
    s(McEmemArbTimingFaw, 6:0, scratch115, 28:22);
    s(EmcCfg2, 11:10, scratch115, 30:29);
    s(EmcCfg2, 14:14, scratch115, 31:31);
    s(EmcPmacroObDdllLongDqRank1_2, 10:0, scratch123, 10:0);
    s(EmcPmacroObDdllLongDqRank1_2, 26:16, scratch123, 21:11);
    s(McEmemArbTimingRap2Pre, 6:0, scratch123, 28:22);
    s(EmcCfg2, 16:15, scratch123, 30:29);
    s(EmcCfg2, 20:20, scratch123, 31:31);
    s(EmcPmacroObDdllLongDqRank1_3, 10:0, scratch124, 10:0);
    s(EmcPmacroObDdllLongDqRank1_3, 26:16, scratch124, 21:11);
    s(McEmemArbTimingWap2Pre, 6:0, scratch124, 28:22);
    s(EmcCfg2, 24:22, scratch124, 31:29);
    s(EmcPmacroObDdllLongDqRank1_4, 10:0, scratch125, 10:0);
    s(EmcPmacroObDdllLongDqRank1_4, 26:16, scratch125, 21:11);
    s(McEmemArbTimingR2W, 6:0, scratch125, 28:22);
    s(EmcCfg2, 25:25, scratch125, 29:29);
    s(EmcCfg2, 29:28, scratch125, 31:30);
    s(EmcPmacroObDdllLongDqRank1_5, 10:0, scratch126, 10:0);
    s(EmcPmacroObDdllLongDqRank1_5, 26:16, scratch126, 21:11);
    s(McEmemArbTimingW2R, 6:0, scratch126, 28:22);
    s(EmcCfg2, 31:30, scratch126, 30:29);
    s(EmcCfgPipe, 0:0, scratch126, 31:31);
    s(EmcPmacroObDdllLongDqsRank0_0, 10:0, scratch127, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_0, 26:16, scratch127, 21:11);
    s(EmcRp, 5:0, scratch127, 27:22);
    s(EmcCfgPipe, 4:1, scratch127, 31:28);
    s(EmcPmacroObDdllLongDqsRank0_1, 10:0, scratch128, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_1, 26:16, scratch128, 21:11);
    s(EmcR2w, 5:0, scratch128, 27:22);
    s(EmcCfgPipe, 8:5, scratch128, 31:28);
    s(EmcPmacroObDdllLongDqsRank0_2, 10:0, scratch129, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_2, 26:16, scratch129, 21:11);
    s(EmcW2r, 5:0, scratch129, 27:22);
    s(EmcCfgPipe, 11:9, scratch129, 30:28);
    s(EmcCfgPipe, 16:16, scratch129, 31:31);
    s(EmcPmacroObDdllLongDqsRank0_3, 10:0, scratch130, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_3, 26:16, scratch130, 21:11);
    s(EmcR2p, 5:0, scratch130, 27:22);
    s(EmcCfgPipe, 20:17, scratch130, 31:28);
    s(EmcPmacroObDdllLongDqsRank0_4, 10:0, scratch131, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_4, 26:16, scratch131, 21:11);
    s(EmcCcdmw, 5:0, scratch131, 27:22);
    s(EmcCfgPipe, 24:21, scratch131, 31:28);
    s(EmcPmacroObDdllLongDqsRank0_5, 10:0, scratch132, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_5, 26:16, scratch132, 21:11);
    s(EmcRdRcd, 5:0, scratch132, 27:22);
    s(EmcCfgPipe, 27:25, scratch132, 30:28);
    s(EmcPmacroTxPwrd0, 0:0, scratch132, 31:31);
    s(EmcPmacroObDdllLongDqsRank1_0, 10:0, scratch133, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_0, 26:16, scratch133, 21:11);
    s(EmcWrRcd, 5:0, scratch133, 27:22);
    s(EmcPmacroTxPwrd0, 4:1, scratch133, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_1, 10:0, scratch134, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_1, 26:16, scratch134, 21:11);
    s(EmcWdv, 5:0, scratch134, 27:22);
    s(EmcPmacroTxPwrd0, 8:5, scratch134, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_2, 10:0, scratch135, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_2, 26:16, scratch135, 21:11);
    s(EmcQUse, 5:0, scratch135, 27:22);
    s(EmcPmacroTxPwrd0, 12:9, scratch135, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_3, 10:0, scratch136, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_3, 26:16, scratch136, 21:11);
    s(EmcPdEx2Wr, 5:0, scratch136, 27:22);
    s(EmcPmacroTxPwrd0, 13:13, scratch136, 28:28);
    s(EmcPmacroTxPwrd0, 18:16, scratch136, 31:29);
    s(EmcPmacroObDdllLongDqsRank1_4, 10:0, scratch137, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_4, 26:16, scratch137, 21:11);
    s(EmcPdEx2Rd, 5:0, scratch137, 27:22);
    s(EmcPmacroTxPwrd0, 22:19, scratch137, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_5, 10:0, scratch138, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_5, 26:16, scratch138, 21:11);
    s(EmcPdex2Cke, 5:0, scratch138, 27:22);
    s(EmcPmacroTxPwrd0, 26:23, scratch138, 31:28);
    s(EmcPmacroIbDdllLongDqsRank0_0, 10:0, scratch139, 10:0);
    s(EmcPmacroIbDdllLongDqsRank0_0, 26:16, scratch139, 21:11);
    s(EmcPChg2Pden, 5:0, scratch139, 27:22);
    s(EmcPmacroTxPwrd0, 29:27, scratch139, 30:28);
    s(EmcPmacroTxPwrd1, 0:0, scratch139, 31:31);
    s(EmcPmacroIbDdllLongDqsRank0_1, 10:0, scratch140, 10:0);
    s(EmcPmacroIbDdllLongDqsRank0_1, 26:16, scratch140, 21:11);
    s(EmcAct2Pden, 5:0, scratch140, 27:22);
    s(EmcPmacroTxPwrd1, 4:1, scratch140, 31:28);
    s(EmcPmacroIbDdllLongDqsRank0_2, 10:0, scratch141, 10:0);
    s(EmcPmacroIbDdllLongDqsRank0_2, 26:16, scratch141, 21:11);
    s(EmcCke2Pden, 5:0, scratch141, 27:22);
    s(EmcPmacroTxPwrd1, 8:5, scratch141, 31:28);
    s(EmcPmacroIbDdllLongDqsRank0_3, 10:0, scratch142, 10:0);
    s(EmcPmacroIbDdllLongDqsRank0_3, 26:16, scratch142, 21:11);
    s(EmcTcke, 5:0, scratch142, 27:22);
    s(EmcPmacroTxPwrd1, 12:9, scratch142, 31:28);
    s(EmcPmacroIbDdllLongDqsRank1_0, 10:0, scratch143, 10:0);
    s(EmcPmacroIbDdllLongDqsRank1_0, 26:16, scratch143, 21:11);
    s(EmcTrpab, 5:0, scratch143, 27:22);
    s(EmcPmacroTxPwrd1, 13:13, scratch143, 28:28);
    s(EmcPmacroTxPwrd1, 18:16, scratch143, 31:29);
    s(EmcPmacroIbDdllLongDqsRank1_1, 10:0, scratch144, 10:0);
    s(EmcPmacroIbDdllLongDqsRank1_1, 26:16, scratch144, 21:11);
    s(EmcClkenOverride, 3:1, scratch144, 24:22);
    s(EmcClkenOverride, 8:6, scratch144, 27:25);
    s(EmcPmacroTxPwrd1, 22:19, scratch144, 31:28);
    s(EmcPmacroIbDdllLongDqsRank1_2, 10:0, scratch145, 10:0);
    s(EmcPmacroIbDdllLongDqsRank1_2, 26:16, scratch145, 21:11);
    s(EmcEInput, 5:0, scratch145, 27:22);
    s(EmcPmacroTxPwrd1, 26:23, scratch145, 31:28);
    s(EmcPmacroIbDdllLongDqsRank1_3, 10:0, scratch146, 10:0);
    s(EmcPmacroIbDdllLongDqsRank1_3, 26:16, scratch146, 21:11);
    s(EmcEInputDuration, 5:0, scratch146, 27:22);
    s(EmcPmacroTxPwrd1, 29:27, scratch146, 30:28);
    s(EmcPmacroTxPwrd2, 0:0, scratch146, 31:31);
    s(EmcPmacroDdllLongCmd_0, 10:0, scratch147, 10:0);
    s(EmcPmacroDdllLongCmd_0, 26:16, scratch147, 21:11);
    s(EmcPutermExtra, 5:0, scratch147, 27:22);
    s(EmcPmacroTxPwrd2, 4:1, scratch147, 31:28);
    s(EmcPmacroDdllLongCmd_1, 10:0, scratch148, 10:0);
    s(EmcPmacroDdllLongCmd_1, 26:16, scratch148, 21:11);
    s(EmcTckesr, 5:0, scratch148, 27:22);
    s(EmcPmacroTxPwrd2, 8:5, scratch148, 31:28);
    s(EmcPmacroDdllLongCmd_2, 10:0, scratch149, 10:0);
    s(EmcPmacroDdllLongCmd_2, 26:16, scratch149, 21:11);
    s(EmcTpd, 5:0, scratch149, 27:22);
    s(EmcPmacroTxPwrd2, 12:9, scratch149, 31:28);
    s(EmcPmacroDdllLongCmd_3, 10:0, scratch150, 10:0);
    s(EmcPmacroDdllLongCmd_3, 26:16, scratch150, 21:11);
    s(EmcWdvMask, 5:0, scratch150, 27:22);
    s(EmcPmacroTxPwrd2, 13:13, scratch150, 28:28);
    s(EmcPmacroTxPwrd2, 18:16, scratch150, 31:29);
    s(McEmemArbCfg, 8:0, scratch151, 8:0);
    s(McEmemArbCfg, 20:16, scratch151, 13:9);
    s(McEmemArbCfg, 31:24, scratch151, 21:14);
    s(EmcWdvChk, 5:0, scratch151, 27:22);
    s(EmcPmacroTxPwrd2, 22:19, scratch151, 31:28);
    s(McEmemArbMisc1, 12:0, scratch152, 12:0);
    s(McEmemArbMisc1, 25:21, scratch152, 17:13);
    s(McEmemArbMisc1, 31:28, scratch152, 21:18);
    s(EmcCmdBrlshft0, 5:0, scratch152, 27:22);
    s(EmcPmacroTxPwrd2, 26:23, scratch152, 31:28);
    s(EmcMrsWaitCnt2, 9:0, scratch153, 9:0);
    s(EmcMrsWaitCnt2, 26:16, scratch153, 20:10);
    s(EmcPmacroIbRxrt, 10:0, scratch153, 31:21);
    s(EmcMrsWaitCnt, 9:0, scratch154, 9:0);
    s(EmcMrsWaitCnt, 26:16, scratch154, 20:10);
    s(EmcPmacroDdllLongCmd_4, 10:0, scratch154, 31:21);
    s(EmcAutoCalInterval, 20:0, scratch155, 20:0);
    s(McEmemArbOutstandingReq, 8:0, scratch155, 29:21);
    s(McEmemArbOutstandingReq, 31:30, scratch155, 31:30);
    s(McEmemArbRefpbHpCtrl, 6:0, scratch156, 6:0);
    s(McEmemArbRefpbHpCtrl, 14:8, scratch156, 13:7);
    s(McEmemArbRefpbHpCtrl, 22:16, scratch156, 20:14);
    s(EmcCmdBrlshft1, 5:0, scratch156, 26:21);
    s(EmcRrd, 4:0, scratch156, 31:27);
    s(EmcQuseBrlshft0, 19:0, scratch157, 19:0);
    s(EmcFbioCfg8, 27:16, scratch157, 31:20);
    s(EmcQuseBrlshft1, 19:0, scratch158, 19:0);
    s(EmcTxsrDll, 11:0, scratch158, 31:20);
    s(EmcQuseBrlshft2, 19:0, scratch159, 19:0);
    s(EmcTxdsrvttgen, 11:0, scratch159, 31:20);
    s(EmcQuseBrlshft3, 19:0, scratch160, 19:0);
    s(EmcPmacroVttgenCtrl0, 3:0, scratch160, 23:20);
    s(EmcPmacroVttgenCtrl0, 11:8, scratch160, 27:24);
    s(EmcPmacroVttgenCtrl0, 19:16, scratch160, 31:28);
    s(EmcPmacroVttgenCtrl1, 19:0, scratch161, 19:0);
    s(EmcCmdBrlshft2, 5:0, scratch161, 25:20);
    s(EmcCmdBrlshft3, 5:0, scratch161, 31:26);
    s(EmcAutoCalConfig3, 5:0, scratch162, 5:0);
    s(EmcAutoCalConfig3, 13:8, scratch162, 11:6);
    s(EmcAutoCalConfig3, 18:16, scratch162, 14:12);
    s(EmcAutoCalConfig3, 22:20, scratch162, 17:15);
    s(EmcTRefBw, 13:0, scratch162, 31:18);
    s(EmcAutoCalConfig4, 5:0, scratch163, 5:0);
    s(EmcAutoCalConfig4, 13:8, scratch163, 11:6);
    s(EmcAutoCalConfig4, 18:16, scratch163, 14:12);
    s(EmcAutoCalConfig4, 22:20, scratch163, 17:15);
    s(EmcQpop, 6:0, scratch163, 24:18);
    s(EmcQpop, 22:16, scratch163, 31:25);
    s(EmcAutoCalConfig5, 5:0, scratch164, 5:0);
    s(EmcAutoCalConfig5, 13:8, scratch164, 11:6);
    s(EmcAutoCalConfig5, 18:16, scratch164, 14:12);
    s(EmcAutoCalConfig5, 22:20, scratch164, 17:15);
    s(EmcPmacroAutocalCfgCommon, 5:0, scratch164, 23:18);
    s(EmcPmacroAutocalCfgCommon, 13:8, scratch164, 29:24);
    s(EmcPmacroAutocalCfgCommon, 16:16, scratch164, 30:30);
    s(EmcPmacroTxPwrd2, 27:27, scratch164, 31:31);
    s(EmcAutoCalConfig6, 5:0, scratch165, 5:0);
    s(EmcAutoCalConfig6, 13:8, scratch165, 11:6);
    s(EmcAutoCalConfig6, 18:16, scratch165, 14:12);
    s(EmcAutoCalConfig6, 22:20, scratch165, 17:15);
    s(EmcWev, 5:0, scratch165, 23:18);
    s(EmcWsv, 5:0, scratch165, 29:24);
    s(EmcPmacroTxPwrd2, 29:28, scratch165, 31:30);
    s(EmcAutoCalConfig7, 5:0, scratch166, 5:0);
    s(EmcAutoCalConfig7, 13:8, scratch166, 11:6);
    s(EmcAutoCalConfig7, 18:16, scratch166, 14:12);
    s(EmcAutoCalConfig7, 22:20, scratch166, 17:15);
    s(EmcCfg3, 2:0, scratch166, 20:18);
    s(EmcCfg3, 6:4, scratch166, 23:21);
    s(EmcQuseWidth, 3:0, scratch166, 27:24);
    s(EmcQuseWidth, 29:28, scratch166, 29:28);
    s(EmcPmacroTxPwrd3, 1:0, scratch166, 31:30);
    s(EmcAutoCalConfig8, 5:0, scratch167, 5:0);
    s(EmcAutoCalConfig8, 13:8, scratch167, 11:6);
    s(EmcAutoCalConfig8, 18:16, scratch167, 14:12);
    s(EmcAutoCalConfig8, 22:20, scratch167, 17:15);
    s(EmcPmacroBgBiasCtrl0, 2:0, scratch167, 20:18);
    s(EmcPmacroBgBiasCtrl0, 6:4, scratch167, 23:21);
    s(McEmemArbTimingRcd, 5:0, scratch167, 29:24);
    s(EmcPmacroTxPwrd3, 3:2, scratch167, 31:30);
    s(EmcXm2CompPadCtrl2, 17:0, scratch168, 17:0);
    s(McEmemArbTimingCcdmw, 5:0, scratch168, 23:18);
    s(McEmemArbOverride, 27:27, scratch168, 24:24);
    s(McEmemArbOverride, 26:26, scratch168, 25:25);
    s(McEmemArbOverride, 16:16, scratch168, 26:26);
    s(McEmemArbOverride, 10:10, scratch168, 27:27);
    s(McEmemArbOverride, 4:4, scratch168, 28:28);
    s(McEmemArbOverride, 3:3, scratch168, 29:29);
    s(EmcPmacroTxPwrd3, 5:4, scratch168, 31:30);
    s(EmcXm2CompPadCtrl3, 17:0, scratch169, 17:0);
    s(EmcRext, 4:0, scratch169, 22:18);
    s(EmcTClkStop, 4:0, scratch169, 27:23);
    s(EmcPmacroTxPwrd3, 9:6, scratch169, 31:28);
    s(EmcZcalWaitCnt, 10:0, scratch170, 10:0);
    s(EmcZcalWaitCnt, 21:16, scratch170, 16:11);
    s(EmcZcalWaitCnt, 31:31, scratch170, 17:17);
    s(EmcWext, 4:0, scratch170, 22:18);
    s(EmcRefctrl2, 0:0, scratch170, 23:23);
    s(EmcRefctrl2, 26:24, scratch170, 26:24);
    s(EmcRefctrl2, 31:31, scratch170, 27:27);
    s(EmcPmacroTxPwrd3, 13:10, scratch170, 31:28);
    s(EmcZcalMrwCmd, 7:0, scratch171, 7:0);
    s(EmcZcalMrwCmd, 23:16, scratch171, 15:8);
    s(EmcZcalMrwCmd, 31:30, scratch171, 17:16);
    s(EmcWeDuration, 4:0, scratch171, 22:18);
    s(EmcWsDuration, 4:0, scratch171, 27:23);
    s(EmcPmacroTxPwrd3, 19:16, scratch171, 31:28);
    s(EmcSwizzleRank0Byte0, 2:0, scratch172, 2:0);
    s(EmcSwizzleRank0Byte0, 6:4, scratch172, 5:3);
    s(EmcSwizzleRank0Byte0, 10:8, scratch172, 8:6);
    s(EmcSwizzleRank0Byte0, 14:12, scratch172, 11:9);
    s(EmcSwizzleRank0Byte0, 18:16, scratch172, 14:12);
    s(EmcSwizzleRank0Byte0, 22:20, scratch172, 17:15);
    s(EmcPutermWidth, 31:31, scratch172, 18:18);
    s(EmcPutermWidth, 3:0, scratch172, 22:19);
    s(McEmemArbTimingRrd, 4:0, scratch172, 27:23);
    s(EmcPmacroTxPwrd3, 23:20, scratch172, 31:28);
    s(EmcSwizzleRank0Byte1, 2:0, scratch173, 2:0);
    s(EmcSwizzleRank0Byte1, 6:4, scratch173, 5:3);
    s(EmcSwizzleRank0Byte1, 10:8, scratch173, 8:6);
    s(EmcSwizzleRank0Byte1, 14:12, scratch173, 11:9);
    s(EmcSwizzleRank0Byte1, 18:16, scratch173, 14:12);
    s(EmcSwizzleRank0Byte1, 22:20, scratch173, 17:15);
    s(McEmemArbTimingR2R, 4:0, scratch173, 22:18);
    s(McEmemArbTimingW2W, 4:0, scratch173, 27:23);
    s(EmcPmacroTxPwrd3, 27:24, scratch173, 31:28);
    s(EmcSwizzleRank0Byte2, 2:0, scratch174, 2:0);
    s(EmcSwizzleRank0Byte2, 6:4, scratch174, 5:3);
    s(EmcSwizzleRank0Byte2, 10:8, scratch174, 8:6);
    s(EmcSwizzleRank0Byte2, 14:12, scratch174, 11:9);
    s(EmcSwizzleRank0Byte2, 18:16, scratch174, 14:12);
    s(EmcSwizzleRank0Byte2, 22:20, scratch174, 17:15);
    s(EmcPmacroTxPwrd3, 29:28, scratch174, 19:18);
    s(EmcPmacroTxSelClkSrc0, 11:0, scratch174, 31:20);
    s(EmcSwizzleRank0Byte3, 2:0, scratch175, 2:0);
    s(EmcSwizzleRank0Byte3, 6:4, scratch175, 5:3);
    s(EmcSwizzleRank0Byte3, 10:8, scratch175, 8:6);
    s(EmcSwizzleRank0Byte3, 14:12, scratch175, 11:9);
    s(EmcSwizzleRank0Byte3, 18:16, scratch175, 14:12);
    s(EmcSwizzleRank0Byte3, 22:20, scratch175, 17:15);
    s(EmcPmacroTxSelClkSrc0, 27:16, scratch175, 29:18);
    s(EmcPmacroTxSelClkSrc1, 1:0, scratch175, 31:30);
    s(EmcSwizzleRank1Byte0, 2:0, scratch176, 2:0);
    s(EmcSwizzleRank1Byte0, 6:4, scratch176, 5:3);
    s(EmcSwizzleRank1Byte0, 10:8, scratch176, 8:6);
    s(EmcSwizzleRank1Byte0, 14:12, scratch176, 11:9);
    s(EmcSwizzleRank1Byte0, 18:16, scratch176, 14:12);
    s(EmcSwizzleRank1Byte0, 22:20, scratch176, 17:15);
    s(EmcPmacroTxSelClkSrc1, 11:2, scratch176, 27:18);
    s(EmcPmacroTxSelClkSrc1, 19:16, scratch176, 31:28);
    s(EmcSwizzleRank1Byte1, 2:0, scratch177, 2:0);
    s(EmcSwizzleRank1Byte1, 6:4, scratch177, 5:3);
    s(EmcSwizzleRank1Byte1, 10:8, scratch177, 8:6);
    s(EmcSwizzleRank1Byte1, 14:12, scratch177, 11:9);
    s(EmcSwizzleRank1Byte1, 18:16, scratch177, 14:12);
    s(EmcSwizzleRank1Byte1, 22:20, scratch177, 17:15);
    s(EmcPmacroTxSelClkSrc1, 27:20, scratch177, 25:18);
    s(EmcPmacroTxSelClkSrc3, 5:0, scratch177, 31:26);
    s(EmcSwizzleRank1Byte2, 2:0, scratch178, 2:0);
    s(EmcSwizzleRank1Byte2, 6:4, scratch178, 5:3);
    s(EmcSwizzleRank1Byte2, 10:8, scratch178, 8:6);
    s(EmcSwizzleRank1Byte2, 14:12, scratch178, 11:9);
    s(EmcSwizzleRank1Byte2, 18:16, scratch178, 14:12);
    s(EmcSwizzleRank1Byte2, 22:20, scratch178, 17:15);
    s(EmcPmacroTxSelClkSrc3, 11:6, scratch178, 23:18);
    s(EmcPmacroTxSelClkSrc3, 23:16, scratch178, 31:24);
    s(EmcSwizzleRank1Byte3, 2:0, scratch179, 2:0);
    s(EmcSwizzleRank1Byte3, 6:4, scratch179, 5:3);
    s(EmcSwizzleRank1Byte3, 10:8, scratch179, 8:6);
    s(EmcSwizzleRank1Byte3, 14:12, scratch179, 11:9);
    s(EmcSwizzleRank1Byte3, 18:16, scratch179, 14:12);
    s(EmcSwizzleRank1Byte3, 22:20, scratch179, 17:15);
    s(EmcPmacroTxSelClkSrc3, 27:24, scratch179, 21:18);
    s(EmcPmacroTxSelClkSrc2, 9:0, scratch179, 31:22);
    s(EmcPmacroCmdBrickCtrlFdpd, 17:0, scratch180, 17:0);
    s(EmcPmacroTxSelClkSrc2, 11:10, scratch180, 19:18);
    s(EmcPmacroTxSelClkSrc2, 27:16, scratch180, 31:20);
    s(EmcPmacroDataBrickCtrlFdpd, 17:0, scratch181, 17:0);
    s(EmcPmacroTxSelClkSrc4, 11:0, scratch181, 29:18);
    s(EmcPmacroTxSelClkSrc4, 17:16, scratch181, 31:30);
    s(EmcFbioCfg7, 16:0, scratch182, 16:0);
    s(McEmemArbRefpbBankCtrl, 6:0, scratch182, 23:17);
    s(McEmemArbRefpbBankCtrl, 14:8, scratch182, 30:24);
    s(McEmemArbRefpbBankCtrl, 31:31, scratch182, 31:31);
    s(EmcDynSelfRefControl, 15:0, scratch183, 15:0);
    s(EmcDynSelfRefControl, 31:31, scratch183, 16:16);
    s(EmcPmacroTxSelClkSrc4, 27:18, scratch183, 26:17);
    s(EmcPmacroTxSelClkSrc5, 4:0, scratch183, 31:27);
    s(EmcDllCfg1, 16:0, scratch184, 16:0);
    s(EmcPmacroTxSelClkSrc5, 11:5, scratch184, 23:17);
    s(EmcPmacroTxSelClkSrc5, 23:16, scratch184, 31:24);
    s(EmcPmacroPadCfgCtrl, 1:0, scratch185, 1:0);
    s(EmcPmacroPadCfgCtrl, 6:5, scratch185, 3:2);
    s(EmcPmacroPadCfgCtrl, 11:9, scratch185, 6:4);
    s(EmcPmacroPadCfgCtrl, 13:13, scratch185, 7:7);
    s(EmcPmacroPadCfgCtrl, 17:16, scratch185, 9:8);
    s(EmcPmacroPadCfgCtrl, 21:20, scratch185, 11:10);
    s(EmcPmacroPadCfgCtrl, 25:24, scratch185, 13:12);
    s(EmcPmacroPadCfgCtrl, 30:28, scratch185, 16:14);
    s(EmcPmacroTxSelClkSrc5, 27:24, scratch185, 20:17);
    s(EmcPmacroCmdPadTxCtrl, 1:0, scratch185, 22:21);
    s(EmcPmacroCmdPadTxCtrl, 5:4, scratch185, 24:23);
    s(EmcPmacroCmdPadTxCtrl, 9:8, scratch185, 26:25);
    s(EmcPmacroCmdPadTxCtrl, 13:12, scratch185, 28:27);
    s(EmcPmacroCmdPadTxCtrl, 16:16, scratch185, 29:29);
    s(EmcPmacroCmdPadTxCtrl, 21:20, scratch185, 31:30);
    s(EmcRefresh, 15:0, scratch186, 15:0);
    s(EmcCmdQ, 4:0, scratch186, 20:16);
    s(EmcCmdQ, 10:8, scratch186, 23:21);
    s(EmcCmdQ, 14:12, scratch186, 26:24);
    s(EmcCmdQ, 28:24, scratch186, 31:27);
    s(EmcAcpdControl, 15:0, scratch187, 15:0);
    s(EmcAutoCalVrefSel1, 15:0, scratch187, 31:16);
    s(EmcXm2CompPadCtrl, 1:0, scratch188, 1:0);
    s(EmcXm2CompPadCtrl, 6:3, scratch188, 5:2);
    s(EmcXm2CompPadCtrl, 9:9, scratch188, 6:6);
    s(EmcXm2CompPadCtrl, 19:11, scratch188, 15:7);
    s(EmcCfgDigDllPeriod, 15:0, scratch188, 31:16);
    s(EmcCfgDigDll_1, 15:0, scratch189, 15:0);
    s(EmcPreRefreshReqCnt, 15:0, scratch189, 31:16);
    s(EmcPmacroCmdPadTxCtrl, 27:24, scratch190, 19:16);
    s(EmcPmacroDataPadTxCtrl, 1:0, scratch190, 21:20);
    s(EmcPmacroDataPadTxCtrl, 5:4, scratch190, 23:22);
    s(EmcPmacroDataPadTxCtrl, 9:8, scratch190, 25:24);
    s(EmcPmacroDataPadTxCtrl, 13:12, scratch190, 27:26);
    s(EmcPmacroDataPadTxCtrl, 16:16, scratch190, 28:28);
    s(EmcPmacroDataPadTxCtrl, 21:20, scratch190, 30:29);
    s(EmcPmacroDataPadTxCtrl, 24:24, scratch190, 31:31);
    s(EmcPmacroDataPadTxCtrl, 27:25, scratch191, 2:0);

    s(EmcPinGpio, 1:0, scratch8, 31:30);
    s(EmcPinGpioEn, 1:0, scratch9, 31:30);
    s(EmcDevSelect, 1:0, scratch10, 31:30);
    s(EmcZcalWarmColdBootEnables, 1:0, scratch11, 31:30);
    s(EmcCfgDigDllPeriodWarmBoot, 1:0, scratch12, 31:30);
    s32(EmcBctSpare13, scratch31);
    s32(EmcBctSpare12, scratch32);
    s32(EmcBctSpare7, scratch33);
    s32(EmcBctSpare6, scratch40);
    s32(EmcBctSpare5, scratch42);
    s32(EmcBctSpare4, scratch44);
    s32(EmcBctSpare3, scratch45);
    s32(EmcBctSpare2, scratch46);
    s32(EmcBctSpare1, scratch47);
    s32(EmcBctSpare0, scratch48);
    s32(EmcBctSpare9, scratch50);
    s32(EmcBctSpare8, scratch51);
    s32(BootRomPatchData, scratch56);
    s32(BootRomPatchControl, scratch57);
    s(McClkenOverrideAllWarmBoot, 0:0, scratch58, 31:31);
    s(EmcClkenOverrideAllWarmBoot, 0:0, scratch59, 30:30);
    s(EmcMrsWarmBootEnable, 0:0, scratch59, 31:31);
    s(ClearClk2Mc1, 0:0, scratch60, 30:30);
    s(EmcWarmBootExtraModeRegWriteEnable, 0:0, scratch60, 31:31);
    s(ClkRstControllerPllmMisc2OverrideEnable, 0:0, scratch61, 30:30);
    s(EmcDbgWriteMux, 0:0, scratch61, 31:31);
    s(EmcExtraRefreshNum, 2:0, scratch62, 31:29);
    s(PmcIoDpd3ReqWait, 2:0, scratch68, 30:28);
    s(AhbArbitrationXbarCtrlMemInitDone, 0:0, scratch68, 31:31);
    s(MemoryType, 2:0, scratch69, 30:28);
    s(PmcIoDpd4ReqWait, 2:0, scratch70, 30:28);
    s(EmcTimingControlWait, 7:0, scratch86, 31:24);
    s(EmcZcalWarmBootWait, 7:0, scratch87, 31:24);
    s(WarmBootWait, 7:0, scratch88, 31:24);
    s(EmcPinProgramWait, 7:0, scratch89, 31:24);
    s(EmcAutoCalWait, 9:0, scratch101, 31:22);
    s(SwizzleRankByteEncode, 15:0, scratch190, 15:0);

    switch (params->MemoryType) {
        case NvBootMemoryType_LpDdr2:
        case NvBootMemoryType_LpDdr4:
            s(EmcMrwLpddr2ZcalWarmBoot, 23:16, scratch5, 7:0);
            s(EmcMrwLpddr2ZcalWarmBoot, 7:0, scratch5, 15:8);
            s(EmcWarmBootMrwExtra, 23:16, scratch5, 23:16);
            s(EmcWarmBootMrwExtra, 7:0, scratch5, 31:24);
            s(EmcMrwLpddr2ZcalWarmBoot, 31:30, scratch6, 1:0);
            s(EmcWarmBootMrwExtra, 31:30, scratch6, 3:2);
            s(EmcMrwLpddr2ZcalWarmBoot, 27:26, scratch6, 5:4);
            s(EmcWarmBootMrwExtra, 27:26, scratch6, 7:6);
            s(EmcMrw6, 27:0, scratch8, 27:0);
            s(EmcMrw6, 31:30, scratch8, 29:28);
            s(EmcMrw8, 27:0, scratch9, 27:0);
            s(EmcMrw8, 31:30, scratch9, 29:28);
            s(EmcMrw9, 27:0, scratch10, 27:0);
            s(EmcMrw9, 31:30, scratch10, 29:28);
            s(EmcMrw10, 27:0, scratch11, 27:0);
            s(EmcMrw10, 31:30, scratch11, 29:28);
            s(EmcMrw12, 27:0, scratch12, 27:0);
            s(EmcMrw12, 31:30, scratch12, 29:28);
            s(EmcMrw13, 27:0, scratch13, 27:0);
            s(EmcMrw13, 31:30, scratch13, 29:28);
            s(EmcMrw14, 27:0, scratch14, 27:0);
            s(EmcMrw14, 31:30, scratch14, 29:28);
            s(EmcMrw1, 7:0, scratch15, 7:0);
            s(EmcMrw1, 23:16, scratch15, 15:8);
            s(EmcMrw1, 27:26, scratch15, 17:16);
            s(EmcMrw1, 31:30, scratch15, 19:18);
            s(EmcWarmBootMrwExtra, 7:0, scratch16, 7:0);
            s(EmcWarmBootMrwExtra, 23:16, scratch16, 15:8);
            s(EmcWarmBootMrwExtra, 27:26, scratch16, 17:16);
            s(EmcWarmBootMrwExtra, 31:30, scratch16, 19:18);
            s(EmcMrw2, 7:0, scratch17, 7:0);
            s(EmcMrw2, 23:16, scratch17, 15:8);
            s(EmcMrw2, 27:26, scratch17, 17:16);
            s(EmcMrw2, 31:30, scratch17, 19:18);
            s(EmcMrw3, 7:0, scratch18, 7:0);
            s(EmcMrw3, 23:16, scratch18, 15:8);
            s(EmcMrw3, 27:26, scratch18, 17:16);
            s(EmcMrw3, 31:30, scratch18, 19:18);
            s(EmcMrw4, 7:0, scratch19, 7:0);
            s(EmcMrw4, 23:16, scratch19, 15:8);
            s(EmcMrw4, 27:26, scratch19, 17:16);
            s(EmcMrw4, 31:30, scratch19, 19:18);
            break;
        case NvBootMemoryType_Ddr3:
            s(EmcMrs, 13:0, scratch5, 13:0);
            s(EmcEmrs, 13:0, scratch5, 27:14);
            s(EmcMrs, 21:20, scratch5, 29:28);
            s(EmcMrs, 31:30, scratch5, 31:30);
            s(EmcEmrs2, 13:0, scratch8, 13:0);
            s(EmcEmrs3, 13:0, scratch8, 27:14);
            s(EmcEmrs, 21:20, scratch8, 29:28);
            s(EmcWarmBootMrsExtra, 13:0, scratch9, 13:0);
            s(EmcEmrs, 31:30, scratch9, 15:14);
            s(EmcEmrs2, 21:20, scratch9, 17:16);
            s(EmcEmrs2, 31:30, scratch9, 19:18);
            s(EmcEmrs3, 21:20, scratch9, 21:20);
            s(EmcEmrs3, 31:30, scratch9, 23:22);
            s(EmcWarmBootMrsExtra, 31:30, scratch9, 25:24);
            s(EmcWarmBootMrsExtra, 21:20, scratch9, 27:26);
            s(EmcZqCalDdr3WarmBoot, 31:30, scratch9, 29:28);
            s(EmcMrs, 27:26, scratch10, 1:0);
            s(EmcEmrs, 27:26, scratch10, 3:2);
            s(EmcEmrs2, 27:26, scratch10, 5:4);
            s(EmcEmrs3, 27:26, scratch10, 7:6);
            s(EmcWarmBootMrsExtra, 27:27, scratch10, 8:8);
            s(EmcWarmBootMrsExtra, 26:26, scratch10, 9:9);
            s(EmcZqCalDdr3WarmBoot, 0:0, scratch10, 10:10);
            s(EmcZqCalDdr3WarmBoot, 4:4, scratch10, 11:11);
            break;
        default: break;
    }

    s32(EmcCmdMappingByte, secure_scratch8);
    s32(EmcPmacroBrickMapping0, secure_scratch9);
    s32(EmcPmacroBrickMapping1, secure_scratch10);
    s32(EmcPmacroBrickMapping2, secure_scratch11);
    s32(McVideoProtectGpuOverride0, secure_scratch12);
    s(EmcCmdMappingCmd0_0, 6:0, secure_scratch13, 6:0);
    s(EmcCmdMappingCmd0_0, 14:8, secure_scratch13, 13:7);
    s(EmcCmdMappingCmd0_0, 22:16, secure_scratch13, 20:14);
    s(EmcCmdMappingCmd0_0, 30:24, secure_scratch13, 27:21);
    s(McVideoProtectBomAdrHi, 1:0, secure_scratch13, 29:28);
    s(McVideoProtectWriteAccess, 1:0, secure_scratch13, 31:30);
    s(EmcCmdMappingCmd0_1, 6:0, secure_scratch14, 6:0);
    s(EmcCmdMappingCmd0_1, 14:8, secure_scratch14, 13:7);
    s(EmcCmdMappingCmd0_1, 22:16, secure_scratch14, 20:14);
    s(EmcCmdMappingCmd0_1, 30:24, secure_scratch14, 27:21);
    s(McSecCarveoutAdrHi, 1:0, secure_scratch14, 29:28);
    s(McMtsCarveoutAdrHi, 1:0, secure_scratch14, 31:30);
    s(EmcCmdMappingCmd1_0, 6:0, secure_scratch15, 6:0);
    s(EmcCmdMappingCmd1_0, 14:8, secure_scratch15, 13:7);
    s(EmcCmdMappingCmd1_0, 22:16, secure_scratch15, 20:14);
    s(EmcCmdMappingCmd1_0, 30:24, secure_scratch15, 27:21);
    s(McGeneralizedCarveout5BomHi, 1:0, secure_scratch15, 29:28);
    s(McGeneralizedCarveout3BomHi, 1:0, secure_scratch15, 31:30);
    s(EmcCmdMappingCmd1_1, 6:0, secure_scratch16, 6:0);
    s(EmcCmdMappingCmd1_1, 14:8, secure_scratch16, 13:7);
    s(EmcCmdMappingCmd1_1, 22:16, secure_scratch16, 20:14);
    s(EmcCmdMappingCmd1_1, 30:24, secure_scratch16, 27:21);
    s(McGeneralizedCarveout2BomHi, 1:0, secure_scratch16, 29:28);
    s(McGeneralizedCarveout4BomHi, 1:0, secure_scratch16, 31:30);
    s(EmcCmdMappingCmd2_0, 6:0, secure_scratch17, 6:0);
    s(EmcCmdMappingCmd2_0, 14:8, secure_scratch17, 13:7);
    s(EmcCmdMappingCmd2_0, 22:16, secure_scratch17, 20:14);
    s(EmcCmdMappingCmd2_0, 30:24, secure_scratch17, 27:21);
    s(McGeneralizedCarveout1BomHi, 1:0, secure_scratch17, 29:28);
    s(EmcAdrCfg, 0:0, secure_scratch17, 30:30);
    s(EmcFbioSpare, 1:1, secure_scratch17, 31:31);
    s(EmcCmdMappingCmd2_1, 6:0, secure_scratch18, 6:0);
    s(EmcCmdMappingCmd2_1, 14:8, secure_scratch18, 13:7);
    s(EmcCmdMappingCmd2_1, 22:16, secure_scratch18, 20:14);
    s(EmcCmdMappingCmd2_1, 30:24, secure_scratch18, 27:21);
    s(EmcFbioCfg8, 15:15, secure_scratch18, 28:28);
    s(McEmemAdrCfg, 0:0, secure_scratch18, 29:29);
    s(McSecCarveoutProtectWriteAccess, 0:0, secure_scratch18, 30:30);
    s(McMtsCarveoutRegCtrl, 0:0, secure_scratch18, 31:31);
    s(EmcCmdMappingCmd3_0, 6:0, secure_scratch19, 6:0);
    s(EmcCmdMappingCmd3_0, 14:8, secure_scratch19, 13:7);
    s(EmcCmdMappingCmd3_0, 22:16, secure_scratch19, 20:14);
    s(EmcCmdMappingCmd3_0, 30:24, secure_scratch19, 27:21);
    s(McGeneralizedCarveout2Cfg0, 6:3, secure_scratch19, 31:28);
    s(EmcCmdMappingCmd3_1, 6:0, secure_scratch20, 6:0);
    s(EmcCmdMappingCmd3_1, 14:8, secure_scratch20, 13:7);
    s(EmcCmdMappingCmd3_1, 22:16, secure_scratch20, 20:14);
    s(EmcCmdMappingCmd3_1, 30:24, secure_scratch20, 27:21);
    s(McGeneralizedCarveout2Cfg0, 10:7, secure_scratch20, 31:28);
    s(McGeneralizedCarveout4Cfg0, 26:0, secure_scratch39, 26:0);
    s(McGeneralizedCarveout2Cfg0, 17:14, secure_scratch39, 30:27);
    s(McVideoProtectVprOverride, 0:0, secure_scratch39, 31:31);
    s(McGeneralizedCarveout5Cfg0, 26:0, secure_scratch40, 26:0);
    s(McGeneralizedCarveout2Cfg0, 21:18, secure_scratch40, 30:27);
    s(McVideoProtectVprOverride, 1:1, secure_scratch40, 31:31);
    s(EmcCmdMappingCmd0_2, 6:0, secure_scratch41, 6:0);
    s(EmcCmdMappingCmd0_2, 14:8, secure_scratch41, 13:7);
    s(EmcCmdMappingCmd0_2, 22:16, secure_scratch41, 20:14);
    s(EmcCmdMappingCmd0_2, 27:24, secure_scratch41, 24:21);
    s(McGeneralizedCarveout1Cfg0, 6:3, secure_scratch41, 28:25);
    s(McGeneralizedCarveout2Cfg0, 13:11, secure_scratch41, 31:29);
    s(EmcCmdMappingCmd1_2, 6:0, secure_scratch42, 6:0);
    s(EmcCmdMappingCmd1_2, 14:8, secure_scratch42, 13:7);
    s(EmcCmdMappingCmd1_2, 22:16, secure_scratch42, 20:14);
    s(EmcCmdMappingCmd1_2, 27:24, secure_scratch42, 24:21);
    s(McGeneralizedCarveout1Cfg0, 13:7, secure_scratch42, 31:25);
    s(EmcCmdMappingCmd2_2, 6:0, secure_scratch43, 6:0);
    s(EmcCmdMappingCmd2_2, 14:8, secure_scratch43, 13:7);
    s(EmcCmdMappingCmd2_2, 22:16, secure_scratch43, 20:14);
    s(EmcCmdMappingCmd2_2, 27:24, secure_scratch43, 24:21);
    s(McGeneralizedCarveout1Cfg0, 17:14, secure_scratch43, 28:25);
    s(McGeneralizedCarveout3Cfg0, 13:11, secure_scratch43, 31:29);
    s(EmcCmdMappingCmd3_2, 6:0, secure_scratch44, 6:0);
    s(EmcCmdMappingCmd3_2, 14:8, secure_scratch44, 13:7);
    s(EmcCmdMappingCmd3_2, 22:16, secure_scratch44, 20:14);
    s(EmcCmdMappingCmd3_2, 27:24, secure_scratch44, 24:21);
    s(McGeneralizedCarveout1Cfg0, 21:18, secure_scratch44, 28:25);
    s(McVideoProtectVprOverride, 3:2, secure_scratch44, 30:29);
    s(McVideoProtectVprOverride, 6:6, secure_scratch44, 31:31);
    s(McEmemAdrCfgChannelMask, 31:9, secure_scratch45, 22:0);
    s(McEmemAdrCfgDev0, 2:0, secure_scratch45, 25:23);
    s(McEmemAdrCfgDev0, 9:8, secure_scratch45, 27:26);
    s(McEmemAdrCfgDev0, 19:16, secure_scratch45, 31:28);
    s(McEmemAdrCfgBankMask0, 31:10, secure_scratch46, 21:0);
    s(McEmemAdrCfgDev1, 2:0, secure_scratch46, 24:22);
    s(McEmemAdrCfgDev1, 9:8, secure_scratch46, 26:25);
    s(McEmemAdrCfgDev1, 19:16, secure_scratch46, 30:27);
    s(McVideoProtectVprOverride, 7:7, secure_scratch46, 31:31);
    s(McEmemAdrCfgBankMask1, 31:10, secure_scratch47, 21:0);
    s(McGeneralizedCarveout3Cfg0, 10:3, secure_scratch47, 29:22);
    s(McVideoProtectVprOverride, 9:8, secure_scratch47, 31:30);
    s(McEmemAdrCfgBankMask2, 31:10, secure_scratch48, 21:0);
    s(McGeneralizedCarveout3Cfg0, 21:14, secure_scratch48, 29:22);
    s(McVideoProtectVprOverride, 11:11, secure_scratch48, 30:30);
    s(McVideoProtectVprOverride, 14:14, secure_scratch48, 31:31);
    s(McVideoProtectGpuOverride1, 15:0, secure_scratch49, 15:0);
    s(McEmemCfg, 13:0, secure_scratch49, 29:16);
    s(McEmemCfg, 31:31, secure_scratch49, 30:30);
    s(McVideoProtectVprOverride, 15:15, secure_scratch49, 31:31);
    s(McGeneralizedCarveout3Bom, 31:17, secure_scratch50, 14:0);
    s(McGeneralizedCarveout1Bom, 31:17, secure_scratch50, 29:15);
    s(McVideoProtectVprOverride, 18:17, secure_scratch50, 31:30);
    s(McGeneralizedCarveout4Bom, 31:17, secure_scratch51, 14:0);
    s(McGeneralizedCarveout2Bom, 31:17, secure_scratch51, 29:15);
    s(McVideoProtectVprOverride, 20:19, secure_scratch51, 31:30);
    s(McGeneralizedCarveout5Bom, 31:17, secure_scratch52, 14:0);
    s(McVideoProtectBom, 31:20, secure_scratch52, 26:15);
    s(McVideoProtectVprOverride, 23:21, secure_scratch52, 29:27);
    s(McVideoProtectVprOverride, 26:26, secure_scratch52, 30:30);
    s(McVideoProtectVprOverride, 29:29, secure_scratch52, 31:31);
    s(McVideoProtectSizeMb, 11:0, secure_scratch53, 11:0);
    s(McSecCarveoutBom, 31:20, secure_scratch53, 23:12);
    s(McVideoProtectVprOverride, 31:30, secure_scratch53, 25:24);
    s(McVideoProtectVprOverride1, 1:0, secure_scratch53, 27:26);
    s(McVideoProtectVprOverride1, 7:4, secure_scratch53, 31:28);
    s(McSecCarveoutSizeMb, 11:0, secure_scratch54, 11:0);
    s(McMtsCarveoutBom, 31:20, secure_scratch54, 23:12);
    s(McVideoProtectVprOverride1, 15:8, secure_scratch54, 31:24);
    s(McMtsCarveoutSizeMb, 11:0, secure_scratch55, 11:0);
    s(McGeneralizedCarveout4Size128kb, 11:0, secure_scratch55, 23:12);
    s(McVideoProtectVprOverride1, 16:16, secure_scratch55, 24:24);
    s(McGeneralizedCarveout2Cfg0, 2:0, secure_scratch55, 27:25);
    s(McGeneralizedCarveout2Cfg0, 25:22, secure_scratch55, 31:28);
    s(McGeneralizedCarveout3Size128kb, 11:0, secure_scratch56, 11:0);
    s(McGeneralizedCarveout2Size128kb, 11:0, secure_scratch56, 23:12);
    s(McGeneralizedCarveout2Cfg0, 26:26, secure_scratch56, 24:24);
    s(McGeneralizedCarveout1Cfg0, 2:0, secure_scratch56, 27:25);
    s(McGeneralizedCarveout1Cfg0, 25:22, secure_scratch56, 31:28);
    s(McGeneralizedCarveout1Size128kb, 11:0, secure_scratch57, 11:0);
    s(McGeneralizedCarveout5Size128kb, 11:0, secure_scratch57, 23:12);
    s(McGeneralizedCarveout1Cfg0, 26:26, secure_scratch57, 24:24);
    s(McGeneralizedCarveout3Cfg0, 2:0, secure_scratch57, 27:25);
    s(McGeneralizedCarveout3Cfg0, 25:22, secure_scratch57, 31:28);
    s(McGeneralizedCarveout3Cfg0, 26:26, secure_scratch58, 0:0);

    s32(McGeneralizedCarveout1Access0, secure_scratch59);
    s32(McGeneralizedCarveout1Access1, secure_scratch60);
    s32(McGeneralizedCarveout1Access2, secure_scratch61);
    s32(McGeneralizedCarveout1Access3, secure_scratch62);
    s32(McGeneralizedCarveout1Access4, secure_scratch63);
    s32(McGeneralizedCarveout2Access0, secure_scratch64);
    s32(McGeneralizedCarveout2Access1, secure_scratch65);
    s32(McGeneralizedCarveout2Access2, secure_scratch66);
    s32(McGeneralizedCarveout2Access3, secure_scratch67);
    s32(McGeneralizedCarveout2Access4, secure_scratch68);
    s32(McGeneralizedCarveout3Access0, secure_scratch69);
    s32(McGeneralizedCarveout3Access1, secure_scratch70);
    s32(McGeneralizedCarveout3Access2, secure_scratch71);
    s32(McGeneralizedCarveout3Access3, secure_scratch72);
    s32(McGeneralizedCarveout3Access4, secure_scratch73);
    s32(McGeneralizedCarveout4Access0, secure_scratch74);
    s32(McGeneralizedCarveout4Access1, secure_scratch75);
    s32(McGeneralizedCarveout4Access2, secure_scratch76);
    s32(McGeneralizedCarveout4Access3, secure_scratch77);
    s32(McGeneralizedCarveout4Access4, secure_scratch78);
    s32(McGeneralizedCarveout5Access0, secure_scratch79);
    s32(McGeneralizedCarveout5Access1, secure_scratch80);
    s32(McGeneralizedCarveout5Access2, secure_scratch81);
    s32(McGeneralizedCarveout5Access3, secure_scratch82);
    s32(McGeneralizedCarveout1ForceInternalAccess0, secure_scratch84);
    s32(McGeneralizedCarveout1ForceInternalAccess1, secure_scratch85);
    s32(McGeneralizedCarveout1ForceInternalAccess2, secure_scratch86);
    s32(McGeneralizedCarveout1ForceInternalAccess3, secure_scratch87);
    s32(McGeneralizedCarveout1ForceInternalAccess4, secure_scratch88);
    s32(McGeneralizedCarveout2ForceInternalAccess0, secure_scratch89);
    s32(McGeneralizedCarveout2ForceInternalAccess1, secure_scratch90);
    s32(McGeneralizedCarveout2ForceInternalAccess2, secure_scratch91);
    s32(McGeneralizedCarveout2ForceInternalAccess3, secure_scratch92);
    s32(McGeneralizedCarveout2ForceInternalAccess4, secure_scratch93);
    s32(McGeneralizedCarveout3ForceInternalAccess0, secure_scratch94);
    s32(McGeneralizedCarveout3ForceInternalAccess1, secure_scratch95);
    s32(McGeneralizedCarveout3ForceInternalAccess2, secure_scratch96);
    s32(McGeneralizedCarveout3ForceInternalAccess3, secure_scratch97);
    s32(McGeneralizedCarveout3ForceInternalAccess4, secure_scratch98);
    s32(McGeneralizedCarveout4ForceInternalAccess0, secure_scratch99);
    s32(McGeneralizedCarveout4ForceInternalAccess1, secure_scratch100);
    s32(McGeneralizedCarveout4ForceInternalAccess2, secure_scratch101);
    s32(McGeneralizedCarveout4ForceInternalAccess3, secure_scratch102);
    s32(McGeneralizedCarveout4ForceInternalAccess4, secure_scratch103);
    s32(McGeneralizedCarveout5ForceInternalAccess0, secure_scratch104);
    s32(McGeneralizedCarveout5ForceInternalAccess1, secure_scratch105);
    s32(McGeneralizedCarveout5ForceInternalAccess2, secure_scratch106);
    s32(McGeneralizedCarveout5ForceInternalAccess3, secure_scratch107);

    c32(0, scratch2);
    s(PllMInputDivider, 7:0, scratch2, 7:0);
    s(PllMFeedbackDivider, 7:0, scratch2, 15:8);
    s(PllMPostDivider, 4:0, scratch2, 20:16);
    s(PllMKVCO, 0:0, scratch2, 21:21);
    s(PllMKCP, 1:0, scratch2, 23:22);

    c32(0, scratch35);
    s(PllMSetupControl, 15:0, scratch35, 15:0);

    c32(0, scratch3);
    s(PllMInputDivider, 7:0, scratch3, 7:0);
    c(0x3e, scratch3, 15:8);
    c(0, scratch3, 20:16);
    s(PllMKVCO, 0:0, scratch3, 21:21);
    s(PllMKCP, 1:0, scratch3, 23:22);

    c32(0, scratch36);
    s(PllMSetupControl, 23:0, scratch36, 23:0);

    c32(0, scratch4);
    s(PllMStableTime, 9:0, scratch4, 9:0);
}

static void sdram_save_params_mariko(const void *save_params) {
    const sdram_params_mariko_t *params = (const sdram_params_mariko_t *)save_params;
    volatile tegra_pmc_t *pmc = pmc_get_regs();

#define pack(src, src_bits, dst, dst_bits) { \
    uint32_t mask = 0xffffffff >> (31 - ((1 ? src_bits) - (0 ? src_bits))); \
    dst &= ~(mask << (0 ? dst_bits)); \
    dst |= ((src >> (0 ? src_bits)) & mask) << (0 ? dst_bits); \
}

#define s(param, src_bits, pmcreg, dst_bits) \
    pack(params->param, src_bits, pmc->pmcreg, dst_bits)

#define c(value, pmcreg, dst_bits) \
    pack(value, (1 ? dst_bits) - (0 ? dst_bits) : 0, pmc->pmcreg, dst_bits)

/* 32 bits version of s macro. */
#define s32(param, pmcreg) pmc->pmcreg = params->param

/* 32 bits version c macro. */
#define c32(value, pmcreg) pmc->pmcreg = value

    /* Patch carveout parameters. */
    *(volatile uint32_t *)&params->McGeneralizedCarveout1Cfg0 = 0;
    *(volatile uint32_t *)&params->McGeneralizedCarveout2Cfg0 = 0;
    *(volatile uint32_t *)&params->McGeneralizedCarveout3Cfg0 = 0;
    *(volatile uint32_t *)&params->McGeneralizedCarveout4Cfg0 = 0;
    *(volatile uint32_t *)&params->McGeneralizedCarveout5Cfg0 = 0;

    /* Patch SDRAM parameters. */
    *(volatile uint32_t *)&params->SwizzleRankByteEncode = 0;
    uint32_t t0 = params->EmcSwizzleRank0Byte0 << 5 >> 29 > params->EmcSwizzleRank0Byte0 << 1 >> 29;
    uint32_t t1 = (t0 & 0xFFFFFFEF) | ((params->EmcSwizzleRank1Byte0 << 5 >> 29 > params->EmcSwizzleRank1Byte0 << 1 >> 29) << 4);
    uint32_t t2 = (t1 & 0xFFFFFFFD) | ((params->EmcSwizzleRank0Byte1 << 5 >> 29 > params->EmcSwizzleRank0Byte1 << 1 >> 29) << 1);
    uint32_t t3 = (t2 & 0xFFFFFFDF) | ((params->EmcSwizzleRank1Byte1 << 5 >> 29 > params->EmcSwizzleRank1Byte1 << 1 >> 29) << 5);
    uint32_t t4 = (t3 & 0xFFFFFFFB) | ((params->EmcSwizzleRank0Byte2 << 5 >> 29 > params->EmcSwizzleRank0Byte2 << 1 >> 29) << 2);
    uint32_t t5 = (t4 & 0xFFFFFFBF) | ((params->EmcSwizzleRank1Byte2 << 5 >> 29 > params->EmcSwizzleRank1Byte2 << 1 >> 29) << 6);
    uint32_t t6 = (t5 & 0xFFFFFFF7) | ((params->EmcSwizzleRank0Byte3 << 5 >> 29 > params->EmcSwizzleRank0Byte3 << 1 >> 29) << 3);
    uint32_t t7 = (t6 & 0xFFFFFF7F) | ((params->EmcSwizzleRank1Byte3 << 5 >> 29 > params->EmcSwizzleRank1Byte3 << 1 >> 29) << 7);
    *(volatile uint32_t *)&params->SwizzleRankByteEncode = t7;
    *(volatile uint32_t *)&params->EmcBctSpare2 = 0x40000DD8;
    *(volatile uint32_t *)&params->EmcBctSpare3 = t7;

    s(EmcClockSource, 7:0, scratch6, 15:8);
    s(EmcClockSourceDll, 7:0, scratch6, 23:16);
    s(EmcClockSource, 31:29, scratch6, 26:24);
    s(EmcClockSourceDll, 31:29, scratch6, 29:27);
    s(EmcClockSourceDll, 11:10, scratch6, 31:30);
    s(ClkRstControllerPllmMisc2Override, 9:8, scratch7, 1:0);
    s(ClkRstControllerPllmMisc2Override, 2:1, scratch7, 3:2);
    s(EmcZqCalLpDdr4WarmBoot, 31:30, scratch7, 5:4);
    s(EmcClockSource, 27:27, scratch7, 6:6);
    s(EmcClockSource, 26:26, scratch7, 7:7);
    s(EmcClockSource, 15:15, scratch7, 8:8);
    s(EmcClockSource, 25:25, scratch7, 9:9);
    s(EmcClockSource, 19:19, scratch7, 10:10);
    s(EmcClockSource, 20:20, scratch7, 11:11);
    s(EmcClockSource, 16:16, scratch7, 12:12);
    s(ClkRstControllerPllmMisc2Override, 13:13, scratch7, 13:13);
    s(ClkRstControllerPllmMisc2Override, 12:12, scratch7, 14:14);
    s(ClkRstControllerPllmMisc2Override, 11:11, scratch7, 15:15);
    s(ClkRstControllerPllmMisc2Override, 10:10, scratch7, 16:16);
    s(ClkRstControllerPllmMisc2Override, 5:5, scratch7, 17:17);
    s(ClkRstControllerPllmMisc2Override, 4:4, scratch7, 18:18);
    s(ClkRstControllerPllmMisc2Override, 3:3, scratch7, 19:19);
    s(ClkRstControllerPllmMisc2Override, 0:0, scratch7, 20:20);
    s(EmcZqCalLpDdr4WarmBoot, 1:0, scratch7, 22:21);
    s(EmcZqCalLpDdr4WarmBoot, 4:4, scratch7, 23:23);
    s(EmcRc, 7:0, scratch7, 31:24);
    s(EmcPmacroBgBiasCtrl0, 13:12, scratch8, 31:30);
    s(EmcFdpdCtrlCmdNoRamp, 0:0, scratch14, 30:30);
    s(EmcCfgPipeClk, 0:0, scratch14, 31:31);
    s(EmcQRst, 6:0, scratch15, 26:20);
    s(EmcQRst, 20:16, scratch15, 31:27);
    s(EmcPmacroCmdTxDrv, 5:0, scratch16, 25:20);
    s(EmcPmacroCmdTxDrv, 13:8, scratch16, 31:26);
    s(EmcFbioCfg8, 27:16, scratch17, 31:20);
    s(EmcTxsrDll, 11:0, scratch18, 31:20);
    s(EmcTxdsrvttgen, 11:0, scratch19, 31:20);
    s32(EmcCfgRsv,scratch22);
    s32(EmcAutoCalConfig, scratch23);
    s32(EmcAutoCalVrefSel0, scratch24);
    s32(EmcPmacroBrickCtrlRfu1, scratch25);
    s32(EmcPmacroBrickCtrlRfu2, scratch26);
    s32(EmcPmcScratch1, scratch27);
    s32(EmcPmcScratch2, scratch28);
    s32(EmcPmcScratch3, scratch29);
    s32(EmcPmacroPerbitRfuCtrl0, scratch30);
    s32(EmcPmacroPerbitRfuCtrl1, scratch31);
    s32(EmcPmacroPerbitRfuCtrl2, scratch32);
    s32(EmcPmacroPerbitRfuCtrl3, scratch33);
    s32(EmcPmacroPerbitRfuCtrl4, scratch40);
    s32(EmcPmacroPerbitRfuCtrl5, scratch42);
    s32(McEmemArbDaTurns, scratch44);
    s(EmcFbioSpare, 31:24, scratch64, 7:0);
    s(EmcFbioSpare, 23:16, scratch64, 15:8);
    s(EmcFbioSpare, 15:8, scratch64, 23:16);
    s(EmcFbioSpare, 7:2, scratch64, 29:24);
    s(EmcFbioSpare, 0:0, scratch64, 30:30);
    s(McEmemArbMisc2, 0:0, scratch64, 31:31);
    s(McEmemArbMisc0, 14:0, scratch65, 14:0);
    s(McEmemArbMisc0, 30:16, scratch65, 29:15);
    s(McDaCfg0, 0:0, scratch65, 30:30);
    s(EmcFdpdCtrlCmd, 16:0, scratch66, 16:0);
    s(EmcFdpdCtrlCmd, 31:20, scratch66, 28:17);
    s(EmcAutoCalConfig2, 27:0, scratch67, 27:0);
    s(EmcBurstRefreshNum, 3:0, scratch67, 31:28);
    s(EmcCfgDigDll, 10:0, scratch68, 10:0);
    s(EmcCfgDigDll, 25:12, scratch68, 24:11);
    s(EmcCfgDigDll, 27:27, scratch68, 25:25);
    s(EmcCfgDigDll, 31:30, scratch68, 27:26);
    s(EmcTppd, 3:0, scratch68, 31:28);
    s(EmcFdpdCtrlDq, 16:0, scratch69, 16:0);
    s(EmcFdpdCtrlDq, 28:20, scratch69, 25:17);
    s(EmcFdpdCtrlDq, 31:30, scratch69, 27:26);
    s(EmcR2r, 3:0, scratch69, 31:28);
    s(EmcPmacroIbVrefDq_0, 6:0, scratch70, 6:0);
    s(EmcPmacroIbVrefDq_0, 14:8, scratch70, 13:7);
    s(EmcPmacroIbVrefDq_0, 22:16, scratch70, 20:14);
    s(EmcPmacroIbVrefDq_0, 30:24, scratch70, 27:21);
    s(EmcW2w, 3:0, scratch70, 31:28);
    s(EmcPmacroIbVrefDq_1, 6:0, scratch71, 6:0);
    s(EmcPmacroIbVrefDq_1, 14:8, scratch71, 13:7);
    s(EmcPmacroIbVrefDq_1, 22:16, scratch71, 20:14);
    s(EmcPmacroIbVrefDq_1, 30:24, scratch71, 27:21);
    s(EmcPmacroVttgenCtrl0, 19:16, scratch71, 31:28);
    s(EmcPmacroIbVrefDqs_0, 6:0, scratch72, 6:0);
    s(EmcPmacroIbVrefDqs_0, 14:8, scratch72, 13:7);
    s(EmcPmacroIbVrefDqs_0, 22:16, scratch72, 20:14);
    s(EmcPmacroIbVrefDqs_0, 30:24, scratch72, 27:21);
    s(EmcPmacroIbVrefDqs_0, 6:0, scratch73, 6:0);
    s(EmcPmacroIbVrefDqs_0, 14:8, scratch73, 13:7);
    s(EmcPmacroIbVrefDqs_0, 22:16, scratch73, 20:14);
    s(EmcPmacroIbVrefDqs_0, 30:24, scratch73, 27:21);
    s(EmcPmacroDdllShortCmd_0, 6:0, scratch74, 6:0);
    s(EmcPmacroDdllShortCmd_0, 14:8, scratch74, 13:7);
    s(EmcPmacroDdllShortCmd_0, 22:16, scratch74, 20:14);
    s(EmcPmacroDdllShortCmd_0, 30:24, scratch74, 27:21);
    s(EmcPmacroDdllShortCmd_1, 6:0, scratch75, 6:0);
    s(EmcPmacroDdllShortCmd_1, 14:8, scratch75, 13:7);
    s(EmcPmacroDdllShortCmd_1, 22:16, scratch75, 20:14);
    s(EmcPmacroDdllShortCmd_1, 30:24, scratch75, 27:21);
    s(EmcPmacroDllCfg0, 29:4, scratch76, 25:0);
    s(EmcRp, 5:0, scratch76, 31:26);
    s(EmcPmacroTxPwrd0, 10:0, scratch77, 10:0);
    s(EmcPmacroTxPwrd0, 13:12, scratch77, 12:11);
    s(EmcPmacroTxPwrd0, 26:16, scratch77, 23:13);
    s(EmcPmacroTxPwrd0, 29:28, scratch77, 25:24);
    s(EmcR2w, 5:0, scratch77, 31:26);
    s(EmcPmacroTxPwrd1, 10:0, scratch78, 10:0);
    s(EmcPmacroTxPwrd1, 13:12, scratch78, 12:11);
    s(EmcPmacroTxPwrd1, 26:16, scratch78, 23:13);
    s(EmcPmacroTxPwrd1, 29:28, scratch78, 25:24);
    s(EmcW2r, 5:0, scratch78, 31:26);
    s(EmcPmacroTxPwrd2, 10:0, scratch79, 10:0);
    s(EmcPmacroTxPwrd2, 13:12, scratch79, 12:11);
    s(EmcPmacroTxPwrd2, 26:16, scratch79, 23:13);
    s(EmcPmacroTxPwrd2, 29:28, scratch79, 25:24);
    s(EmcR2p, 5:0, scratch79, 31:26);
    s(EmcPmacroTxPwrd3, 10:0, scratch80, 10:0);
    s(EmcPmacroTxPwrd3, 13:12, scratch80, 12:11);
    s(EmcPmacroTxPwrd3, 26:16, scratch80, 23:13);
    s(EmcPmacroTxPwrd3, 29:28, scratch80, 25:24);
    s(EmcCcdmw, 5:0, scratch80, 31:26);
    s(EmcPmacroTxPwrd4, 10:0, scratch81, 10:0);
    s(EmcPmacroTxPwrd4, 13:12, scratch81, 12:11);
    s(EmcPmacroTxPwrd4, 26:16, scratch81, 23:13);
    s(EmcPmacroTxPwrd4, 29:28, scratch81, 25:24);
    s(EmcRdRcd, 5:0, scratch81, 31:26);
    s(EmcPmacroTxPwrd5, 10:0, scratch82, 10:0);
    s(EmcPmacroTxPwrd5, 13:12, scratch82, 12:11);
    s(EmcPmacroTxPwrd5, 26:16, scratch82, 23:13);
    s(EmcPmacroTxPwrd5, 29:28, scratch82, 25:24);
    s(EmcWrRcd, 5:0, scratch82, 31:26);
    s(EmcAutoCalChannel, 5:0, scratch83, 5:0);
    s(EmcAutoCalChannel, 11:8, scratch83, 9:6);
    s(EmcAutoCalChannel, 27:16, scratch83, 21:10);
    s(EmcAutoCalChannel, 31:29, scratch83, 24:22);
    s(EmcConfigSampleDelay, 6:0, scratch83, 31:25);
    s(EmcPmacroRxTerm, 5:0, scratch84, 5:0);
    s(EmcPmacroRxTerm, 13:8, scratch84, 11:6);
    s(EmcPmacroRxTerm, 21:16, scratch84, 17:12);
    s(EmcPmacroRxTerm, 29:24, scratch84, 23:18);
    s(EmcSelDpdCtrl, 5:2, scratch84, 27:24);
    s(EmcSelDpdCtrl, 8:8, scratch84, 28:28);
    s(EmcSelDpdCtrl, 18:16, scratch84, 31:29);
    s(EmcPmacroDqTxDrv, 5:0, scratch85, 5:0);
    s(EmcPmacroDqTxDrv, 13:8, scratch85, 11:6);
    s(EmcPmacroDqTxDrv, 21:16, scratch85, 17:12);
    s(EmcPmacroDqTxDrv, 29:24, scratch85, 23:18);
    s(EmcObdly, 5:0, scratch85, 29:24);
    s(EmcObdly, 29:28, scratch85, 31:30);
    s(EmcPmacroCaTxDrv, 5:0, scratch86, 5:0);
    s(EmcPmacroCaTxDrv, 13:8, scratch86, 11:6);
    s(EmcPmacroCaTxDrv, 21:16, scratch86, 17:12);
    s(EmcPmacroCaTxDrv, 29:24, scratch86, 23:18);
    s(EmcPmacroVttgenCtrl1, 15:10, scratch86, 29:24);
    s(EmcPmacroVttgenCtrl1, 21:20, scratch86, 31:30);
    s(EmcPmacroZctrl, 27:4, scratch87, 23:0);
    s(EmcPmacroVttgenCtrl2, 23:16, scratch87, 31:24);
    s(EmcZcalInterval, 23:10, scratch88, 13:0);
    s(EmcZcalInterval, 9:0, scratch88, 23:14);
    s(McEmemArbTimingRc, 7:0, scratch88, 31:24);
    s(EmcDataBrlshft0, 23:0, scratch89, 23:0);
    s(McEmemArbRsv, 7:0, scratch89, 31:24);
    s(EmcDataBrlshft1, 23:0, scratch90, 23:0);
    s(EmcDqsBrlshft0, 23:0, scratch91, 23:0);
    s(EmcDqsBrlshft1, 23:0, scratch92, 23:0);
    s(EmcSwizzleRank0Byte0, 2:0, scratch93, 2:0);
    s(EmcSwizzleRank0Byte0, 6:4, scratch93, 5:3);
    s(EmcSwizzleRank0Byte0, 10:8, scratch93, 8:6);
    s(EmcSwizzleRank0Byte0, 14:12, scratch93, 11:9);
    s(EmcSwizzleRank0Byte0, 18:16, scratch93, 14:12);
    s(EmcSwizzleRank0Byte0, 22:20, scratch93, 17:15);
    s(EmcSwizzleRank0Byte0, 26:24, scratch93, 20:18);
    s(EmcSwizzleRank0Byte0, 30:28, scratch93, 23:21);
    s(EmcSwizzleRank0Byte1, 2:0, scratch94, 2:0);
    s(EmcSwizzleRank0Byte1, 6:4, scratch94, 5:3);
    s(EmcSwizzleRank0Byte1, 10:8, scratch94, 8:6);
    s(EmcSwizzleRank0Byte1, 14:12, scratch94, 11:9);
    s(EmcSwizzleRank0Byte1, 18:16, scratch94, 14:12);
    s(EmcSwizzleRank0Byte1, 22:20, scratch94, 17:15);
    s(EmcSwizzleRank0Byte1, 26:24, scratch94, 20:18);
    s(EmcSwizzleRank0Byte1, 30:28, scratch94, 23:21);
    s(EmcRas, 6:0, scratch94, 30:24);
    s(EmcCfg, 4:4, scratch94, 31:31);
    s(EmcSwizzleRank0Byte2, 2:0, scratch95, 2:0);
    s(EmcSwizzleRank0Byte2, 6:4, scratch95, 5:3);
    s(EmcSwizzleRank0Byte2, 10:8, scratch95, 8:6);
    s(EmcSwizzleRank0Byte2, 14:12, scratch95, 11:9);
    s(EmcSwizzleRank0Byte2, 18:16, scratch95, 14:12);
    s(EmcSwizzleRank0Byte2, 22:20, scratch95, 17:15);
    s(EmcSwizzleRank0Byte2, 26:24, scratch95, 20:18);
    s(EmcSwizzleRank0Byte2, 30:28, scratch95, 23:21);
    s(EmcW2p, 6:0, scratch95, 30:24);
    s(EmcCfg, 5:5, scratch95, 31:31);
    s(EmcSwizzleRank0Byte3, 2:0, scratch96, 2:0);
    s(EmcSwizzleRank0Byte3, 6:4, scratch96, 5:3);
    s(EmcSwizzleRank0Byte3, 10:8, scratch96, 8:6);
    s(EmcSwizzleRank0Byte3, 14:12, scratch96, 11:9);
    s(EmcSwizzleRank0Byte3, 18:16, scratch96, 14:12);
    s(EmcSwizzleRank0Byte3, 22:20, scratch96, 17:15);
    s(EmcSwizzleRank0Byte3, 26:24, scratch96, 20:18);
    s(EmcSwizzleRank0Byte3, 30:28, scratch96, 23:21);
    s(EmcQSafe, 6:0, scratch96, 30:24);
    s(EmcCfg, 6:6, scratch96, 31:31);
    s(EmcSwizzleRank1Byte0, 2:0, scratch97, 2:0);
    s(EmcSwizzleRank1Byte0, 6:4, scratch97, 5:3);
    s(EmcSwizzleRank1Byte0, 10:8, scratch97, 8:6);
    s(EmcSwizzleRank1Byte0, 14:12, scratch97, 11:9);
    s(EmcSwizzleRank1Byte0, 18:16, scratch97, 14:12);
    s(EmcSwizzleRank1Byte0, 22:20, scratch97, 17:15);
    s(EmcSwizzleRank1Byte0, 26:24, scratch97, 20:18);
    s(EmcSwizzleRank1Byte0, 30:28, scratch97, 23:21);
    s(EmcRdv, 6:0, scratch97, 30:24);
    s(EmcCfg, 7:7, scratch97, 31:31);
    s(EmcSwizzleRank1Byte1, 2:0, scratch98, 2:0);
    s(EmcSwizzleRank1Byte1, 6:4, scratch98, 5:3);
    s(EmcSwizzleRank1Byte1, 10:8, scratch98, 8:6);
    s(EmcSwizzleRank1Byte1, 14:12, scratch98, 11:9);
    s(EmcSwizzleRank1Byte1, 18:16, scratch98, 14:12);
    s(EmcSwizzleRank1Byte1, 22:20, scratch98, 17:15);
    s(EmcSwizzleRank1Byte1, 26:24, scratch98, 20:18);
    s(EmcSwizzleRank1Byte1, 30:28, scratch98, 23:21);
    s(EmcRw2Pden, 6:0, scratch98, 30:24);
    s(EmcCfg, 8:8, scratch98, 31:31);
    s(EmcSwizzleRank1Byte2, 2:0, scratch99, 2:0);
    s(EmcSwizzleRank1Byte2, 6:4, scratch99, 5:3);
    s(EmcSwizzleRank1Byte2, 10:8, scratch99, 8:6);
    s(EmcSwizzleRank1Byte2, 14:12, scratch99, 11:9);
    s(EmcSwizzleRank1Byte2, 18:16, scratch99, 14:12);
    s(EmcSwizzleRank1Byte2, 22:20, scratch99, 17:15);
    s(EmcSwizzleRank1Byte2, 26:24, scratch99, 20:18);
    s(EmcSwizzleRank1Byte2, 30:28, scratch99, 23:21);
    s(EmcTfaw, 6:0, scratch99, 30:24);
    s(EmcCfg, 9:9, scratch99, 31:31);
    s(EmcSwizzleRank1Byte3, 2:0, scratch100, 2:0);
    s(EmcSwizzleRank1Byte3, 6:4, scratch100, 5:3);
    s(EmcSwizzleRank1Byte3, 10:8, scratch100, 8:6);
    s(EmcSwizzleRank1Byte3, 14:12, scratch100, 11:9);
    s(EmcSwizzleRank1Byte3, 18:16, scratch100, 14:12);
    s(EmcSwizzleRank1Byte3, 22:20, scratch100, 17:15);
    s(EmcSwizzleRank1Byte3, 26:24, scratch100, 20:18);
    s(EmcSwizzleRank1Byte3, 30:28, scratch100, 23:21);
    s(EmcTClkStable, 6:0, scratch100, 30:24);
    s(EmcCfg, 18:18, scratch100, 31:31);
    s(EmcCfgPipe2, 11:0, scratch101, 11:0);
    s(EmcCfgPipe2, 27:16, scratch101, 23:12);
    s(EmcTrtm, 6:0, scratch101, 30:24);
    s(EmcCfg, 21:21, scratch101, 31:31);
    s(EmcCfgPipe1, 11:0, scratch102, 11:0);
    s(EmcCfgPipe1, 27:16, scratch102, 23:12);
    s(EmcTwtm, 6:0, scratch102, 30:24);
    s(EmcCfg, 22:22, scratch102, 31:31);
    s(EmcPmacroDdllPwrd0, 4:1, scratch103, 3:0);
    s(EmcPmacroDdllPwrd0, 7:6, scratch103, 5:4);
    s(EmcPmacroDdllPwrd0, 12:9, scratch103, 9:6);
    s(EmcPmacroDdllPwrd0, 15:14, scratch103, 11:10);
    s(EmcPmacroDdllPwrd0, 20:17, scratch103, 15:12);
    s(EmcPmacroDdllPwrd0, 23:22, scratch103, 17:16);
    s(EmcPmacroDdllPwrd0, 28:25, scratch103, 21:18);
    s(EmcPmacroDdllPwrd0, 31:30, scratch103, 23:22);
    s(EmcTratm, 6:0, scratch103, 30:24);
    s(EmcCfg, 23:23, scratch103, 31:31);
    s(EmcPmacroDdllPwrd1, 4:1, scratch104, 3:0);
    s(EmcPmacroDdllPwrd1, 7:6, scratch104, 5:4);
    s(EmcPmacroDdllPwrd1, 12:9, scratch104, 9:6);
    s(EmcPmacroDdllPwrd1, 15:14, scratch104, 11:10);
    s(EmcPmacroDdllPwrd1, 20:17, scratch104, 15:12);
    s(EmcPmacroDdllPwrd1, 23:22, scratch104, 17:16);
    s(EmcPmacroDdllPwrd1, 28:25, scratch104, 21:18);
    s(EmcPmacroDdllPwrd1, 31:30, scratch104, 23:22);
    s(EmcTwatm, 6:0, scratch104, 30:24);
    s(EmcCfg, 24:24, scratch104, 31:31);
    s(EmcPmacroDdllPwrd2, 4:1, scratch105, 3:0);
    s(EmcPmacroDdllPwrd2, 7:6, scratch105, 5:4);
    s(EmcPmacroDdllPwrd2, 12:9, scratch105, 9:6);
    s(EmcPmacroDdllPwrd2, 15:14, scratch105, 11:10);
    s(EmcPmacroDdllPwrd2, 20:17, scratch105, 15:12);
    s(EmcPmacroDdllPwrd2, 23:22, scratch105, 17:16);
    s(EmcPmacroDdllPwrd2, 28:25, scratch105, 21:18);
    s(EmcPmacroDdllPwrd2, 31:30, scratch105, 23:22);
    s(EmcTr2ref, 6:0, scratch105, 30:24);
    s(EmcCfg, 25:25, scratch105, 31:31);
    s(EmcPmacroDdllPeriodicOffset, 5:0, scratch106, 5:0);
    s(EmcPmacroDdllPeriodicOffset, 16:8, scratch106, 14:6);
    s(EmcPmacroDdllPeriodicOffset, 28:20, scratch106, 23:15);
    s(EmcPdex2Mrr, 6:0, scratch106, 30:24);
    s(EmcCfg, 26:26, scratch106, 31:31);
    s(McEmemArbDaCovers, 23:0, scratch107, 23:0);
    s(EmcClkenOverride, 3:1, scratch107, 26:24);
    s(EmcClkenOverride, 8:6, scratch107, 29:27);
    s(EmcClkenOverride, 16:16, scratch107, 30:30);
    s(EmcCfg, 28:28, scratch107, 31:31);
    s(EmcXm2CompPadCtrl, 1:0, scratch108, 1:0);
    s(EmcXm2CompPadCtrl, 6:4, scratch108, 4:2);
    s(EmcXm2CompPadCtrl, 9:9, scratch108, 5:5);
    s(EmcXm2CompPadCtrl, 19:11, scratch108, 14:6);
    s(EmcXm2CompPadCtrl, 31:24, scratch108, 22:15);
    s(EmcRfcPb, 8:0, scratch108, 31:23);
    s(EmcAutoCalConfig3, 6:0, scratch109, 6:0);
    s(EmcAutoCalConfig3, 14:8, scratch109, 13:7);
    s(EmcAutoCalConfig3, 23:16, scratch109, 21:14);
    s(EmcCfgUpdate, 2:0, scratch109, 24:22);
    s(EmcCfgUpdate, 10:8, scratch109, 27:25);
    s(EmcCfgUpdate, 31:28, scratch109, 31:28);
    s(EmcAutoCalConfig4, 6:0, scratch110, 6:0);
    s(EmcAutoCalConfig4, 14:8, scratch110, 13:7);
    s(EmcAutoCalConfig4, 23:16, scratch110, 21:14);
    s(EmcRfc, 9:0, scratch110, 31:22);
    s(EmcAutoCalConfig5, 6:0, scratch111, 6:0);
    s(EmcAutoCalConfig5, 14:8, scratch111, 13:7);
    s(EmcAutoCalConfig5, 23:16, scratch111, 21:14);
    s(EmcTxsr, 9:0, scratch111, 31:22);
    s(EmcAutoCalConfig6, 6:0, scratch112, 6:0);
    s(EmcAutoCalConfig6, 14:8, scratch112, 13:7);
    s(EmcAutoCalConfig6, 23:16, scratch112, 21:14);
    s(EmcMc2EmcQ, 2:0, scratch112, 24:22);
    s(EmcMc2EmcQ, 10:8, scratch112, 27:25);
    s(EmcMc2EmcQ, 27:24, scratch112, 31:28);
    s(EmcAutoCalConfig7, 6:0, scratch113, 6:0);
    s(EmcAutoCalConfig7, 14:8, scratch113, 13:7);
    s(EmcAutoCalConfig7, 23:16, scratch113, 21:14);
    s(McEmemArbRing1Throttle, 4:0, scratch113, 26:22);
    s(McEmemArbRing1Throttle, 20:16, scratch113, 31:27);
    s(EmcAutoCalConfig8, 6:0, scratch114, 6:0);
    s(EmcAutoCalConfig8, 14:8, scratch114, 13:7);
    s(EmcAutoCalConfig8, 23:16, scratch114, 21:14);
    s(EmcFbioCfg7, 21:0, scratch115, 21:0);
    s(EmcAr2Pden, 8:0, scratch115, 30:22);
    s(EmcCfg, 29:29, scratch115, 31:31);
    s(EmcPmacroQuseDdllRank0_0, 10:0, scratch123, 10:0);
    s(EmcPmacroQuseDdllRank0_0, 26:16, scratch123, 21:11);
    s(EmcRfcSlr, 8:0, scratch123, 30:22);
    s(EmcCfg, 30:30, scratch123, 31:31);
    s(EmcPmacroQuseDdllRank0_1, 10:0, scratch124, 10:0);
    s(EmcPmacroQuseDdllRank0_1, 26:16, scratch124, 21:11);
    s(EmcIbdly, 6:0, scratch124, 28:22);
    s(EmcIbdly, 29:28, scratch124, 30:29);
    s(EmcCfg, 31:31, scratch124, 31:31);
    s(EmcPmacroQuseDdllRank0_2, 10:0, scratch125, 10:0);
    s(EmcPmacroQuseDdllRank0_2, 26:16, scratch125, 21:11);
    s(McEmemArbTimingRFCPB, 8:0, scratch125, 30:22);
    s(EmcFbioCfg5, 4:4, scratch125, 31:31);
    s(EmcPmacroQuseDdllRank0_3, 10:0, scratch126, 10:0);
    s(EmcPmacroQuseDdllRank0_3, 26:16, scratch126, 21:11);
    s(EmcAutoCalConfig9, 6:0, scratch126, 28:22);
    s(EmcFbioCfg5, 15:13, scratch126, 31:29);
    s(EmcPmacroQuseDdllRank0_4, 10:0, scratch127, 10:0);
    s(EmcPmacroQuseDdllRank0_4, 26:16, scratch127, 21:11);
    s(EmcRdvMask, 6:0, scratch127, 28:22);
    s(EmcCfg2, 5:3, scratch127, 31:29);
    s(EmcPmacroQuseDdllRank0_5, 10:0, scratch128, 10:0);
    s(EmcPmacroQuseDdllRank0_5, 26:16, scratch128, 21:11);
    s(EmcRdvEarlyMask, 6:0, scratch128, 28:22);
    s(EmcPmacroCmdPadTxCtrl, 4:2, scratch128, 31:29);
    s(EmcPmacroQuseDdllRank0_5, 10:0, scratch129, 10:0);
    s(EmcPmacroQuseDdllRank0_5, 26:16, scratch129, 21:11);
    s(EmcRdvEarly, 6:0, scratch129, 28:22);
    s(EmcPmacroCmdPadTxCtrl, 9:7, scratch129, 31:29);
    s(EmcPmacroQuseDdllRank1_1, 10:0, scratch130, 10:0);
    s(EmcPmacroQuseDdllRank1_1, 26:16, scratch130, 21:11);
    s(EmcQuseWidth, 4:0, scratch130, 26:22);
    s(EmcQuseWidth, 29:28, scratch130, 28:27);
    s(EmcPmacroCmdPadTxCtrl, 14:12, scratch130, 31:29);
    s(EmcPmacroQuseDdllRank1_2, 10:0, scratch131, 10:0);
    s(EmcPmacroQuseDdllRank1_2, 26:16, scratch131, 21:11);
    s(EmcPmacroDdllShortCmd_2, 6:0, scratch131, 28:22);
    s(EmcPmacroCmdPadTxCtrl, 19:17, scratch131, 31:29);
    s(EmcPmacroQuseDdllRank1_3, 10:0, scratch132, 10:0);
    s(EmcPmacroQuseDdllRank1_3, 26:16, scratch132, 21:11);
    s(EmcPmacroCmdRxTermMode, 1:0, scratch132, 23:22);
    s(EmcPmacroCmdRxTermMode, 5:4, scratch132, 25:24);
    s(EmcPmacroCmdRxTermMode, 9:8, scratch132, 27:26);
    s(EmcPmacroCmdRxTermMode, 13:13, scratch132, 28:28);
    s(EmcPmacroDataPadTxCtrl, 4:2, scratch132, 31:29);
    s(EmcPmacroQuseDdllRank1_4, 10:0, scratch133, 10:0);
    s(EmcPmacroQuseDdllRank1_4, 26:16, scratch133, 21:11);
    s(EmcPmacroDataRxTermMode, 1:0, scratch133, 23:22);
    s(EmcPmacroDataRxTermMode, 5:4, scratch133, 25:24);
    s(EmcPmacroDataRxTermMode, 9:8, scratch133, 27:26);
    s(EmcPmacroDataRxTermMode, 13:13, scratch133, 28:28);
    s(EmcPmacroDataPadTxCtrl, 9:7, scratch133, 31:29);
    s(EmcPmacroQuseDdllRank1_5, 10:0, scratch134, 10:0);
    s(EmcPmacroQuseDdllRank1_5, 26:16, scratch134, 21:11);
    s(McEmemArbTimingRp, 6:0, scratch134, 28:22);
    s(EmcPmacroDataPadTxCtrl, 14:12, scratch134, 31:29);
    s(EmcPmacroObDdllLongDqRank0_0, 10:0, scratch135, 10:0);
    s(EmcPmacroObDdllLongDqRank0_0, 26:16, scratch135, 21:11);
    s(McEmemArbTimingRas, 6:0, scratch135, 28:22);
    s(EmcPmacroDataPadTxCtrl, 19:17, scratch135, 31:29);
    s(EmcPmacroObDdllLongDqRank0_1, 10:0, scratch136, 10:0);
    s(EmcPmacroObDdllLongDqRank0_1, 26:16, scratch136, 21:11);
    s(McEmemArbTimingFaw, 6:0, scratch136, 28:22);
    s(EmcCfg, 17:16, scratch136, 30:29);
    s(EmcFbioCfg5, 8:8, scratch136, 31:31);
    s(EmcPmacroObDdllLongDqRank0_2, 10:0, scratch137, 10:0);
    s(EmcPmacroObDdllLongDqRank0_2, 26:16, scratch137, 21:11);
    s(McEmemArbTimingRap2Pre, 6:0, scratch137, 28:22);
    s(EmcFbioCfg5, 1:0, scratch137, 30:29);
    s(EmcFbioCfg5, 10:10, scratch137, 31:31);
    s(EmcPmacroObDdllLongDqRank0_3, 10:0, scratch138, 10:0);
    s(EmcPmacroObDdllLongDqRank0_3, 26:16, scratch138, 21:11);
    s(McEmemArbTimingWap2Pre, 6:0, scratch138, 28:22);
    s(EmcFbioCfg5, 3:2, scratch138, 30:29);
    s(EmcFbioCfg5, 12:12, scratch138, 31:31);
    s(EmcPmacroObDdllLongDqRank0_4, 10:0, scratch139, 10:0);
    s(EmcPmacroObDdllLongDqRank0_4, 26:16, scratch139, 21:11);
    s(McEmemArbTimingR2W, 6:0, scratch139, 28:22);
    s(EmcCfg2, 27:26, scratch139, 30:29);
    s(EmcFbioCfg5, 24:24, scratch139, 31:31);
    s(EmcPmacroObDdllLongDqRank0_5, 10:0, scratch140, 10:0);
    s(EmcPmacroObDdllLongDqRank0_5, 26:16, scratch140, 21:11);
    s(McEmemArbTimingW2R, 6:0, scratch140, 28:22);
    s(EmcFbioCfg5, 27:25, scratch140, 31:29);
    s(EmcPmacroObDdllLongDqRank1_0, 10:0, scratch141, 10:0);
    s(EmcPmacroObDdllLongDqRank1_0, 26:16, scratch141, 21:11);
    s(EmcWdv, 5:0, scratch141, 27:22);
    s(EmcFbioCfg5, 23:20, scratch141, 31:28);
    s(EmcPmacroObDdllLongDqRank1_1, 10:0, scratch142, 10:0);
    s(EmcPmacroObDdllLongDqRank1_1, 26:16, scratch142, 21:11);
    s(EmcQUse, 4:0, scratch142, 27:22);
    s(EmcFbioCfg5, 28:28, scratch142, 28:28);
    s(EmcFbioCfg5, 31:30, scratch142, 30:29);
    s(EmcCfg2, 0:0, scratch142, 31:31);
    s(EmcPmacroObDdllLongDqRank1_2, 10:0, scratch143, 10:0);
    s(EmcPmacroObDdllLongDqRank1_2, 26:16, scratch143, 21:11);
    s(EmcPdEx2Wr, 5:0, scratch143, 27:22);
    s(EmcCfg2, 2:1, scratch143, 29:28);
    s(EmcCfg2, 7:7, scratch143, 30:30);
    s(EmcCfg2, 10:10, scratch143, 31:31);
    s(EmcPmacroObDdllLongDqRank1_3, 10:0, scratch144, 10:0);
    s(EmcPmacroObDdllLongDqRank1_3, 26:16, scratch144, 21:11);
    s(EmcPdEx2Rd, 5:0, scratch144, 27:22);
    s(EmcCfg2, 11:11, scratch144, 28:28);
    s(EmcCfg2, 16:14, scratch144, 31:29);
    s(EmcPmacroObDdllLongDqRank1_4, 10:0, scratch145, 10:0);
    s(EmcPmacroObDdllLongDqRank1_4, 26:16, scratch145, 21:11);
    s(EmcPdex2Cke, 5:0, scratch145, 27:22);
    s(EmcCfg2, 20:20, scratch145, 28:28);
    s(EmcCfg2, 24:22, scratch145, 31:29);
    s(EmcPmacroObDdllLongDqRank1_5, 10:0, scratch146, 10:0);
    s(EmcPmacroObDdllLongDqRank1_5, 26:16, scratch146, 21:11);
    s(EmcPChg2Pden, 5:0, scratch146, 27:22);
    s(EmcCfg2, 25:25, scratch146, 28:28);
    s(EmcCfg2, 30:28, scratch146, 31:29);
    s(EmcPmacroObDdllLongDqsRank0_0, 10:0, scratch147, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_0, 26:16, scratch147, 21:11);
    s(EmcAct2Pden, 5:0, scratch147, 27:22);
    s(EmcCfg2, 31:31, scratch147, 28:28);
    s(EmcCfgPipe, 2:0, scratch147, 31:29);
    s(EmcPmacroObDdllLongDqsRank0_1, 10:0, scratch148, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_1, 26:16, scratch148, 21:11);
    s(EmcCke2Pden, 5:0, scratch148, 27:22);
    s(EmcCfgPipe, 6:3, scratch148, 31:28);
    s(EmcPmacroObDdllLongDqsRank0_2, 10:0, scratch149, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_2, 26:16, scratch149, 21:11);
    s(EmcTcke, 5:0, scratch149, 27:22);
    s(EmcCfgPipe, 10:7, scratch149, 31:28);
    s(EmcPmacroObDdllLongDqsRank0_3, 10:0, scratch150, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_3, 26:16, scratch150, 21:11);
    s(EmcTrpab, 5:0, scratch150, 27:22);
    s(EmcCfgPipe, 11:11, scratch150, 28:28);
    s(EmcCfgPipe, 18:16, scratch150, 31:29);
    s(EmcPmacroObDdllLongDqsRank0_4, 10:0, scratch151, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_4, 26:16, scratch151, 21:11);
    s(EmcEInput, 5:0, scratch151, 27:22);
    s(EmcCfgPipe, 22:19, scratch151, 31:28);
    s(EmcPmacroObDdllLongDqsRank0_5, 10:0, scratch152, 10:0);
    s(EmcPmacroObDdllLongDqsRank0_5, 26:16, scratch152, 21:11);
    s(EmcEInputDuration, 5:0, scratch152, 27:22);
    s(EmcCfgPipe, 26:23, scratch152, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_0, 10:0, scratch153, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_0, 26:16, scratch153, 21:11);
    s(EmcPutermExtra, 5:0, scratch153, 27:22);
    s(EmcCfgPipe, 27:27, scratch153, 28:28);
    s(EmcPmacroTxSelClkSrc0, 2:0, scratch153, 31:29);
    s(EmcPmacroObDdllLongDqsRank1_1, 10:0, scratch154, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_1, 26:16, scratch154, 21:11);
    s(EmcTckesr, 5:0, scratch154, 27:22);
    s(EmcPmacroTxSelClkSrc0, 6:3, scratch154, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_2, 10:0, scratch155, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_2, 26:16, scratch155, 21:11);
    s(EmcTpd, 5:0, scratch155, 27:22);
    s(EmcPmacroTxSelClkSrc0, 10:7, scratch155, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_3, 10:0, scratch156, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_3, 26:16, scratch156, 21:11);
    s(EmcWdvMask, 5:0, scratch156, 27:22);
    s(EmcPmacroTxSelClkSrc0, 19:16, scratch156, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_4, 10:0, scratch157, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_4, 26:16, scratch157, 21:11);
    s(EmcWdvChk, 5:0, scratch157, 27:22);
    s(EmcPmacroTxSelClkSrc0, 23:20, scratch157, 31:28);
    s(EmcPmacroObDdllLongDqsRank1_5, 10:0, scratch158, 10:0);
    s(EmcPmacroObDdllLongDqsRank1_5, 26:16, scratch158, 21:11);
    s(EmcCmdBrlshft0, 5:0, scratch158, 27:22);
    s(EmcPmacroTxSelClkSrc0, 26:24, scratch158, 30:28);
    s(EmcPmacroTxSelClkSrc1, 0:0, scratch158, 31:31);
    s(EmcPmacroIbDdllLongDqsRank0_0, 10:0, scratch159, 10:0);
    s(EmcPmacroIbDdllLongDqsRank0_0, 26:16, scratch159, 21:11);
    s(EmcCmdBrlshft1, 5:0, scratch159, 27:22);
    s(EmcPmacroTxSelClkSrc1, 4:1, scratch159, 31:28);
    s(EmcPmacroIbDdllLongDqsRank0_1, 10:0, scratch160, 10:0);
    s(EmcPmacroIbDdllLongDqsRank0_1, 26:16, scratch160, 21:11);
    s(EmcCmdBrlshft2, 5:0, scratch160, 27:22);
    s(EmcPmacroTxSelClkSrc1, 8:5, scratch160, 31:28);
    s(EmcPmacroIbDdllLongDqsRank0_2, 10:0, scratch161, 10:0);
    s(EmcPmacroIbDdllLongDqsRank0_2, 26:16, scratch161, 21:11);
    s(EmcCmdBrlshft3, 5:0, scratch161, 27:22);
    s(EmcPmacroTxSelClkSrc1, 10:9, scratch161, 29:28);
    s(EmcPmacroTxSelClkSrc1, 17:16, scratch161, 31:30);
    s(EmcPmacroIbDdllLongDqsRank0_3, 10:0, scratch162, 10:0);
    s(EmcPmacroIbDdllLongDqsRank0_3, 26:16, scratch162, 21:11);
    s(EmcWev, 5:0, scratch162, 27:22);
    s(EmcPmacroTxSelClkSrc1, 21:18, scratch162, 31:28);
    s(EmcPmacroIbDdllLongDqsRank1_0, 10:0, scratch163, 10:0);
    s(EmcPmacroIbDdllLongDqsRank1_0, 26:16, scratch163, 21:11);
    s(EmcWsv, 5:0, scratch163, 27:22);
    s(EmcPmacroTxSelClkSrc1, 25:22, scratch163, 31:28);
    s(EmcPmacroIbDdllLongDqsRank1_1, 10:0, scratch164, 10:0);
    s(EmcPmacroIbDdllLongDqsRank1_1, 26:16, scratch164, 21:11);
    s(EmcCfg3, 2:0, scratch164, 24:22);
    s(EmcCfg3, 6:4, scratch164, 27:25);
    s(EmcPmacroTxSelClkSrc1, 26:26, scratch164, 28:28);
    s(EmcPmacroTxSelClkSrc3, 2:0, scratch164, 31:29);
    s(EmcPmacroIbDdllLongDqsRank1_2, 10:0, scratch165, 10:0);
    s(EmcPmacroIbDdllLongDqsRank1_2, 26:16, scratch165, 21:11);
    s(EmcPutermWidth, 31:31, scratch165, 22:22);
    s(EmcPutermWidth, 4:0, scratch165, 27:23);
    s(EmcPmacroTxSelClkSrc3, 6:3, scratch165, 31:28);
    s(EmcPmacroIbDdllLongDqsRank1_3, 10:0, scratch166, 10:0);
    s(EmcPmacroIbDdllLongDqsRank1_3, 26:16, scratch166, 21:11);
    s(McEmemArbTimingRcd, 5:0, scratch166, 27:22);
    s(EmcPmacroTxSelClkSrc3, 10:7, scratch166, 31:28);
    s(EmcPmacroDdllLongCmd_0, 10:0, scratch167, 10:0);
    s(EmcPmacroDdllLongCmd_0, 26:16, scratch167, 21:11);
    s(McEmemArbTimingCcdmw, 5:0, scratch167, 27:22);
    s(EmcPmacroTxSelClkSrc3, 19:16, scratch167, 31:28);
    s(EmcPmacroDdllLongCmd_1, 10:0, scratch168, 10:0);
    s(EmcPmacroDdllLongCmd_1, 26:16, scratch168, 21:11);
    s(McEmemArbOverride, 27:26, scratch168, 23:22);
    s(McEmemArbOverride, 16:16, scratch168, 24:24);
    s(McEmemArbOverride, 10:10, scratch168, 25:25);
    s(McEmemArbOverride, 4:3, scratch168, 27:26);
    s(EmcPmacroTxSelClkSrc3, 23:20, scratch168, 31:28);
    s(EmcPmacroDdllLongCmd_2, 10:0, scratch169, 10:0);
    s(EmcPmacroDdllLongCmd_2, 26:16, scratch169, 21:11);
    s(EmcRrd, 4:0, scratch169, 26:22);
    s(EmcRext, 4:0, scratch169, 31:27);
    s(EmcPmacroDdllLongCmd_3, 10:0, scratch170, 10:0);
    s(EmcPmacroDdllLongCmd_3, 26:16, scratch170, 21:11);
    s(EmcTClkStop, 4:0, scratch170, 26:22);
    s(EmcWext, 4:0, scratch170, 31:27);
    s(EmcPmacroPerbitFgcgCtrl0, 10:0, scratch171, 10:0);
    s(EmcPmacroPerbitFgcgCtrl0, 26:16, scratch171, 21:11);
    s(EmcRefctrl2, 0:0, scratch171, 22:22);
    s(EmcRefctrl2, 26:24, scratch171, 25:23);
    s(EmcRefctrl2, 31:31, scratch171, 26:26);
    s(EmcWeDuration, 4:0, scratch171, 31:27);
    s(EmcPmacroPerbitFgcgCtrl1, 10:0, scratch172, 10:0);
    s(EmcPmacroPerbitFgcgCtrl1, 26:16, scratch172, 21:11);
    s(EmcWsDuration, 4:0, scratch172, 26:22);
    s(EmcPmacroPadCfgCtrl, 0:0, scratch172, 27:27);
    s(EmcPmacroPadCfgCtrl, 9:9, scratch172, 28:28);
    s(EmcPmacroPadCfgCtrl, 13:13, scratch172, 29:29);
    s(EmcPmacroPadCfgCtrl, 17:16, scratch172, 31:30);
    s(EmcPmacroPerbitFgcgCtrl2, 10:0, scratch173, 10:0);
    s(EmcPmacroPerbitFgcgCtrl2, 26:16, scratch173, 21:11);
    s(McEmemArbTimingRrd, 4:0, scratch173, 26:22);
    s(McEmemArbTimingR2R, 4:0, scratch173, 31:27);
    s(EmcPmacroPerbitFgcgCtrl3, 10:0, scratch174, 10:0);
    s(EmcPmacroPerbitFgcgCtrl3, 26:16, scratch174, 21:11);
    s(McEmemArbTimingW2W, 4:0, scratch174, 26:22);
    s(EmcPmacroTxSelClkSrc3, 26:24, scratch174, 29:27);
    s(EmcPmacroTxSelClkSrc2, 1:0, scratch174, 31:30);
    s(EmcPmacroPerbitFgcgCtrl4, 10:0, scratch175, 10:0);
    s(EmcPmacroPerbitFgcgCtrl4, 26:16, scratch175, 21:11);
    s(EmcPmacroTxSelClkSrc2, 10:2, scratch175, 30:22);
    s(EmcPmacroTxSelClkSrc2, 16:16, scratch175, 31:31);
    s(EmcPmacroPerbitFgcgCtrl5, 10:0, scratch176, 10:0);
    s(EmcPmacroPerbitFgcgCtrl5, 26:16, scratch176, 21:11);
    s(EmcPmacroTxSelClkSrc2, 26:17, scratch176, 31:22);
    s(McEmemArbCfg, 8:0, scratch177, 8:0);
    s(McEmemArbCfg, 20:16, scratch177, 13:9);
    s(McEmemArbCfg, 31:24, scratch177, 21:14);
    s(EmcPmacroTxSelClkSrc4, 9:0, scratch177, 31:22);
    s(McEmemArbMisc1, 12:0, scratch178, 12:0);
    s(McEmemArbMisc1, 25:21, scratch178, 17:13);
    s(McEmemArbMisc1, 31:28, scratch178, 21:18);
    s(EmcPmacroTxSelClkSrc4, 10:10, scratch178, 22:22);
    s(EmcPmacroTxSelClkSrc4, 24:16, scratch178, 31:23);
    s(EmcMrsWaitCnt2, 9:0, scratch179, 9:0);
    s(EmcMrsWaitCnt2, 26:16, scratch179, 20:10);
    s(EmcOdtWrite, 5:0, scratch179, 26:21);
    s(EmcOdtWrite, 11:8, scratch179, 30:27);
    s(EmcOdtWrite, 31:31, scratch179, 31:31);
    s(EmcMrsWaitCnt, 9:0, scratch180, 9:0);
    s(EmcMrsWaitCnt, 26:16, scratch180, 20:10);
    s(EmcPmacroIbRxrt, 10:0, scratch180, 31:21);
    s(EmcAutoCalInterval, 20:0, scratch181, 20:0);
    s(EmcPmacroDdllLongCmd_4, 10:0, scratch181, 31:21);
    s(McEmemArbRefpbHpCtrl, 6:0, scratch182, 6:0);
    s(McEmemArbRefpbHpCtrl, 14:8, scratch182, 13:7);
    s(McEmemArbRefpbHpCtrl, 22:16, scratch182, 20:14);
    s(McEmemArbOutstandingReq, 8:0, scratch182, 29:21);
    s(McEmemArbOutstandingReq, 31:30, scratch182, 31:30);
    s(EmcXm2CompPadCtrl2, 5:0, scratch183, 5:0);
    s(EmcXm2CompPadCtrl2, 17:12, scratch183, 11:6);
    s(EmcXm2CompPadCtrl2, 21:20, scratch183, 13:12);
    s(EmcXm2CompPadCtrl2, 29:24, scratch183, 19:14);
    s(EmcPmacroCmdCtrl0, 0:0, scratch183, 20:20);
    s(EmcPmacroCmdCtrl0, 5:4, scratch183, 22:21);
    s(EmcPmacroCmdCtrl0, 8:8, scratch183, 23:23);
    s(EmcPmacroCmdCtrl0, 13:12, scratch183, 25:24);
    s(EmcPmacroCmdCtrl0, 16:16, scratch183, 26:26);
    s(EmcPmacroCmdCtrl0, 21:20, scratch183, 28:27);
    s(EmcPmacroCmdCtrl0, 24:24, scratch183, 29:29);
    s(EmcPmacroCmdCtrl0, 29:28, scratch183, 31:30);
    s(EmcCfgDigDll_1, 19:0, scratch184, 19:0);
    s(EmcPmacroCmdCtrl1, 0:0, scratch184, 20:20);
    s(EmcPmacroCmdCtrl1, 5:4, scratch184, 22:21);
    s(EmcPmacroCmdCtrl1, 8:8, scratch184, 23:23);
    s(EmcPmacroCmdCtrl1, 13:12, scratch184, 25:24);
    s(EmcPmacroCmdCtrl1, 16:16, scratch184, 26:26);
    s(EmcPmacroCmdCtrl1, 21:20, scratch184, 28:27);
    s(EmcPmacroCmdCtrl1, 24:24, scratch184, 29:29);
    s(EmcPmacroCmdCtrl1, 29:28, scratch184, 31:30);
    s(EmcQuseBrlshft0, 19:0, scratch185, 19:0);
    s(EmcPmacroCmdCtrl2, 0:0, scratch185, 20:20);
    s(EmcPmacroCmdCtrl2, 5:4, scratch185, 22:21);
    s(EmcPmacroCmdCtrl2, 8:8, scratch185, 23:23);
    s(EmcPmacroCmdCtrl2, 13:12, scratch185, 25:24);
    s(EmcPmacroCmdCtrl2, 16:16, scratch185, 26:26);
    s(EmcPmacroCmdCtrl2, 21:20, scratch185, 28:27);
    s(EmcPmacroCmdCtrl2, 24:24, scratch185, 29:29);
    s(EmcPmacroCmdCtrl2, 29:28, scratch185, 31:30);
    s(EmcQuseBrlshft1, 19:0, scratch186, 19:0);
    s(EmcPmacroDsrVttgenCtrl0, 3:0, scratch186, 23:20);
    s(EmcPmacroDsrVttgenCtrl0, 15:8, scratch186, 31:24);
    s(EmcQuseBrlshft2, 19:0, scratch187, 19:0);
    s(EmcPmacroPerbitRfu1Ctrl0, 5:0, scratch187, 25:20);
    s(EmcPmacroPerbitRfu1Ctrl0, 21:20, scratch187, 31:26);
    s(EmcQuseBrlshft3, 19:0, scratch188, 19:0);
    s(EmcPmacroPerbitRfu1Ctrl1, 5:0, scratch188, 25:20);
    s(EmcPmacroPerbitRfu1Ctrl1, 21:20, scratch188, 31:26);
    s(EmcDbg, 4:0, scratch189, 4:0);
    s(EmcDbg, 13:9, scratch189, 9:5);
    s(EmcDbg, 31:24, scratch189, 17:10);
    s(EmcTRefBw, 13:0, scratch189, 31:18);
    s(EmcZcalWaitCnt, 10:0, scratch191, 10:0);
    s(EmcZcalWaitCnt, 21:16, scratch191, 16:11);
    s(EmcZcalWaitCnt, 31:31, scratch191, 17:17);
    s(EmcQpop, 6:0, scratch191, 24:18);
    s(EmcQpop, 22:16, scratch191, 31:25);
    s(EmcZcalMrwCmd, 7:0, scratch192, 7:0);
    s(EmcZcalMrwCmd, 23:16, scratch192, 15:8);
    s(EmcZcalMrwCmd, 31:30, scratch192, 17:16);
    s(EmcPmacroAutocalCfgCommon, 5:0, scratch192, 23:18);
    s(EmcPmacroAutocalCfgCommon, 13:8, scratch192, 29:24);
    s(EmcPmacroAutocalCfgCommon, 16:16, scratch192, 30:30);
    s(EmcPmacroTxSelClkSrc4, 25:25, scratch192, 31:31);
    s(EmcPmacroDllCfg1, 10:0, scratch193, 10:0);
    s(EmcPmacroDllCfg1, 13:12, scratch193, 12:11);
    s(EmcPmacroDllCfg1, 17:16, scratch193, 14:13);
    s(EmcPmacroDllCfg1, 21:20, scratch193, 16:15);
    s(EmcPmacroDllCfg1, 24:24, scratch193, 17:17);
    s(EmcPmacroPerbitRfu1Ctrl2, 5:0, scratch193, 23:18);
    s(EmcPmacroPerbitRfu1Ctrl2, 21:16, scratch193, 29:24);
    s(EmcPmacroTxSelClkSrc4, 26:26, scratch193, 30:30);
    s(EmcPmacroTxSelClkSrc5, 0:0, scratch193, 31:31);
    s(EmcPmacroCmdBrickCtrlFdpd, 17:0, scratch194, 17:0);
    s(EmcPmacroPerbitRfu1Ctrl3, 5:0, scratch194, 23:18);
    s(EmcPmacroPerbitRfu1Ctrl3, 21:16, scratch194, 29:24);
    s(EmcPmacroTxSelClkSrc5, 2:1, scratch194, 31:30);
    s(EmcPmacroDataBrickCtrlFdpd, 17:0, scratch195, 17:0);
    s(EmcPmacroPerbitRfu1Ctrl4, 5:0, scratch195, 23:18);
    s(EmcPmacroPerbitRfu1Ctrl4, 21:16, scratch195, 29:24);
    s(EmcPmacroTxSelClkSrc5, 4:3, scratch195, 31:30);
    s(EmcDynSelfRefControl, 15:0, scratch196, 15:0);
    s(EmcDynSelfRefControl, 31:31, scratch196, 16:16);
    s(McEmemArbRefpbBankCtrl, 6:0, scratch196, 23:17);
    s(McEmemArbRefpbBankCtrl, 14:8, scratch196, 30:24);
    s(McEmemArbRefpbBankCtrl, 31:31, scratch196, 31:31);
    s(EmcPmacroCmdPadRxCtrl, 1:0, scratch197, 1:0);
    s(EmcPmacroCmdPadRxCtrl, 5:4, scratch197, 3:2);
    s(EmcPmacroCmdPadRxCtrl, 12:12, scratch197, 4:4);
    s(EmcPmacroCmdPadRxCtrl, 19:15, scratch197, 9:5);
    s(EmcPmacroCmdPadRxCtrl, 27:21, scratch197, 16:10);
    s(EmcPmacroPerbitRfu1Ctrl5, 5:0, scratch197, 22:17);
    s(EmcPmacroPerbitRfu1Ctrl5, 21:16, scratch197, 28:23);
    s(EmcPmacroTxSelClkSrc5, 7:5, scratch197, 31:29);
    s(EmcPmacroDataPadRxCtrl, 1:0, scratch198, 1:0);
    s(EmcPmacroDataPadRxCtrl, 5:4, scratch198, 3:2);
    s(EmcPmacroDataPadRxCtrl, 12:12, scratch198, 4:4);
    s(EmcPmacroDataPadRxCtrl, 19:15, scratch198, 9:5);
    s(EmcPmacroDataPadRxCtrl, 27:21, scratch198, 16:10);
    s(EmcPmacroTxSelClkSrc5, 10:8, scratch198, 19:17);
    s(EmcPmacroTxSelClkSrc5, 26:16, scratch198, 30:20);
    s(EmcPmacroCmdPadTxCtrl, 0:0, scratch198, 31:31);
    s(EmcRefresh, 15:0, scratch199, 15:0);
    s(EmcCmdQ, 4:0, scratch199, 20:16);
    s(EmcCmdQ, 10:8, scratch199, 23:21);
    s(EmcCmdQ, 14:12, scratch199, 26:24);
    s(EmcCmdQ, 28:24, scratch199, 31:27);
    s(EmcAcpdControl, 15:0, scratch210, 15:0);
    s(EmcAutoCalVrefSel1, 15:0, scratch210, 31:16);
    s(EmcPmacroAutocalCfg0, 3:0, scratch211, 3:0);
    s(EmcPmacroAutocalCfg0, 11:8, scratch211, 7:4);
    s(EmcPmacroAutocalCfg0, 19:16, scratch211, 11:8);
    s(EmcPmacroAutocalCfg0, 27:24, scratch211, 15:12);
    s(EmcPmacroAutocalCfg1, 3:0, scratch211, 19:16);
    s(EmcPmacroAutocalCfg1, 11:8, scratch211, 23:20);
    s(EmcPmacroAutocalCfg1, 19:16, scratch211, 27:24);
    s(EmcPmacroAutocalCfg1, 27:24, scratch211, 31:28);
    s(EmcPmacroAutocalCfg2, 3:0, scratch212, 3:0);
    s(EmcPmacroAutocalCfg2, 11:8, scratch212, 7:4);
    s(EmcPmacroAutocalCfg2, 19:16, scratch212, 11:8);
    s(EmcPmacroAutocalCfg2, 27:24, scratch212, 15:12);
    s(EmcXm2CompPadCtrl3, 5:0, scratch212, 21:16);
    s(EmcXm2CompPadCtrl3, 17:12, scratch212, 27:22);
    s(EmcXm2CompPadCtrl3, 23:20, scratch212, 31:28);
    s(EmcCfgDigDllPeriod, 15:0, scratch213, 15:0);
    s(EmcPreRefreshReqCnt, 15:0, scratch213, 31:16);
    s(EmcPmacroDdllBypass, 0:0, scratch214, 0:0);
    s(EmcPmacroDdllBypass, 11:8, scratch214, 4:1);
    s(EmcPmacroDdllBypass, 16:13, scratch214, 8:5);
    s(EmcPmacroDdllBypass, 27:24, scratch214, 12:9);
    s(EmcPmacroDdllBypass, 31:29, scratch214, 15:13);
    s(EmcPmacroDataPiCtrl, 4:0, scratch214, 20:16);
    s(EmcPmacroDataPiCtrl, 12:8, scratch214, 26:21);
    s(EmcPmacroDataPiCtrl, 21:16, scratch214, 31:26);
    s(EmcPmacroCmdPiCtrl, 4:0, scratch215, 4:0);
    s(EmcPmacroCmdPiCtrl, 12:8, scratch215, 9:5);
    s(EmcPmacroCmdPiCtrl, 21:16, scratch215, 15:10);
    s(EmcPmacroCmdPadTxCtrl, 6:5, scratch216, 1:0);
    s(EmcPmacroCmdPadTxCtrl, 10:10, scratch216, 2:2);
    s(EmcPmacroCmdPadTxCtrl, 16:15, scratch216, 4:3);
    s(EmcPmacroCmdPadTxCtrl, 30:21, scratch216, 14:5);
    s(EmcPmacroDataPadTxCtrl, 0:0, scratch216, 15:15);
    s(EmcPmacroDataPadTxCtrl, 6:5, scratch216, 17:16);
    s(EmcPmacroDataPadTxCtrl, 10:10, scratch216, 18:18);
    s(EmcPmacroDataPadTxCtrl, 16:15, scratch216, 20:19);
    s(EmcPmacroDataPadTxCtrl, 30:21, scratch216, 30:21);

    s(EmcPinGpio, 1:0, scratch9, 31:30);
    s(EmcPinGpioEn, 1:0, scratch10, 31:30);
    s(EmcDevSelect, 1:0, scratch11, 31:30);
    s(EmcZcalWarmColdBootEnables, 1:0, scratch12, 31:30);
    s(EmcCfgDigDllPeriodWarmBoot, 1:0, scratch13, 31:30);
    s32(EmcBctSpare13, scratch45);
    s32(EmcBctSpare12, scratch46);
    s32(EmcBctSpare7, scratch47);
    s32(EmcBctSpare6, scratch48);
    s32(EmcBctSpare5, scratch50);
    s32(EmcBctSpare4, scratch51);
    s32(EmcBctSpare3, scratch56);
    s32(EmcBctSpare2, scratch57);
    s32(EmcBctSpare1, scratch58);
    s32(EmcBctSpare0, scratch59);
    s32(EmcBctSpare9, scratch60);
    s32(EmcBctSpare8, scratch61);
    s32(BootRomPatchData, scratch62);
    s32(BootRomPatchControl, scratch63);
    s(McClkenOverrideAllWarmBoot, 0:0, scratch65, 31:31);
    s(EmcExtraRefreshNum, 2:0, scratch66, 31:29);
    s(PmcIoDpd3ReqWait, 2:0, scratch72, 30:28);
    s(EmcClkenOverrideAllWarmBoot, 0:0, scratch72, 31:31);
    s(MemoryType, 2:0, scratch73, 30:28);
    s(EmcMrsWarmBootEnable, 0:0, scratch73, 31:31);
    s(PmcIoDpd4ReqWait, 2:0, scratch74, 30:28);
    s(ClearClk2Mc1, 0:0, scratch74, 31:31);
    s(EmcWarmBootExtraModeRegWriteEnable, 0:0, scratch75, 28:28);
    s(ClkRstControllerPllmMisc2OverrideEnable, 0:0, scratch75, 29:29);
    s(EmcDbgWriteMux, 0:0, scratch75, 30:30);
    s(AhbArbitrationXbarCtrlMemInitDone, 0:0, scratch75, 31:31);
    s(EmcTimingControlWait, 7:0, scratch90, 31:24);
    s(EmcZcalWarmBootWait, 7:0, scratch91, 31:24);
    s(WarmBootWait, 7:0, scratch92, 31:24);
    s(EmcPinProgramWait, 7:0, scratch93, 31:24);
    s(EmcAutoCalWait, 9:0, scratch114, 31:22);
    s(SwizzleRankByteEncode, 15:0, scratch215, 31:16);

    switch (params->MemoryType) {
        case NvBootMemoryType_LpDdr2:
        case NvBootMemoryType_LpDdr4:
            s(EmcMrwLpddr2ZcalWarmBoot, 23:16, scratch5, 7:0);
            s(EmcMrwLpddr2ZcalWarmBoot, 7:0, scratch5, 15:8);
            s(EmcWarmBootMrwExtra, 23:16, scratch5, 23:16);
            s(EmcWarmBootMrwExtra, 7:0, scratch5, 31:24);
            s(EmcMrwLpddr2ZcalWarmBoot, 31:30, scratch6, 1:0);
            s(EmcWarmBootMrwExtra, 31:30, scratch6, 3:2);
            s(EmcMrwLpddr2ZcalWarmBoot, 27:26, scratch6, 5:4);
            s(EmcWarmBootMrwExtra, 27:26, scratch6, 7:6);
            s(EmcMrw6, 27:0, scratch8, 27:0);
            s(EmcMrw6, 31:30, scratch8, 29:28);
            s(EmcMrw8, 27:0, scratch9, 27:0);
            s(EmcMrw8, 31:30, scratch9, 29:28);
            s(EmcMrw9, 27:0, scratch10, 27:0);
            s(EmcMrw9, 31:30, scratch10, 29:28);
            s(EmcMrw10, 27:0, scratch11, 27:0);
            s(EmcMrw10, 31:30, scratch11, 29:28);
            s(EmcMrw12, 27:0, scratch12, 27:0);
            s(EmcMrw12, 31:30, scratch12, 29:28);
            s(EmcMrw13, 27:0, scratch13, 27:0);
            s(EmcMrw13, 31:30, scratch13, 29:28);
            s(EmcMrw14, 27:0, scratch14, 27:0);
            s(EmcMrw14, 31:30, scratch14, 29:28);
            s(EmcMrw1, 7:0, scratch15, 7:0);
            s(EmcMrw1, 23:16, scratch15, 15:8);
            s(EmcMrw1, 27:26, scratch15, 17:16);
            s(EmcMrw1, 31:30, scratch15, 19:18);
            s(EmcWarmBootMrwExtra, 7:0, scratch16, 7:0);
            s(EmcWarmBootMrwExtra, 23:16, scratch16, 15:8);
            s(EmcWarmBootMrwExtra, 27:26, scratch16, 17:16);
            s(EmcWarmBootMrwExtra, 31:30, scratch16, 19:18);
            s(EmcMrw2, 7:0, scratch17, 7:0);
            s(EmcMrw2, 23:16, scratch17, 15:8);
            s(EmcMrw2, 27:26, scratch17, 17:16);
            s(EmcMrw2, 31:30, scratch17, 19:18);
            s(EmcMrw3, 7:0, scratch18, 7:0);
            s(EmcMrw3, 23:16, scratch18, 15:8);
            s(EmcMrw3, 27:26, scratch18, 17:16);
            s(EmcMrw3, 31:30, scratch18, 19:18);
            s(EmcMrw4, 7:0, scratch19, 7:0);
            s(EmcMrw4, 23:16, scratch19, 15:8);
            s(EmcMrw4, 27:26, scratch19, 17:16);
            s(EmcMrw4, 31:30, scratch19, 19:18);
            break;
        case NvBootMemoryType_Ddr3:
            s(EmcMrs, 13:0, scratch5, 13:0);
            s(EmcEmrs, 13:0, scratch5, 27:14);
            s(EmcMrs, 21:20, scratch5, 29:28);
            s(EmcMrs, 31:30, scratch5, 31:30);
            s(EmcEmrs2, 13:0, scratch8, 13:0);
            s(EmcEmrs3, 13:0, scratch8, 27:14);
            s(EmcEmrs, 21:20, scratch8, 29:28);
            s(EmcWarmBootMrsExtra, 13:0, scratch9, 13:0);
            s(EmcEmrs, 31:30, scratch9, 15:14);
            s(EmcEmrs2, 21:20, scratch9, 17:16);
            s(EmcEmrs2, 31:30, scratch9, 19:18);
            s(EmcEmrs3, 21:20, scratch9, 21:20);
            s(EmcEmrs3, 31:30, scratch9, 23:22);
            s(EmcWarmBootMrsExtra, 31:30, scratch9, 25:24);
            s(EmcWarmBootMrsExtra, 21:20, scratch9, 27:26);
            s(EmcZqCalDdr3WarmBoot, 31:30, scratch9, 29:28);
            s(EmcMrs, 27:26, scratch10, 1:0);
            s(EmcEmrs, 27:26, scratch10, 3:2);
            s(EmcEmrs2, 27:26, scratch10, 5:4);
            s(EmcEmrs3, 27:26, scratch10, 7:6);
            s(EmcWarmBootMrsExtra, 27:27, scratch10, 8:8);
            s(EmcWarmBootMrsExtra, 26:26, scratch10, 9:9);
            s(EmcZqCalDdr3WarmBoot, 0:0, scratch10, 10:10);
            s(EmcZqCalDdr3WarmBoot, 4:4, scratch10, 11:11);
            break;
        default: break;
    }

    s32(EmcCmdMappingByte, secure_scratch8);
    s32(EmcPmacroBrickMapping0, secure_scratch9);
    s32(EmcPmacroBrickMapping1, secure_scratch10);
    s32(EmcPmacroBrickMapping2, secure_scratch11);
    s32(McVideoProtectGpuOverride0, secure_scratch12);
    s(EmcCmdMappingCmd0_0, 6:0, secure_scratch13, 6:0);
    s(EmcCmdMappingCmd0_0, 14:8, secure_scratch13, 13:7);
    s(EmcCmdMappingCmd0_0, 22:16, secure_scratch13, 20:14);
    s(EmcCmdMappingCmd0_0, 30:24, secure_scratch13, 27:21);
    s(McUntranslatedRegionCheck, 0:0, secure_scratch13, 28:28);
    s(McUntranslatedRegionCheck, 9:8, secure_scratch13, 30:29);
    s(EmcAdrCfg, 0:0, secure_scratch13, 31:31);
    s(EmcCmdMappingCmd0_1, 6:0, secure_scratch14, 6:0);
    s(EmcCmdMappingCmd0_1, 14:8, secure_scratch14, 13:7);
    s(EmcCmdMappingCmd0_1, 22:16, secure_scratch14, 20:14);
    s(EmcCmdMappingCmd0_1, 30:24, secure_scratch14, 27:21);
    s(McVideoProtectBomAdrHi, 1:0, secure_scratch14, 29:28);
    s(McVideoProtectWriteAccess, 1:0, secure_scratch14, 31:30);
    s(EmcCmdMappingCmd1_0, 6:0, secure_scratch15, 6:0);
    s(EmcCmdMappingCmd1_0, 14:8, secure_scratch15, 13:7);
    s(EmcCmdMappingCmd1_0, 22:16, secure_scratch15, 20:14);
    s(EmcCmdMappingCmd1_0, 30:24, secure_scratch15, 27:21);
    s(McSecCarveoutAdrHi, 1:0, secure_scratch15, 29:28);
    s(McMtsCarveoutAdrHi, 1:0, secure_scratch15, 31:30);
    s(EmcCmdMappingCmd1_1, 6:0, secure_scratch16, 6:0);
    s(EmcCmdMappingCmd1_1, 14:8, secure_scratch16, 13:7);
    s(EmcCmdMappingCmd1_1, 22:16, secure_scratch16, 20:14);
    s(EmcCmdMappingCmd1_1, 30:24, secure_scratch16, 27:21);
    s(McGeneralizedCarveout5BomHi, 1:0, secure_scratch16, 29:28);
    s(McGeneralizedCarveout3BomHi, 1:0, secure_scratch16, 31:30);
    s(EmcCmdMappingCmd2_0, 6:0, secure_scratch17, 6:0);
    s(EmcCmdMappingCmd2_0, 14:8, secure_scratch17, 13:7);
    s(EmcCmdMappingCmd2_0, 22:16, secure_scratch17, 20:14);
    s(EmcCmdMappingCmd2_0, 30:24, secure_scratch17, 27:21);
    s(McGeneralizedCarveout2BomHi, 1:0, secure_scratch17, 29:28);
    s(McGeneralizedCarveout4BomHi, 1:0, secure_scratch17, 31:30);
    s(EmcCmdMappingCmd2_1, 6:0, secure_scratch18, 6:0);
    s(EmcCmdMappingCmd2_1, 14:8, secure_scratch18, 13:7);
    s(EmcCmdMappingCmd2_1, 22:16, secure_scratch18, 20:14);
    s(EmcCmdMappingCmd2_1, 30:24, secure_scratch18, 27:21);
    s(McGeneralizedCarveout1BomHi, 1:0, secure_scratch18, 29:28);
    s(EmcFbioSpare, 1:1, secure_scratch18, 30:30);
    s(EmcFbioCfg8, 15:15, secure_scratch18, 31:31);
    s(EmcCmdMappingCmd3_0, 6:0, secure_scratch19, 6:0);
    s(EmcCmdMappingCmd3_0, 14:8, secure_scratch19, 13:7);
    s(EmcCmdMappingCmd3_0, 22:16, secure_scratch19, 20:14);
    s(EmcCmdMappingCmd3_0, 30:24, secure_scratch19, 27:21);
    s(McEmemAdrCfg, 0:0, secure_scratch19, 28:28);
    s(McSecCarveoutProtectWriteAccess, 0:0, secure_scratch19, 29:29);
    s(McMtsCarveoutRegCtrl, 0:0, secure_scratch19, 30:30);
    s(McVideoProtectVprOverride, 0:0, secure_scratch19, 31:31);
    s(EmcCmdMappingCmd3_1, 6:0, secure_scratch20, 6:0);
    s(EmcCmdMappingCmd3_1, 14:8, secure_scratch20, 13:7);
    s(EmcCmdMappingCmd3_1, 22:16, secure_scratch20, 20:14);
    s(EmcCmdMappingCmd3_1, 30:24, secure_scratch20, 27:21);
    s(McGeneralizedCarveout2Cfg0, 6:3, secure_scratch20, 31:28);
    s(McGeneralizedCarveout4Cfg0, 26:0, secure_scratch39, 26:0);
    s(McGeneralizedCarveout2Cfg0, 10:7, secure_scratch39, 30:27);
    s(McVideoProtectVprOverride, 1:1, secure_scratch39, 31:31);
    s(McGeneralizedCarveout5Cfg0, 26:0, secure_scratch40, 26:0);
    s(McGeneralizedCarveout2Cfg0, 17:14, secure_scratch40, 30:27);
    s(McVideoProtectVprOverride, 2:2, secure_scratch40, 31:31);
    s(EmcCmdMappingCmd0_2, 6:0, secure_scratch41, 6:0);
    s(EmcCmdMappingCmd0_2, 14:8, secure_scratch41, 13:7);
    s(EmcCmdMappingCmd0_2, 22:16, secure_scratch41, 20:14);
    s(EmcCmdMappingCmd0_2, 27:24, secure_scratch41, 24:21);
    s(McGeneralizedCarveout2Cfg0, 21:18, secure_scratch41, 28:25);
    s(McGeneralizedCarveout2Cfg0, 13:11, secure_scratch41, 31:29);
    s(EmcCmdMappingCmd1_2, 6:0, secure_scratch42, 6:0);
    s(EmcCmdMappingCmd1_2, 14:8, secure_scratch42, 13:7);
    s(EmcCmdMappingCmd1_2, 22:16, secure_scratch42, 20:14);
    s(EmcCmdMappingCmd1_2, 27:24, secure_scratch42, 24:21);
    s(McGeneralizedCarveout1Cfg0, 6:3, secure_scratch42, 28:25);
    s(McGeneralizedCarveout1Cfg0, 13:11, secure_scratch42, 31:29);
    s(EmcCmdMappingCmd2_2, 6:0, secure_scratch43, 6:0);
    s(EmcCmdMappingCmd2_2, 14:8, secure_scratch43, 13:7);
    s(EmcCmdMappingCmd2_2, 22:16, secure_scratch43, 20:14);
    s(EmcCmdMappingCmd2_2, 27:24, secure_scratch43, 24:21);
    s(McGeneralizedCarveout1Cfg0, 10:7, secure_scratch43, 28:25);
    s(McGeneralizedCarveout3Cfg0, 13:11, secure_scratch43, 31:29);
    s(EmcCmdMappingCmd3_2, 6:0, secure_scratch44, 6:0);
    s(EmcCmdMappingCmd3_2, 14:8, secure_scratch44, 13:7);
    s(EmcCmdMappingCmd3_2, 22:16, secure_scratch44, 20:14);
    s(EmcCmdMappingCmd3_2, 27:24, secure_scratch44, 24:21);
    s(McGeneralizedCarveout1Cfg0, 17:14, secure_scratch44, 28:25);
    s(McVideoProtectVprOverride, 3:3, secure_scratch44, 29:29);
    s(McVideoProtectVprOverride, 7:6, secure_scratch44, 31:30);
    s(McEmemAdrCfgChannelMask, 31:9, secure_scratch45, 22:0);
    s(McEmemAdrCfgDev0, 2:0, secure_scratch45, 25:23);
    s(McEmemAdrCfgDev0, 9:8, secure_scratch45, 27:26);
    s(McEmemAdrCfgDev0, 19:16, secure_scratch45, 31:28);
    s(McEmemAdrCfgBankMask0, 31:10, secure_scratch46, 21:0);
    s(McEmemAdrCfgDev1, 2:0, secure_scratch46, 24:22);
    s(McEmemAdrCfgDev1, 9:8, secure_scratch46, 26:25);
    s(McEmemAdrCfgDev1, 19:16, secure_scratch46, 30:27);
    s(McVideoProtectVprOverride, 8:8, secure_scratch46, 31:31);
    s(McEmemAdrCfgBankMask1, 31:10, secure_scratch47, 21:0);
    s(McGeneralizedCarveout1Cfg0, 21:18, secure_scratch47, 25:22);
    s(McGeneralizedCarveout3Cfg0, 6:3, secure_scratch47, 29:26);
    s(McVideoProtectVprOverride, 9:9, secure_scratch47, 30:30);
    s(McVideoProtectVprOverride, 11:11, secure_scratch47, 31:31);
    s(McEmemAdrCfgBankMask2, 31:10, secure_scratch48, 21:0);
    s(McGeneralizedCarveout3Cfg0, 10:7, secure_scratch48, 25:22);
    s(McGeneralizedCarveout3Cfg0, 17:14, secure_scratch48, 29:26);
    s(McVideoProtectVprOverride, 15:14, secure_scratch48, 31:30);
    s(McVideoProtectGpuOverride1, 15:0, secure_scratch49, 15:0);
    s(McEmemCfg, 13:0, secure_scratch49, 29:16);
    s(McEmemCfg, 31:31, secure_scratch49, 30:30);
    s(McVideoProtectVprOverride, 17:17, secure_scratch49, 31:31);
    s(McGeneralizedCarveout3Bom, 31:17, secure_scratch50, 14:0);
    s(McGeneralizedCarveout1Bom, 31:17, secure_scratch50, 29:15);
    s(McVideoProtectVprOverride, 19:18, secure_scratch50, 31:30);
    s(McGeneralizedCarveout4Bom, 31:17, secure_scratch51, 14:0);
    s(McGeneralizedCarveout2Bom, 31:17, secure_scratch51, 29:15);
    s(McVideoProtectVprOverride, 21:20, secure_scratch51, 31:30);
    s(McGeneralizedCarveout5Bom, 31:17, secure_scratch52, 14:0);
    s(McVideoProtectBom, 31:20, secure_scratch52, 26:15);
    s(McGeneralizedCarveout3Cfg0, 21:18, secure_scratch52, 30:27);
    s(McVideoProtectVprOverride, 22:22, secure_scratch52, 31:31);
    s(McVideoProtectSizeMb, 11:0, secure_scratch53, 11:0);
    s(McSecCarveoutBom, 31:20, secure_scratch53, 23:12);
    s(McVideoProtectVprOverride, 23:23, secure_scratch53, 24:24);
    s(McVideoProtectVprOverride, 26:26, secure_scratch53, 25:25);
    s(McVideoProtectVprOverride, 31:29, secure_scratch53, 28:26);
    s(McVideoProtectVprOverride1, 1:0, secure_scratch53, 30:29);
    s(McVideoProtectVprOverride1, 4:4, secure_scratch53, 31:31);
    s(McSecCarveoutSizeMb, 11:0, secure_scratch54, 11:0);
    s(McMtsCarveoutBom, 31:20, secure_scratch54, 23:12);
    s(McVideoProtectVprOverride1, 12:5, secure_scratch54, 31:24);
    s(McMtsCarveoutSizeMb, 11:0, secure_scratch55, 11:0);
    s(McGeneralizedCarveout4Size128kb, 11:0, secure_scratch55, 23:12);
    s(McVideoProtectVprOverride1, 16:13, secure_scratch55, 27:24);
    s(McVideoProtectVprOverride1, 26:25, secure_scratch55, 29:28);
    s(McGeneralizedCarveout2Cfg0, 1:0, secure_scratch55, 31:30);
    s(McGeneralizedCarveout3Size128kb, 11:0, secure_scratch56, 11:0);
    s(McGeneralizedCarveout2Size128kb, 11:0, secure_scratch56, 23:12);
    s(McGeneralizedCarveout2Cfg0, 2:2, secure_scratch56, 24:24);
    s(McGeneralizedCarveout2Cfg0, 26:22, secure_scratch56, 29:25);
    s(McGeneralizedCarveout1Cfg0, 1:0, secure_scratch56, 31:30);
    s(McGeneralizedCarveout1Size128kb, 11:0, secure_scratch57, 11:0);
    s(McGeneralizedCarveout5Size128kb, 11:0, secure_scratch57, 23:12);
    s(McGeneralizedCarveout1Cfg0, 2:2, secure_scratch57, 24:24);
    s(McGeneralizedCarveout1Cfg0, 26:22, secure_scratch57, 29:25);
    s(McGeneralizedCarveout3Cfg0, 1:0, secure_scratch57, 31:30);
    s(McGeneralizedCarveout3Cfg0, 2:2, secure_scratch58, 0:0);
    s(McGeneralizedCarveout3Cfg0, 26:22, secure_scratch58, 5:1);

    s32(McGeneralizedCarveout1Access0, secure_scratch59);
    s32(McGeneralizedCarveout1Access1, secure_scratch60);
    s32(McGeneralizedCarveout1Access2, secure_scratch61);
    s32(McGeneralizedCarveout1Access3, secure_scratch62);
    s32(McGeneralizedCarveout1Access4, secure_scratch63);
    s32(McGeneralizedCarveout2Access0, secure_scratch64);
    s32(McGeneralizedCarveout2Access1, secure_scratch65);
    s32(McGeneralizedCarveout2Access2, secure_scratch66);
    s32(McGeneralizedCarveout2Access3, secure_scratch67);
    s32(McGeneralizedCarveout2Access4, secure_scratch68);
    s32(McGeneralizedCarveout3Access0, secure_scratch69);
    s32(McGeneralizedCarveout3Access1, secure_scratch70);
    s32(McGeneralizedCarveout3Access2, secure_scratch71);
    s32(McGeneralizedCarveout3Access3, secure_scratch72);
    s32(McGeneralizedCarveout3Access4, secure_scratch73);
    s32(McGeneralizedCarveout4Access0, secure_scratch74);
    s32(McGeneralizedCarveout4Access1, secure_scratch75);
    s32(McGeneralizedCarveout4Access2, secure_scratch76);
    s32(McGeneralizedCarveout4Access3, secure_scratch77);
    s32(McGeneralizedCarveout4Access4, secure_scratch78);
    s32(McGeneralizedCarveout5Access0, secure_scratch79);
    s32(McGeneralizedCarveout5Access1, secure_scratch80);
    s32(McGeneralizedCarveout5Access2, secure_scratch81);
    s32(McGeneralizedCarveout5Access3, secure_scratch82);
    s32(McGeneralizedCarveout1ForceInternalAccess0, secure_scratch84);
    s32(McGeneralizedCarveout1ForceInternalAccess1, secure_scratch85);
    s32(McGeneralizedCarveout1ForceInternalAccess2, secure_scratch86);
    s32(McGeneralizedCarveout1ForceInternalAccess3, secure_scratch87);
    s32(McGeneralizedCarveout1ForceInternalAccess4, secure_scratch88);
    s32(McGeneralizedCarveout2ForceInternalAccess0, secure_scratch89);
    s32(McGeneralizedCarveout2ForceInternalAccess1, secure_scratch90);
    s32(McGeneralizedCarveout2ForceInternalAccess2, secure_scratch91);
    s32(McGeneralizedCarveout2ForceInternalAccess3, secure_scratch92);
    s32(McGeneralizedCarveout2ForceInternalAccess4, secure_scratch93);
    s32(McGeneralizedCarveout3ForceInternalAccess0, secure_scratch94);
    s32(McGeneralizedCarveout3ForceInternalAccess1, secure_scratch95);
    s32(McGeneralizedCarveout3ForceInternalAccess2, secure_scratch96);
    s32(McGeneralizedCarveout3ForceInternalAccess3, secure_scratch97);
    s32(McGeneralizedCarveout3ForceInternalAccess4, secure_scratch98);
    s32(McGeneralizedCarveout4ForceInternalAccess0, secure_scratch99);
    s32(McGeneralizedCarveout4ForceInternalAccess1, secure_scratch100);
    s32(McGeneralizedCarveout4ForceInternalAccess2, secure_scratch101);
    s32(McGeneralizedCarveout4ForceInternalAccess3, secure_scratch102);
    s32(McGeneralizedCarveout4ForceInternalAccess4, secure_scratch103);
    s32(McGeneralizedCarveout5ForceInternalAccess0, secure_scratch104);
    s32(McGeneralizedCarveout5ForceInternalAccess1, secure_scratch105);
    s32(McGeneralizedCarveout5ForceInternalAccess2, secure_scratch106);
    s32(McGeneralizedCarveout5ForceInternalAccess3, secure_scratch107);

    c32(0, scratch2);
    s(PllMInputDivider, 7:0, scratch2, 7:0);
    s(PllMFeedbackDivider, 7:0, scratch2, 15:8);
    s(PllMPostDivider, 0:0, scratch2, 16:16);
    s(PllMKVCO, 0:0, scratch2, 17:17);
    s(PllMKCP, 1:0, scratch2, 19:18);

    c32(0, scratch35);
    s(PllMSetupControl, 27:0, scratch35, 27:0);

    c32(0, scratch3);
    s(PllMInputDivider, 7:0, scratch3, 7:0);
    c(0x3e, scratch3, 15:8);
    c(0, scratch3, 20:16);
    s(PllMKVCO, 0:0, scratch3, 21:21);
    s(PllMKCP, 1:0, scratch3, 23:22);

    c32(0, scratch36);
    s(PllMSetupControl, 23:0, scratch36, 23:0);

    c32(0, scratch4);
    s(PllMStableTime, 9:0, scratch4, 9:0);
    s(PllMStableTime, 31:0, scratch4, 31:10);
}

void sdram_init(void) {
    if (is_soc_mariko()) {
        sdram_init_mariko();
    } else {
        sdram_init_erista();
    }
}

const void *sdram_get_params(uint32_t dram_id) {
    if (is_soc_mariko()) {
        return sdram_get_params_mariko(dram_id);
    } else {
        return sdram_get_params_erista(dram_id);
    }
}

void sdram_save_params(const void *save_params) {
    if (is_soc_mariko()) {
        sdram_save_params_mariko(save_params);
    } else {
        sdram_save_params_erista(save_params);
    }
}