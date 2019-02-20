/*
 * Copyright (c) 2013-2015, NVIDIA CORPORATION.  All rights reserved.
 * Copyright 2014 Google Inc.
 * Copyright (c) 2018 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

/**
 * Defines the SDRAM parameter structure.
 *
 * Note that PLLM is used by EMC. The field names are in camel case to ease
 * directly converting BCT config files (*.cfg) into C structure.
 */

#ifndef __SOC_NVIDIA_TEGRA210_SDRAM_PARAM_H__
#define __SOC_NVIDIA_TEGRA210_SDRAM_PARAM_H__

#include <stdint.h>

enum
{
    /* Specifies the memory type to be undefined */
    NvBootMemoryType_None = 0,

    /* Specifies the memory type to be DDR SDRAM */
    NvBootMemoryType_Ddr = 0,

    /* Specifies the memory type to be LPDDR SDRAM */
    NvBootMemoryType_LpDdr = 0,

    /* Specifies the memory type to be DDR2 SDRAM */
    NvBootMemoryType_Ddr2 = 0,

    /* Specifies the memory type to be LPDDR2 SDRAM */
    NvBootMemoryType_LpDdr2,

    /* Specifies the memory type to be DDR3 SDRAM */
    NvBootMemoryType_Ddr3,

    /* Specifies the memory type to be LPDDR4 SDRAM */
    NvBootMemoryType_LpDdr4,

    NvBootMemoryType_Num,

    /* Specifies an entry in the ram_code table that's not in use */
    NvBootMemoryType_Unused = 0X7FFFFFF,
};

/**
 * Defines the SDRAM parameter structure
 */
struct sdram_params
{

    /* Specifies the type of memory device */
    uint32_t MemoryType;

    /* MC/EMC clock source configuration */

    /* Specifies the M value for PllM */
    uint32_t PllMInputDivider;
    /* Specifies the N value for PllM */
    uint32_t PllMFeedbackDivider;
    /* Specifies the time to wait for PLLM to lock (in microseconds) */
    uint32_t PllMStableTime;
    /* Specifies misc. control bits */
    uint32_t PllMSetupControl;
    /* Specifies the P value for PLLM */
    uint32_t PllMPostDivider;
    /* Specifies value for Charge Pump Gain Control */
    uint32_t PllMKCP;
    /* Specifies VCO gain */
    uint32_t PllMKVCO;
    /* Spare BCT param */
    uint32_t EmcBctSpare0;
    /* Spare BCT param */
    uint32_t EmcBctSpare1;
    /* Spare BCT param */
    uint32_t EmcBctSpare2;
    /* Spare BCT param */
    uint32_t EmcBctSpare3;
    /* Spare BCT param */
    uint32_t EmcBctSpare4;
    /* Spare BCT param */
    uint32_t EmcBctSpare5;
    /* Spare BCT param */
    uint32_t EmcBctSpare6;
    /* Spare BCT param */
    uint32_t EmcBctSpare7;
    /* Spare BCT param */
    uint32_t EmcBctSpare8;
    /* Spare BCT param */
    uint32_t EmcBctSpare9;
    /* Spare BCT param */
    uint32_t EmcBctSpare10;
    /* Spare BCT param */
    uint32_t EmcBctSpare11;
    /* Spare BCT param */
    uint32_t EmcBctSpare12;
    /* Spare BCT param */
    uint32_t EmcBctSpare13;

    /* Defines EMC_2X_CLK_SRC, EMC_2X_CLK_DIVISOR, EMC_INVERT_DCD */
    uint32_t EmcClockSource;
    uint32_t EmcClockSourceDll;

    /* Defines possible override for PLLLM_MISC2 */
    uint32_t ClkRstControllerPllmMisc2Override;
    /* enables override for PLLLM_MISC2 */
    uint32_t ClkRstControllerPllmMisc2OverrideEnable;
    /* defines CLK_ENB_MC1 in register clk_rst_controller_clk_enb_w_clr */
    uint32_t ClearClk2Mc1;

    /* Auto-calibration of EMC pads */

    /* Specifies the value for EMC_AUTO_CAL_INTERVAL */
    uint32_t EmcAutoCalInterval;
    /*
     * Specifies the value for EMC_AUTO_CAL_CONFIG
     * Note: Trigger bits are set by the SDRAM code.
     */
    uint32_t EmcAutoCalConfig;

    /* Specifies the value for EMC_AUTO_CAL_CONFIG2 */
    uint32_t EmcAutoCalConfig2;

    /* Specifies the value for EMC_AUTO_CAL_CONFIG3 */
    uint32_t EmcAutoCalConfig3;

    /* Specifies the values for EMC_AUTO_CAL_CONFIG4-8 */
    uint32_t EmcAutoCalConfig4;
    uint32_t EmcAutoCalConfig5;
    uint32_t EmcAutoCalConfig6;
    uint32_t EmcAutoCalConfig7;
    uint32_t EmcAutoCalConfig8;

    /* Specifies the value for EMC_AUTO_CAL_VREF_SEL_0 */
    uint32_t EmcAutoCalVrefSel0;
    uint32_t EmcAutoCalVrefSel1;

    /* Specifies the value for EMC_AUTO_CAL_CHANNEL */
    uint32_t EmcAutoCalChannel;

    /* Specifies the value for EMC_PMACRO_AUTOCAL_CFG_0 */
    uint32_t EmcPmacroAutocalCfg0;
    uint32_t EmcPmacroAutocalCfg1;
    uint32_t EmcPmacroAutocalCfg2;
    uint32_t EmcPmacroRxTerm;
    uint32_t EmcPmacroDqTxDrv;
    uint32_t EmcPmacroCaTxDrv;
    uint32_t EmcPmacroCmdTxDrv;
    uint32_t EmcPmacroAutocalCfgCommon;
    uint32_t EmcPmacroZctrl;

    /*
     * Specifies the time for the calibration
     * to stabilize (in microseconds)
     */
    uint32_t EmcAutoCalWait;

    uint32_t EmcXm2CompPadCtrl;
    uint32_t EmcXm2CompPadCtrl2;
    uint32_t EmcXm2CompPadCtrl3;

    /*
     * DRAM size information
     * Specifies the value for EMC_ADR_CFG
     */
    uint32_t EmcAdrCfg;

    /*
     * Specifies the time to wait after asserting pin
     * CKE (in microseconds)
     */
    uint32_t EmcPinProgramWait;
    /* Specifies the extra delay before/after pin RESET/CKE command */
    uint32_t EmcPinExtraWait;

    uint32_t EmcPinGpioEn;
    uint32_t EmcPinGpio;

    /*
     * Specifies the extra delay after the first writing
     * of EMC_TIMING_CONTROL
     */
    uint32_t EmcTimingControlWait;

    /* Timing parameters required for the SDRAM */

    /* Specifies the value for EMC_RC */
    uint32_t EmcRc;
    /* Specifies the value for EMC_RFC */
    uint32_t EmcRfc;
    /* Specifies the value for EMC_RFC_PB */
    uint32_t EmcRfcPb;
    /* Specifies the value for EMC_RFC_CTRL2 */
    uint32_t EmcRefctrl2;
    /* Specifies the value for EMC_RFC_SLR */
    uint32_t EmcRfcSlr;
    /* Specifies the value for EMC_RAS */
    uint32_t EmcRas;
    /* Specifies the value for EMC_RP */
    uint32_t EmcRp;
    /* Specifies the value for EMC_R2R */
    uint32_t EmcR2r;
    /* Specifies the value for EMC_W2W */
    uint32_t EmcW2w;
    /* Specifies the value for EMC_R2W */
    uint32_t EmcR2w;
    /* Specifies the value for EMC_W2R */
    uint32_t EmcW2r;
    /* Specifies the value for EMC_R2P */
    uint32_t EmcR2p;
    /* Specifies the value for EMC_W2P */
    uint32_t EmcW2p;

    uint32_t EmcTppd;
    uint32_t EmcCcdmw;

    /* Specifies the value for EMC_RD_RCD */
    uint32_t EmcRdRcd;
    /* Specifies the value for EMC_WR_RCD */
    uint32_t EmcWrRcd;
    /* Specifies the value for EMC_RRD */
    uint32_t EmcRrd;
    /* Specifies the value for EMC_REXT */
    uint32_t EmcRext;
    /* Specifies the value for EMC_WEXT */
    uint32_t EmcWext;
    /* Specifies the value for EMC_WDV */
    uint32_t EmcWdv;

    uint32_t EmcWdvChk;
    uint32_t EmcWsv;
    uint32_t EmcWev;

    /* Specifies the value for EMC_WDV_MASK */
    uint32_t EmcWdvMask;

    uint32_t EmcWsDuration;
    uint32_t EmcWeDuration;

    /* Specifies the value for EMC_QUSE */
    uint32_t EmcQUse;
    /* Specifies the value for EMC_QUSE_WIDTH */
    uint32_t EmcQuseWidth;
    /* Specifies the value for EMC_IBDLY */
    uint32_t EmcIbdly;
    /* Specifies the value for EMC_OBDLY */
    uint32_t EmcObdly;
    /* Specifies the value for EMC_EINPUT */
    uint32_t EmcEInput;
    /* Specifies the value for EMC_EINPUT_DURATION */
    uint32_t EmcEInputDuration;
    /* Specifies the value for EMC_PUTERM_EXTRA */
    uint32_t EmcPutermExtra;
    /* Specifies the value for EMC_PUTERM_WIDTH */
    uint32_t EmcPutermWidth;
    /* Specifies the value for EMC_PUTERM_ADJ */
    ////uint32_t EmcPutermAdj;

    /* Specifies the value for EMC_QRST */
    uint32_t EmcQRst;
    /* Specifies the value for EMC_QSAFE */
    uint32_t EmcQSafe;
    /* Specifies the value for EMC_RDV */
    uint32_t EmcRdv;
    /* Specifies the value for EMC_RDV_MASK */
    uint32_t EmcRdvMask;
    /* Specifies the value for EMC_RDV_EARLY */
    uint32_t EmcRdvEarly;
    /* Specifies the value for EMC_RDV_EARLY_MASK */
    uint32_t EmcRdvEarlyMask;
    /* Specifies the value for EMC_QPOP */
    uint32_t EmcQpop;

    /* Specifies the value for EMC_REFRESH */
    uint32_t EmcRefresh;
    /* Specifies the value for EMC_BURST_REFRESH_NUM */
    uint32_t EmcBurstRefreshNum;
    /* Specifies the value for EMC_PRE_REFRESH_REQ_CNT */
    uint32_t EmcPreRefreshReqCnt;
    /* Specifies the value for EMC_PDEX2WR */
    uint32_t EmcPdEx2Wr;
    /* Specifies the value for EMC_PDEX2RD */
    uint32_t EmcPdEx2Rd;
    /* Specifies the value for EMC_PCHG2PDEN */
    uint32_t EmcPChg2Pden;
    /* Specifies the value for EMC_ACT2PDEN */
    uint32_t EmcAct2Pden;
    /* Specifies the value for EMC_AR2PDEN */
    uint32_t EmcAr2Pden;
    /* Specifies the value for EMC_RW2PDEN */
    uint32_t EmcRw2Pden;
    /* Specifies the value for EMC_CKE2PDEN */
    uint32_t EmcCke2Pden;
    /* Specifies the value for EMC_PDEX2CKE */
    uint32_t EmcPdex2Cke;
    /* Specifies the value for EMC_PDEX2MRR */
    uint32_t EmcPdex2Mrr;
    /* Specifies the value for EMC_TXSR */
    uint32_t EmcTxsr;
    /* Specifies the value for EMC_TXSRDLL */
    uint32_t EmcTxsrDll;
    /* Specifies the value for EMC_TCKE */
    uint32_t EmcTcke;
    /* Specifies the value for EMC_TCKESR */
    uint32_t EmcTckesr;
    /* Specifies the value for EMC_TPD */
    uint32_t EmcTpd;
    /* Specifies the value for EMC_TFAW */
    uint32_t EmcTfaw;
    /* Specifies the value for EMC_TRPAB */
    uint32_t EmcTrpab;
    /* Specifies the value for EMC_TCLKSTABLE */
    uint32_t EmcTClkStable;
    /* Specifies the value for EMC_TCLKSTOP */
    uint32_t EmcTClkStop;
    /* Specifies the value for EMC_TREFBW */
    uint32_t EmcTRefBw;

    /* FBIO configuration values */

    /* Specifies the value for EMC_FBIO_CFG5 */
    uint32_t EmcFbioCfg5;
    /* Specifies the value for EMC_FBIO_CFG7 */
    uint32_t EmcFbioCfg7;
    /* Specifies the value for EMC_FBIO_CFG8 */
    uint32_t EmcFbioCfg8;

    /* Command mapping for CMD brick 0 */
    uint32_t EmcCmdMappingCmd0_0;
    uint32_t EmcCmdMappingCmd0_1;
    uint32_t EmcCmdMappingCmd0_2;
    uint32_t EmcCmdMappingCmd1_0;
    uint32_t EmcCmdMappingCmd1_1;
    uint32_t EmcCmdMappingCmd1_2;
    uint32_t EmcCmdMappingCmd2_0;
    uint32_t EmcCmdMappingCmd2_1;
    uint32_t EmcCmdMappingCmd2_2;
    uint32_t EmcCmdMappingCmd3_0;
    uint32_t EmcCmdMappingCmd3_1;
    uint32_t EmcCmdMappingCmd3_2;
    uint32_t EmcCmdMappingByte;

    /* Specifies the value for EMC_FBIO_SPARE */
    uint32_t EmcFbioSpare;

    /* Specifies the value for EMC_CFG_RSV */
    uint32_t EmcCfgRsv;

    /* MRS command values */

    /* Specifies the value for EMC_MRS */
    uint32_t EmcMrs;
    /* Specifies the MP0 command to initialize mode registers */
    uint32_t EmcEmrs;
    /* Specifies the MP2 command to initialize mode registers */
    uint32_t EmcEmrs2;
    /* Specifies the MP3 command to initialize mode registers */
    uint32_t EmcEmrs3;
    /* Specifies the programming to LPDDR2 Mode Register 1 at cold boot */
    uint32_t EmcMrw1;
    /* Specifies the programming to LPDDR2 Mode Register 2 at cold boot */
    uint32_t EmcMrw2;
    /* Specifies the programming to LPDDR2 Mode Register 3 at cold boot */
    uint32_t EmcMrw3;
    /* Specifies the programming to LPDDR2 Mode Register 11 at cold boot */
    uint32_t EmcMrw4;
    /* Specifies the programming to LPDDR2 Mode Register 3? at cold boot */
    uint32_t EmcMrw6;
    /* Specifies the programming to LPDDR2 Mode Register 11 at cold boot */
    uint32_t EmcMrw8;
    /* Specifies the programming to LPDDR2 Mode Register 11? at cold boot */
    uint32_t EmcMrw9;
    /* Specifies the programming to LPDDR2 Mode Register 12 at cold boot */
    uint32_t EmcMrw10;
    /* Specifies the programming to LPDDR2 Mode Register 14 at cold boot */
    uint32_t EmcMrw12;
    /* Specifies the programming to LPDDR2 Mode Register 14? at cold boot */
    uint32_t EmcMrw13;
    /* Specifies the programming to LPDDR2 Mode Register 22 at cold boot */
    uint32_t EmcMrw14;
    /*
     * Specifies the programming to extra LPDDR2 Mode Register
     * at cold boot
     */
    uint32_t EmcMrwExtra;
    /*
     * Specifies the programming to extra LPDDR2 Mode Register
     * at warm boot
     */
    uint32_t EmcWarmBootMrwExtra;
    /*
     * Specify the enable of extra Mode Register programming at
     * warm boot
     */
    uint32_t EmcWarmBootExtraModeRegWriteEnable;
    /*
     * Specify the enable of extra Mode Register programming at
     * cold boot
     */
    uint32_t EmcExtraModeRegWriteEnable;

    /* Specifies the EMC_MRW reset command value */
    uint32_t EmcMrwResetCommand;
    /* Specifies the EMC Reset wait time (in microseconds) */
    uint32_t EmcMrwResetNInitWait;
    /* Specifies the value for EMC_MRS_WAIT_CNT */
    uint32_t EmcMrsWaitCnt;
    /* Specifies the value for EMC_MRS_WAIT_CNT2 */
    uint32_t EmcMrsWaitCnt2;

    /* EMC miscellaneous configurations */

    /* Specifies the value for EMC_CFG */
    uint32_t EmcCfg;
    /* Specifies the value for EMC_CFG_2 */
    uint32_t EmcCfg2;
    /* Specifies the pipe bypass controls */
    uint32_t EmcCfgPipe;
    uint32_t EmcCfgPipeClk;
    uint32_t EmcFdpdCtrlCmdNoRamp;
    uint32_t EmcCfgUpdate;

    /* Specifies the value for EMC_DBG */
    uint32_t EmcDbg;
    uint32_t EmcDbgWriteMux;

    /* Specifies the value for EMC_CMDQ */
    uint32_t EmcCmdQ;
    /* Specifies the value for EMC_MC2EMCQ */
    uint32_t EmcMc2EmcQ;
    /* Specifies the value for EMC_DYN_SELF_REF_CONTROL */
    uint32_t EmcDynSelfRefControl;

    /* Specifies the value for MEM_INIT_DONE */
    uint32_t AhbArbitrationXbarCtrlMemInitDone;

    /* Specifies the value for EMC_CFG_DIG_DLL */
    uint32_t EmcCfgDigDll;
    uint32_t EmcCfgDigDll_1;
    /* Specifies the value for EMC_CFG_DIG_DLL_PERIOD */
    uint32_t EmcCfgDigDllPeriod;
    /* Specifies the value of *DEV_SELECTN of various EMC registers */
    uint32_t EmcDevSelect;

    /* Specifies the value for EMC_SEL_DPD_CTRL */
    uint32_t EmcSelDpdCtrl;

    /* Pads trimmer delays */
    uint32_t EmcFdpdCtrlDq;
    uint32_t EmcFdpdCtrlCmd;
    uint32_t EmcPmacroIbVrefDq_0;
    uint32_t EmcPmacroIbVrefDq_1;
    uint32_t EmcPmacroIbVrefDqs_0;
    uint32_t EmcPmacroIbVrefDqs_1;
    uint32_t EmcPmacroIbRxrt;
    uint32_t EmcCfgPipe1;
    uint32_t EmcCfgPipe2;

    /* Specifies the value for EMC_PMACRO_QUSE_DDLL_RANK0_0 */
    uint32_t EmcPmacroQuseDdllRank0_0;
    uint32_t EmcPmacroQuseDdllRank0_1;
    uint32_t EmcPmacroQuseDdllRank0_2;
    uint32_t EmcPmacroQuseDdllRank0_3;
    uint32_t EmcPmacroQuseDdllRank0_4;
    uint32_t EmcPmacroQuseDdllRank0_5;
    uint32_t EmcPmacroQuseDdllRank1_0;
    uint32_t EmcPmacroQuseDdllRank1_1;
    uint32_t EmcPmacroQuseDdllRank1_2;
    uint32_t EmcPmacroQuseDdllRank1_3;
    uint32_t EmcPmacroQuseDdllRank1_4;
    uint32_t EmcPmacroQuseDdllRank1_5;

    uint32_t EmcPmacroObDdllLongDqRank0_0;
    uint32_t EmcPmacroObDdllLongDqRank0_1;
    uint32_t EmcPmacroObDdllLongDqRank0_2;
    uint32_t EmcPmacroObDdllLongDqRank0_3;
    uint32_t EmcPmacroObDdllLongDqRank0_4;
    uint32_t EmcPmacroObDdllLongDqRank0_5;
    uint32_t EmcPmacroObDdllLongDqRank1_0;
    uint32_t EmcPmacroObDdllLongDqRank1_1;
    uint32_t EmcPmacroObDdllLongDqRank1_2;
    uint32_t EmcPmacroObDdllLongDqRank1_3;
    uint32_t EmcPmacroObDdllLongDqRank1_4;
    uint32_t EmcPmacroObDdllLongDqRank1_5;

    uint32_t EmcPmacroObDdllLongDqsRank0_0;
    uint32_t EmcPmacroObDdllLongDqsRank0_1;
    uint32_t EmcPmacroObDdllLongDqsRank0_2;
    uint32_t EmcPmacroObDdllLongDqsRank0_3;
    uint32_t EmcPmacroObDdllLongDqsRank0_4;
    uint32_t EmcPmacroObDdllLongDqsRank0_5;
    uint32_t EmcPmacroObDdllLongDqsRank1_0;
    uint32_t EmcPmacroObDdllLongDqsRank1_1;
    uint32_t EmcPmacroObDdllLongDqsRank1_2;
    uint32_t EmcPmacroObDdllLongDqsRank1_3;
    uint32_t EmcPmacroObDdllLongDqsRank1_4;
    uint32_t EmcPmacroObDdllLongDqsRank1_5;

    uint32_t EmcPmacroIbDdllLongDqsRank0_0;
    uint32_t EmcPmacroIbDdllLongDqsRank0_1;
    uint32_t EmcPmacroIbDdllLongDqsRank0_2;
    uint32_t EmcPmacroIbDdllLongDqsRank0_3;
    uint32_t EmcPmacroIbDdllLongDqsRank1_0;
    uint32_t EmcPmacroIbDdllLongDqsRank1_1;
    uint32_t EmcPmacroIbDdllLongDqsRank1_2;
    uint32_t EmcPmacroIbDdllLongDqsRank1_3;

    uint32_t EmcPmacroDdllLongCmd_0;
    uint32_t EmcPmacroDdllLongCmd_1;
    uint32_t EmcPmacroDdllLongCmd_2;
    uint32_t EmcPmacroDdllLongCmd_3;
    uint32_t EmcPmacroDdllLongCmd_4;
    uint32_t EmcPmacroDdllShortCmd_0;
    uint32_t EmcPmacroDdllShortCmd_1;
    uint32_t EmcPmacroDdllShortCmd_2;

    /*
     * Specifies the delay after asserting CKE pin during a WarmBoot0
     * sequence (in microseconds)
     */
    uint32_t WarmBootWait;

    /* Specifies the value for EMC_ODT_WRITE */
    uint32_t EmcOdtWrite;

    /* Periodic ZQ calibration */

    /*
     * Specifies the value for EMC_ZCAL_INTERVAL
     * Value 0 disables ZQ calibration
     */
    uint32_t EmcZcalInterval;
    /* Specifies the value for EMC_ZCAL_WAIT_CNT */
    uint32_t EmcZcalWaitCnt;
    /* Specifies the value for EMC_ZCAL_MRW_CMD */
    uint32_t EmcZcalMrwCmd;

    /* DRAM initialization sequence flow control */

    /* Specifies the MRS command value for resetting DLL */
    uint32_t EmcMrsResetDll;
    /* Specifies the command for ZQ initialization of device 0 */
    uint32_t EmcZcalInitDev0;
    /* Specifies the command for ZQ initialization of device 1 */
    uint32_t EmcZcalInitDev1;
    /*
     * Specifies the wait time after programming a ZQ initialization
     * command (in microseconds)
     */
    uint32_t EmcZcalInitWait;
    /*
     * Specifies the enable for ZQ calibration at cold boot [bit 0]
     * and warm boot [bit 1]
     */
    uint32_t EmcZcalWarmColdBootEnables;

    /*
     * Specifies the MRW command to LPDDR2 for ZQ calibration
     * on warmboot
     */
    /* Is issued to both devices separately */
    uint32_t EmcMrwLpddr2ZcalWarmBoot;
    /*
     * Specifies the ZQ command to DDR3 for ZQ calibration on warmboot
     * Is issued to both devices separately
     */
    uint32_t EmcZqCalDdr3WarmBoot;
    uint32_t EmcZqCalLpDdr4WarmBoot;
    /*
     * Specifies the wait time for ZQ calibration on warmboot
     * (in microseconds)
     */
    uint32_t EmcZcalWarmBootWait;
    /*
     * Specifies the enable for DRAM Mode Register programming
     * at warm boot
     */
    uint32_t EmcMrsWarmBootEnable;
    /*
     * Specifies the wait time after sending an MRS DLL reset command
     * in microseconds)
     */
    uint32_t EmcMrsResetDllWait;
    /* Specifies the extra MRS command to initialize mode registers */
    uint32_t EmcMrsExtra;
    /* Specifies the extra MRS command at warm boot */
    uint32_t EmcWarmBootMrsExtra;
    /* Specifies the EMRS command to enable the DDR2 DLL */
    uint32_t EmcEmrsDdr2DllEnable;
    /* Specifies the MRS command to reset the DDR2 DLL */
    uint32_t EmcMrsDdr2DllReset;
    /* Specifies the EMRS command to set OCD calibration */
    uint32_t EmcEmrsDdr2OcdCalib;
    /*
     * Specifies the wait between initializing DDR and setting OCD
     * calibration (in microseconds)
     */
    uint32_t EmcDdr2Wait;
    /* Specifies the value for EMC_CLKEN_OVERRIDE */
    uint32_t EmcClkenOverride;

    /*
     * Specifies LOG2 of the extra refresh numbers after booting
     * Program 0 to disable
     */
    uint32_t EmcExtraRefreshNum;
    /* Specifies the master override for all EMC clocks */
    uint32_t EmcClkenOverrideAllWarmBoot;
    /* Specifies the master override for all MC clocks */
    uint32_t McClkenOverrideAllWarmBoot;
    /* Specifies digital dll period, choosing between 4 to 64 ms */
    uint32_t EmcCfgDigDllPeriodWarmBoot;

    /* Pad controls */

    /* Specifies the value for PMC_VDDP_SEL */
    uint32_t PmcVddpSel;
    /* Specifies the wait time after programming PMC_VDDP_SEL */
    uint32_t PmcVddpSelWait;
    /* Specifies the value for PMC_DDR_PWR */
    uint32_t PmcDdrPwr;
    /* Specifies the value for PMC_DDR_CFG */
    uint32_t PmcDdrCfg;
    /* Specifies the value for PMC_IO_DPD3_REQ */
    uint32_t PmcIoDpd3Req;
    /* Specifies the wait time after programming PMC_IO_DPD3_REQ */
    uint32_t PmcIoDpd3ReqWait;
    uint32_t PmcIoDpd4ReqWait;

    /* Specifies the value for PMC_REG_SHORT */
    uint32_t PmcRegShort;
    /* Specifies the value for PMC_NO_IOPOWER */
    uint32_t PmcNoIoPower;

    uint32_t PmcDdrCntrlWait;
    uint32_t PmcDdrCntrl;

    /* Specifies the value for EMC_ACPD_CONTROL */
    uint32_t EmcAcpdControl;

    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE_CFG */
    ////uint32_t EmcSwizzleRank0ByteCfg;
    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE0 */
    uint32_t EmcSwizzleRank0Byte0;
    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE1 */
    uint32_t EmcSwizzleRank0Byte1;
    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE2 */
    uint32_t EmcSwizzleRank0Byte2;
    /* Specifies the value for EMC_SWIZZLE_RANK0_BYTE3 */
    uint32_t EmcSwizzleRank0Byte3;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE_CFG */
    ////uint32_t EmcSwizzleRank1ByteCfg;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE0 */
    uint32_t EmcSwizzleRank1Byte0;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE1 */
    uint32_t EmcSwizzleRank1Byte1;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE2 */
    uint32_t EmcSwizzleRank1Byte2;
    /* Specifies the value for EMC_SWIZZLE_RANK1_BYTE3 */
    uint32_t EmcSwizzleRank1Byte3;

    /* Specifies the value for EMC_TXDSRVTTGEN */
    uint32_t EmcTxdsrvttgen;

    /* Specifies the value for EMC_DATA_BRLSHFT_0 */
    uint32_t EmcDataBrlshft0;
    uint32_t EmcDataBrlshft1;

    uint32_t EmcDqsBrlshft0;
    uint32_t EmcDqsBrlshft1;

    uint32_t EmcCmdBrlshft0;
    uint32_t EmcCmdBrlshft1;
    uint32_t EmcCmdBrlshft2;
    uint32_t EmcCmdBrlshft3;

    uint32_t EmcQuseBrlshft0;
    uint32_t EmcQuseBrlshft1;
    uint32_t EmcQuseBrlshft2;
    uint32_t EmcQuseBrlshft3;

    uint32_t EmcDllCfg0;
    uint32_t EmcDllCfg1;

    uint32_t EmcPmcScratch1;
    uint32_t EmcPmcScratch2;
    uint32_t EmcPmcScratch3;

    uint32_t EmcPmacroPadCfgCtrl;

    uint32_t EmcPmacroVttgenCtrl0;
    uint32_t EmcPmacroVttgenCtrl1;
    uint32_t EmcPmacroVttgenCtrl2;

    uint32_t EmcPmacroBrickCtrlRfu1;
    uint32_t EmcPmacroCmdBrickCtrlFdpd;
    uint32_t EmcPmacroBrickCtrlRfu2;
    uint32_t EmcPmacroDataBrickCtrlFdpd;
    uint32_t EmcPmacroBgBiasCtrl0;
    uint32_t EmcPmacroDataPadRxCtrl;
    uint32_t EmcPmacroCmdPadRxCtrl;
    uint32_t EmcPmacroDataRxTermMode;
    uint32_t EmcPmacroCmdRxTermMode;
    uint32_t EmcPmacroDataPadTxCtrl;
    uint32_t EmcPmacroCommonPadTxCtrl;
    uint32_t EmcPmacroCmdPadTxCtrl;
    uint32_t EmcCfg3;

    uint32_t EmcPmacroTxPwrd0;
    uint32_t EmcPmacroTxPwrd1;
    uint32_t EmcPmacroTxPwrd2;
    uint32_t EmcPmacroTxPwrd3;
    uint32_t EmcPmacroTxPwrd4;
    uint32_t EmcPmacroTxPwrd5;

    uint32_t EmcConfigSampleDelay;

    uint32_t EmcPmacroBrickMapping0;
    uint32_t EmcPmacroBrickMapping1;
    uint32_t EmcPmacroBrickMapping2;

    uint32_t EmcPmacroTxSelClkSrc0;
    uint32_t EmcPmacroTxSelClkSrc1;
    uint32_t EmcPmacroTxSelClkSrc2;
    uint32_t EmcPmacroTxSelClkSrc3;
    uint32_t EmcPmacroTxSelClkSrc4;
    uint32_t EmcPmacroTxSelClkSrc5;

    uint32_t EmcPmacroDdllBypass;

    uint32_t EmcPmacroDdllPwrd0;
    uint32_t EmcPmacroDdllPwrd1;
    uint32_t EmcPmacroDdllPwrd2;

    uint32_t EmcPmacroCmdCtrl0;
    uint32_t EmcPmacroCmdCtrl1;
    uint32_t EmcPmacroCmdCtrl2;

    /* DRAM size information */

    /* Specifies the value for MC_EMEM_ADR_CFG */
    uint32_t McEmemAdrCfg;
    /* Specifies the value for MC_EMEM_ADR_CFG_DEV0 */
    uint32_t McEmemAdrCfgDev0;
    /* Specifies the value for MC_EMEM_ADR_CFG_DEV1 */
    uint32_t McEmemAdrCfgDev1;
    uint32_t McEmemAdrCfgChannelMask;

    /* Specifies the value for MC_EMEM_BANK_SWIZZLECfg0 */
    uint32_t McEmemAdrCfgBankMask0;
    /* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG1 */
    uint32_t McEmemAdrCfgBankMask1;
    /* Specifies the value for MC_EMEM_BANK_SWIZZLE_CFG2 */
    uint32_t McEmemAdrCfgBankMask2;

    /*
     * Specifies the value for MC_EMEM_CFG which holds the external memory
     * size (in KBytes)
     */
    uint32_t McEmemCfg;

    /* MC arbitration configuration */

    /* Specifies the value for MC_EMEM_ARB_CFG */
    uint32_t McEmemArbCfg;
    /* Specifies the value for MC_EMEM_ARB_OUTSTANDING_REQ */
    uint32_t McEmemArbOutstandingReq;

    uint32_t McEmemArbRefpbHpCtrl;
    uint32_t McEmemArbRefpbBankCtrl;

    /* Specifies the value for MC_EMEM_ARB_TIMING_RCD */
    uint32_t McEmemArbTimingRcd;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RP */
    uint32_t McEmemArbTimingRp;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RC */
    uint32_t McEmemArbTimingRc;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RAS */
    uint32_t McEmemArbTimingRas;
    /* Specifies the value for MC_EMEM_ARB_TIMING_FAW */
    uint32_t McEmemArbTimingFaw;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RRD */
    uint32_t McEmemArbTimingRrd;
    /* Specifies the value for MC_EMEM_ARB_TIMING_RAP2PRE */
    uint32_t McEmemArbTimingRap2Pre;
    /* Specifies the value for MC_EMEM_ARB_TIMING_WAP2PRE */
    uint32_t McEmemArbTimingWap2Pre;
    /* Specifies the value for MC_EMEM_ARB_TIMING_R2R */
    uint32_t McEmemArbTimingR2R;
    /* Specifies the value for MC_EMEM_ARB_TIMING_W2W */
    uint32_t McEmemArbTimingW2W;
    /* Specifies the value for MC_EMEM_ARB_TIMING_R2W */
    uint32_t McEmemArbTimingR2W;
    /* Specifies the value for MC_EMEM_ARB_TIMING_W2R */
    uint32_t McEmemArbTimingW2R;

    uint32_t McEmemArbTimingRFCPB;

    /* Specifies the value for MC_EMEM_ARB_DA_TURNS */
    uint32_t McEmemArbDaTurns;
    /* Specifies the value for MC_EMEM_ARB_DA_COVERS */
    uint32_t McEmemArbDaCovers;
    /* Specifies the value for MC_EMEM_ARB_MISC0 */
    uint32_t McEmemArbMisc0;
    /* Specifies the value for MC_EMEM_ARB_MISC1 */
    uint32_t McEmemArbMisc1;
    uint32_t McEmemArbMisc2;

    /* Specifies the value for MC_EMEM_ARB_RING1_THROTTLE */
    uint32_t McEmemArbRing1Throttle;
    /* Specifies the value for MC_EMEM_ARB_OVERRIDE */
    uint32_t McEmemArbOverride;
    /* Specifies the value for MC_EMEM_ARB_OVERRIDE_1 */
    uint32_t McEmemArbOverride1;
    /* Specifies the value for MC_EMEM_ARB_RSV */
    uint32_t McEmemArbRsv;

    uint32_t McDaCfg0;
    uint32_t McEmemArbTimingCcdmw;

    /* Specifies the value for MC_CLKEN_OVERRIDE */
    uint32_t McClkenOverride;

    /* Specifies the value for MC_STAT_CONTROL */
    uint32_t McStatControl;

    /* Specifies the value for MC_VIDEO_PROTECT_BOM */
    uint32_t McVideoProtectBom;
    /* Specifies the value for MC_VIDEO_PROTECT_BOM_ADR_HI */
    uint32_t McVideoProtectBomAdrHi;
    /* Specifies the value for MC_VIDEO_PROTECT_SIZE_MB */
    uint32_t McVideoProtectSizeMb;
    /* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE */
    uint32_t McVideoProtectVprOverride;
    /* Specifies the value for MC_VIDEO_PROTECT_VPR_OVERRIDE1 */
    uint32_t McVideoProtectVprOverride1;
    /* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_0 */
    uint32_t McVideoProtectGpuOverride0;
    /* Specifies the value for MC_VIDEO_PROTECT_GPU_OVERRIDE_1 */
    uint32_t McVideoProtectGpuOverride1;
    /* Specifies the value for MC_SEC_CARVEOUT_BOM */
    uint32_t McSecCarveoutBom;
    /* Specifies the value for MC_SEC_CARVEOUT_ADR_HI */
    uint32_t McSecCarveoutAdrHi;
    /* Specifies the value for MC_SEC_CARVEOUT_SIZE_MB */
    uint32_t McSecCarveoutSizeMb;
    /* Specifies the value for MC_VIDEO_PROTECT_REG_CTRL.
       VIDEO_PROTECT_WRITEAccess */
    uint32_t McVideoProtectWriteAccess;
    /* Specifies the value for MC_SEC_CARVEOUT_REG_CTRL.
       SEC_CARVEOUT_WRITEAccess */
    uint32_t McSecCarveoutProtectWriteAccess;

    /* Write-Protect Regions (WPR) */
    uint32_t McGeneralizedCarveout1Bom;
    uint32_t McGeneralizedCarveout1BomHi;
    uint32_t McGeneralizedCarveout1Size128kb;
    uint32_t McGeneralizedCarveout1Access0;
    uint32_t McGeneralizedCarveout1Access1;
    uint32_t McGeneralizedCarveout1Access2;
    uint32_t McGeneralizedCarveout1Access3;
    uint32_t McGeneralizedCarveout1Access4;
    uint32_t McGeneralizedCarveout1ForceInternalAccess0;
    uint32_t McGeneralizedCarveout1ForceInternalAccess1;
    uint32_t McGeneralizedCarveout1ForceInternalAccess2;
    uint32_t McGeneralizedCarveout1ForceInternalAccess3;
    uint32_t McGeneralizedCarveout1ForceInternalAccess4;
    uint32_t McGeneralizedCarveout1Cfg0;

    uint32_t McGeneralizedCarveout2Bom;
    uint32_t McGeneralizedCarveout2BomHi;
    uint32_t McGeneralizedCarveout2Size128kb;
    uint32_t McGeneralizedCarveout2Access0;
    uint32_t McGeneralizedCarveout2Access1;
    uint32_t McGeneralizedCarveout2Access2;
    uint32_t McGeneralizedCarveout2Access3;
    uint32_t McGeneralizedCarveout2Access4;
    uint32_t McGeneralizedCarveout2ForceInternalAccess0;
    uint32_t McGeneralizedCarveout2ForceInternalAccess1;
    uint32_t McGeneralizedCarveout2ForceInternalAccess2;
    uint32_t McGeneralizedCarveout2ForceInternalAccess3;
    uint32_t McGeneralizedCarveout2ForceInternalAccess4;
    uint32_t McGeneralizedCarveout2Cfg0;

    uint32_t McGeneralizedCarveout3Bom;
    uint32_t McGeneralizedCarveout3BomHi;
    uint32_t McGeneralizedCarveout3Size128kb;
    uint32_t McGeneralizedCarveout3Access0;
    uint32_t McGeneralizedCarveout3Access1;
    uint32_t McGeneralizedCarveout3Access2;
    uint32_t McGeneralizedCarveout3Access3;
    uint32_t McGeneralizedCarveout3Access4;
    uint32_t McGeneralizedCarveout3ForceInternalAccess0;
    uint32_t McGeneralizedCarveout3ForceInternalAccess1;
    uint32_t McGeneralizedCarveout3ForceInternalAccess2;
    uint32_t McGeneralizedCarveout3ForceInternalAccess3;
    uint32_t McGeneralizedCarveout3ForceInternalAccess4;
    uint32_t McGeneralizedCarveout3Cfg0;

    uint32_t McGeneralizedCarveout4Bom;
    uint32_t McGeneralizedCarveout4BomHi;
    uint32_t McGeneralizedCarveout4Size128kb;
    uint32_t McGeneralizedCarveout4Access0;
    uint32_t McGeneralizedCarveout4Access1;
    uint32_t McGeneralizedCarveout4Access2;
    uint32_t McGeneralizedCarveout4Access3;
    uint32_t McGeneralizedCarveout4Access4;
    uint32_t McGeneralizedCarveout4ForceInternalAccess0;
    uint32_t McGeneralizedCarveout4ForceInternalAccess1;
    uint32_t McGeneralizedCarveout4ForceInternalAccess2;
    uint32_t McGeneralizedCarveout4ForceInternalAccess3;
    uint32_t McGeneralizedCarveout4ForceInternalAccess4;
    uint32_t McGeneralizedCarveout4Cfg0;

    uint32_t McGeneralizedCarveout5Bom;
    uint32_t McGeneralizedCarveout5BomHi;
    uint32_t McGeneralizedCarveout5Size128kb;
    uint32_t McGeneralizedCarveout5Access0;
    uint32_t McGeneralizedCarveout5Access1;
    uint32_t McGeneralizedCarveout5Access2;
    uint32_t McGeneralizedCarveout5Access3;
    uint32_t McGeneralizedCarveout5Access4;
    uint32_t McGeneralizedCarveout5ForceInternalAccess0;
    uint32_t McGeneralizedCarveout5ForceInternalAccess1;
    uint32_t McGeneralizedCarveout5ForceInternalAccess2;
    uint32_t McGeneralizedCarveout5ForceInternalAccess3;
    uint32_t McGeneralizedCarveout5ForceInternalAccess4;
    uint32_t McGeneralizedCarveout5Cfg0;

    /* Specifies enable for CA training */
    uint32_t EmcCaTrainingEnable;

    /* Set if bit 6 select is greater than bit 7 select; uses aremc.
       spec packet SWIZZLE_BIT6_GT_BIT7 */
    uint32_t SwizzleRankByteEncode;
    /* Specifies enable and offset for patched boot ROM write */
    uint32_t BootRomPatchControl;
    /* Specifies data for patched boot ROM write */
    uint32_t BootRomPatchData;

    /* Specifies the value for MC_MTS_CARVEOUT_BOM */
    uint32_t McMtsCarveoutBom;
    /* Specifies the value for MC_MTS_CARVEOUT_ADR_HI */
    uint32_t McMtsCarveoutAdrHi;
    /* Specifies the value for MC_MTS_CARVEOUT_SIZE_MB */
    uint32_t McMtsCarveoutSizeMb;
    /* Specifies the value for MC_MTS_CARVEOUT_REG_CTRL */
    uint32_t McMtsCarveoutRegCtrl;

    /* End */
};

#endif /* __SOC_NVIDIA_TEGRA210_SDRAM_PARAM_H__ */
