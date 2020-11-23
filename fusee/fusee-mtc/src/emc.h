/*
 * arch/arm/mach-tegra/tegra21_emc.h
 *
 * Copyright (c) 2014-2015, NVIDIA CORPORATION.  All rights reserved.
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
 
#ifndef FUSEE_EMC_H_
#define FUSEE_EMC_H_

#define EMC_BASE 0x7001B000
#define EMC0_BASE 0x7001E000
#define EMC1_BASE 0x7001F000
#define MAKE_EMC_REG(n) MAKE_REG32(EMC_BASE + n)
#define MAKE_EMC0_REG(n) MAKE_REG32(EMC0_BASE + n)
#define MAKE_EMC1_REG(n) MAKE_REG32(EMC1_BASE + n)

#define EMC_INTSTATUS                       0x0
#define EMC_INTSTATUS_MRR_DIVLD                 (0x1 << 5)
#define EMC_INTSTATUS_CLKCHANGE_COMPLETE            (0x1 << 4)

#define EMC_INTMASK                     0x4
#define EMC_DBG                         0x8
#define EMC_DBG_WRITE_MUX_ACTIVE                (1 << 1)
#define EMC_DBG_CFG_SWAP_SHIFT                  26
#define EMC_DBG_CFG_SWAP_MASK                   \
    (0x3 << EMC_DBG_CFG_SWAP_SHIFT)
#define EMC_DBG_WRITE_ACTIVE_ONLY               (1 << 30)

#define EMC_CONFIG_SAMPLE_DELAY                 0x5f0
#define EMC_CFG_UPDATE                      0x5f4
#define EMC_CFG_UPDATE_UPDATE_DLL_IN_UPDATE_SHIFT       9
#define EMC_CFG_UPDATE_UPDATE_DLL_IN_UPDATE_MASK        \
    (0x3 << EMC_CFG_UPDATE_UPDATE_DLL_IN_UPDATE_SHIFT)
#define EMC_CFG                         0xc
#define EMC_CFG_DRAM_CLKSTOP_PD                 (1 << 31)
#define EMC_CFG_DRAM_CLKSTOP_SR                 (1 << 30)
#define EMC_CFG_DRAM_ACPD                   (1 << 29)
#define EMC_CFG_DYN_SELF_REF                    (1 << 28)
#define EMC_CFG_REQACT_ASYNC                    (1 << 26)
#define EMC_CFG_AUTO_PRE_WR                 (1 << 25)
#define EMC_CFG_AUTO_PRE_RD                 (1 << 24)
#define EMC_CFG_MAM_PRE_WR                  (1 << 23)
#define EMC_CFG_MAN_PRE_RD                  (1 << 22)
#define EMC_CFG_PERIODIC_QRST                   (1 << 21)
#define EMC_CFG_PERIODIC_QRST_SHIFT             (21)
#define EMC_CFG_EN_DYNAMIC_PUTERM               (1 << 20)
#define EMC_CFG_DLY_WR_DQ_HALF_CLOCK                (1 << 19)
#define EMC_CFG_DSR_VTTGEN_DRV_EN               (1 << 18)
#define EMC_CFG_EMC2MC_CLK_RATIO                (3 << 16)
#define EMC_CFG_WAIT_FOR_ISP2B_READY_B4_CC          (1 << 9)
#define EMC_CFG_WAIT_FOR_VI2_READY_B4_CC            (1 << 8)
#define EMC_CFG_WAIT_FOR_ISP2_READY_B4_CC           (1 << 7)
#define EMC_CFG_INVERT_DQM                  (1 << 6)
#define EMC_CFG_WAIT_FOR_DISPLAYB_READY_B4_CC           (1 << 5)
#define EMC_CFG_WAIT_FOR_DISPLAY_READY_B4_CC            (1 << 4)
#define EMC_CFG_EMC2PMACRO_CFG_BYPASS_DATAPIPE2         (1 << 3)
#define EMC_CFG_EMC2PMACRO_CFG_BYPASS_DATAPIPE1         (1 << 2)
#define EMC_CFG_EMC2PMACRO_CFG_BYPASS_ADDRPIPE          (1 << 1)

#define EMC_ADR_CFG                     0x10
#define EMC_REFCTRL                     0x20
#define EMC_REFCTRL_DEV_SEL_SHIFT               0
#define EMC_REFCTRL_DEV_SEL_MASK                \
    (0x3 << EMC_REFCTRL_DEV_SEL_SHIFT)
#define EMC_REFCTRL_ENABLE                  (0x1 << 31)
#define EMC_REFCTRL_ENABLE_ALL(num)             \
    (((((num) > 1) ? 0 : 2) << EMC_REFCTRL_DEV_SEL_SHIFT)   \
     | EMC_REFCTRL_ENABLE)
#define EMC_REFCTRL_DISABLE_ALL(num)                \
    ((((num) > 1) ? 0 : 2) << EMC_REFCTRL_DEV_SEL_SHIFT)

#define EMC_PIN                         0x24
#define EMC_PIN_PIN_CKE_PER_DEV                 (1 << 2)
#define EMC_PIN_PIN_CKEB                    (1 << 1)
#define EMC_PIN_PIN_CKE                     (1 << 0)

#define EMC_CLK_FORCE_CC_TRIGGER                (1 << 27)

#define EMC_TIMING_CONTROL                  0x28
#define EMC_RC                          0x2c
#define EMC_RFC                         0x30
#define EMC_RFCPB                       0x590
#define EMC_RAS                         0x34
#define EMC_RP                          0x38
#define EMC_R2W                         0x3c
#define EMC_W2R                         0x40
#define EMC_R2P                         0x44
#define EMC_W2P                         0x48
#define EMC_CCDMW                       0x5c0
#define EMC_RD_RCD                      0x4c
#define EMC_WR_RCD                      0x50
#define EMC_RRD                         0x54
#define EMC_REXT                        0x58
#define EMC_WDV                         0x5c
#define EMC_QUSE                        0x60
#define EMC_QRST                        0x64
#define EMC_ISSUE_QRST                      0x428
#define EMC_QSAFE                       0x68
#define EMC_RDV                         0x6c
#define EMC_REFRESH                     0x70
#define EMC_BURST_REFRESH_NUM                   0x74
#define EMC_PDEX2WR                     0x78
#define EMC_PDEX2RD                     0x7c
#define EMC_PDEX2CKE                        0x118
#define EMC_PCHG2PDEN                       0x80
#define EMC_ACT2PDEN                        0x84
#define EMC_AR2PDEN                     0x88
#define EMC_RW2PDEN                     0x8c
#define EMC_CKE2PDEN                        0x11c
#define EMC_TXSR                        0x90
#define EMC_TCKE                        0x94
#define EMC_TFAW                        0x98
#define EMC_TRPAB                       0x9c
#define EMC_TCLKSTABLE                      0xa0
#define EMC_TCLKSTOP                        0xa4
#define EMC_TREFBW                      0xa8
#define EMC_TPPD                        0xac
#define EMC_PDEX2MRR                        0xb4
#define EMC_ODT_WRITE                       0xb0
#define EMC_WEXT                        0xb8
#define EMC_RFC_SLR                     0xc0
#define EMC_MRS_WAIT_CNT2                   0xc4
#define EMC_MRS_WAIT_CNT2_MRS_EXT2_WAIT_CNT_SHIFT       16
#define EMC_MRS_WAIT_CNT2_MRS_EXT2_WAIT_CNT_MASK        \
    (0x7ff << EMC_MRS_WAIT_CNT2_MRS_EXT2_WAIT_CNT_SHIFT)
#define EMC_MRS_WAIT_CNT2_MRS_EXT1_WAIT_CNT_SHIFT       0
#define EMC_MRS_WAIT_CNT2_MRS_EXT1_WAIT_CNT_MASK        \
    (0x3ff << EMC_MRS_WAIT_CNT2_MRS_EXT1_WAIT_CNT_SHIFT)

#define EMC_MRS_WAIT_CNT                    0xc8
#define EMC_MRS_WAIT_CNT_SHORT_WAIT_SHIFT           0
#define EMC_MRS_WAIT_CNT_SHORT_WAIT_MASK            \
    (0x3FF << EMC_MRS_WAIT_CNT_SHORT_WAIT_SHIFT)
#define EMC_MRS_WAIT_CNT_LONG_WAIT_SHIFT            16
#define EMC_MRS_WAIT_CNT_LONG_WAIT_MASK             \
    (0x3FF << EMC_MRS_WAIT_CNT_LONG_WAIT_SHIFT)

#define EMC_MRS                         0xcc
#define EMC_MODE_SET_DLL_RESET                  (1 << 8)
#define EMC_MRS_USE_MRS_LONG_CNT                (1 << 26)

#define EMC_EMRS                        0xd0
#define EMC_EMRS_USE_EMRS_LONG_CNT              (1 << 26)

#define EMC_REF                         0xd4
#define EMC_REF_FORCE_CMD                   1

#define EMC_PRE                         0xd8
#define EMC_NOP                         0xdc
#define EMC_SELF_REF                            0xe0
#define EMC_SELF_REF_CMD_ENABLED                (1 << 0)
#define EMC_SELF_REF_ACTIVE_SELF_REF                (1 << 8)
#define EMC_SELF_REF_DEV_SEL_SHIFT              30
#define EMC_SELF_REF_DEV_SEL_MASK               \
    (0x3 << EMC_SELF_REF_DEV_SEL_SHIFT)

#define EMC_DPD                         0xe4
#define EMC_MRW                         0xe8
#define EMC_MRW_MRW_OP_SHIFT                    0
#define EMC_MRW_MRW_OP_MASK                 \
    (0xff << EMC_MRW_MRW_OP_SHIFT)
#define EMC_MRW_MRW_MA_SHIFT                    16
#define EMC_MRW_MRW_MA_MASK                 \
    (0xff << EMC_MRW_MRW_MA_SHIFT)
#define EMC_MRW_USE_MRW_LONG_CNT                26
#define EMC_MRW_USE_MRW_EXT_CNT                 27
#define EMC_MRW_MRW_DEV_SELECTN_SHIFT               30
#define EMC_MRW_MRW_DEV_SELECTN_MASK                \
    (0x3 << EMC_MRW_MRW_DEV_SELECTN_SHIFT)

#define EMC_MRR                         0xec
#define EMC_MRR_DEV_SEL_SHIFT                   30
#define EMC_MRR_DEV_SEL_MASK                    \
    (0x3 << EMC_SELF_REF_DEV_SEL_SHIFT)
#define EMC_MRR_MA_SHIFT                    16
#define EMC_MRR_MA_MASK                     \
    (0xff << EMC_MRR_MA_SHIFT)
#define EMC_MRR_DATA_SHIFT                  0
#define EMC_MRR_DATA_MASK                   \
    (0xffff << EMC_MRR_DATA_SHIFT)
#define LPDDR2_MR4_TEMP_SHIFT                   0
#define LPDDR2_MR4_TEMP_MASK                    \
    (0x7 << LPDDR2_MR4_TEMP_SHIFT)

#define EMC_CMDQ                        0xf0
#define EMC_MC2EMCQ                     0xf4
#define EMC_FBIO_SPARE                      0x100
#define EMC_FBIO_CFG5                       0x104
#define EMC_FBIO_CFG5_DRAM_TYPE_SHIFT               0
#define EMC_FBIO_CFG5_DRAM_TYPE_MASK                \
    (0x3 << EMC_FBIO_CFG5_DRAM_TYPE_SHIFT)
#define EMC_FBIO_CFG5_CMD_TX_DIS                (1 << 8)
#define EMC_FBIO_CFG5_CMD_BUS_RETURN_TO_ZERO            (1 << 27)

#define EMC_CFG5_QUSE_MODE_SHIFT                13
#define EMC_CFG5_QUSE_MODE_MASK                 \
    (0x7 << EMC_CFG5_QUSE_MODE_SHIFT)

#define EMC_CFG_RSV                     0x120
#define EMC_ACPD_CONTROL                    0x124
#define EMC_MPC                         0x128
#define EMC_EMRS2                       0x12c
#define EMC_EMRS2_USE_EMRS2_LONG_CNT                (1 << 26)

#define EMC_EMRS3                       0x130
#define EMC_MRW2                        0x134
#define EMC_MRW3                        0x138
#define EMC_MRW4                        0x13c
#define EMC_MRW5                        0x4a0
#define EMC_MRW6                        0x4a4
#define EMC_MRW7                        0x4a8
#define EMC_MRW8                        0x4ac
#define EMC_MRW9                        0x4b0
#define EMC_MRW10                       0x4b4
#define EMC_MRW11                       0x4b8
#define EMC_MRW12                       0x4bc
#define EMC_MRW13                       0x4c0
#define EMC_MRW14                       0x4c4
#define EMC_MRW15                       0x4d0
#define EMC_CFG_SYNC                        0x4d4
#define EMC_CLKEN_OVERRIDE                  0x140
#define EMC_R2R                         0x144
#define EMC_W2W                         0x148
#define EMC_EINPUT                      0x14c
#define EMC_EINPUT_DURATION                     0x150
#define EMC_PUTERM_EXTRA                    0x154
#define EMC_TCKESR                      0x158
#define EMC_TPD                         0x15c
#define EMC_STAT_CONTROL                    0x160
#define EMC_STAT_STATUS                     0x164
#define EMC_STAT_DRAM_CLOCK_LIMIT_LO                0x19c
#define EMC_STAT_DRAM_CLOCK_LIMIT_HI                0x1a0
#define EMC_STAT_DRAM_CLOCKS_LO                 0x1a4
#define EMC_STAT_DRAM_CLOCKS_HI                 0x1a8
#define EMC_STAT_DRAM_DEV0_ACTIVATE_CNT_LO          0x1ac
#define EMC_STAT_DRAM_DEV0_ACTIVATE_CNT_HI          0x1b0
#define EMC_STAT_DRAM_DEV0_READ_CNT_LO              0x1b4
#define EMC_STAT_DRAM_DEV0_READ_CNT_HI              0x1b8
#define EMC_STAT_DRAM_DEV0_READ8_CNT_LO             0x1bc
#define EMC_STAT_DRAM_DEV0_READ8_CNT_HI             0x1c0
#define EMC_STAT_DRAM_DEV0_WRITE_CNT_LO             0x1c4
#define EMC_STAT_DRAM_DEV0_WRITE_CNT_HI             0x1c8
#define EMC_STAT_DRAM_DEV0_WRITE8_CNT_LO            0x1cc
#define EMC_STAT_DRAM_DEV0_WRITE8_CNT_HI            0x1d0
#define EMC_STAT_DRAM_DEV0_REF_CNT_LO               0x1d4
#define EMC_STAT_DRAM_DEV0_REF_CNT_HI               0x1d8
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO  0x1dc
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI  0x1e0
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO  0x1e4
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI  0x1e8
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO  0x1ec
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI  0x1f0
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO  0x1f4
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI  0x1f8
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO    0x1fc
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI    0x200
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO    0x204
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI    0x208
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO    0x20c
#define EMC_STAT_DRAM_DEV0_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI    0x210
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO    0x214
#define EMC_STAT_DRAM_DEV0_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI    0x218
#define EMC_STAT_DRAM_DEV0_SR_CKE_EQ0_CLKS_LO               0x21c
#define EMC_STAT_DRAM_DEV0_SR_CKE_EQ0_CLKS_HI               0x220
#define EMC_STAT_DRAM_DEV0_DSR                      0x224
#define EMC_STAT_DRAM_DEV1_ACTIVATE_CNT_LO              0x228
#define EMC_STAT_DRAM_DEV1_ACTIVATE_CNT_HI              0x22c
#define EMC_STAT_DRAM_DEV1_READ_CNT_LO                  0x230
#define EMC_STAT_DRAM_DEV1_READ_CNT_HI                  0x234
#define EMC_STAT_DRAM_DEV1_READ8_CNT_LO                 0x238
#define EMC_STAT_DRAM_DEV1_READ8_CNT_HI                 0x23c
#define EMC_STAT_DRAM_DEV1_WRITE_CNT_LO                 0x240
#define EMC_STAT_DRAM_DEV1_WRITE_CNT_HI                 0x244
#define EMC_STAT_DRAM_DEV1_WRITE8_CNT_LO                0x248
#define EMC_STAT_DRAM_DEV1_WRITE8_CNT_HI                0x24c
#define EMC_STAT_DRAM_DEV1_REF_CNT_LO                   0x250
#define EMC_STAT_DRAM_DEV1_REF_CNT_HI                   0x254
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO  0x258
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI  0x25c
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO  0x260
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI  0x264
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO  0x268
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI  0x26c
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO  0x270
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI  0x274
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO    0x278
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI    0x27c
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO    0x280
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI    0x284
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO    0x288
#define EMC_STAT_DRAM_DEV1_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI    0x28c
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO    0x290
#define EMC_STAT_DRAM_DEV1_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI    0x294
#define EMC_STAT_DRAM_DEV1_SR_CKE_EQ0_CLKS_LO               0x298
#define EMC_STAT_DRAM_DEV1_SR_CKE_EQ0_CLKS_HI               0x29c
#define EMC_STAT_DRAM_DEV1_DSR                      0x2a0
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO    0xc8c
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI    0xc90
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_LO    0xc94
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ0_NO_BANKS_ACTIVE_CLKS_HI    0xc98
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO    0xc9c
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI    0xca0
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_LO    0xca4
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ1_NO_BANKS_ACTIVE_CLKS_HI    0xca8
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO  0xcac
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI  0xcb0
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_LO  0xcb4
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ0_SOME_BANKS_ACTIVE_CLKS_HI  0xcb8
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO  0xcbc
#define EMC_STAT_DRAM_IO_EXTCLKS_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI  0xcc0
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_LO  0xcc4
#define EMC_STAT_DRAM_IO_CLKSTOP_CKE_EQ1_SOME_BANKS_ACTIVE_CLKS_HI  0xcc8
#define EMC_STAT_DRAM_IO_SR_CKE_EQ0_CLKS_LO         0xccc
#define EMC_STAT_DRAM_IO_SR_CKE_EQ0_CLKS_HI         0xcd0
#define EMC_STAT_DRAM_IO_DSR                    0xcd4
#define EMC_AUTO_CAL_CONFIG                 0x2a4
#define EMC_AUTO_CAL_CONFIG_AUTO_CAL_COMPUTE_START      (1 << 0)
#define EMC_AUTO_CAL_CONFIG_AUTO_CAL_MEASURE_STALL      (1 << 9)
#define EMC_AUTO_CAL_CONFIG_AUTO_CAL_UPDATE_STALL       (1 << 10)
#define EMC_AUTO_CAL_CONFIG_AUTO_CAL_ENABLE         (1 << 29)
#define EMC_AUTO_CAL_CONFIG_AUTO_CAL_START          (1 << 31)

#define EMC_AUTO_CAL_CONFIG2                    0x458
#define EMC_AUTO_CAL_CONFIG3                    0x45c
#define EMC_AUTO_CAL_CONFIG4                    0x5b0
#define EMC_AUTO_CAL_CONFIG5                    0x5b4
#define EMC_AUTO_CAL_CONFIG6                    0x5cc
#define EMC_AUTO_CAL_CONFIG7                    0x574
#define EMC_AUTO_CAL_CONFIG8                    0x2dc
#define EMC_AUTO_CAL_VREF_SEL_0                 0x2f8
#define EMC_AUTO_CAL_VREF_SEL_1                 0x300
#define EMC_AUTO_CAL_INTERVAL                   0x2a8
#define EMC_AUTO_CAL_STATUS                 0x2ac
#define EMC_AUTO_CAL_STATUS2                    0x3d4
#define EMC_AUTO_CAL_CHANNEL                    0x464
#define EMC_PMACRO_RX_TERM                  0xc48
#define EMC_PMACRO_DQ_TX_DRV                    0xc70
#define EMC_PMACRO_CA_TX_DRV                    0xc74
#define EMC_PMACRO_CMD_TX_DRV                   0xc4c
#define EMC_PMACRO_AUTOCAL_CFG_0                0x700
#define EMC_PMACRO_AUTOCAL_CFG_1                0x704
#define EMC_PMACRO_AUTOCAL_CFG_2                0x708
#define EMC_PMACRO_AUTOCAL_CFG_COMMON               0xc78
#define EMC_PMACRO_AUTOCAL_CFG_COMMON_E_CAL_BYPASS_DVFS     (1 << 16)

#define EMC_PMACRO_ZCTRL                    0xc44
#define EMC_XM2COMPPADCTRL                  0x30c
#define EMC_XM2COMPPADCTRL_VREF_CAL_ENABLE          (1 << 10)

#define EMC_XM2COMPPADCTRL2                 0x578
#define EMC_XM2COMPPADCTRL3                 0x2f4
#define EMC_COMP_PAD_SW_CTRL                    0x57c
#define EMC_REQ_CTRL                        0x2b0
#define EMC_EMC_STATUS                      0x2b4
#define EMC_EMC_STATUS_MRR_DIVLD                (1 << 20)
#define EMC_EMC_STATUS_TIMING_UPDATE_STALLED            (1 << 23)
#define EMC_EMC_STATUS_DRAM_IN_POWERDOWN_SHIFT          4
#define EMC_EMC_STATUS_DRAM_IN_POWERDOWN_MASK           \
    (0x3 << EMC_EMC_STATUS_DRAM_IN_POWERDOWN_SHIFT)
#define EMC_EMC_STATUS_DRAM_IN_SELF_REFRESH_SHIFT       8
#define EMC_EMC_STATUS_DRAM_IN_SELF_REFRESH_MASK        \
    (0x3 << EMC_EMC_STATUS_DRAM_IN_SELF_REFRESH_SHIFT)

#define EMC_CFG_2                       0x2b8
#define EMC_CFG_DIG_DLL                     0x2bc
#define EMC_CFG_DIG_DLL_CFG_DLL_EN              (1 << 0)
#define EMC_CFG_DIG_DLL_CFG_DLL_STALL_ALL_UNTIL_LOCK        (1 << 1)
#define EMC_CFG_DIG_DLL_CFG_DLL_STALL_ALL_TRAFFIC       (1 << 3)
#define EMC_CFG_DIG_DLL_CFG_DLL_STALL_RW_UNTIL_LOCK     (1 << 4)
#define EMC_CFG_DIG_DLL_CFG_DLL_MODE_SHIFT          6
#define EMC_CFG_DIG_DLL_CFG_DLL_MODE_MASK           \
    (0x3 << EMC_CFG_DIG_DLL_CFG_DLL_MODE_SHIFT)
#define EMC_CFG_DIG_DLL_CFG_DLL_LOCK_LIMIT_SHIFT        8
#define EMC_CFG_DIG_DLL_CFG_DLL_LOCK_LIMIT_MASK         \
    (0x7 << EMC_CFG_DIG_DLL_CFG_DLL_LOCK_LIMIT_SHIFT)

#define EMC_CFG_DIG_DLL_PERIOD                  0x2c0
#define EMC_DIG_DLL_STATUS                  0x2c4
#define EMC_DIG_DLL_STATUS_DLL_LOCK             (1 << 15)
#define EMC_DIG_DLL_STATUS_DLL_PRIV_UPDATED         (1 << 17)
#define EMC_DIG_DLL_STATUS_DLL_OUT_SHIFT            0
#define EMC_DIG_DLL_STATUS_DLL_OUT_MASK             \
    (0x7ff << EMC_DIG_DLL_STATUS_DLL_OUT_SHIFT)

#define EMC_CFG_DIG_DLL_1                   0x2c8
#define EMC_RDV_MASK                        0x2cc
#define EMC_WDV_MASK                        0x2d0
#define EMC_RDV_EARLY_MASK                  0x2d4
#define EMC_RDV_EARLY                       0x2d8
#define EMC_WDV_CHK                     0x4e0
#define EMC_ZCAL_INTERVAL                   0x2e0
#define EMC_ZCAL_WAIT_CNT                   0x2e4
#define EMC_ZCAL_WAIT_CNT_ZCAL_WAIT_CNT_MASK            0x7ff
#define EMC_ZCAL_WAIT_CNT_ZCAL_WAIT_CNT_SHIFT           0

#define EMC_ZCAL_MRW_CMD                    0x2e8
#define EMC_ZQ_CAL                      0x2ec
#define EMC_ZQ_CAL_DEV_SEL_SHIFT                30
#define EMC_ZQ_CAL_DEV_SEL_MASK                 \
    (0x3 << EMC_SELF_REF_DEV_SEL_SHIFT)
#define EMC_ZQ_CAL_LONG                     (1 << 4)
#define EMC_ZQ_CAL_ZQ_LATCH_CMD                 (1 << 1)
#define EMC_ZQ_CAL_ZQ_CAL_CMD                   (1 << 0)
#define EMC_ZQ_CAL_LONG_CMD_DEV0                \
    (DRAM_DEV_SEL_0 | EMC_ZQ_CAL_LONG | EMC_ZQ_CAL_CMD)
#define EMC_ZQ_CAL_LONG_CMD_DEV1                \
    (DRAM_DEV_SEL_1 | EMC_ZQ_CAL_LONG | EMC_ZQ_CAL_CMD)

#define EMC_SCRATCH0                        0x324
#define EMC_STALL_THEN_EXE_BEFORE_CLKCHANGE         0x3c8
#define EMC_STALL_THEN_EXE_AFTER_CLKCHANGE          0x3cc
#define EMC_UNSTALL_RW_AFTER_CLKCHANGE              0x3d0
#define EMC_FDPD_CTRL_CMD_NO_RAMP               0x4d8
#define EMC_FDPD_CTRL_CMD_NO_RAMP_CMD_DPD_NO_RAMP_ENABLE    (1 << 0)

#define EMC_SEL_DPD_CTRL                    0x3d8
#define EMC_SEL_DPD_CTRL_DATA_SEL_DPD_EN            (1 << 8)
#define EMC_SEL_DPD_CTRL_ODT_SEL_DPD_EN             (1 << 5)
#define EMC_SEL_DPD_CTRL_RESET_SEL_DPD_EN           (1 << 4)
#define EMC_SEL_DPD_CTRL_CA_SEL_DPD_EN              (1 << 3)
#define EMC_SEL_DPD_CTRL_CLK_SEL_DPD_EN             (1 << 2)
#define EMC_SEL_DPD_CTRL_DDR3_MASK              \
    ((0xf << 2) | (0x1 << 8))
#define EMC_SEL_DPD_CTRL_MAS                    \
    ((0x3 << 2) | (0x1 << 5) | (0x1 << 8))

#define EMC_FDPD_CTRL_DQ                    0x310
#define EMC_FDPD_CTRL_CMD                   0x314
#define EMC_PRE_REFRESH_REQ_CNT                 0x3dc
#define EMC_REFCTRL2                        0x580
#define EMC_FBIO_CFG7                       0x584
#define EMC_FBIO_CFG7_CH0_ENABLE                (1 << 1)
#define EMC_FBIO_CFG7_CH1_ENABLE                (1 << 2)

#define EMC_DATA_BRLSHFT_0                  0x588
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE7_DATA_BRLSHFT_SHIFT   21
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE7_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_0_RANK0_BYTE7_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE6_DATA_BRLSHFT_SHIFT   18
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE6_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_0_RANK0_BYTE6_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE5_DATA_BRLSHFT_SHIFT   15
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE5_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_0_RANK0_BYTE5_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE4_DATA_BRLSHFT_SHIFT   12
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE4_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_0_RANK0_BYTE4_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE3_DATA_BRLSHFT_SHIFT   9
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE3_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_0_RANK0_BYTE3_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE2_DATA_BRLSHFT_SHIFT   6
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE2_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_0_RANK0_BYTE2_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE1_DATA_BRLSHFT_SHIFT   3
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE1_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_0_RANK0_BYTE1_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE0_DATA_BRLSHFT_SHIFT   0
#define EMC_DATA_BRLSHFT_0_RANK0_BYTE0_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_0_RANK0_BYTE0_DATA_BRLSHFT_SHIFT)

#define EMC_DATA_BRLSHFT_1                  0x58c
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE7_DATA_BRLSHFT_SHIFT   21
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE7_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_1_RANK1_BYTE7_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE6_DATA_BRLSHFT_SHIFT   18
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE6_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_1_RANK1_BYTE6_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE5_DATA_BRLSHFT_SHIFT   15
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE5_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_1_RANK1_BYTE5_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE4_DATA_BRLSHFT_SHIFT   12
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE4_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_1_RANK1_BYTE4_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE3_DATA_BRLSHFT_SHIFT   9
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE3_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_1_RANK1_BYTE3_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE2_DATA_BRLSHFT_SHIFT   6
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE2_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_1_RANK1_BYTE2_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE1_DATA_BRLSHFT_SHIFT   3
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE1_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_1_RANK1_BYTE1_DATA_BRLSHFT_SHIFT)
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE0_DATA_BRLSHFT_SHIFT   0
#define EMC_DATA_BRLSHFT_1_RANK1_BYTE0_DATA_BRLSHFT_MASK    \
    (0x7 << EMC_DATA_BRLSHFT_1_RANK1_BYTE0_DATA_BRLSHFT_SHIFT)

#define EMC_DQS_BRLSHFT_0                   0x594
#define EMC_DQS_BRLSHFT_1                   0x598
#define EMC_CMD_BRLSHFT_0                   0x59c
#define EMC_CMD_BRLSHFT_1                   0x5a0
#define EMC_CMD_BRLSHFT_2                   0x5a4
#define EMC_CMD_BRLSHFT_3                   0x5a8
#define EMC_QUSE_BRLSHFT_0                  0x5ac
#define EMC_QUSE_BRLSHFT_1                  0x5b8
#define EMC_QUSE_BRLSHFT_2                  0x5bc
#define EMC_QUSE_BRLSHFT_3                  0x5c4
#define EMC_FBIO_CFG8                       0x5c8
#define EMC_CMD_MAPPING_CMD0_0                  0x380
#define EMC_CMD_MAPPING_CMD0_1                  0x384
#define EMC_CMD_MAPPING_CMD0_2                  0x388
#define EMC_CMD_MAPPING_CMD1_0                  0x38c
#define EMC_CMD_MAPPING_CMD1_1                  0x390
#define EMC_CMD_MAPPING_CMD1_2                  0x394
#define EMC_CMD_MAPPING_CMD2_0                  0x398
#define EMC_CMD_MAPPING_CMD2_1                  0x39c
#define EMC_CMD_MAPPING_CMD2_2                  0x3a0
#define EMC_CMD_MAPPING_CMD3_0                  0x3a4
#define EMC_CMD_MAPPING_CMD3_1                  0x3a8
#define EMC_CMD_MAPPING_CMD3_2                  0x3ac
#define EMC_CMD_MAPPING_BYTE                    0x3b0
#define EMC_DYN_SELF_REF_CONTROL                0x3e0
#define EMC_TXSRDLL                     0x3e4
#define EMC_CCFIFO_ADDR                     0x3e8
#define EMC_CCFIFO_DATA                     0x3ec
#define EMC_CCFIFO_STATUS                   0x3f0
#define EMC_SWIZZLE_RANK0_BYTE0                 0x404
#define EMC_SWIZZLE_RANK0_BYTE1                 0x408
#define EMC_SWIZZLE_RANK0_BYTE2                 0x40c
#define EMC_SWIZZLE_RANK0_BYTE3                 0x410
#define EMC_SWIZZLE_RANK1_BYTE0                 0x418
#define EMC_SWIZZLE_RANK1_BYTE1                 0x41c
#define EMC_SWIZZLE_RANK1_BYTE2                 0x420
#define EMC_SWIZZLE_RANK1_BYTE3                 0x424
#define EMC_TR_TIMING_0                     0x3b4
#define EMC_TR_CTRL_0                       0x3b8
#define EMC_TR_CTRL_1                       0x3bc
#define EMC_TR_DVFS                     0x460
#define EMC_TR_DVFS_TRAINING_DVFS               (1 << 0)

#define EMC_SWITCH_BACK_CTRL                    0x3c0
#define EMC_TR_RDV                      0x3c4
#define EMC_TR_QPOP                     0x3f4
#define EMC_TR_RDV_MASK                     0x3f8
#define EMC_TR_QSAFE                        0x3fc
#define EMC_TR_QRST                     0x400
#define EMC_IBDLY                       0x468
#define EMC_OBDLY                       0x46c
#define EMC_TXDSRVTTGEN                     0x480
#define EMC_WE_DURATION                     0x48c
#define EMC_WS_DURATION                     0x490
#define EMC_WEV                         0x494
#define EMC_WSV                         0x498
#define EMC_CFG_3                       0x49c
#define EMC_CFG_PIPE_2                      0x554
#define EMC_CFG_PIPE_CLK                    0x558
#define EMC_CFG_PIPE_CLK_CLK_ALWAYS_ON              (1 << 0)

#define EMC_CFG_PIPE_1                      0x55c
#define EMC_CFG_PIPE                        0x560
#define EMC_QPOP                        0x564
#define EMC_QUSE_WIDTH                      0x568
#define EMC_PUTERM_WIDTH                    0x56c
#define EMC_PROTOBIST_CONFIG_ADR_1              0x5d0
#define EMC_PROTOBIST_CONFIG_ADR_2              0x5d4
#define EMC_PROTOBIST_MISC                  0x5d8
#define EMC_PROTOBIST_WDATA_LOWER               0x5dc
#define EMC_PROTOBIST_WDATA_UPPER               0x5e0
#define EMC_PROTOBIST_RDATA                 0x5ec
#define EMC_DLL_CFG_0                       0x5e4
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_IGNORE_START         (1 << 29)
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_DUAL_PASS_LOCK       (1 << 28)
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_STEP_SIZE_SHIFT      24
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_STEP_SIZE_MASK       \
    (0xf << EMC_DLL_CFG_0_DDLLCAL_CTRL_STEP_SIZE_SHIFT)
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_END_COUNT_SHIFT      20
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_END_COUNT_MASK       \
    (0xf << EMC_DLL_CFG_0_DDLLCAL_CTRL_END_COUNT_SHIFT)
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_FILTER_BITS_SHIFT        16
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_FILTER_BITS_MASK     \
    (0xf << EMC_DLL_CFG_0_DDLLCAL_CTRL_FILTER_BITS_SHIFT)
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_SAMPLE_COUNT_SHIFT       12
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_SAMPLE_COUNT_MASK        \
    (0xf << EMC_DLL_CFG_0_DDLLCAL_CTRL_SAMPLE_COUNT_SHIFT)
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_SAMPLE_DELAY_SHIFT       4
#define EMC_DLL_CFG_0_DDLLCAL_CTRL_SAMPLE_DELAY_MASK        \
    (0xff << EMC_DLL_CFG_0_DDLLCAL_CTRL_SAMPLE_DELAY_SHIFT)
#define EMC_DLL_CFG_0_DDLLCAL_UPDATE_CNT_LIMIT_SHIFT        0
#define EMC_DLL_CFG_0_DDLLCAL_UPDATE_CNT_LIMIT_MASK     \
    (0xf << EMC_DLL_CFG_0_DDLLCAL_UPDATE_CNT_LIMIT_SHIFT)

#define EMC_DLL_CFG_1                       0x5e8
#define EMC_DLL_CFG_1_DDLLCAL_CTRL_START_TRIM_SHIFT     10
#define EMC_DLL_CFG_1_DDLLCAL_CTRL_START_TRIM_MASK      \
    (0x7ff << EMC_DLL_CFG_1_DDLLCAL_CTRL_START_TRIM_SHIFT)

#define EMC_TRAINING_CMD                    0xe00
#define EMC_TRAINING_CMD_PRIME                  (1 << 0)
#define EMC_TRAINING_CMD_CA                 (1 << 1)
#define EMC_TRAINING_CMD_RD                 (1 << 2)
#define EMC_TRAINING_CMD_WR                 (1 << 3)
#define EMC_TRAINING_CMD_QUSE                   (1 << 4)
#define EMC_TRAINING_CMD_CA_VREF                (1 << 5)
#define EMC_TRAINING_CMD_RD_VREF                (1 << 6)
#define EMC_TRAINING_CMD_WR_VREF                (1 << 7)
#define EMC_TRAINING_CMD_QUSE_VREF              (1 << 8)
#define EMC_TRAINING_CMD_GO                 (1 << 31)

#define EMC_TRAINING_CTRL                   0xe04
#define EMC_TRAINING_CTRL_SWAP_RANK             (1 << 14)

#define EMC_TRAINING_STATUS                 0xe08
#define EMC_TRAINING_QUSE_CORS_CTRL             0xe0c
#define EMC_TRAINING_QUSE_FINE_CTRL             0xe10
#define EMC_TRAINING_QUSE_CTRL_MISC             0xe14
#define EMC_TRAINING_WRITE_FINE_CTRL                0xe18
#define EMC_TRAINING_WRITE_CTRL_MISC                0xe1c
#define EMC_TRAINING_WRITE_VREF_CTRL                0xe20
#define EMC_TRAINING_READ_FINE_CTRL             0xe24
#define EMC_TRAINING_READ_CTRL_MISC             0xe28
#define EMC_TRAINING_READ_VREF_CTRL             0xe2c
#define EMC_TRAINING_CA_FINE_CTRL               0xe30
#define EMC_TRAINING_CA_CTRL_MISC               0xe34
#define EMC_TRAINING_CA_CTRL_MISC1              0xe38
#define EMC_TRAINING_CA_VREF_CTRL               0xe3c
#define EMC_TRAINING_CA_TADR_CTRL               0xe40
#define EMC_TRAINING_SETTLE                 0xe44
#define EMC_TRAINING_DEBUG_CTRL                 0xe48
#define EMC_TRAINING_DEBUG_DQ0                  0xe4c
#define EMC_TRAINING_DEBUG_DQ1                  0xe50
#define EMC_TRAINING_DEBUG_DQ2                  0xe54
#define EMC_TRAINING_DEBUG_DQ3                  0xe58
#define EMC_TRAINING_MPC                    0xe5c
#define EMC_TRAINING_PATRAM_CTRL                0xe60
#define EMC_TRAINING_PATRAM_DQ                  0xe64
#define EMC_TRAINING_PATRAM_DMI                 0xe68
#define EMC_TRAINING_VREF_SETTLE                0xe6c
#define EMC_TRAINING_RW_EYE_CENTER_IB_BYTE0         0xe70
#define EMC_TRAINING_RW_EYE_CENTER_IB_BYTE1         0xe74
#define EMC_TRAINING_RW_EYE_CENTER_IB_BYTE2         0xe78
#define EMC_TRAINING_RW_EYE_CENTER_IB_BYTE3         0xe7c
#define EMC_TRAINING_RW_EYE_CENTER_IB_MISC          0xe80
#define EMC_TRAINING_RW_EYE_CENTER_OB_BYTE0         0xe84
#define EMC_TRAINING_RW_EYE_CENTER_OB_BYTE1         0xe88
#define EMC_TRAINING_RW_EYE_CENTER_OB_BYTE2         0xe8c
#define EMC_TRAINING_RW_EYE_CENTER_OB_BYTE3         0xe90
#define EMC_TRAINING_RW_EYE_CENTER_OB_MISC          0xe94
#define EMC_TRAINING_RW_OFFSET_IB_BYTE0             0xe98
#define EMC_TRAINING_RW_OFFSET_IB_BYTE1             0xe9c
#define EMC_TRAINING_RW_OFFSET_IB_BYTE2             0xea0
#define EMC_TRAINING_RW_OFFSET_IB_BYTE3             0xea4
#define EMC_TRAINING_RW_OFFSET_IB_MISC              0xea8
#define EMC_TRAINING_RW_OFFSET_OB_BYTE0             0xeac
#define EMC_TRAINING_RW_OFFSET_OB_BYTE1             0xeb0
#define EMC_TRAINING_RW_OFFSET_OB_BYTE2             0xeb4
#define EMC_TRAINING_RW_OFFSET_OB_BYTE3             0xeb8
#define EMC_TRAINING_RW_OFFSET_OB_MISC              0xebc
#define EMC_TRAINING_OPT_CA_VREF                0xec0
#define EMC_TRAINING_OPT_DQ_OB_VREF             0xec4
#define EMC_TRAINING_OPT_DQ_IB_VREF_RANK0           0xec8
#define EMC_TRAINING_OPT_DQ_IB_VREF_RANK1           0xecc
#define EMC_TRAINING_QUSE_VREF_CTRL             0xed0
#define EMC_TRAINING_OPT_DQS_IB_VREF_RANK0          0xed4
#define EMC_TRAINING_OPT_DQS_IB_VREF_RANK1          0xed8
#define EMC_TRAINING_DRAMC_TIMING               0xedc
#define EMC_PMACRO_QUSE_DDLL_RANK0_0                0x600
#define EMC_PMACRO_QUSE_DDLL_RANK0_1                0x604
#define EMC_PMACRO_QUSE_DDLL_RANK0_2                0x608
#define EMC_PMACRO_QUSE_DDLL_RANK0_3                0x60c
#define EMC_PMACRO_QUSE_DDLL_RANK0_4                0x610
#define EMC_PMACRO_QUSE_DDLL_RANK0_5                0x614
#define EMC_PMACRO_QUSE_DDLL_RANK1_0                0x620
#define EMC_PMACRO_QUSE_DDLL_RANK1_1                0x624
#define EMC_PMACRO_QUSE_DDLL_RANK1_2                0x628
#define EMC_PMACRO_QUSE_DDLL_RANK1_3                0x62c
#define EMC_PMACRO_QUSE_DDLL_RANK1_4                0x630
#define EMC_PMACRO_QUSE_DDLL_RANK1_5                0x634
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0          0x640
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0_OB_DDLL_LONG_DQ_RANK0_BYTE1_SHIFT \
    16
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0_OB_DDLL_LONG_DQ_RANK0_BYTE1_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0_OB_DDLL_LONG_DQ_RANK0_BYTE1_SHIFT
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0_OB_DDLL_LONG_DQ_RANK0_BYTE0_SHIFT \
    0
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0_OB_DDLL_LONG_DQ_RANK0_BYTE0_MASK \
    0x3ff <<                                \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_0_OB_DDLL_LONG_DQ_RANK0_BYTE0_SHIFT

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1          0x644
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1_OB_DDLL_LONG_DQ_RANK0_BYTE3_SHIFT \
    16
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1_OB_DDLL_LONG_DQ_RANK0_BYTE3_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1_OB_DDLL_LONG_DQ_RANK0_BYTE3_SHIFT
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1_OB_DDLL_LONG_DQ_RANK0_BYTE2_SHIFT \
    0
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1_OB_DDLL_LONG_DQ_RANK0_BYTE2_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_1_OB_DDLL_LONG_DQ_RANK0_BYTE2_SHIFT

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2          0x648
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2_OB_DDLL_LONG_DQ_RANK0_BYTE5_SHIFT  \
    16
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2_OB_DDLL_LONG_DQ_RANK0_BYTE5_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2_OB_DDLL_LONG_DQ_RANK0_BYTE5_SHIFT
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2_OB_DDLL_LONG_DQ_RANK0_BYTE4_SHIFT \
    0
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2_OB_DDLL_LONG_DQ_RANK0_BYTE4_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_2_OB_DDLL_LONG_DQ_RANK0_BYTE4_SHIFT

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3          0x64c
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3_OB_DDLL_LONG_DQ_RANK0_BYTE7_SHIFT \
    16
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3_OB_DDLL_LONG_DQ_RANK0_BYTE7_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3_OB_DDLL_LONG_DQ_RANK0_BYTE7_SHIFT
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3_OB_DDLL_LONG_DQ_RANK0_BYTE6_SHIFT \
    0
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3_OB_DDLL_LONG_DQ_RANK0_BYTE6_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_3_OB_DDLL_LONG_DQ_RANK0_BYTE6_SHIFT

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_4          0x650
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK0_5          0x654

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0          0x660
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0_OB_DDLL_LONG_DQ_RANK1_BYTE1_SHIFT \
    16
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0_OB_DDLL_LONG_DQ_RANK1_BYTE1_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0_OB_DDLL_LONG_DQ_RANK1_BYTE1_SHIFT
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0_OB_DDLL_LONG_DQ_RANK1_BYTE0_SHIFT \
    0
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0_OB_DDLL_LONG_DQ_RANK1_BYTE0_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_0_OB_DDLL_LONG_DQ_RANK1_BYTE0_SHIFT

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1          0x664
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1_OB_DDLL_LONG_DQ_RANK1_BYTE3_SHIFT \
    16
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1_OB_DDLL_LONG_DQ_RANK1_BYTE3_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1_OB_DDLL_LONG_DQ_RANK1_BYTE3_SHIFT
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1_OB_DDLL_LONG_DQ_RANK1_BYTE2_SHIFT \
    0
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1_OB_DDLL_LONG_DQ_RANK1_BYTE2_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_1_OB_DDLL_LONG_DQ_RANK1_BYTE2_SHIFT

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2          0x668
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2_OB_DDLL_LONG_DQ_RANK1_BYTE5_SHIFT \
    16
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2_OB_DDLL_LONG_DQ_RANK1_BYTE5_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2_OB_DDLL_LONG_DQ_RANK1_BYTE5_SHIFT
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2_OB_DDLL_LONG_DQ_RANK1_BYTE4_SHIFT \
    0
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2_OB_DDLL_LONG_DQ_RANK1_BYTE4_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_2_OB_DDLL_LONG_DQ_RANK1_BYTE4_SHIFT

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3          0x66c
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3_OB_DDLL_LONG_DQ_RANK1_BYTE7_SHIFT \
    16
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3_OB_DDLL_LONG_DQ_RANK1_BYTE7_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3_OB_DDLL_LONG_DQ_RANK1_BYTE7_SHIFT
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3_OB_DDLL_LONG_DQ_RANK1_BYTE6_SHIFT \
    0
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3_OB_DDLL_LONG_DQ_RANK1_BYTE6_MASK  \
    0x3ff <<                                 \
    EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_3_OB_DDLL_LONG_DQ_RANK1_BYTE6_SHIFT

#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_4          0x670
#define EMC_PMACRO_OB_DDLL_LONG_DQ_RANK1_5          0x674
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_0         0x680
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_1         0x684
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_2         0x688
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_3         0x68c
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_4         0x690
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK0_5         0x694
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_0         0x6a0
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_1         0x6a4
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_2         0x6a8
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_3         0x6ac
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_4         0x6b0
#define EMC_PMACRO_OB_DDLL_LONG_DQS_RANK1_5         0x6b4
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_0         0x6c0
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_1         0x6c4
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_2         0x6c8
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_3         0x6cc
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_4         0x6d0
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK0_5         0x6d4
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_0         0x6e0
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_1         0x6e4
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_2         0x6e8
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_3         0x6ec
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_4         0x6f0
#define EMC_PMACRO_IB_DDLL_LONG_DQS_RANK1_5         0x6f4
#define EMC_PMACRO_TX_PWRD_0                    0x720
#define EMC_PMACRO_TX_PWRD_1                    0x724
#define EMC_PMACRO_TX_PWRD_2                    0x728
#define EMC_PMACRO_TX_PWRD_3                    0x72c
#define EMC_PMACRO_TX_PWRD_4                    0x730
#define EMC_PMACRO_TX_PWRD_5                    0x734
#define EMC_PMACRO_TX_SEL_CLK_SRC_0             0x740
#define EMC_PMACRO_TX_SEL_CLK_SRC_1             0x744
#define EMC_PMACRO_TX_SEL_CLK_SRC_3             0x74c
#define EMC_PMACRO_TX_SEL_CLK_SRC_2             0x748
#define EMC_PMACRO_TX_SEL_CLK_SRC_4             0x750
#define EMC_PMACRO_TX_SEL_CLK_SRC_5             0x754
#define EMC_PMACRO_DDLL_BYPASS                  0x760
#define EMC_PMACRO_DDLL_PWRD_0                  0x770
#define EMC_PMACRO_DDLL_PWRD_1                  0x774
#define EMC_PMACRO_DDLL_PWRD_2                  0x778
#define EMC_PMACRO_CMD_CTRL_0                   0x780
#define EMC_PMACRO_CMD_CTRL_1                   0x784
#define EMC_PMACRO_CMD_CTRL_2                   0x788
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_0       0x800
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_1       0x804
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_2       0x808
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE0_3       0x80c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_0       0x810
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_1       0x814
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_2       0x818
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE1_3       0x81c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_0       0x820
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_1       0x824
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_2       0x828
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE2_3       0x82c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_0       0x830
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_1       0x834
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_2       0x838
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE3_3       0x83c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_0       0x840
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_1       0x844
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_2       0x848
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE4_3       0x84c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_0       0x850
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_1       0x854
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_2       0x858
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE5_3       0x85c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_0       0x860
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_1       0x864
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_2       0x868
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE6_3       0x86c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_0       0x870
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_1       0x874
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_2       0x878
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_BYTE7_3       0x87c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_0        0x880
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_1        0x884
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_2        0x888
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD0_3        0x88c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_0        0x890
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_1        0x894
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_2        0x898
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD1_3        0x89c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_0        0x8a0
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_1        0x8a4
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_2        0x8a8
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD2_3        0x8ac
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_0        0x8b0
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_1        0x8b4
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_2        0x8b8
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK0_CMD3_3        0x8bc
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_0       0x900
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_1       0x904
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_2       0x908
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE0_3       0x90c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_0       0x910
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_1       0x914
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_2       0x918
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE1_3       0x91c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_0       0x920
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_1       0x924
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_2       0x928
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE2_3       0x92c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_0       0x930
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_1       0x934
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_2       0x938
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE3_3       0x93c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_0       0x940
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_1       0x944
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_2       0x948
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE4_3       0x94c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_0       0x950
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_1       0x954
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_2       0x958
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE5_3       0x95c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_0       0x960
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_1       0x964
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_2       0x968
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE6_3       0x96c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_0       0x970
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_1       0x974
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_2       0x978
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_BYTE7_3       0x97c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_0        0x980
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_1        0x984
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_2        0x988
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD0_3        0x98c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_0        0x990
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_1        0x994
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_2        0x998
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD1_3        0x99c
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_0        0x9a0
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_1        0x9a4
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_2        0x9a8
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD2_3        0x9ac
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_0        0x9b0
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_1        0x9b4
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_2        0x9b8
#define EMC_PMACRO_OB_DDLL_SHORT_DQ_RANK1_CMD3_3        0x9bc
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_0       0xa00
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_1       0xa04
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE0_2       0xa08
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_0       0xa10
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_1       0xa14
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE1_2       0xa18
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_0       0xa20
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_1       0xa24
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE2_2       0xa28
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_0       0xa30
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_1       0xa34
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE3_2       0xa38
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_0       0xa40
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_1       0xa44
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE4_2       0xa48
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_0       0xa50
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_1       0xa54
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE5_2       0xa58
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_0       0xa60
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_1       0xa64
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE6_2       0xa68
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_0       0xa70
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_1       0xa74
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_BYTE7_2       0xa78
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD0_0        0xa80
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD0_1        0xa84
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD0_2        0xa88
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD1_0        0xa90
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD1_1        0xa94
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD1_2        0xa98
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD2_0        0xaa0
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD2_1        0xaa4
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD2_2        0xaa8
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD3_0        0xab0
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD3_1        0xab4
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK0_CMD3_2        0xab8
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_0       0xb00
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_1       0xb04
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE0_2       0xb08
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_0       0xb10
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_1       0xb14
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE1_2       0xb18
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_0       0xb20
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_1       0xb24
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE2_2       0xb28
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_0       0xb30
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_1       0xb34
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE3_2       0xb38
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_0       0xb40
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_1       0xb44
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE4_2       0xb48
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_0       0xb50
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_1       0xb54
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE5_2       0xb58
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_0       0xb60
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_1       0xb64
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE6_2       0xb68
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_0       0xb70
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_1       0xb74
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_BYTE7_2       0xb78
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD0_0        0xb80
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD0_1        0xb84
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD0_2        0xb88
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD1_0        0xb90
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD1_1        0xb94
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD1_2        0xb98
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD2_0        0xba0
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD2_1        0xba4
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD2_2        0xba8
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD3_0        0xbb0
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD3_1        0xbb4
#define EMC_PMACRO_IB_DDLL_SHORT_DQ_RANK1_CMD3_2        0xbb8
#define EMC_PMACRO_IB_VREF_DQ_0                 0xbe0
#define EMC_PMACRO_IB_VREF_DQ_1                 0xbe4
#define EMC_PMACRO_IB_VREF_DQ_2                 0xbe8
#define EMC_PMACRO_IB_VREF_DQS_0                0xbf0
#define EMC_PMACRO_IB_VREF_DQS_1                0xbf4
#define EMC_PMACRO_IB_VREF_DQS_2                0xbf8
#define EMC_PMACRO_IB_RXRT                  0xcf4
#define EMC_PMACRO_DDLL_LONG_CMD_0              0xc00
#define EMC_PMACRO_DDLL_LONG_CMD_1              0xc04
#define EMC_PMACRO_DDLL_LONG_CMD_2              0xc08
#define EMC_PMACRO_DDLL_LONG_CMD_3              0xc0c
#define EMC_PMACRO_DDLL_LONG_CMD_4              0xc10
#define EMC_PMACRO_DDLL_LONG_CMD_5              0xc14
#define EMC_PMACRO_DDLL_SHORT_CMD_0             0xc20
#define EMC_PMACRO_DDLL_SHORT_CMD_1             0xc24
#define EMC_PMACRO_DDLL_SHORT_CMD_2             0xc28
#define EMC_PMACRO_CFG_PM_GLOBAL_0              0xc30
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE0        (1 << 16)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE1        (1 << 17)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE2        (1 << 18)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE3        (1 << 19)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE4        (1 << 20)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE5        (1 << 21)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE6        (1 << 22)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_BYTE7        (1 << 23)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD0     (1 << 24)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD1     (1 << 25)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD2     (1 << 26)
#define EMC_PMACRO_CFG_PM_GLOBAL_0_DISABLE_CFG_CMD3     (1 << 27)

#define EMC_PMACRO_VTTGEN_CTRL_0                0xc34
#define EMC_PMACRO_VTTGEN_CTRL_1                0xc38
#define EMC_PMACRO_VTTGEN_CTRL_2                0xcf0
#define EMC_PMACRO_BG_BIAS_CTRL_0               0xc3c
#define EMC_PMACRO_BG_BIAS_CTRL_0_BG_E_PWRD         (1 << 0)
#define EMC_PMACRO_BG_BIAS_CTRL_0_BG_MODE           (1 << 1)
#define EMC_PMACRO_BG_BIAS_CTRL_0_BGLP_E_PWRD           (1 << 2)

#define EMC_PMACRO_PAD_CFG_CTRL                 0xc40
#define EMC_PMACRO_CMD_PAD_RX_CTRL              0xc50
#define EMC_PMACRO_DATA_PAD_RX_CTRL             0xc54
#define EMC_PMACRO_CMD_RX_TERM_MODE             0xc58
#define EMC_PMACRO_DATA_RX_TERM_MODE                0xc5c
#define EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQSN_RX_TERM_MODE_SHIFT 8
#define EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQSN_RX_TERM_MODE_MASK (0x3 << \
    EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQSN_RX_TERM_MODE_SHIFT)
#define EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQSP_RX_TERM_MODE_SHIFT 4
#define EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQSP_RX_TERM_MODE_MASK (0x3 << \
    EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQSP_RX_TERM_MODE_SHIFT)
#define EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQ_RX_TERM_MODE_SHIFT   0
#define EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQ_RX_TERM_MODE_MASK   (0x3 << \
    EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQ_RX_TERM_MODE_SHIFT)

#define RX_TERM_MODE                            \
    ~(EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQSN_RX_TERM_MODE_MASK |    \
      EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQSP_RX_TERM_MODE_MASK |    \
      EMC_PMACRO_DATA_RX_TERM_MODE_DATA_DQ_RX_TERM_MODE_MASK)

#define EMC_PMACRO_CMD_PAD_TX_CTRL              0xc60
#define EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_E_DCC      (1 << 1)
#define EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSP_TX_E_DCC        (1 << 9)
#define EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQSN_TX_E_DCC        (1 << 16)
#define EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_CMD_TX_E_DCC     (1 << 24)
#define EMC_PMACRO_CMD_PAD_TX_CTRL_CMD_DQ_TX_DRVFORCEON     (1 << 26)

#define EMC_PMACRO_DATA_PAD_TX_CTRL             0xc64
#define EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_E_IVREF     (1 << 0)
#define EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQ_TX_E_DCC        (1 << 1)
#define EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQS_E_IVREF        (1 << 8)
#define EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSP_TX_E_DCC      (1 << 9)
#define EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_DQSN_TX_E_DCC      (1 << 16)
#define EMC_PMACRO_DATA_PAD_TX_CTRL_DATA_CMD_TX_E_DCC       (1 << 24)

#define EMC_PMACRO_COMMON_PAD_TX_CTRL               0xc68
#define EMC_PMACRO_BRICK_MAPPING_0              0xc80
#define EMC_PMACRO_BRICK_MAPPING_1              0xc84
#define EMC_PMACRO_BRICK_MAPPING_2              0xc88
#define EMC_PMACRO_DDLLCAL_CAL                  0xce0
#define EMC_PMACRO_DDLL_OFFSET                  0xce4
#define EMC_PMACRO_DDLL_PERIODIC_OFFSET             0xce8
#define EMC_PMACRO_BRICK_CTRL_RFU1              0x330
#define EMC_PMACRO_BRICK_CTRL_RFU2              0x334
#define EMC_PMACRO_CMD_BRICK_CTRL_FDPD              0x318
#define EMC_PMACRO_DATA_BRICK_CTRL_FDPD             0x31c
#define EMC_PMACRO_TRAINING_CTRL_0              0xcf8
#define EMC_PMACRO_TRAINING_CTRL_0_CH0_TRAINING_E_WRPTR     (1 << 3)

#define EMC_PMACRO_TRAINING_CTRL_1              0xcfc
#define EMC_PMACRO_TRAINING_CTRL_1_CH1_TRAINING_E_WRPTR     (1 << 3)

#define EMC_PMC_SCRATCH1                    0x440
#define EMC_PMC_SCRATCH2                    0x444
#define EMC_PMC_SCRATCH3                    0x448

#define EMC_PMACRO_PERBIT_FGCG_CTRL_0                    0xd40
#define EMC_PMACRO_PERBIT_FGCG_CTRL_1                    0xd44
#define EMC_PMACRO_PERBIT_FGCG_CTRL_2                    0xd48
#define EMC_PMACRO_PERBIT_FGCG_CTRL_3                    0xd4c
#define EMC_PMACRO_PERBIT_FGCG_CTRL_4                    0xd50
#define EMC_PMACRO_PERBIT_FGCG_CTRL_5                    0xd54
#define EMC_PMACRO_PERBIT_RFU_CTRL_0                    0xd60
#define EMC_PMACRO_PERBIT_RFU_CTRL_1                    0xd64
#define EMC_PMACRO_PERBIT_RFU_CTRL_2                    0xd68
#define EMC_PMACRO_PERBIT_RFU_CTRL_3                    0xd6c
#define EMC_PMACRO_PERBIT_RFU_CTRL_4                    0xd70
#define EMC_PMACRO_PERBIT_RFU_CTRL_5                    0xd74
#define EMC_PMACRO_PERBIT_RFU1_CTRL_0                    0xd80
#define EMC_PMACRO_PERBIT_RFU1_CTRL_1                    0xd84
#define EMC_PMACRO_PERBIT_RFU1_CTRL_2                    0xd88
#define EMC_PMACRO_PERBIT_RFU1_CTRL_3                    0xd8c
#define EMC_PMACRO_PERBIT_RFU1_CTRL_4                    0xd90
#define EMC_PMACRO_PERBIT_RFU1_CTRL_5                    0xd94

#define EMC_PMACRO_PMU_OUT_EOFF1_0                    0xda0
#define EMC_PMACRO_PMU_OUT_EOFF1_1                    0xda4
#define EMC_PMACRO_PMU_OUT_EOFF1_2                    0xda8
#define EMC_PMACRO_PMU_OUT_EOFF1_3                    0xdac
#define EMC_PMACRO_PMU_OUT_EOFF1_4                    0xdb0
#define EMC_PMACRO_PMU_OUT_EOFF1_5                    0xdb4

#define EMC_PMACRO_COMP_PMU_OUT                    0xdc0
#define EMC_PMACRO_DATA_PI_CTRL                    0x110
#define EMC_PMACRO_CMD_PI_CTRL                    0x114

#define EMC_AUTO_CAL_CONFIG9                    0x42c

#define EMC_TRTM                    0xbc
#define EMC_TWTM                    0xf8
#define EMC_TRATM                    0xfc
#define EMC_TWATM                    0x108
#define EMC_TR2REF                    0x10c

#define EMC_PMACRO_DSR_VTTGEN_CTRL_0                    0xc6c

#endif
