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
#include <exosphere.hpp>
#include "fusee_sdram.hpp"
#include "../fusee_uncompress.hpp"

namespace ams::nxboot {

    namespace {

        template<fuse::SocType SocType>
        struct SdramParamsImpl;

        template<> struct SdramParamsImpl<fuse::SocType_Erista> { using Type = br::erista::BootSdramParams; };
        template<> struct SdramParamsImpl<fuse::SocType_Mariko> { using Type = br::mariko::BootSdramParams; };

        template<fuse::SocType SocType>
        using BootSdramParams = SdramParamsImpl<SocType>::Type;

        constexpr inline const uintptr_t CLKRST = secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
        constexpr inline const uintptr_t PMC    = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
        constexpr inline const uintptr_t APB    = secmon::MemoryRegionPhysicalDeviceApbMisc.GetAddress();
        constexpr inline const uintptr_t MC     = secmon::MemoryRegionPhysicalDeviceMemoryController.GetAddress();
        constexpr inline const uintptr_t EMC    = EMC_ADDRESS(0);
        constexpr inline const uintptr_t AHB    = AHB_ARBC(0);

        #include "fusee_sdram_params.inc"

        #include "fusee_sdram_params_lp0_erista.inc"
        #include "fusee_sdram_params_lp0_mariko.inc"

        void *GetSdramParams(fuse::SocType soc_type) {
            /* Get DRAM Id. */
            const auto dram_id = fuse::GetDramId();

            /* Extract to work buffer. */
            void *sdram_params_work_buffer;

            if (soc_type == fuse::SocType_Erista) {
                sdram_params_work_buffer =  reinterpret_cast<void *>(0x4003E000 - 2 * sizeof(BootSdramParams<fuse::SocType_Erista>));
                #define HANDLE_DRAM_CASE(_DRAM_ID_, _INDEX_)                                                                                                                            \
                    case _DRAM_ID_:                                                                                                                                                     \
                        Uncompress(sdram_params_work_buffer, 2 * sizeof(BootSdramParams<fuse::SocType_Erista>), SdramParamsErista##_INDEX_, SdramParamsSizeErista##_INDEX_);            \
                        if (_INDEX_ & 1) {                                                                                                                                              \
                            sdram_params_work_buffer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(sdram_params_work_buffer) + sizeof(BootSdramParams<fuse::SocType_Erista>)); \
                        }                                                                                                                                                               \
                        break;
                switch (dram_id) {
                    HANDLE_DRAM_CASE(0, 0)
                    HANDLE_DRAM_CASE(1, 1)
                    HANDLE_DRAM_CASE(2, 2)
                    HANDLE_DRAM_CASE(3, 3)
                    HANDLE_DRAM_CASE(4, 4)
                    HANDLE_DRAM_CASE(5, 5)
                    HANDLE_DRAM_CASE(6, 6)
                    default:
                        AMS_ABORT("Invalid DRAM id");
                }
                #undef HANDLE_DRAM_CASE

                return static_cast<BootSdramParams<fuse::SocType_Erista> *>(sdram_params_work_buffer);
            } else /* if (soc_type == fuse::SocType_Mariko) */ {
                sdram_params_work_buffer =  reinterpret_cast<void *>(0x4003E000 - 2 * sizeof(BootSdramParams<fuse::SocType_Mariko>));
                #define HANDLE_DRAM_CASE(_DRAM_ID_, _INDEX_)                                                                                                                            \
                    case _DRAM_ID_:                                                                                                                                                     \
                        Uncompress(sdram_params_work_buffer, 2 * sizeof(BootSdramParams<fuse::SocType_Mariko>), SdramParamsMariko##_INDEX_, SdramParamsSizeMariko##_INDEX_);            \
                        if (_INDEX_ & 1) {                                                                                                                                              \
                            sdram_params_work_buffer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(sdram_params_work_buffer) + sizeof(BootSdramParams<fuse::SocType_Mariko>)); \
                        }                                                                                                                                                               \
                        break;
                switch (dram_id) {
                    HANDLE_DRAM_CASE( 3, 12)
                    HANDLE_DRAM_CASE( 5, 12)
                    HANDLE_DRAM_CASE( 6, 12)
                    HANDLE_DRAM_CASE( 7,  0)
                    HANDLE_DRAM_CASE( 8,  1)
                    HANDLE_DRAM_CASE( 9,  2)
                    HANDLE_DRAM_CASE(10,  3)
                    HANDLE_DRAM_CASE(11,  4)
                    HANDLE_DRAM_CASE(12,  1)
                    HANDLE_DRAM_CASE(13,  2)
                    HANDLE_DRAM_CASE(14,  3)
                    HANDLE_DRAM_CASE(15,  4)
                    HANDLE_DRAM_CASE(16,  5)
                    HANDLE_DRAM_CASE(17,  6)
                    HANDLE_DRAM_CASE(18,  7)
                    HANDLE_DRAM_CASE(19,  6)
                    HANDLE_DRAM_CASE(20,  8)
                    HANDLE_DRAM_CASE(21,  9)
                    HANDLE_DRAM_CASE(22, 10)
                    HANDLE_DRAM_CASE(23,  7)
                    HANDLE_DRAM_CASE(24,  6)
                    HANDLE_DRAM_CASE(25, 11)
                    HANDLE_DRAM_CASE(26, 11)
                    HANDLE_DRAM_CASE(27, 11)
                    HANDLE_DRAM_CASE(28,  7)
                    default:
                        AMS_ABORT("Invalid DRAM id");
                }
                #undef HANDLE_DRAM_CASE

                return static_cast<BootSdramParams<fuse::SocType_Mariko> *>(sdram_params_work_buffer);
            }
        }

        template<fuse::SocType SocType>
        void SpareWrite(u32 reg, u32 value) {
            if (reg) {
                if constexpr (SocType == fuse::SocType_Erista) {
                    reg::Write(reinterpret_cast<volatile u32 *>(reg), value);
                } else if constexpr (SocType == fuse::SocType_Mariko) {
                    /* TODO: Validate the write. */
                    reg::Write(reinterpret_cast<volatile u32 *>(reg), value);
                }
            }
        }

        template<fuse::SocType SocType>
        void InitializeSdramImpl(BootSdramParams<SocType> *params) {
            /* Perform initial soc-specific setup. */
            if constexpr (SocType == fuse::SocType_Erista) {
                /* Enable sel_dpd on unused pins. */
                reg::Write(PMC + APBDEV_PMC_IO_DPD3_REQ, (((params->EmcPmcScratch1 & 0x3FFFFFFF) | 0x80000000) ^ 0xFFFF) & 0xC000FFFF);
                util::WaitMicroSeconds(params->PmcIoDpd3ReqWait);

                /* Disable e_dpd_vttgen. */
                u32 dpd4 = (params->EmcPmcScratch2 & 0x3FFFFFFF) | 0x80000000;
                reg::Write(PMC + APBDEV_PMC_IO_DPD4_REQ, (dpd4 ^ 0x3FFF0000) & 0xFFFF0000);
                util::WaitMicroSeconds(params->PmcIoDpd4ReqWait);

                /* Disable e_dpd_bg. */
                reg::Write(PMC + APBDEV_PMC_IO_DPD4_REQ, (dpd4 ^ 0x0000FFFF) & 0xC000FFFF);
                util::WaitMicroSeconds(params->PmcIoDpd4ReqWait);

                reg::Write(PMC + APBDEV_PMC_WEAK_BIAS, 0);
                util::WaitMicroSeconds(1);

                /* Enable memory clock. */
                {
                    /* Initialize pllm. */
                    {
                        reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_MISC1, params->PllMSetupControl);
                        reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_MISC2, 0);

                        reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, CLK_RST_REG_BITS_ENUM (PLLM_BASE_PLLM_ENABLE,                     DISABLE),
                                                                          CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVP,       params->PllMPostDivider),
                                                                          CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVN,   params->PllMFeedbackDivider),
                                                                          CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVM,      params->PllMInputDivider));


                        reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, CLK_RST_REG_BITS_ENUM (PLLM_BASE_PLLM_ENABLE,                      ENABLE),
                                                                          CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVP,       params->PllMPostDivider),
                                                                          CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVN,   params->PllMFeedbackDivider),
                                                                          CLK_RST_REG_BITS_VALUE(PLLM_BASE_PLLM_DIVM,      params->PllMInputDivider));

                        /* Wait 300us for stability. */
                        const auto stable_time = util::GetMicroSeconds() + 300;
                        while (true) {
                            if (reg::HasValue(CLKRST + CLK_RST_CONTROLLER_PLLM_BASE, CLK_RST_REG_BITS_ENUM(PLLM_BASE_PLLM_LOCK, LOCK))) {
                                util::WaitMicroSeconds(10);
                                break;
                            }

                            if (util::GetMicroSeconds() >= stable_time) {
                                break;
                            }
                        }
                    }

                    /* Set CLK_SOURCE_EMC, using McEmcmArbMisc0 as MC_EMC_SAME_FREQ. */
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, (params->EmcClockSource & ~0x10000) | ((params->McEmemArbMisc0 >> 11) & 0x10000));

                    if (params->EmcClockSourceDll) {
                        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL, params->EmcClockSourceDll);
                    }

                    if (params->ClearClk2Mc1) {
                        reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_W_CLR, CLK_RST_REG_BITS_ENUM(CLK_ENB_W_CLK_ENB_MC1, ENABLE));
                    }

                    /* Enable EMC/Mem. */
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_H_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLK_ENB_EMC, ENABLE),
                                                                          CLK_RST_REG_BITS_ENUM(CLK_ENB_H_CLK_ENB_MEM, ENABLE));

                    /* Enable EMC DLL. */
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_ENB_X_SET, CLK_RST_REG_BITS_ENUM(CLK_ENB_X_CLK_ENB_EMC_DLL, ENABLE));
                }

                /* Clear reset for MEM/EMC. */
                reg::Write(CLKRST + CLK_RST_CONTROLLER_RST_DEV_H_CLR, CLK_RST_REG_BITS_ENUM(RST_DEV_H_EMC_RST, ENABLE),
                                                                      CLK_RST_REG_BITS_ENUM(RST_DEV_H_MEM_RST, ENABLE));

                /* Set pad macros. */
                reg::Write(EMC + EMC_PMACRO_VTTGEN_CTRL_0, params->EmcPmacroVttgenCtrl0);
                reg::Write(EMC + EMC_PMACRO_VTTGEN_CTRL_1, params->EmcPmacroVttgenCtrl1);
                reg::Write(EMC + EMC_PMACRO_VTTGEN_CTRL_2, params->EmcPmacroVttgenCtrl2);

                reg::Write(EMC + EMC_TIMING_CONTROL, 1);
                util::WaitMicroSeconds(1);

                /* Select EMC write mux. */
                reg::Write(EMC + EMC_DBG, params->EmcDbg | reg::EncodeValue(EMC_REG_BITS_VALUE(DBG_WRITE_MUX, params->EmcDbgWriteMux)));

                /* Patch 2. */
                SpareWrite<SocType>(params->EmcBctSpare2, params->EmcBctSpare3);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                /* Patch 1 */
                SpareWrite<SocType>(params->EmcBctSpare0, params->EmcBctSpare1);

                if (params->ClkRstControllerPllmMisc2OverrideEnable) {
                    reg::Write(CLKRST + CLK_RST_CONTROLLER_PLLM_MISC2, params->ClkRstControllerPllmMisc2Override);
                }

                /* Enable sel_dpd on unused pins. */
                {
                    u32 val = (~params->EmcPmcScratch1 & 0x00000FFF) << 18;
                    val |= ((~params->EmcPmcScratch1 & 0x00001000) << 19) | ((~params->EmcPmcScratch1 & 0x00008000) << 15);
                    reg::Write(PMC + APBDEV_PMC_WEAK_BIAS, val);
                }

                reg::Write(PMC + APBDEV_PMC_IO_DPD3_REQ, 0x80000000 | (~params->EmcPmcScratch1 & 0x00009FFF));
                util::WaitMicroSeconds(params->PmcIoDpd3ReqWait);

                /* Disable e_dpd_vttgen. */
                reg::Write(PMC + APBDEV_PMC_IO_DPD4_REQ, 0x80000000 | (~params->EmcPmcScratch2 & 0x3FFF0000));
                util::WaitMicroSeconds(params->PmcIoDpd4ReqWait);

                /* Disable e_dpd_bg. */
                reg::Write(PMC + APBDEV_PMC_IO_DPD4_REQ, 0x80000000 | (~params->EmcPmcScratch2 & 0x00001FFF));
                util::WaitMicroSeconds(1);
            }

            /* Common phase 1. */

            /* Program CMD mapping. */
            reg::Write(EMC + EMC_FBIO_CFG7, params->EmcFbioCfg7);

            reg::Write(EMC + EMC_CMD_MAPPING_CMD0_0, params->EmcCmdMappingCmd0_0);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD0_1, params->EmcCmdMappingCmd0_1);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD0_2, params->EmcCmdMappingCmd0_2);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD1_0, params->EmcCmdMappingCmd1_0);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD1_1, params->EmcCmdMappingCmd1_1);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD1_2, params->EmcCmdMappingCmd1_2);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD2_0, params->EmcCmdMappingCmd2_0);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD2_1, params->EmcCmdMappingCmd2_1);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD2_2, params->EmcCmdMappingCmd2_2);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD3_0, params->EmcCmdMappingCmd3_0);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD3_1, params->EmcCmdMappingCmd3_1);
            reg::Write(EMC + EMC_CMD_MAPPING_CMD3_2, params->EmcCmdMappingCmd3_2);
            reg::Write(EMC + EMC_CMD_MAPPING_BYTE, params->EmcCmdMappingByte);

            /* Program brick mapping. */
            reg::Write(EMC + EMC_PMACRO_BRICK_MAPPING_0, params->EmcPmacroBrickMapping0);
            reg::Write(EMC + EMC_PMACRO_BRICK_MAPPING_1, params->EmcPmacroBrickMapping1);
            reg::Write(EMC + EMC_PMACRO_BRICK_MAPPING_2, params->EmcPmacroBrickMapping2);

            /* Specific phase 2. */
            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_PMACRO_BRICK_CTRL_RFU1, (params->EmcPmacroBrickCtrlRfu1 | ~0x01120112) & 0x1FFF1FFF);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                /* Set pad macros. */
                reg::Write(EMC + EMC_PMACRO_VTTGEN_CTRL_0, params->EmcPmacroVttgenCtrl0);
                reg::Write(EMC + EMC_PMACRO_VTTGEN_CTRL_1, params->EmcPmacroVttgenCtrl1);
                reg::Write(EMC + EMC_PMACRO_VTTGEN_CTRL_2, params->EmcPmacroVttgenCtrl2);

                /* Set pad macro bias. */
                reg::Write(EMC + EMC_PMACRO_BG_BIAS_CTRL_0, params->EmcPmacroBgBiasCtrl0);

                SpareWrite<SocType>(params->EmcBctSpareSecure0, params->EmcBctSpareSecure1);
                SpareWrite<SocType>(params->EmcBctSpareSecure2, params->EmcBctSpareSecure3);
                SpareWrite<SocType>(params->EmcBctSpareSecure4, params->EmcBctSpareSecure5);

                /* Trigger timing update. */
                reg::Write(EMC + EMC_TIMING_CONTROL, 1);
                util::WaitMicroSeconds(params->PmcVddpSelWait + 2);

                /* Set clock sources. */
                reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC, params->EmcClockSource);
                reg::Write(CLKRST + CLK_RST_CONTROLLER_CLK_SOURCE_EMC_DLL, params->EmcClockSourceDll);

                /* Select EMC write mux. */
                reg::Write(EMC + EMC_DBG, params->EmcDbg | reg::EncodeValue(EMC_REG_BITS_VALUE(DBG_WRITE_MUX, params->EmcDbgWriteMux)));

                /* Patch 2. */
                SpareWrite<SocType>(params->EmcBctSpare2, params->EmcBctSpare3);
            }

            /* Common phase 2. */
            reg::Write(EMC + EMC_CONFIG_SAMPLE_DELAY, params->EmcConfigSampleDelay);
            reg::Write(EMC + EMC_FBIO_CFG8, params->EmcFbioCfg8);

            /* Program swizzle registers. */
            reg::Write(EMC + EMC_SWIZZLE_RANK0_BYTE0, params->EmcSwizzleRank0Byte0);
            reg::Write(EMC + EMC_SWIZZLE_RANK0_BYTE1, params->EmcSwizzleRank0Byte1);
            reg::Write(EMC + EMC_SWIZZLE_RANK0_BYTE2, params->EmcSwizzleRank0Byte2);
            reg::Write(EMC + EMC_SWIZZLE_RANK0_BYTE3, params->EmcSwizzleRank0Byte3);
            reg::Write(EMC + EMC_SWIZZLE_RANK1_BYTE0, params->EmcSwizzleRank1Byte0);
            reg::Write(EMC + EMC_SWIZZLE_RANK1_BYTE1, params->EmcSwizzleRank1Byte1);
            reg::Write(EMC + EMC_SWIZZLE_RANK1_BYTE2, params->EmcSwizzleRank1Byte2);
            reg::Write(EMC + EMC_SWIZZLE_RANK1_BYTE3, params->EmcSwizzleRank1Byte3);

            /* Patch 3. */
            SpareWrite<SocType>(params->EmcBctSpare6, params->EmcBctSpare7);

            /* Program pad controls. */
            reg::Write(EMC + EMC_XM2COMPPADCTRL, params->EmcXm2CompPadCtrl);
            reg::Write(EMC + EMC_XM2COMPPADCTRL2, params->EmcXm2CompPadCtrl2);
            reg::Write(EMC + EMC_XM2COMPPADCTRL3, params->EmcXm2CompPadCtrl3);

            /* Program autocal controls with shadowed register fields. */
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG2, params->EmcAutoCalConfig2);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG3, params->EmcAutoCalConfig3);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG4, params->EmcAutoCalConfig4);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG5, params->EmcAutoCalConfig5);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG6, params->EmcAutoCalConfig6);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG7, params->EmcAutoCalConfig7);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG8, params->EmcAutoCalConfig8);

            reg::Write(EMC + EMC_PMACRO_RX_TERM, params->EmcPmacroRxTerm);
            reg::Write(EMC + EMC_PMACRO_DQ_TX_DRV, params->EmcPmacroDqTxDrv);
            reg::Write(EMC + EMC_PMACRO_CA_TX_DRV, params->EmcPmacroCaTxDrv);
            reg::Write(EMC + EMC_PMACRO_CMD_TX_DRV, params->EmcPmacroCmdTxDrv);
            reg::Write(EMC + EMC_PMACRO_AUTOCAL_CFG_COMMON, params->EmcPmacroAutocalCfgCommon);
            reg::Write(EMC + EMC_AUTO_CAL_CHANNEL, params->EmcAutoCalChannel);
            reg::Write(EMC + EMC_PMACRO_ZCTRL, params->EmcPmacroZctrl);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_DLL_CFG_0, params->EmcDllCfg0);
                reg::Write(EMC + EMC_DLL_CFG_1, params->EmcDllCfg1);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_PMACRO_DLL_CFG_0, params->EmcPmacroDllCfg0);
                reg::Write(EMC + EMC_PMACRO_DLL_CFG_1, params->EmcPmacroDllCfg1);
            }

            reg::Write(EMC + EMC_CFG_DIG_DLL_1, params->EmcCfgDigDll_1);

            reg::Write(EMC + EMC_DATA_BRLSHFT_0, params->EmcDataBrlshft0);
            reg::Write(EMC + EMC_DATA_BRLSHFT_1, params->EmcDataBrlshft1);
            reg::Write(EMC + EMC_DQS_BRLSHFT_0, params->EmcDqsBrlshft0);
            reg::Write(EMC + EMC_DQS_BRLSHFT_1, params->EmcDqsBrlshft1);
            reg::Write(EMC + EMC_CMD_BRLSHFT_0, params->EmcCmdBrlshft0);
            reg::Write(EMC + EMC_CMD_BRLSHFT_1, params->EmcCmdBrlshft1);
            reg::Write(EMC + EMC_CMD_BRLSHFT_2, params->EmcCmdBrlshft2);
            reg::Write(EMC + EMC_CMD_BRLSHFT_3, params->EmcCmdBrlshft3);
            reg::Write(EMC + EMC_QUSE_BRLSHFT_0, params->EmcQuseBrlshft0);
            reg::Write(EMC + EMC_QUSE_BRLSHFT_1, params->EmcQuseBrlshft1);
            reg::Write(EMC + EMC_QUSE_BRLSHFT_2, params->EmcQuseBrlshft2);
            reg::Write(EMC + EMC_QUSE_BRLSHFT_3, params->EmcQuseBrlshft3);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_PMACRO_BRICK_CTRL_RFU1, (params->EmcPmacroBrickCtrlRfu1 | ~0x01BF01BF) & 0x1FFF1FFF);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_PMACRO_BRICK_CTRL_RFU1, params->EmcPmacroBrickCtrlRfu1);
            }

            reg::Write(EMC + EMC_PMACRO_PAD_CFG_CTRL, params->EmcPmacroPadCfgCtrl);
            reg::Write(EMC + EMC_PMACRO_CMD_BRICK_CTRL_FDPD, params->EmcPmacroCmdBrickCtrlFdpd);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_PMACRO_BRICK_CTRL_RFU2, params->EmcPmacroBrickCtrlRfu2 & 0xFF7FFF7F);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_PMACRO_BRICK_CTRL_RFU2, params->EmcPmacroBrickCtrlRfu2);
            }

            reg::Write(EMC + EMC_PMACRO_DATA_BRICK_CTRL_FDPD, params->EmcPmacroDataBrickCtrlFdpd);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_PMACRO_BG_BIAS_CTRL_0, params->EmcPmacroBgBiasCtrl0);
            }

            reg::Write(EMC + EMC_PMACRO_DATA_PAD_RX_CTRL, params->EmcPmacroDataPadRxCtrl);
            reg::Write(EMC + EMC_PMACRO_CMD_PAD_RX_CTRL, params->EmcPmacroCmdPadRxCtrl);
            reg::Write(EMC + EMC_PMACRO_DATA_PAD_TX_CTRL, params->EmcPmacroDataPadTxCtrl);
            reg::Write(EMC + EMC_PMACRO_DATA_RX_TERM_MODE, params->EmcPmacroDataRxTermMode);
            reg::Write(EMC + EMC_PMACRO_CMD_RX_TERM_MODE, params->EmcPmacroCmdRxTermMode);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_PMACRO_CMD_PAD_TX_CTRL, params->EmcPmacroCmdPadTxCtrl);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_PMACRO_CMD_PAD_TX_CTRL, params->EmcPmacroCmdPadTxCtrl & 0xEFFFFFFF);
            }

            reg::Write(EMC + EMC_CFG_3, params->EmcCfg3);

            reg::Write(EMC + EMC_PMACRO_TX_PWRD_0, params->EmcPmacroTxPwrd0);
            reg::Write(EMC + EMC_PMACRO_TX_PWRD_1, params->EmcPmacroTxPwrd1);
            reg::Write(EMC + EMC_PMACRO_TX_PWRD_2, params->EmcPmacroTxPwrd2);
            reg::Write(EMC + EMC_PMACRO_TX_PWRD_3, params->EmcPmacroTxPwrd3);
            reg::Write(EMC + EMC_PMACRO_TX_PWRD_4, params->EmcPmacroTxPwrd4);
            reg::Write(EMC + EMC_PMACRO_TX_PWRD_5, params->EmcPmacroTxPwrd5);

            reg::Write(EMC + EMC_PMACRO_TX_SEL_CLK_SRC_0, params->EmcPmacroTxSelClkSrc0);
            reg::Write(EMC + EMC_PMACRO_TX_SEL_CLK_SRC_1, params->EmcPmacroTxSelClkSrc1);
            reg::Write(EMC + EMC_PMACRO_TX_SEL_CLK_SRC_2, params->EmcPmacroTxSelClkSrc2);
            reg::Write(EMC + EMC_PMACRO_TX_SEL_CLK_SRC_3, params->EmcPmacroTxSelClkSrc3);
            reg::Write(EMC + EMC_PMACRO_TX_SEL_CLK_SRC_4, params->EmcPmacroTxSelClkSrc4);
            reg::Write(EMC + EMC_PMACRO_TX_SEL_CLK_SRC_5, params->EmcPmacroTxSelClkSrc5);

            if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_PMACRO_PERBIT_FGCG_CTRL_0, params->EmcPmacroPerbitFgcgCtrl0);
                reg::Write(EMC + EMC_PMACRO_PERBIT_FGCG_CTRL_1, params->EmcPmacroPerbitFgcgCtrl1);
                reg::Write(EMC + EMC_PMACRO_PERBIT_FGCG_CTRL_2, params->EmcPmacroPerbitFgcgCtrl2);
                reg::Write(EMC + EMC_PMACRO_PERBIT_FGCG_CTRL_3, params->EmcPmacroPerbitFgcgCtrl3);
                reg::Write(EMC + EMC_PMACRO_PERBIT_FGCG_CTRL_4, params->EmcPmacroPerbitFgcgCtrl4);
                reg::Write(EMC + EMC_PMACRO_PERBIT_FGCG_CTRL_5, params->EmcPmacroPerbitFgcgCtrl5);

                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU_CTRL_0, params->EmcPmacroPerbitRfuCtrl0);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU_CTRL_1, params->EmcPmacroPerbitRfuCtrl1);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU_CTRL_2, params->EmcPmacroPerbitRfuCtrl2);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU_CTRL_3, params->EmcPmacroPerbitRfuCtrl3);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU_CTRL_4, params->EmcPmacroPerbitRfuCtrl4);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU_CTRL_5, params->EmcPmacroPerbitRfuCtrl5);

                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU1_CTRL_0, params->EmcPmacroPerbitRfu1Ctrl0);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU1_CTRL_1, params->EmcPmacroPerbitRfu1Ctrl1);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU1_CTRL_2, params->EmcPmacroPerbitRfu1Ctrl2);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU1_CTRL_3, params->EmcPmacroPerbitRfu1Ctrl3);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU1_CTRL_4, params->EmcPmacroPerbitRfu1Ctrl4);
                reg::Write(EMC + EMC_PMACRO_PERBIT_RFU1_CTRL_5, params->EmcPmacroPerbitRfu1Ctrl5);

                reg::Write(EMC + EMC_PMACRO_DATA_PI_CTRL, params->EmcPmacroDataPiCtrl);
                reg::Write(EMC + EMC_PMACRO_CMD_PI_CTRL, params->EmcPmacroCmdPiCtrl);
            }

            reg::Write(EMC + EMC_PMACRO_DDLL_BYPASS, params->EmcPmacroDdllBypass);
            reg::Write(EMC + EMC_PMACRO_DDLL_PWRD_0, params->EmcPmacroDdllPwrd0);
            reg::Write(EMC + EMC_PMACRO_DDLL_PWRD_1, params->EmcPmacroDdllPwrd1);
            reg::Write(EMC + EMC_PMACRO_DDLL_PWRD_2, params->EmcPmacroDdllPwrd2);
            reg::Write(EMC + EMC_PMACRO_CMD_CTRL_0, params->EmcPmacroCmdCtrl0);
            reg::Write(EMC + EMC_PMACRO_CMD_CTRL_1, params->EmcPmacroCmdCtrl1);
            reg::Write(EMC + EMC_PMACRO_CMD_CTRL_2, params->EmcPmacroCmdCtrl2);
            reg::Write(EMC + EMC_PMACRO_IB_VREF_DQ_0, params->EmcPmacroIbVrefDq_0);
            reg::Write(EMC + EMC_PMACRO_IB_VREF_DQ_1, params->EmcPmacroIbVrefDq_1);
            reg::Write(EMC + EMC_PMACRO_IB_VREF_DQS_0, params->EmcPmacroIbVrefDqs_0);
            reg::Write(EMC + EMC_PMACRO_IB_VREF_DQS_1, params->EmcPmacroIbVrefDqs_1);
            reg::Write(EMC + EMC_PMACRO_IB_RXRT, params->EmcPmacroIbRxrt);

            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK0_0, params->EmcPmacroQuseDdllRank0_0);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK0_1, params->EmcPmacroQuseDdllRank0_1);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK0_2, params->EmcPmacroQuseDdllRank0_2);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK0_3, params->EmcPmacroQuseDdllRank0_3);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK0_4, params->EmcPmacroQuseDdllRank0_4);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK0_5, params->EmcPmacroQuseDdllRank0_5);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK1_0, params->EmcPmacroQuseDdllRank1_0);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK1_1, params->EmcPmacroQuseDdllRank1_1);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK1_2, params->EmcPmacroQuseDdllRank1_2);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK1_3, params->EmcPmacroQuseDdllRank1_3);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK1_4, params->EmcPmacroQuseDdllRank1_4);
            reg::Write(EMC + EMC_PMACRO_QUSE_DDLL_RANK1_5, params->EmcPmacroQuseDdllRank1_5);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_PMACRO_BRICK_CTRL_RFU1, params->EmcPmacroBrickCtrlRfu1);
            }

            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0, params->EmcPmacroObDdllLongDqRank0_0);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1, params->EmcPmacroObDdllLongDqRank0_1);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2, params->EmcPmacroObDdllLongDqRank0_2);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3, params->EmcPmacroObDdllLongDqRank0_3);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4, params->EmcPmacroObDdllLongDqRank0_4);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5, params->EmcPmacroObDdllLongDqRank0_5);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0, params->EmcPmacroObDdllLongDqRank1_0);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1, params->EmcPmacroObDdllLongDqRank1_1);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2, params->EmcPmacroObDdllLongDqRank1_2);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3, params->EmcPmacroObDdllLongDqRank1_3);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_4, params->EmcPmacroObDdllLongDqRank1_4);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_5, params->EmcPmacroObDdllLongDqRank1_5);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_0, params->EmcPmacroObDdllLongDqsRank0_0);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_1, params->EmcPmacroObDdllLongDqsRank0_1);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_2, params->EmcPmacroObDdllLongDqsRank0_2);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_3, params->EmcPmacroObDdllLongDqsRank0_3);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_4, params->EmcPmacroObDdllLongDqsRank0_4);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_5, params->EmcPmacroObDdllLongDqsRank0_5);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_0, params->EmcPmacroObDdllLongDqsRank1_0);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_1, params->EmcPmacroObDdllLongDqsRank1_1);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_2, params->EmcPmacroObDdllLongDqsRank1_2);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_3, params->EmcPmacroObDdllLongDqsRank1_3);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_4, params->EmcPmacroObDdllLongDqsRank1_4);
            reg::Write(EMC + EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_5, params->EmcPmacroObDdllLongDqsRank1_5);
            reg::Write(EMC + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0, params->EmcPmacroIbDdllLongDqsRank0_0);
            reg::Write(EMC + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1, params->EmcPmacroIbDdllLongDqsRank0_1);
            reg::Write(EMC + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2, params->EmcPmacroIbDdllLongDqsRank0_2);
            reg::Write(EMC + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3, params->EmcPmacroIbDdllLongDqsRank0_3);
            reg::Write(EMC + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0, params->EmcPmacroIbDdllLongDqsRank1_0);
            reg::Write(EMC + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1, params->EmcPmacroIbDdllLongDqsRank1_1);
            reg::Write(EMC + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2, params->EmcPmacroIbDdllLongDqsRank1_2);
            reg::Write(EMC + EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3, params->EmcPmacroIbDdllLongDqsRank1_3);

            reg::Write(EMC + EMC_PMACRO_DDLL_LONG_CMD_0, params->EmcPmacroDdllLongCmd_0);
            reg::Write(EMC + EMC_PMACRO_DDLL_LONG_CMD_1, params->EmcPmacroDdllLongCmd_1);
            reg::Write(EMC + EMC_PMACRO_DDLL_LONG_CMD_2, params->EmcPmacroDdllLongCmd_2);
            reg::Write(EMC + EMC_PMACRO_DDLL_LONG_CMD_3, params->EmcPmacroDdllLongCmd_3);
            reg::Write(EMC + EMC_PMACRO_DDLL_LONG_CMD_4, params->EmcPmacroDdllLongCmd_4);
            reg::Write(EMC + EMC_PMACRO_DDLL_SHORT_CMD_0, params->EmcPmacroDdllShortCmd_0);
            reg::Write(EMC + EMC_PMACRO_DDLL_SHORT_CMD_1, params->EmcPmacroDdllShortCmd_1);
            reg::Write(EMC + EMC_PMACRO_DDLL_SHORT_CMD_2, params->EmcPmacroDdllShortCmd_2);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_PMACRO_COMMON_PAD_TX_CTRL, (params->EmcPmacroCommonPadTxCtrl | ~0x1) & 0xF);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_PMACRO_DDLL_PERIODIC_OFFSET, params->EmcPmacroDdllPeriodicOffset);
            }

            if constexpr (SocType == fuse::SocType_Erista) {
                SpareWrite<SocType>(params->EmcBctSpare4, params->EmcBctSpare5);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                SpareWrite<SocType>(params->EmcBctSpare4, params->EmcBctSpare5);
                SpareWrite<SocType>(params->EmcBctSpareSecure6, params->EmcBctSpareSecure7);
                SpareWrite<SocType>(params->EmcBctSpareSecure8, params->EmcBctSpareSecure9);
                SpareWrite<SocType>(params->EmcBctSpareSecure10, params->EmcBctSpareSecure11);
            }

            reg::Write(EMC + EMC_TIMING_CONTROL, 1);

            /* Initialize MC VPR settings. */
            reg::Write(MC + MC_VIDEO_PROTECT_BOM, params->McVideoProtectBom);
            reg::Write(MC + MC_VIDEO_PROTECT_BOM_ADR_HI, params->McVideoProtectBomAdrHi);
            reg::Write(MC + MC_VIDEO_PROTECT_SIZE_MB, params->McVideoProtectSizeMb);
            reg::Write(MC + MC_VIDEO_PROTECT_VPR_OVERRIDE, params->McVideoProtectVprOverride);
            reg::Write(MC + MC_VIDEO_PROTECT_VPR_OVERRIDE1, params->McVideoProtectVprOverride1);
            reg::Write(MC + MC_VIDEO_PROTECT_GPU_OVERRIDE_0, params->McVideoProtectGpuOverride0);
            reg::Write(MC + MC_VIDEO_PROTECT_GPU_OVERRIDE_1, params->McVideoProtectGpuOverride1);

            /* Program SDRAM geometry parameters. */
            reg::Write(MC + MC_EMEM_ADR_CFG, params->McEmemAdrCfg);
            reg::Write(MC + MC_EMEM_ADR_CFG_DEV0, params->McEmemAdrCfgDev0);
            reg::Write(MC + MC_EMEM_ADR_CFG_DEV1, params->McEmemAdrCfgDev1);
            reg::Write(MC + MC_EMEM_ADR_CFG_CHANNEL_MASK, params->McEmemAdrCfgChannelMask);

            /* Program bank swizzling. */
            reg::Write(MC + MC_EMEM_ADR_CFG_BANK_MASK_0, params->McEmemAdrCfgBankMask0);
            reg::Write(MC + MC_EMEM_ADR_CFG_BANK_MASK_1, params->McEmemAdrCfgBankMask1);
            reg::Write(MC + MC_EMEM_ADR_CFG_BANK_MASK_2, params->McEmemAdrCfgBankMask2);

            /* Program external memory aperture (base and size). */
            reg::Write(MC + MC_EMEM_CFG, params->McEmemCfg);

            /* Program SEC carveout (base and size). */
            reg::Write(MC + MC_SEC_CARVEOUT_BOM, params->McSecCarveoutBom);
            reg::Write(MC + MC_SEC_CARVEOUT_ADR_HI, params->McSecCarveoutAdrHi);
            reg::Write(MC + MC_SEC_CARVEOUT_SIZE_MB, params->McSecCarveoutSizeMb);

            /* Program MTS carveout (base and size). */
            reg::Write(MC + MC_MTS_CARVEOUT_BOM, params->McMtsCarveoutBom);
            reg::Write(MC + MC_MTS_CARVEOUT_ADR_HI, params->McMtsCarveoutAdrHi);
            reg::Write(MC + MC_MTS_CARVEOUT_SIZE_MB, params->McMtsCarveoutSizeMb);

            /* Program the memory arbiter. */
            reg::Write(MC + MC_EMEM_ARB_CFG, params->McEmemArbCfg);
            reg::Write(MC + MC_EMEM_ARB_OUTSTANDING_REQ, params->McEmemArbOutstandingReq);
            reg::Write(MC + MC_EMEM_ARB_REFPB_HP_CTRL, params->McEmemArbRefpbHpCtrl);
            reg::Write(MC + MC_EMEM_ARB_REFPB_BANK_CTRL, params->McEmemArbRefpbBankCtrl);
            reg::Write(MC + MC_EMEM_ARB_TIMING_RCD, params->McEmemArbTimingRcd);
            reg::Write(MC + MC_EMEM_ARB_TIMING_RP, params->McEmemArbTimingRp);
            reg::Write(MC + MC_EMEM_ARB_TIMING_RC, params->McEmemArbTimingRc);
            reg::Write(MC + MC_EMEM_ARB_TIMING_RAS, params->McEmemArbTimingRas);
            reg::Write(MC + MC_EMEM_ARB_TIMING_FAW, params->McEmemArbTimingFaw);
            reg::Write(MC + MC_EMEM_ARB_TIMING_RRD, params->McEmemArbTimingRrd);
            reg::Write(MC + MC_EMEM_ARB_TIMING_RAP2PRE, params->McEmemArbTimingRap2Pre);
            reg::Write(MC + MC_EMEM_ARB_TIMING_WAP2PRE, params->McEmemArbTimingWap2Pre);
            reg::Write(MC + MC_EMEM_ARB_TIMING_R2R, params->McEmemArbTimingR2R);
            reg::Write(MC + MC_EMEM_ARB_TIMING_W2W, params->McEmemArbTimingW2W);
            reg::Write(MC + MC_EMEM_ARB_TIMING_CCDMW, params->McEmemArbTimingCcdmw);
            reg::Write(MC + MC_EMEM_ARB_TIMING_R2W, params->McEmemArbTimingR2W);
            reg::Write(MC + MC_EMEM_ARB_TIMING_W2R, params->McEmemArbTimingW2R);
            reg::Write(MC + MC_EMEM_ARB_TIMING_RFCPB, params->McEmemArbTimingRFCPB);
            reg::Write(MC + MC_EMEM_ARB_DA_TURNS, params->McEmemArbDaTurns);
            reg::Write(MC + MC_EMEM_ARB_DA_COVERS, params->McEmemArbDaCovers);
            reg::Write(MC + MC_EMEM_ARB_MISC0, params->McEmemArbMisc0);
            reg::Write(MC + MC_EMEM_ARB_MISC1, params->McEmemArbMisc1);
            reg::Write(MC + MC_EMEM_ARB_MISC2, params->McEmemArbMisc2);
            reg::Write(MC + MC_EMEM_ARB_RING1_THROTTLE, params->McEmemArbRing1Throttle);
            reg::Write(MC + MC_EMEM_ARB_OVERRIDE, params->McEmemArbOverride);
            reg::Write(MC + MC_EMEM_ARB_OVERRIDE_1, params->McEmemArbOverride1);
            reg::Write(MC + MC_EMEM_ARB_RSV, params->McEmemArbRsv);
            reg::Write(MC + MC_DA_CONFIG0, params->McDaCfg0);

            /* Trigger MC timing update. */
            reg::Write(MC + MC_TIMING_CONTROL, 1);

            /* Program second-level clock enable overrides. */
            reg::Write(MC + MC_CLKEN_OVERRIDE, params->McClkenOverride);

            /* Program statistics gathering. */
            reg::Write(MC + MC_STAT_CONTROL, params->McStatControl);

            /* Program SDRAM geometry parameters. */
            reg::Write(EMC + EMC_ADR_CFG, params->EmcAdrCfg);

            /* Program second-level clock enable overrides. */
            reg::Write(EMC + EMC_CLKEN_OVERRIDE, params->EmcClkenOverride);

            /* Program EMC pad auto calibration. */
            reg::Write(EMC + EMC_PMACRO_AUTOCAL_CFG_0, params->EmcPmacroAutocalCfg0);
            reg::Write(EMC + EMC_PMACRO_AUTOCAL_CFG_1, params->EmcPmacroAutocalCfg1);
            reg::Write(EMC + EMC_PMACRO_AUTOCAL_CFG_2, params->EmcPmacroAutocalCfg2);

            reg::Write(EMC + EMC_AUTO_CAL_VREF_SEL_0, params->EmcAutoCalVrefSel0);
            reg::Write(EMC + EMC_AUTO_CAL_VREF_SEL_1, params->EmcAutoCalVrefSel1);

            reg::Write(EMC + EMC_AUTO_CAL_INTERVAL, params->EmcAutoCalInterval);
            reg::Write(EMC + EMC_AUTO_CAL_CONFIG, params->EmcAutoCalConfig);
            util::WaitMicroSeconds(params->EmcAutoCalWait);

            /* Patch 5. */
            if constexpr (SocType == fuse::SocType_Erista) {
                SpareWrite<SocType>(params->EmcBctSpare8, params->EmcBctSpare9);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                SpareWrite<SocType>(params->EmcBctSpare8, params->EmcBctSpare9);
                reg::Write(EMC + EMC_AUTO_CAL_CONFIG9, params->EmcAutoCalConfig9);
            }

            /* Program EMC timing configuration. */
            reg::Write(EMC + EMC_CFG_2, params->EmcCfg2);
            reg::Write(EMC + EMC_CFG_PIPE, params->EmcCfgPipe);
            reg::Write(EMC + EMC_CFG_PIPE_1, params->EmcCfgPipe1);
            reg::Write(EMC + EMC_CFG_PIPE_2, params->EmcCfgPipe2);
            reg::Write(EMC + EMC_CMDQ, params->EmcCmdQ);
            reg::Write(EMC + EMC_MC2EMCQ, params->EmcMc2EmcQ);
            reg::Write(EMC + EMC_MRS_WAIT_CNT, params->EmcMrsWaitCnt);
            reg::Write(EMC + EMC_MRS_WAIT_CNT2, params->EmcMrsWaitCnt2);
            reg::Write(EMC + EMC_FBIO_CFG5, params->EmcFbioCfg5);
            reg::Write(EMC + EMC_RC, params->EmcRc);
            reg::Write(EMC + EMC_RFC, params->EmcRfc);
            reg::Write(EMC + EMC_RFCPB, params->EmcRfcPb);
            reg::Write(EMC + EMC_REFCTRL2, params->EmcRefctrl2);
            reg::Write(EMC + EMC_RFC_SLR, params->EmcRfcSlr);
            reg::Write(EMC + EMC_RAS, params->EmcRas);
            reg::Write(EMC + EMC_RP, params->EmcRp);
            reg::Write(EMC + EMC_TPPD, params->EmcTppd);

            if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_TRTM, params->EmcTrtm);
                reg::Write(EMC + EMC_TWTM, params->EmcTwtm);
                reg::Write(EMC + EMC_TRATM, params->EmcTratm);
                reg::Write(EMC + EMC_TWATM, params->EmcTwatm);
                reg::Write(EMC + EMC_TR2REF, params->EmcTr2ref);
            }

            reg::Write(EMC + EMC_R2R, params->EmcR2r);
            reg::Write(EMC + EMC_W2W, params->EmcW2w);
            reg::Write(EMC + EMC_R2W, params->EmcR2w);
            reg::Write(EMC + EMC_W2R, params->EmcW2r);
            reg::Write(EMC + EMC_R2P, params->EmcR2p);
            reg::Write(EMC + EMC_W2P, params->EmcW2p);
            reg::Write(EMC + EMC_CCDMW, params->EmcCcdmw);
            reg::Write(EMC + EMC_RD_RCD, params->EmcRdRcd);
            reg::Write(EMC + EMC_WR_RCD, params->EmcWrRcd);
            reg::Write(EMC + EMC_RRD, params->EmcRrd);
            reg::Write(EMC + EMC_REXT, params->EmcRext);
            reg::Write(EMC + EMC_WEXT, params->EmcWext);
            reg::Write(EMC + EMC_WDV, params->EmcWdv);
            reg::Write(EMC + EMC_WDV_CHK, params->EmcWdvChk);
            reg::Write(EMC + EMC_WSV, params->EmcWsv);
            reg::Write(EMC + EMC_WEV, params->EmcWev);
            reg::Write(EMC + EMC_WDV_MASK, params->EmcWdvMask);
            reg::Write(EMC + EMC_WS_DURATION, params->EmcWsDuration);
            reg::Write(EMC + EMC_WE_DURATION, params->EmcWeDuration);
            reg::Write(EMC + EMC_QUSE, params->EmcQUse);
            reg::Write(EMC + EMC_QUSE_WIDTH, params->EmcQuseWidth);
            reg::Write(EMC + EMC_IBDLY, params->EmcIbdly);
            reg::Write(EMC + EMC_OBDLY, params->EmcObdly);
            reg::Write(EMC + EMC_EINPUT, params->EmcEInput);
            reg::Write(EMC + EMC_EINPUT_DURATION, params->EmcEInputDuration);
            reg::Write(EMC + EMC_PUTERM_EXTRA, params->EmcPutermExtra);
            reg::Write(EMC + EMC_PUTERM_WIDTH, params->EmcPutermWidth);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_PMACRO_COMMON_PAD_TX_CTRL, params->EmcPmacroCommonPadTxCtrl);
            }

            reg::Write(EMC + EMC_DBG, params->EmcDbg);
            reg::Write(EMC + EMC_QRST, params->EmcQRst);
            reg::Write(EMC + EMC_ISSUE_QRST, 1);
            reg::Write(EMC + EMC_ISSUE_QRST, 0);
            reg::Write(EMC + EMC_QSAFE, params->EmcQSafe);
            reg::Write(EMC + EMC_RDV, params->EmcRdv);
            reg::Write(EMC + EMC_RDV_MASK, params->EmcRdvMask);
            reg::Write(EMC + EMC_RDV_EARLY, params->EmcRdvEarly);
            reg::Write(EMC + EMC_RDV_EARLY_MASK, params->EmcRdvEarlyMask);
            reg::Write(EMC + EMC_QPOP, params->EmcQpop);
            reg::Write(EMC + EMC_REFRESH, params->EmcRefresh);
            reg::Write(EMC + EMC_BURST_REFRESH_NUM, params->EmcBurstRefreshNum);
            reg::Write(EMC + EMC_PRE_REFRESH_REQ_CNT, params->EmcPreRefreshReqCnt);
            reg::Write(EMC + EMC_PDEX2WR, params->EmcPdEx2Wr);
            reg::Write(EMC + EMC_PDEX2RD, params->EmcPdEx2Rd);
            reg::Write(EMC + EMC_PCHG2PDEN, params->EmcPChg2Pden);
            reg::Write(EMC + EMC_ACT2PDEN, params->EmcAct2Pden);
            reg::Write(EMC + EMC_AR2PDEN, params->EmcAr2Pden);
            reg::Write(EMC + EMC_RW2PDEN, params->EmcRw2Pden);
            reg::Write(EMC + EMC_CKE2PDEN, params->EmcCke2Pden);
            reg::Write(EMC + EMC_PDEX2CKE, params->EmcPdex2Cke);
            reg::Write(EMC + EMC_PDEX2MRR, params->EmcPdex2Mrr);
            reg::Write(EMC + EMC_TXSR, params->EmcTxsr);
            reg::Write(EMC + EMC_TXSRDLL, params->EmcTxsrDll);
            reg::Write(EMC + EMC_TCKE, params->EmcTcke);
            reg::Write(EMC + EMC_TCKESR, params->EmcTckesr);
            reg::Write(EMC + EMC_TPD, params->EmcTpd);
            reg::Write(EMC + EMC_TFAW, params->EmcTfaw);
            reg::Write(EMC + EMC_TRPAB, params->EmcTrpab);
            reg::Write(EMC + EMC_TCLKSTABLE, params->EmcTClkStable);
            reg::Write(EMC + EMC_TCLKSTOP, params->EmcTClkStop);
            reg::Write(EMC + EMC_TREFBW, params->EmcTRefBw);
            reg::Write(EMC + EMC_ODT_WRITE, params->EmcOdtWrite);
            reg::Write(EMC + EMC_CFG_DIG_DLL, params->EmcCfgDigDll);
            reg::Write(EMC + EMC_CFG_DIG_DLL_PERIOD, params->EmcCfgDigDllPeriod);

            /* Lock bit written later for CFG_ADR_EN. */
            reg::Write(EMC + EMC_FBIO_SPARE, params->EmcFbioSpare & 0xFFFFFFFD);
            reg::Write(EMC + EMC_CFG_RSV, params->EmcCfgRsv);
            reg::Write(EMC + EMC_PMC_SCRATCH1, params->EmcPmcScratch1);
            reg::Write(EMC + EMC_PMC_SCRATCH2, params->EmcPmcScratch2);
            reg::Write(EMC + EMC_PMC_SCRATCH3, params->EmcPmcScratch3);
            reg::Write(EMC + EMC_ACPD_CONTROL, params->EmcAcpdControl);
            reg::Write(EMC + EMC_TXDSRVTTGEN, params->EmcTxdsrvttgen);

            if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_PMACRO_DSR_VTTGEN_CTRL_0, params->EmcPmacroDsrVttgenCtrl0);
            }

            /* Set pipe bypass enable bits before sending any DRAM commands. */
            reg::Write(EMC + EMC_CFG, (params->EmcCfg * 0xE) | 0x03C00000);

            /* Perform bootrom patch. */
            if constexpr (SocType == fuse::SocType_Erista) {
                if (params->BootRomPatchControl & 0x80000000) {
                    reg::Write(APB + ((params->BootRomPatchControl & 0x3FFFFFFF) << 2), params->BootRomPatchData);
                    reg::Write(MC + MC_TIMING_CONTROL, 1);
                }
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                if (params->BootRomPatchControl) {
                    SpareWrite<SocType>(params->BootRomPatchControl, params->BootRomPatchData);
                    reg::Write(MC + MC_TIMING_CONTROL, 1);
                }

                SpareWrite<SocType>(params->EmcBctSpareSecure12, params->EmcBctSpareSecure13);
                SpareWrite<SocType>(params->EmcBctSpareSecure14, params->EmcBctSpareSecure15);
                SpareWrite<SocType>(params->EmcBctSpareSecure16, params->EmcBctSpareSecure17);
            }

            /* Release SEL_DPD_CMD. */
            reg::Write(PMC + APBDEV_PMC_IO_DPD3_REQ, ((params->EmcPmcScratch1 & 0x3FFFFFFF) | 0x40000000) & 0xCFFF0000);
            util::WaitMicroSeconds(params->PmcIoDpd3ReqWait);

            if constexpr (SocType == fuse::SocType_Erista) {
                if (params->EmcAutoCalInterval == 0) {
                    reg::Write(EMC + EMC_AUTO_CAL_CONFIG, params->EmcAutoCalConfig | 0x200);
                }

                reg::Write(EMC + EMC_PMACRO_BRICK_CTRL_RFU2, params->EmcPmacroBrickCtrlRfu2);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_PMACRO_CMD_PAD_TX_CTRL, params->EmcPmacroCmdPadTxCtrl);
            }

            /* ZQ CAL setup */
            if (params->EmcZcalWarmColdBootEnables & 1) {
                if (params->MemoryType == br::BootMemoryType_Ddr3) {
                    reg::Write(EMC + EMC_ZCAL_WAIT_CNT, params->EmcZcalWaitCnt << 3);
                } else if (params->MemoryType == br::BootMemoryType_LpDdr4) {
                    reg::Write(EMC + EMC_ZCAL_WAIT_CNT, params->EmcZcalWaitCnt);
                    reg::Write(EMC + EMC_ZCAL_MRW_CMD, params->EmcZcalMrwCmd);
                }
            }

            /* Trigger timing update. */
            reg::Write(EMC + EMC_TIMING_CONTROL, 1);
            util::WaitMicroSeconds(params->EmcTimingControlWait);

            /* Deassert HOLD_CKE_LOW. */
            if constexpr (SocType == fuse::SocType_Erista) {
                reg::ClearBits(PMC + APBDEV_PMC_DDR_CNTRL, ~0xFFF8007F);
            } else if constexpr (SocType == fuse::SocType_Mariko) {
                reg::ClearBits(PMC + APBDEV_PMC_DDR_CNTRL, ~0xFF78007F);
            }

            util::WaitMicroSeconds(params->PmcDdrCntrlWait);

            /* Set clock enable signal. */
            const u32 pin_gpio_cfg = (params->EmcPinGpioEn << 16) | (params->EmcPinGpio << 12);
            if (params->MemoryType == br::BootMemoryType_Ddr3 || params->MemoryType == br::BootMemoryType_LpDdr4) {
                reg::Write(EMC + EMC_PIN, pin_gpio_cfg);
                reg::Read(EMC + EMC_PIN);
                util::WaitMicroSeconds(200 + params->EmcPinExtraWait);

                reg::Write(EMC + EMC_PIN, pin_gpio_cfg | 0x100);
                reg::Read(EMC + EMC_PIN);

                const u32 wait = params->MemoryType == br::BootMemoryType_Ddr3 ? 500 : 2000;
                util::WaitMicroSeconds(wait + params->EmcPinExtraWait);
            }

            /* Set clock enable signal. */
            reg::Write(EMC + EMC_PIN, pin_gpio_cfg | 0x101);
            reg::Read(EMC + EMC_PIN);
            util::WaitMicroSeconds(params->EmcPinProgramWait);

            /* Send NOP */
            if (params->MemoryType != br::BootMemoryType_LpDdr4) {
                reg::Write(EMC + EMC_NOP, (params->EmcDevSelect << 30) | 1);
            }

            /* On coldboot with LPDDR2/3, wait 200us after asserting CKE high. */
            if (params->MemoryType != br::BootMemoryType_LpDdr2) {
                util::WaitMicroSeconds(200 + params->EmcPinExtraWait);
            }

            /* Init ZQ calibration. */
            if (params->MemoryType == br::BootMemoryType_LpDdr4) {
                SpareWrite<SocType>(params->EmcBctSpare10, params->EmcBctSpare11);

                reg::Write(EMC + EMC_MRW2, params->EmcMrw2);
                reg::Write(EMC + EMC_MRW, params->EmcMrw1);
                reg::Write(EMC + EMC_MRW3, params->EmcMrw3);
                reg::Write(EMC + EMC_MRW4, params->EmcMrw4);
                reg::Write(EMC + EMC_MRW6, params->EmcMrw6);
                reg::Write(EMC + EMC_MRW14, params->EmcMrw14);

                reg::Write(EMC + EMC_MRW8, params->EmcMrw8);
                reg::Write(EMC + EMC_MRW12, params->EmcMrw12);
                reg::Write(EMC + EMC_MRW9, params->EmcMrw9);
                reg::Write(EMC + EMC_MRW13, params->EmcMrw13);

                if (params->EmcZcalWarmColdBootEnables & 1) {
                    /* Issue ZQCAL start, device 0. */
                    reg::Write(EMC + EMC_ZQ_CAL, params->EmcZcalInitDev0);
                    util::WaitMicroSeconds(params->EmcZcalInitWait);

                    /* Issue ZQCAL latch. */
                    reg::Write(EMC + EMC_ZQ_CAL, params->EmcZcalInitDev0 ^ 0x3);

                    /* Do the same for device 1. */
                    if ((params->EmcDevSelect & 2) == 0) {
                        reg::Write(EMC + EMC_ZQ_CAL, params->EmcZcalInitDev1);
                        util::WaitMicroSeconds(params->EmcZcalInitWait);

                        reg::Write(EMC + EMC_ZQ_CAL, params->EmcZcalInitDev1 ^ 0x3);
                    }
                }
            }

            /* Patches 10-12. */
            if constexpr (SocType == fuse::SocType_Mariko) {
                SpareWrite<SocType>(params->EmcBctSpareSecure18, params->EmcBctSpareSecure19);
                SpareWrite<SocType>(params->EmcBctSpareSecure20, params->EmcBctSpareSecure21);
                SpareWrite<SocType>(params->EmcBctSpareSecure22, params->EmcBctSpareSecure23);
            }

            /* Set package and DPD pad control. */
            reg::Write(PMC + APBDEV_PMC_DDR_CFG, params->PmcDdrCfg);

            /* Start periodic ZQ calibration. */
            if (params->MemoryType == br::BootMemoryType_LpDdr2 || params->MemoryType == br::BootMemoryType_Ddr3 || params->MemoryType == br::BootMemoryType_LpDdr4) {
                reg::Write(EMC + EMC_ZCAL_INTERVAL, params->EmcZcalInterval);
                reg::Write(EMC + EMC_ZCAL_WAIT_CNT, params->EmcZcalWaitCnt);
                reg::Write(EMC + EMC_ZCAL_MRW_CMD, params->EmcZcalMrwCmd);
            }

            SpareWrite<SocType>(params->EmcBctSpare12, params->EmcBctSpare13);

            /* Trigger timing update. */
            reg::Write(EMC + EMC_TIMING_CONTROL, 1);

            if (params->EmcExtraRefreshNum) {
                reg::Write(EMC + EMC_REF, (params->EmcDevSelect << 30) | (((1 << params->EmcExtraRefreshNum) - 1) << 8) | 3);
            }

            /* Enable refresh. */
            reg::Write(EMC + EMC_REFCTRL, params->EmcDevSelect | 0x80000000);

            reg::Write(EMC + EMC_DYN_SELF_REF_CONTROL, params->EmcDynSelfRefControl);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(EMC + EMC_CFG_UPDATE, params->EmcCfgUpdate);
            }

            reg::Write(EMC + EMC_CFG, params->EmcCfg);
            reg::Write(EMC + EMC_FDPD_CTRL_DQ, params->EmcFdpdCtrlDq);
            reg::Write(EMC + EMC_FDPD_CTRL_CMD, params->EmcFdpdCtrlCmd);
            reg::Write(EMC + EMC_SEL_DPD_CTRL, params->EmcSelDpdCtrl);

            /* Write addr swizzle lock bit. */
            reg::Write(EMC + EMC_FBIO_SPARE, params->EmcFbioSpare | 2);

            /* Trigger timing update. */
            reg::Write(EMC + EMC_TIMING_CONTROL, 1);

            if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(EMC + EMC_CFG_UPDATE, params->EmcCfgUpdate);
            }

            /* Enable EMC pipe clock gating. */
            reg::Write(EMC + EMC_CFG_PIPE_CLK, params->EmcCfgPipeClk);

            /* Depending on freqency, enable CMD/CLK fdpd. */
            reg::Write(EMC + EMC_FDPD_CTRL_CMD_NO_RAMP, params->EmcFdpdCtrlCmdNoRamp);

            if constexpr (SocType == fuse::SocType_Erista) {
                reg::ReadWrite(AHB + AHB_ARBITRATION_XBAR_CTRL, AHB_REG_BITS_VALUE(ARBITRATION_XBAR_CTRL_MEM_INIT_DONE, params->AhbArbitrationXbarCtrlMemInitDone));
            }

            if constexpr (SocType == fuse::SocType_Mariko) {
                reg::Write(MC + MC_UNTRANSLATED_REGION_CHECK, params->McUntranslatedRegionCheck);
            }

            /* Lock carveouts. */
            reg::Write(MC + MC_VIDEO_PROTECT_REG_CTRL, params->McVideoProtectWriteAccess);
            reg::Write(MC + MC_SEC_CARVEOUT_REG_CTRL, params->McSecCarveoutProtectWriteAccess);
            reg::Write(MC + MC_MTS_CARVEOUT_REG_CTRL, params->McMtsCarveoutRegCtrl);

            reg::Write(MC + MC_EMEM_CFG_ACCESS_CTRL, 1);

            if constexpr (SocType == fuse::SocType_Mariko) {
                reg::ReadWrite(AHB + AHB_ARBITRATION_XBAR_CTRL, AHB_REG_BITS_VALUE(ARBITRATION_XBAR_CTRL_MEM_INIT_DONE, params->AhbArbitrationXbarCtrlMemInitDone));
            }
        }

        consteval u32 GetBitMask32(u32 range) {
            if (range == BITSIZEOF(u32)) {
                return 0xFFFFFFFF;
            } else {
                return (1u << range) - 1;
            }
        }

        template<fuse::SocType SocType>
        void SaveSdramParamsToScratch(BootSdramParams<SocType> *params) {
            /* Clear the carveout parameters. */
            params->McGeneralizedCarveout1Cfg0 = 0;
            params->McGeneralizedCarveout2Cfg0 = 0;
            params->McGeneralizedCarveout3Cfg0 = 0;
            params->McGeneralizedCarveout4Cfg0 = 0;
            params->McGeneralizedCarveout5Cfg0 = 0;

            /* Patch spare write. */
            {
                /* TODO: Clean this up? */
                u32 t0 = params->EmcSwizzleRank0Byte0 << 5 >> 29 > params->EmcSwizzleRank0Byte0 << 1 >> 29;
                u32 t1 = (t0 & 0xFFFFFFEF) | ((params->EmcSwizzleRank1Byte0 << 5 >> 29 > params->EmcSwizzleRank1Byte0 << 1 >> 29) << 4);
                u32 t2 = (t1 & 0xFFFFFFFD) | ((params->EmcSwizzleRank0Byte1 << 5 >> 29 > params->EmcSwizzleRank0Byte1 << 1 >> 29) << 1);
                u32 t3 = (t2 & 0xFFFFFFDF) | ((params->EmcSwizzleRank1Byte1 << 5 >> 29 > params->EmcSwizzleRank1Byte1 << 1 >> 29) << 5);
                u32 t4 = (t3 & 0xFFFFFFFB) | ((params->EmcSwizzleRank0Byte2 << 5 >> 29 > params->EmcSwizzleRank0Byte2 << 1 >> 29) << 2);
                u32 t5 = (t4 & 0xFFFFFFBF) | ((params->EmcSwizzleRank1Byte2 << 5 >> 29 > params->EmcSwizzleRank1Byte2 << 1 >> 29) << 6);
                u32 t6 = (t5 & 0xFFFFFFF7) | ((params->EmcSwizzleRank0Byte3 << 5 >> 29 > params->EmcSwizzleRank0Byte3 << 1 >> 29) << 3);
                u32 t7 = (t6 & 0xFFFFFF7F) | ((params->EmcSwizzleRank1Byte3 << 5 >> 29 > params->EmcSwizzleRank1Byte3 << 1 >> 29) << 7);

                params->SwizzleRankByteEncode = t7;

                params->EmcBctSpare2 = 0x40000DD8;
                params->EmcBctSpare3 = params->SwizzleRankByteEncode;
            }

            /* Save parameters to scratch. */
            {
                u32 cur_reg_offset = APBDEV_PMC_SCRATCH6;
                u32 cur_reg_value  = reg::Read(PMC + cur_reg_offset);

                #define RANGE_HIGH(RANGE) (1 ? RANGE)
                #define RANGE_LOW(RANGE)  (0 ? RANGE)

                static_assert(RANGE_HIGH(31:0) - RANGE_LOW(31:0) + 1 == BITSIZEOF(u32));

                #define PROCESS_IMPL(PARAM, SCRATCH, SRC_RANGE, DST_RANGE, DO_READ)            \
                {                                                                              \
                    constexpr u32 RegisterOffset = APBDEV_PMC_##SCRATCH;                       \
                                                                                               \
                    if (RegisterOffset != cur_reg_offset) {                                    \
                        reg::Write(PMC + cur_reg_offset, cur_reg_value);                       \
                        cur_reg_offset = RegisterOffset;                                       \
                        if constexpr (DO_READ) {                                               \
                            cur_reg_value = reg::Read(PMC + RegisterOffset);                   \
                        } else {                                                               \
                            cur_reg_value = 0;                                                 \
                        }                                                                      \
                    }                                                                          \
                                                                                               \
                    constexpr u32 SrcRange = RANGE_HIGH(SRC_RANGE) - RANGE_LOW(SRC_RANGE) + 1; \
                    constexpr u32 DstRange = RANGE_HIGH(DST_RANGE) - RANGE_LOW(DST_RANGE) + 1; \
                    static_assert(SrcRange == DstRange);                                       \
                    static_assert(SrcRange <= BITSIZEOF(u32));                                 \
                                                                                               \
                    const u32 src_value = params->PARAM;                                       \
                    if constexpr (SrcRange == BITSIZEOF(u32)) {                                \
                        cur_reg_value = src_value;                                             \
                    } else if constexpr (SrcRange < BITSIZEOF(u32)) {                          \
                        constexpr u32 Mask = GetBitMask32(SrcRange) << RANGE_LOW(DST_RANGE);   \
                                                                                               \
                        constexpr u32 SrcLow = RANGE_LOW(SRC_RANGE);                           \
                        constexpr u32 DstLow = RANGE_LOW(DST_RANGE);                           \
                                                                                               \
                        cur_reg_value &= ~Mask;                                                \
                        if constexpr (SrcLow == DstLow) {                                      \
                            cur_reg_value |= (src_value & Mask);                               \
                        } else if constexpr (SrcLow < DstLow) {                                \
                            cur_reg_value |= ((src_value << (DstLow - SrcLow)) & Mask);        \
                        } else {                                                               \
                            cur_reg_value |= ((src_value >> (SrcLow - DstLow)) & Mask);        \
                        }                                                                      \
                    }                                                                          \
                }

                #define PROCESS_SCRATCH(PARAM, S, SRC_RANGE, DST_RANGE) PROCESS_IMPL(PARAM, SCRATCH##S, SRC_RANGE, DST_RANGE, true)
                #define PROCESS_SECURE_SCRATCH(PARAM, S, SRC_RANGE, DST_RANGE) PROCESS_IMPL(PARAM, SECURE_SCRATCH##S, SRC_RANGE, DST_RANGE, true)

                #define PROCESS_COMMON_SCRATCH(PARAM, S, SRC_RANGE, DST_RANGE) PROCESS_IMPL(PARAM, SCRATCH##S, SRC_RANGE, DST_RANGE, false)

                if constexpr (SocType == fuse::SocType_Erista) {
                    FOREACH_SDRAM_SCRATCH_REGISTER_ERISTA(PROCESS_SCRATCH);
                    FOREACH_SDRAM_SECURE_SCRATCH_REGISTER_ERISTA(PROCESS_SECURE_SCRATCH);
                } else /* if constexpr (SocType == fuse::SocType_Mariko) */ {
                    FOREACH_SDRAM_SCRATCH_REGISTER_MARIKO(PROCESS_SCRATCH);
                    FOREACH_SDRAM_SECURE_SCRATCH_REGISTER_MARIKO(PROCESS_SECURE_SCRATCH);
                }

                /* Manually process final fields. */
                PROCESS_COMMON_SCRATCH(PllMInputDivider, 2, 7:0, 7:0);
                PROCESS_COMMON_SCRATCH(PllMFeedbackDivider, 2, 7:0, 15:8);
                PROCESS_COMMON_SCRATCH(PllMPostDivider, 2, 4:0, 20:16);
                PROCESS_COMMON_SCRATCH(PllMKVCO, 2, 0:0, 17:17);
                PROCESS_COMMON_SCRATCH(PllMKCP, 2, 1:0, 19:18);

                PROCESS_COMMON_SCRATCH(PllMSetupControl, 35, 15:0, 15:0);

                PROCESS_COMMON_SCRATCH(PllMInputDivider, 3, 7:0, 7:0);
                cur_reg_value |= 0x3E << 8;
                PROCESS_COMMON_SCRATCH(PllMKVCO, 3, 0:0, 21:21);
                PROCESS_COMMON_SCRATCH(PllMKCP, 3, 1:0, 23:22);

                PROCESS_COMMON_SCRATCH(PllMSetupControl, 36, 23:0, 23:0);

                PROCESS_COMMON_SCRATCH(PllMStableTime, 4, 9:0, 9:0);
                PROCESS_COMMON_SCRATCH(PllMStableTime, 4, 21:0, 31:10);

                /* Write the final field value. */
                reg::Write(PMC + cur_reg_offset, cur_reg_value);

                #undef PROCESS_COMMON_SCRATCH
                #undef PROCESS_SECURE_SCRATCH
                #undef PROCESS_SCRATCH
                #undef PROCESS_IMPL
                #undef RANGE_LOW
                #undef RANGE_HIGH
            }
        }

        template<fuse::SocType SocType>
        void InitializeSdram(void *generic_params) {
            /* Get converted parameters. */
            auto *sdram_params = static_cast<BootSdramParams<SocType> *>(generic_params);

            /* Enable VDD Memory */
            pmic::EnableVddMemory(SocType);

            /* Set VDDP select. */
            reg::Write(PMC + APBDEV_PMC_VDDP_SEL, sdram_params->PmcVddpSel);
            util::WaitMicroSeconds(sdram_params->PmcVddpSelWait);

            /* If Erista, Set DDR pad voltage. */
            if constexpr (SocType == fuse::SocType_Erista) {
                reg::Write(PMC + APBDEV_PMC_DDR_PWR, reg::Read(PMC + APBDEV_PMC_DDR_PWR));
            }

            /* Turn on MEM IO power. */
            reg::Write(PMC + APBDEV_PMC_NO_IOPOWER, sdram_params->PmcNoIoPower);
            reg::Write(PMC + APBDEV_PMC_REG_SHORT, sdram_params->PmcRegShort);
            reg::Write(PMC + APBDEV_PMC_DDR_CNTRL, sdram_params->PmcDdrCntrl);

            /* Apply patch 1. */
            *reinterpret_cast<volatile u32 *>(sdram_params->EmcBctSpare0) = sdram_params->EmcBctSpare1;

            /* Do main init. */
            InitializeSdramImpl<SocType>(sdram_params);

            /* Save parameters to scratch. */
            SaveSdramParamsToScratch<SocType>(sdram_params);
        }

    }

    void InitializeSdram() {
        /* Get soc type. */
        const auto soc_type = fuse::GetSocType();

        /* Get Sdram params. */
        void *sdram_params = GetSdramParams(soc_type);

        /* Perform SoC-specific init. */
        if (soc_type == fuse::SocType_Erista) {
            InitializeSdram<fuse::SocType_Erista>(sdram_params);
        } else /* if (soc_type == fuse::SocType_Mariko) */ {
            InitializeSdram<fuse::SocType_Mariko>(sdram_params);
        }

        /* Lock DRAM scratch. */
        pmc::LockSecureRegister(pmc::SecureRegister_DramParameters);
    }

}
