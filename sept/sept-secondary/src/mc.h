/*
 * Copyright (c) 2014, NVIDIA Corporation. All rights reserved.
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
 
#ifndef FUSEE_MC_H_
#define FUSEE_MC_H_

#include <stdint.h>
#include <stdbool.h>

#define MC_BASE  0x70019000
#define MAKE_MC_REG(n) MAKE_REG32(MC_BASE + n)

#define MC_INTSTATUS                                            0x0
#define MC_INTMASK                                              0x4
#define MC_ERR_STATUS                                           0x8
#define MC_ERR_ADR                                              0xc
#define MC_SMMU_CONFIG                                          0x10
#define MC_SMMU_TLB_CONFIG                                      0x14
#define MC_SMMU_PTC_CONFIG                                      0x18
#define MC_SMMU_PTB_ASID                                        0x1c
#define MC_SMMU_PTB_DATA                                        0x20
#define MC_SMMU_TLB_FLUSH                                       0x30
#define MC_SMMU_PTC_FLUSH                                       0x34
#define MC_SMMU_ASID_SECURITY                                   0x38
#define MC_SMMU_ASID_SECURITY_1                                 0x3c
#define MC_SMMU_ASID_SECURITY_2                                 0x9e0
#define MC_SMMU_ASID_SECURITY_3                                 0x9e4
#define MC_SMMU_ASID_SECURITY_4                                 0x9e8
#define MC_SMMU_ASID_SECURITY_5                                 0x9ec
#define MC_SMMU_ASID_SECURITY_6                                 0x9f0
#define MC_SMMU_ASID_SECURITY_7                                 0x9f4
#define MC_SMMU_AFI_ASID                                        0x238
#define MC_SMMU_AVPC_ASID                                       0x23c
#define MC_SMMU_TSEC_ASID                                       0x294
#define MC_SMMU_PPCS1_ASID                                      0x298
#define MC_SMMU_TRANSLATION_ENABLE_0                            0x228
#define MC_SMMU_TRANSLATION_ENABLE_1                            0x22c
#define MC_SMMU_TRANSLATION_ENABLE_2                            0x230
#define MC_SMMU_TRANSLATION_ENABLE_3                            0x234
#define MC_SMMU_TRANSLATION_ENABLE_4                            0xb98
#define MC_PCFIFO_CLIENT_CONFIG0                                0xdd0
#define MC_PCFIFO_CLIENT_CONFIG1                                0xdd4
#define MC_PCFIFO_CLIENT_CONFIG2                                0xdd8
#define MC_PCFIFO_CLIENT_CONFIG3                                0xddc
#define MC_PCFIFO_CLIENT_CONFIG4                                0xde0
#define MC_EMEM_CFG                                             0x50
#define MC_EMEM_ADR_CFG                                         0x54
#define MC_EMEM_ADR_CFG_DEV0                                    0x58
#define MC_EMEM_ADR_CFG_DEV1                                    0x5c
#define MC_EMEM_ADR_CFG_CHANNEL_MASK                            0x60
#define MC_EMEM_ADR_CFG_BANK_MASK_0                             0x64
#define MC_EMEM_ADR_CFG_BANK_MASK_1                             0x68
#define MC_EMEM_ADR_CFG_BANK_MASK_2                             0x6c
#define MC_SECURITY_CFG0                                        0x70
#define MC_SECURITY_CFG1                                        0x74
#define MC_SECURITY_CFG3                                        0x9bc
#define MC_SECURITY_RSV                                         0x7c
#define MC_EMEM_ARB_CFG                                         0x90
#define MC_EMEM_ARB_OUTSTANDING_REQ                             0x94
#define MC_EMEM_ARB_TIMING_RCD                                  0x98
#define MC_EMEM_ARB_TIMING_RP                                   0x9c
#define MC_EMEM_ARB_TIMING_RC                                   0xa0
#define MC_EMEM_ARB_TIMING_RAS                                  0xa4
#define MC_EMEM_ARB_TIMING_FAW                                  0xa8
#define MC_EMEM_ARB_TIMING_RRD                                  0xac
#define MC_EMEM_ARB_TIMING_RAP2PRE                              0xb0
#define MC_EMEM_ARB_TIMING_WAP2PRE                              0xb4
#define MC_EMEM_ARB_TIMING_R2R                                  0xb8
#define MC_EMEM_ARB_TIMING_W2W                                  0xbc
#define MC_EMEM_ARB_TIMING_R2W                                  0xc0
#define MC_EMEM_ARB_TIMING_W2R                                  0xc4
#define MC_EMEM_ARB_TIMING_RFCPB                                0x6c0
#define MC_EMEM_ARB_TIMING_CCDMW                                0x6c4
#define MC_EMEM_ARB_REFPB_HP_CTRL                               0x6f0
#define MC_EMEM_ARB_REFPB_BANK_CTRL                             0x6f4
#define MC_EMEM_ARB_DA_TURNS                                    0xd0
#define MC_EMEM_ARB_DA_COVERS                                   0xd4
#define MC_EMEM_ARB_MISC0                                       0xd8
#define MC_EMEM_ARB_MISC1                                       0xdc
#define MC_EMEM_ARB_MISC2                                       0xc8
#define MC_EMEM_ARB_RING1_THROTTLE                              0xe0
#define MC_EMEM_ARB_RING3_THROTTLE                              0xe4
#define MC_EMEM_ARB_NISO_THROTTLE                               0x6b0
#define MC_EMEM_ARB_OVERRIDE                                    0xe8
#define MC_EMEM_ARB_RSV                                         0xec
#define MC_CLKEN_OVERRIDE                                       0xf4
#define MC_TIMING_CONTROL_DBG                                   0xf8
#define MC_TIMING_CONTROL                                       0xfc
#define MC_STAT_CONTROL                                         0x100
#define MC_STAT_STATUS                                          0x104
#define MC_STAT_EMC_CLOCK_LIMIT                                 0x108
#define MC_STAT_EMC_CLOCK_LIMIT_MSBS                            0x10c
#define MC_STAT_EMC_CLOCKS                                      0x110
#define MC_STAT_EMC_CLOCKS_MSBS                                 0x114
#define MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_LO                    0x118
#define MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_LO                    0x158
#define MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_HI                    0x11c
#define MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_HI                    0x15c
#define MC_STAT_EMC_FILTER_SET0_ADR_LIMIT_UPPER                 0xa20
#define MC_STAT_EMC_FILTER_SET1_ADR_LIMIT_UPPER                 0xa24
#define MC_STAT_EMC_FILTER_SET0_VIRTUAL_ADR_LIMIT_LO            0x198
#define MC_STAT_EMC_FILTER_SET1_VIRTUAL_ADR_LIMIT_LO            0x1a8
#define MC_STAT_EMC_FILTER_SET0_VIRTUAL_ADR_LIMIT_HI            0x19c
#define MC_STAT_EMC_FILTER_SET1_VIRTUAL_ADR_LIMIT_HI            0x1ac
#define MC_STAT_EMC_FILTER_SET0_VIRTUAL_ADR_LIMIT_UPPER         0xa28
#define MC_STAT_EMC_FILTER_SET1_VIRTUAL_ADR_LIMIT_UPPER         0xa2c
#define MC_STAT_EMC_FILTER_SET0_ASID                            0x1a0
#define MC_STAT_EMC_FILTER_SET1_ASID                            0x1b0
#define MC_STAT_EMC_FILTER_SET0_SLACK_LIMIT                     0x120
#define MC_STAT_EMC_FILTER_SET1_SLACK_LIMIT                     0x160
#define MC_STAT_EMC_FILTER_SET0_CLIENT_0                        0x128
#define MC_STAT_EMC_FILTER_SET1_CLIENT_0                        0x168
#define MC_STAT_EMC_FILTER_SET0_CLIENT_1                        0x12c
#define MC_STAT_EMC_FILTER_SET1_CLIENT_1                        0x16c
#define MC_STAT_EMC_FILTER_SET0_CLIENT_2                        0x130
#define MC_STAT_EMC_FILTER_SET1_CLIENT_2                        0x170
#define MC_STAT_EMC_FILTER_SET0_CLIENT_3                        0x134
#define MC_STAT_EMC_FILTER_SET0_CLIENT_4                        0xb88
#define MC_STAT_EMC_FILTER_SET1_CLIENT_3                        0x174
#define MC_STAT_EMC_FILTER_SET1_CLIENT_4                        0xb8c
#define MC_STAT_EMC_SET0_COUNT                                  0x138
#define MC_STAT_EMC_SET0_COUNT_MSBS                             0x13c
#define MC_STAT_EMC_SET1_COUNT                                  0x178
#define MC_STAT_EMC_SET1_COUNT_MSBS                             0x17c
#define MC_STAT_EMC_SET0_SLACK_ACCUM                            0x140
#define MC_STAT_EMC_SET0_SLACK_ACCUM_MSBS                       0x144
#define MC_STAT_EMC_SET1_SLACK_ACCUM                            0x180
#define MC_STAT_EMC_SET1_SLACK_ACCUM_MSBS                       0x184
#define MC_STAT_EMC_SET0_HISTO_COUNT                            0x148
#define MC_STAT_EMC_SET0_HISTO_COUNT_MSBS                       0x14c
#define MC_STAT_EMC_SET1_HISTO_COUNT                            0x188
#define MC_STAT_EMC_SET1_HISTO_COUNT_MSBS                       0x18c
#define MC_STAT_EMC_SET0_MINIMUM_SLACK_OBSERVED                 0x150
#define MC_STAT_EMC_SET1_MINIMUM_SLACK_OBSERVED                 0x190
#define MC_STAT_EMC_SET0_IDLE_CYCLE_COUNT                       0x1b8
#define MC_STAT_EMC_SET0_IDLE_CYCL_COUNT_MSBS                   0x1bc
#define MC_STAT_EMC_SET1_IDLE_CYCLE_COUNT                       0x1c8
#define MC_STAT_EMC_SET1_IDLE_CYCL_COUNT_MSBS                   0x1cc
#define MC_STAT_EMC_SET0_IDLE_CYCLE_PARTITION_SELECT            0x1c0
#define MC_STAT_EMC_SET1_IDLE_CYCLE_PARTITION_SELECT            0x1d0
#define MC_CLIENT_HOTRESET_CTRL                                 0x200
#define MC_CLIENT_HOTRESET_CTRL_1                               0x970
#define MC_CLIENT_HOTRESET_STATUS                               0x204
#define MC_CLIENT_HOTRESET_STATUS_1                             0x974
#define MC_EMEM_ARB_ISOCHRONOUS_0                               0x208
#define MC_EMEM_ARB_ISOCHRONOUS_1                               0x20c
#define MC_EMEM_ARB_ISOCHRONOUS_2                               0x210
#define MC_EMEM_ARB_ISOCHRONOUS_3                               0x214
#define MC_EMEM_ARB_ISOCHRONOUS_4                               0xb94
#define MC_EMEM_ARB_HYSTERESIS_0                                0x218
#define MC_EMEM_ARB_HYSTERESIS_1                                0x21c
#define MC_EMEM_ARB_HYSTERESIS_2                                0x220
#define MC_EMEM_ARB_HYSTERESIS_3                                0x224
#define MC_EMEM_ARB_HYSTERESIS_4                                0xb84
#define MC_EMEM_ARB_DHYSTERESIS_0                               0xbb0
#define MC_EMEM_ARB_DHYSTERESIS_1                               0xbb4
#define MC_EMEM_ARB_DHYSTERESIS_2                               0xbb8
#define MC_EMEM_ARB_DHYSTERESIS_3                               0xbbc
#define MC_EMEM_ARB_DHYSTERESIS_4                               0xbc0
#define MC_EMEM_ARB_DHYST_CTRL                                  0xbcc
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_0                        0xbd0
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_1                        0xbd4
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_2                        0xbd8
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_3                        0xbdc
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_4                        0xbe0
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_5                        0xbe4
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_6                        0xbe8
#define MC_EMEM_ARB_DHYST_TIMEOUT_UTIL_7                        0xbec
#define MC_RESERVED_RSV                                         0x3fc
#define MC_DISB_EXTRA_SNAP_LEVELS                               0x408
#define MC_APB_EXTRA_SNAP_LEVELS                                0x2a4
#define MC_AHB_EXTRA_SNAP_LEVELS                                0x2a0
#define MC_USBD_EXTRA_SNAP_LEVELS                               0xa18
#define MC_ISP_EXTRA_SNAP_LEVELS                                0xa08
#define MC_AUD_EXTRA_SNAP_LEVELS                                0xa10
#define MC_MSE_EXTRA_SNAP_LEVELS                                0x40c
#define MC_GK2_EXTRA_SNAP_LEVELS                                0xa40
#define MC_A9AVPPC_EXTRA_SNAP_LEVELS                            0x414
#define MC_FTOP_EXTRA_SNAP_LEVELS                               0x2bc
#define MC_JPG_EXTRA_SNAP_LEVELS                                0xa3c
#define MC_HOST_EXTRA_SNAP_LEVELS                               0xa14
#define MC_SAX_EXTRA_SNAP_LEVELS                                0x2c0
#define MC_DIS_EXTRA_SNAP_LEVELS                                0x2ac
#define MC_VICPC_EXTRA_SNAP_LEVELS                              0xa1c
#define MC_HDAPC_EXTRA_SNAP_LEVELS                              0xa48
#define MC_AVP_EXTRA_SNAP_LEVELS                                0x2a8
#define MC_USBX_EXTRA_SNAP_LEVELS                               0x404
#define MC_PCX_EXTRA_SNAP_LEVELS                                0x2b8
#define MC_SD_EXTRA_SNAP_LEVELS                                 0xa04
#define MC_DFD_EXTRA_SNAP_LEVELS                                0xa4c
#define MC_VE_EXTRA_SNAP_LEVELS                                 0x2d8
#define MC_GK_EXTRA_SNAP_LEVELS                                 0xa00
#define MC_VE2_EXTRA_SNAP_LEVELS                                0x410
#define MC_SDM_EXTRA_SNAP_LEVELS                                0xa44
#define MC_VIDEO_PROTECT_BOM                                    0x648
#define MC_VIDEO_PROTECT_SIZE_MB                                0x64c
#define MC_VIDEO_PROTECT_BOM_ADR_HI                             0x978
#define MC_VIDEO_PROTECT_REG_CTRL                               0x650
#define MC_ERR_VPR_STATUS                                       0x654
#define MC_ERR_VPR_ADR                                          0x658
#define MC_VIDEO_PROTECT_VPR_OVERRIDE                           0x418
#define MC_VIDEO_PROTECT_VPR_OVERRIDE1                          0x590
#define MC_IRAM_BOM                                             0x65c
#define MC_IRAM_TOM                                             0x660
#define MC_IRAM_ADR_HI                                          0x980
#define MC_IRAM_REG_CTRL                                        0x964
#define MC_EMEM_CFG_ACCESS_CTRL                                 0x664
#define MC_TZ_SECURITY_CTRL                                     0x668
#define MC_EMEM_ARB_OUTSTANDING_REQ_RING3                       0x66c
#define MC_EMEM_ARB_OUTSTANDING_REQ_NISO                        0x6b4
#define MC_EMEM_ARB_RING0_THROTTLE_MASK                         0x6bc
#define MC_EMEM_ARB_NISO_THROTTLE_MASK                          0x6b8
#define MC_EMEM_ARB_NISO_THROTTLE_MASK_1                        0xb80
#define MC_SEC_CARVEOUT_BOM                                     0x670
#define MC_SEC_CARVEOUT_SIZE_MB                                 0x674
#define MC_SEC_CARVEOUT_ADR_HI                                  0x9d4
#define MC_SEC_CARVEOUT_REG_CTRL                                0x678
#define MC_ERR_SEC_STATUS                                       0x67c
#define MC_ERR_SEC_ADR                                          0x680
#define MC_PC_IDLE_CLOCK_GATE_CONFIG                            0x684
#define MC_STUTTER_CONTROL                                      0x688
#define MC_RESERVED_RSV_1                                       0x958
#define MC_DVFS_PIPE_SELECT                                     0x95c
#define MC_AHB_PTSA_MIN                                         0x4e0
#define MC_AUD_PTSA_MIN                                         0x54c
#define MC_MLL_MPCORER_PTSA_RATE                                0x44c
#define MC_RING2_PTSA_RATE                                      0x440
#define MC_USBD_PTSA_RATE                                       0x530
#define MC_USBX_PTSA_MIN                                        0x528
#define MC_USBD_PTSA_MIN                                        0x534
#define MC_APB_PTSA_MAX                                         0x4f0
#define MC_JPG_PTSA_RATE                                        0x584
#define MC_DIS_PTSA_MIN                                         0x420
#define MC_AVP_PTSA_MAX                                         0x4fc
#define MC_AVP_PTSA_RATE                                        0x4f4
#define MC_RING1_PTSA_MIN                                       0x480
#define MC_DIS_PTSA_MAX                                         0x424
#define MC_SD_PTSA_MAX                                          0x4d8
#define MC_MSE_PTSA_RATE                                        0x4c4
#define MC_VICPC_PTSA_MIN                                       0x558
#define MC_PCX_PTSA_MAX                                         0x4b4
#define MC_ISP_PTSA_RATE                                        0x4a0
#define MC_A9AVPPC_PTSA_MIN                                     0x48c
#define MC_RING2_PTSA_MAX                                       0x448
#define MC_AUD_PTSA_RATE                                        0x548
#define MC_HOST_PTSA_MIN                                        0x51c
#define MC_MLL_MPCORER_PTSA_MAX                                 0x454
#define MC_SD_PTSA_MIN                                          0x4d4
#define MC_RING1_PTSA_RATE                                      0x47c
#define MC_JPG_PTSA_MIN                                         0x588
#define MC_HDAPC_PTSA_MIN                                       0x62c
#define MC_AVP_PTSA_MIN                                         0x4f8
#define MC_JPG_PTSA_MAX                                         0x58c
#define MC_VE_PTSA_MAX                                          0x43c
#define MC_DFD_PTSA_MAX                                         0x63c
#define MC_VICPC_PTSA_RATE                                      0x554
#define MC_GK_PTSA_MAX                                          0x544
#define MC_VICPC_PTSA_MAX                                       0x55c
#define MC_SDM_PTSA_MAX                                         0x624
#define MC_SAX_PTSA_RATE                                        0x4b8
#define MC_PCX_PTSA_MIN                                         0x4b0
#define MC_APB_PTSA_MIN                                         0x4ec
#define MC_GK2_PTSA_MIN                                         0x614
#define MC_PCX_PTSA_RATE                                        0x4ac
#define MC_RING1_PTSA_MAX                                       0x484
#define MC_HDAPC_PTSA_RATE                                      0x628
#define MC_MLL_MPCORER_PTSA_MIN                                 0x450
#define MC_GK2_PTSA_MAX                                         0x618
#define MC_AUD_PTSA_MAX                                         0x550
#define MC_GK2_PTSA_RATE                                        0x610
#define MC_ISP_PTSA_MAX                                         0x4a8
#define MC_DISB_PTSA_RATE                                       0x428
#define MC_VE2_PTSA_MAX                                         0x49c
#define MC_DFD_PTSA_MIN                                         0x638
#define MC_FTOP_PTSA_RATE                                       0x50c
#define MC_A9AVPPC_PTSA_RATE                                    0x488
#define MC_VE2_PTSA_MIN                                         0x498
#define MC_USBX_PTSA_MAX                                        0x52c
#define MC_DIS_PTSA_RATE                                        0x41c
#define MC_USBD_PTSA_MAX                                        0x538
#define MC_A9AVPPC_PTSA_MAX                                     0x490
#define MC_USBX_PTSA_RATE                                       0x524
#define MC_FTOP_PTSA_MAX                                        0x514
#define MC_HDAPC_PTSA_MAX                                       0x630
#define MC_SD_PTSA_RATE                                         0x4d0
#define MC_DFD_PTSA_RATE                                        0x634
#define MC_FTOP_PTSA_MIN                                        0x510
#define MC_SDM_PTSA_RATE                                        0x61c
#define MC_AHB_PTSA_RATE                                        0x4dc
#define MC_SMMU_SMMU_PTSA_MAX                                   0x460
#define MC_RING2_PTSA_MIN                                       0x444
#define MC_SDM_PTSA_MIN                                         0x620
#define MC_APB_PTSA_RATE                                        0x4e8
#define MC_MSE_PTSA_MIN                                         0x4c8
#define MC_HOST_PTSA_RATE                                       0x518
#define MC_VE_PTSA_RATE                                         0x434
#define MC_AHB_PTSA_MAX                                         0x4e4
#define MC_SAX_PTSA_MIN                                         0x4bc
#define MC_SMMU_SMMU_PTSA_MIN                                   0x45c
#define MC_ISP_PTSA_MIN                                         0x4a4
#define MC_HOST_PTSA_MAX                                        0x520
#define MC_SAX_PTSA_MAX                                         0x4c0
#define MC_VE_PTSA_MIN                                          0x438
#define MC_GK_PTSA_MIN                                          0x540
#define MC_MSE_PTSA_MAX                                         0x4cc
#define MC_DISB_PTSA_MAX                                        0x430
#define MC_DISB_PTSA_MIN                                        0x42c
#define MC_SMMU_SMMU_PTSA_RATE                                  0x458
#define MC_VE2_PTSA_RATE                                        0x494
#define MC_GK_PTSA_RATE                                         0x53c
#define MC_PTSA_GRANT_DECREMENT                                 0x960
#define MC_LATENCY_ALLOWANCE_AVPC_0                             0x2e4
#define MC_LATENCY_ALLOWANCE_AXIAP_0                            0x3a0
#define MC_LATENCY_ALLOWANCE_XUSB_1                             0x380
#define MC_LATENCY_ALLOWANCE_ISP2B_0                            0x384
#define MC_LATENCY_ALLOWANCE_SDMMCAA_0                          0x3bc
#define MC_LATENCY_ALLOWANCE_SDMMCA_0                           0x3b8
#define MC_LATENCY_ALLOWANCE_ISP2_0                             0x370
#define MC_LATENCY_ALLOWANCE_SE_0                               0x3e0
#define MC_LATENCY_ALLOWANCE_ISP2_1                             0x374
#define MC_LATENCY_ALLOWANCE_DC_0                               0x2e8
#define MC_LATENCY_ALLOWANCE_VIC_0                              0x394
#define MC_LATENCY_ALLOWANCE_DCB_1                              0x2f8
#define MC_LATENCY_ALLOWANCE_NVDEC_0                            0x3d8
#define MC_LATENCY_ALLOWANCE_DCB_2                              0x2fc
#define MC_LATENCY_ALLOWANCE_TSEC_0                             0x390
#define MC_LATENCY_ALLOWANCE_DC_2                               0x2f0
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0AB                  0x694
#define MC_LATENCY_ALLOWANCE_PPCS_1                             0x348
#define MC_LATENCY_ALLOWANCE_XUSB_0                             0x37c
#define MC_LATENCY_ALLOWANCE_PPCS_0                             0x344
#define MC_LATENCY_ALLOWANCE_TSECB_0                            0x3f0
#define MC_LATENCY_ALLOWANCE_AFI_0                              0x2e0
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0B                   0x698
#define MC_LATENCY_ALLOWANCE_DC_1                               0x2ec
#define MC_LATENCY_ALLOWANCE_APE_0                              0x3dc
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0C                   0x6a0
#define MC_LATENCY_ALLOWANCE_A9AVP_0                            0x3a4
#define MC_LATENCY_ALLOWANCE_GPU2_0                             0x3e8
#define MC_LATENCY_ALLOWANCE_DCB_0                              0x2f4
#define MC_LATENCY_ALLOWANCE_HC_1                               0x314
#define MC_LATENCY_ALLOWANCE_SDMMC_0                            0x3c0
#define MC_LATENCY_ALLOWANCE_NVJPG_0                            0x3e4
#define MC_LATENCY_ALLOWANCE_PTC_0                              0x34c
#define MC_LATENCY_ALLOWANCE_ETR_0                              0x3ec
#define MC_LATENCY_ALLOWANCE_MPCORE_0                           0x320
#define MC_LATENCY_ALLOWANCE_VI2_0                              0x398
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0BB                  0x69c
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0CB                  0x6a4
#define MC_LATENCY_ALLOWANCE_SATA_0                             0x350
#define MC_SCALED_LATENCY_ALLOWANCE_DISPLAY0A                   0x690
#define MC_LATENCY_ALLOWANCE_HC_0                               0x310
#define MC_LATENCY_ALLOWANCE_DC_3                               0x3c8
#define MC_LATENCY_ALLOWANCE_GPU_0                              0x3ac
#define MC_LATENCY_ALLOWANCE_SDMMCAB_0                          0x3c4
#define MC_LATENCY_ALLOWANCE_ISP2B_1                            0x388
#define MC_LATENCY_ALLOWANCE_NVENC_0                            0x328
#define MC_LATENCY_ALLOWANCE_HDA_0                              0x318
#define MC_MIN_LENGTH_APE_0                                     0xb34
#define MC_MIN_LENGTH_DCB_2                                     0x8a8
#define MC_MIN_LENGTH_A9AVP_0                                   0x950
#define MC_MIN_LENGTH_TSEC_0                                    0x93c
#define MC_MIN_LENGTH_DC_1                                      0x898
#define MC_MIN_LENGTH_AXIAP_0                                   0x94c
#define MC_MIN_LENGTH_ISP2B_0                                   0x930
#define MC_MIN_LENGTH_VI2_0                                     0x944
#define MC_MIN_LENGTH_DCB_0                                     0x8a0
#define MC_MIN_LENGTH_DCB_1                                     0x8a4
#define MC_MIN_LENGTH_PPCS_1                                    0x8f4
#define MC_MIN_LENGTH_NVJPG_0                                   0xb3c
#define MC_MIN_LENGTH_HDA_0                                     0x8c4
#define MC_MIN_LENGTH_NVENC_0                                   0x8d4
#define MC_MIN_LENGTH_SDMMC_0                                   0xb18
#define MC_MIN_LENGTH_ISP2B_1                                   0x934
#define MC_MIN_LENGTH_HC_1                                      0x8c0
#define MC_MIN_LENGTH_DC_3                                      0xb20
#define MC_MIN_LENGTH_AVPC_0                                    0x890
#define MC_MIN_LENGTH_VIC_0                                     0x940
#define MC_MIN_LENGTH_ISP2_0                                    0x91c
#define MC_MIN_LENGTH_HC_0                                      0x8bc
#define MC_MIN_LENGTH_SE_0                                      0xb38
#define MC_MIN_LENGTH_NVDEC_0                                   0xb30
#define MC_MIN_LENGTH_SATA_0                                    0x8fc
#define MC_MIN_LENGTH_DC_0                                      0x894
#define MC_MIN_LENGTH_XUSB_1                                    0x92c
#define MC_MIN_LENGTH_DC_2                                      0x89c
#define MC_MIN_LENGTH_SDMMCAA_0                                 0xb14
#define MC_MIN_LENGTH_GPU_0                                     0xb04
#define MC_MIN_LENGTH_ETR_0                                     0xb44
#define MC_MIN_LENGTH_AFI_0                                     0x88c
#define MC_MIN_LENGTH_PPCS_0                                    0x8f0
#define MC_MIN_LENGTH_ISP2_1                                    0x920
#define MC_MIN_LENGTH_XUSB_0                                    0x928
#define MC_MIN_LENGTH_MPCORE_0                                  0x8cc
#define MC_MIN_LENGTH_TSECB_0                                   0xb48
#define MC_MIN_LENGTH_SDMMCA_0                                  0xb10
#define MC_MIN_LENGTH_GPU2_0                                    0xb40
#define MC_MIN_LENGTH_SDMMCAB_0                                 0xb1c
#define MC_MIN_LENGTH_PTC_0                                     0x8f8
#define MC_EMEM_ARB_OVERRIDE_1                                  0x968
#define MC_VIDEO_PROTECT_GPU_OVERRIDE_0                         0x984
#define MC_VIDEO_PROTECT_GPU_OVERRIDE_1                         0x988
#define MC_EMEM_ARB_STATS_0                                     0x990
#define MC_EMEM_ARB_STATS_1                                     0x994
#define MC_MTS_CARVEOUT_BOM                                     0x9a0
#define MC_MTS_CARVEOUT_SIZE_MB                                 0x9a4
#define MC_MTS_CARVEOUT_ADR_HI                                  0x9a8
#define MC_MTS_CARVEOUT_REG_CTRL                                0x9ac
#define MC_ERR_MTS_STATUS                                       0x9b0
#define MC_ERR_MTS_ADR                                          0x9b4
#define MC_ERR_GENERALIZED_CARVEOUT_STATUS                      0xc00
#define MC_ERR_GENERALIZED_CARVEOUT_ADR                         0xc04
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS2     0xd74
#define MC_SECURITY_CARVEOUT4_CFG0                              0xcf8
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS2                    0xd10
#define MC_SECURITY_CARVEOUT4_SIZE_128KB                        0xd04
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS4                    0xc28
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS1     0xc30
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS4     0xc8c
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS0     0xd1c
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS1     0xd70
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS0     0xc2c
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS4     0xd7c
#define MC_SECURITY_CARVEOUT3_SIZE_128KB                        0xcb4
#define MC_SECURITY_CARVEOUT2_CFG0                              0xc58
#define MC_SECURITY_CARVEOUT1_CFG0                              0xc08
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS2     0xc84
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS0                    0xc68
#define MC_SECURITY_CARVEOUT3_BOM                               0xcac
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS2                    0xc70
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS3     0xd78
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS0     0xc7c
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS4                    0xd18
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS1                    0xcbc
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS3     0xc38
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS2     0xc34
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS2                    0xcc0
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS2                    0xd60
#define MC_SECURITY_CARVEOUT3_CFG0                              0xca8
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS0                    0xcb8
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS3     0xc88
#define MC_SECURITY_CARVEOUT2_SIZE_128KB                        0xc64
#define MC_SECURITY_CARVEOUT5_BOM_HI                            0xd50
#define MC_SECURITY_CARVEOUT1_SIZE_128KB                        0xc14
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS3                    0xd14
#define MC_SECURITY_CARVEOUT1_BOM                               0xc0c
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS4     0xd2c
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS4                    0xd68
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS4                    0xcc8
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS0                    0xd58
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS2     0xd24
#define MC_SECURITY_CARVEOUT3_CLIENT_ACCESS3                    0xcc4
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS4                    0xc78
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS1                    0xc1c
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS0                    0xc18
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS3     0xd28
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS1                    0xd5c
#define MC_SECURITY_CARVEOUT3_BOM_HI                            0xcb0
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS3     0xcd8
#define MC_SECURITY_CARVEOUT2_BOM_HI                            0xc60
#define MC_SECURITY_CARVEOUT4_BOM_HI                            0xd00
#define MC_SECURITY_CARVEOUT5_CLIENT_ACCESS3                    0xd64
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS4     0xcdc
#define MC_SECURITY_CARVEOUT2_CLIENT_FORCE_INTERNAL_ACCESS1     0xc80
#define MC_SECURITY_CARVEOUT5_SIZE_128KB                        0xd54
#define MC_SECURITY_CARVEOUT4_CLIENT_FORCE_INTERNAL_ACCESS1     0xd20
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS2     0xcd4
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS1                    0xd0c
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS3                    0xc74
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS0     0xccc
#define MC_SECURITY_CARVEOUT4_BOM                               0xcfc
#define MC_SECURITY_CARVEOUT5_CFG0                              0xd48
#define MC_SECURITY_CARVEOUT2_BOM                               0xc5c
#define MC_SECURITY_CARVEOUT5_BOM                               0xd4c
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS3                    0xc24
#define MC_SECURITY_CARVEOUT5_CLIENT_FORCE_INTERNAL_ACCESS0     0xd6c
#define MC_SECURITY_CARVEOUT3_CLIENT_FORCE_INTERNAL_ACCESS1     0xcd0
#define MC_SECURITY_CARVEOUT1_BOM_HI                            0xc10
#define MC_SECURITY_CARVEOUT1_CLIENT_ACCESS2                    0xc20
#define MC_SECURITY_CARVEOUT1_CLIENT_FORCE_INTERNAL_ACCESS4     0xc3c
#define MC_SECURITY_CARVEOUT2_CLIENT_ACCESS1                    0xc6c
#define MC_SECURITY_CARVEOUT4_CLIENT_ACCESS0                    0xd08
#define MC_ERR_APB_ASID_UPDATE_STATUS                           0x9d0
#define MC_DA_CONFIG0                                           0x9dc
#define MC_UNTRANSLATED_REGION_CHECK                            0x948

/* Memory Controller clients */
#define CLIENT_ACCESS_NUM_CLIENTS   32
typedef enum {
    /* _ACCESS0 */
    CSR_PTCR = (0 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_DISPLAY0A = (1 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_DISPLAY0AB = (2 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_DISPLAY0B = (3 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_DISPLAY0BB = (4 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_DISPLAY0C = (5 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_DISPLAY0CB = (6 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_AFIR = (14 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_AVPCARM7R = (15 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_DISPLAYHC = (16 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_DISPLAYHCB = (17 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_HDAR = (21 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_HOST1XDMAR = (22 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_HOST1XR = (23 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_NVENCSRD = (28 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_PPCSAHBDMAR = (29 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_PPCSAHBSLVR = (30 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    CSR_SATAR = (31 - (CLIENT_ACCESS_NUM_CLIENTS * 0)),
    
    /* _ACCESS1 */
    CSR_VDEBSEVR = (34 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSR_VDEMBER = (35 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSR_VDEMCER = (36 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSR_VDETPER = (37 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSR_MPCORELPR = (38 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSR_MPCORER = (39 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_NVENCSWR = (43 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_AFIW = (49 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_AVPCARM7W = (50 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_HDAW = (53 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_HOST1XW = (54 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_MPCORELPW = (56 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_MPCOREW = (57 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_PPCSAHBDMAW = (59 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_PPCSAHBSLVW = (60 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_SATAW = (61 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_VDEBSEVW = (62 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    CSW_VDEDBGW = (63 - (CLIENT_ACCESS_NUM_CLIENTS * 1)),
    
    /* _ACCESS2 */
    CSW_VDEMBEW = (64 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_VDETPMW = (65 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSR_ISPRA = (68 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_ISPWA = (70 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_ISPWB = (71 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSR_XUSB_HOSTR = (74 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_XUSB_HOSTW = (75 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSR_XUSB_DEVR = (76 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_XUSB_DEVW = (77 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSR_ISPRAB = (78 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_ISPWAB = (80 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_ISPWBB = (81 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSR_TSECSRD = (84 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_TSECSWR = (85 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSR_A9AVPSCR = (86 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_A9AVPSCW = (87 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSR_GPUSRD = (88 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSW_GPUSWR = (89 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    CSR_DISPLAYT = (90 - (CLIENT_ACCESS_NUM_CLIENTS * 2)),
    
    /* _ACCESS3 */
    CSR_SDMMCRA = (96 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSR_SDMMCRAA = (97 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSR_SDMMCR = (98 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSR_SDMMCRAB = (99 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_SDMMCWA = (100 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_SDMMCWAA = (101 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_SDMMCW = (102 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_SDMMCWAB = (103 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSR_VICSRD = (108 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_VICSWR = (109 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_VIW = (114 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSR_DISPLAYD = (115 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSR_NVDECSRD = (120 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_NVDECSWR = (121 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSR_APER = (122 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_APEW = (123 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSR_NVJPGSRD = (126 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    CSW_NVJPGSWR = (127 - (CLIENT_ACCESS_NUM_CLIENTS * 3)),
    
    /* _ACCESS4 */
    CSR_SESRD = (128 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSW_SESWR = (129 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSR_AXIAPR = (130 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSW_AXIAPW = (131 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSR_ETRR = (132 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSW_ETRW = (133 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSR_TSECSRDB = (134 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSW_TSECSWRB = (135 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSR_GPUSRD2 = (136 - (CLIENT_ACCESS_NUM_CLIENTS * 4)),
    CSW_GPUSWR2 = (137 - (CLIENT_ACCESS_NUM_CLIENTS * 4))
} McClient;

void mc_config_tsec_carveout(uint32_t bom, uint32_t size1mb, bool lock);
void mc_config_carveout();
void mc_config_carveout_finalize();
void mc_enable_ahb_redirect();
void mc_disable_ahb_redirect();
void mc_enable();

#endif