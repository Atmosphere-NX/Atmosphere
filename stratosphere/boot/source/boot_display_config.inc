/*
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

struct RegisterWrite {
    u32 offset;
    u32 value;
};

enum DsiSleepOrRegisterWriteKind : u16 {
    DsiSleepOrRegisterWriteKind_Write = 0,
    DsiSleepOrRegisterWriteKind_Sleep = 1,
};

struct DsiSleepOrRegisterWrite {
    DsiSleepOrRegisterWriteKind kind;
    u16 offset;
    u32 value;
};

constexpr RegisterWrite DisplayConfigPlld01Erista[] = {
    {CLK_RST_CONTROLLER_CLK_SOURCE_DISP1, 0x40000000},
    {CLK_RST_CONTROLLER_PLLD_BASE,        0x4830A001},
    {CLK_RST_CONTROLLER_PLLD_MISC1,       0x00000020},
    {CLK_RST_CONTROLLER_PLLD_MISC,        0x002D0AAA},
};

constexpr RegisterWrite DisplayConfigPlld01Mariko[] = {
    {CLK_RST_CONTROLLER_CLK_SOURCE_DISP1, 0x40000000},
    {CLK_RST_CONTROLLER_PLLD_BASE,        0x4830A001},
    {CLK_RST_CONTROLLER_PLLD_MISC1,       0x00000000},
    {CLK_RST_CONTROLLER_PLLD_MISC,        0x002DFC00},
};

constexpr RegisterWrite DisplayConfigDc01[] = {
    {sizeof(u32) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(u32) * DC_CMD_REG_ACT_CONTROL, 0x54},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_DISP_DC_MCCIF_FIFOCTRL, 0},
    {sizeof(u32) * DC_DISP_DISP_MEM_HIGH_PRIORITY, 0},
    {sizeof(u32) * DC_DISP_DISP_MEM_HIGH_PRIORITY_TIMER, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_POWER_CONTROL, PW0_ENABLE | PW1_ENABLE | PW2_ENABLE | PW3_ENABLE | PW4_ENABLE | PM0_ENABLE | PM1_ENABLE},
    {sizeof(u32) * DC_CMD_GENERAL_INCR_SYNCPT_CNTRL, SYNCPT_CNTRL_NO_STALL},
    {sizeof(u32) * DC_CMD_CONT_SYNCPT_VSYNC, SYNCPT_VSYNC_ENABLE | 0x9}, // 9: SYNCPT
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
    {sizeof(u32) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_DV_CONTROL, 0},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    /* Setup default YUV colorspace conversion coefficients */
    {sizeof(u32) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(u32) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(u32) * DC_WIN_CSC_KUR,   0},
    {sizeof(u32) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(u32) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(u32) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(u32) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(u32) * DC_WIN_CSC_KVB,   0},
    /* End of color coefficients */
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_DV_CONTROL, 0},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    /* Setup default YUV colorspace conversion coefficients */
    {sizeof(u32) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(u32) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(u32) * DC_WIN_CSC_KUR,   0},
    {sizeof(u32) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(u32) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(u32) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(u32) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(u32) * DC_WIN_CSC_KVB,   0},
    /* End of color coefficients */
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_DV_CONTROL, 0},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    /* Setup default YUV colorspace conversion coefficients */
    {sizeof(u32) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(u32) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(u32) * DC_WIN_CSC_KUR,   0},
    {sizeof(u32) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(u32) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(u32) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(u32) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(u32) * DC_WIN_CSC_KVB,   0},
    /* End of color coefficients */
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE_888},
    {sizeof(u32) * DC_DISP_DISP_INTERFACE_CONTROL, DISP_DATA_FORMAT_DF1P1C},
    {sizeof(u32) * DC_COM_PIN_OUTPUT_POLARITY(1), 0x1000000},
    {sizeof(u32) * DC_COM_PIN_OUTPUT_POLARITY(3), 0},
    {sizeof(u32) * 0x4E4, 0},
    {sizeof(u32) * DC_COM_CRC_CONTROL, 0},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * 0x716, 0x10000FF},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * 0x716, 0x10000FF},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * 0x716, 0x10000FF},
    {sizeof(u32) * DC_CMD_DISPLAY_COMMAND_OPTION0, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_COMMAND, 0},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ}
};

constexpr RegisterWrite DisplayConfigDsi01Init01[] = {
    {sizeof(u32) * DSI_WR_DATA,         0x0},
    {sizeof(u32) * DSI_INT_ENABLE,      0x0},
    {sizeof(u32) * DSI_INT_STATUS,      0x0},
    {sizeof(u32) * DSI_INT_MASK,        0x0},
    {sizeof(u32) * DSI_INIT_SEQ_DATA_0, 0x0},
    {sizeof(u32) * DSI_INIT_SEQ_DATA_1, 0x0},
    {sizeof(u32) * DSI_INIT_SEQ_DATA_2, 0x0},
    {sizeof(u32) * DSI_INIT_SEQ_DATA_3, 0x0},
};

constexpr RegisterWrite DisplayConfigDsi01Init02Erista[] = {
    {sizeof(u32) * DSI_INIT_SEQ_DATA_15, 0x0},
};

constexpr RegisterWrite DisplayConfigDsi01Init02Mariko[] = {
    {sizeof(u32) * DSI_INIT_SEQ_DATA_15_MARIKO, 0x0},
};

constexpr RegisterWrite DisplayConfigDsi01Init03[] = {
    {sizeof(u32) * DSI_DCS_CMDS,     0},
    {sizeof(u32) * DSI_PKT_SEQ_0_LO, 0},
    {sizeof(u32) * DSI_PKT_SEQ_1_LO, 0},
    {sizeof(u32) * DSI_PKT_SEQ_2_LO, 0},
    {sizeof(u32) * DSI_PKT_SEQ_3_LO, 0},
    {sizeof(u32) * DSI_PKT_SEQ_4_LO, 0},
    {sizeof(u32) * DSI_PKT_SEQ_5_LO, 0},
    {sizeof(u32) * DSI_PKT_SEQ_0_HI, 0},
    {sizeof(u32) * DSI_PKT_SEQ_1_HI, 0},
    {sizeof(u32) * DSI_PKT_SEQ_2_HI, 0},
    {sizeof(u32) * DSI_PKT_SEQ_3_HI, 0},
    {sizeof(u32) * DSI_PKT_SEQ_4_HI, 0},
    {sizeof(u32) * DSI_PKT_SEQ_5_HI, 0},
};

constexpr RegisterWrite DisplayConfigDsi01Init04Erista[] = {
    /* No register writes. */
};

constexpr RegisterWrite DisplayConfigDsi01Init04Mariko[] = {
    {sizeof(u32) * DSI_PAD_CONTROL_1,        0},
    {sizeof(u32) * DSI_PAD_CONTROL_2,        0},
    {sizeof(u32) * DSI_PAD_CONTROL_3,        0},
    {sizeof(u32) * DSI_PAD_CONTROL_4,        0},
    {sizeof(u32) * DSI_PAD_CONTROL_5_MARIKO, 0},
    {sizeof(u32) * DSI_PAD_CONTROL_6_MARIKO, 0},
    {sizeof(u32) * DSI_PAD_CONTROL_7_MARIKO, 0},
};

constexpr RegisterWrite DisplayConfigDsi01Init05[] = {
    {sizeof(u32) * DSI_PAD_CONTROL_CD,   0},
    {sizeof(u32) * DSI_SOL_DELAY,        0x18},
    {sizeof(u32) * DSI_MAX_THRESHOLD,    0x1E0},
    {sizeof(u32) * DSI_TRIGGER,          0},
    {sizeof(u32) * DSI_INIT_SEQ_CONTROL, 0},
    {sizeof(u32) * DSI_PKT_LEN_0_1,      0},
    {sizeof(u32) * DSI_PKT_LEN_2_3,      0},
    {sizeof(u32) * DSI_PKT_LEN_4_5,      0},
    {sizeof(u32) * DSI_PKT_LEN_6_7,      0},
    {sizeof(u32) * DSI_PAD_CONTROL_1,    0},
};

constexpr RegisterWrite DisplayConfigDsi01Init06[] = {
    {sizeof(u32) * DSI_PHY_TIMING_1,  0x40A0E05},
    {sizeof(u32) * DSI_PHY_TIMING_2,  0x30109},
    {sizeof(u32) * DSI_BTA_TIMING,    0x190A14},
    {sizeof(u32) * DSI_TIMEOUT_0,     DSI_TIMEOUT_LRX(0x2000) | DSI_TIMEOUT_HTX(0xFFFF)},
    {sizeof(u32) * DSI_TIMEOUT_1,     DSI_TIMEOUT_PR(0x765) | DSI_TIMEOUT_TA(0x2000)},
    {sizeof(u32) * DSI_TO_TALLY,      0},
    {sizeof(u32) * DSI_PAD_CONTROL_0, DSI_PAD_CONTROL_VS1_PULLDN(0) | DSI_PAD_CONTROL_VS1_PDIO(0)}, // Enable
    {sizeof(u32) * DSI_POWER_CONTROL, DSI_POWER_CONTROL_ENABLE},
    {sizeof(u32) * DSI_POWER_CONTROL, DSI_POWER_CONTROL_ENABLE},
    {sizeof(u32) * DSI_POWER_CONTROL, 0},
    {sizeof(u32) * DSI_POWER_CONTROL, 0},
    {sizeof(u32) * DSI_PAD_CONTROL_1, 0},

};

constexpr RegisterWrite DisplayConfigDsi01Init07[] = {
    {sizeof(u32) * DSI_PHY_TIMING_1,        0x40A0E05},
    {sizeof(u32) * DSI_PHY_TIMING_2,        0x30118},
    {sizeof(u32) * DSI_BTA_TIMING,          0x190A14},
    {sizeof(u32) * DSI_TIMEOUT_0,           DSI_TIMEOUT_LRX(0x2000) | DSI_TIMEOUT_HTX(0xFFFF)},
    {sizeof(u32) * DSI_TIMEOUT_1,           DSI_TIMEOUT_PR(0x1343) | DSI_TIMEOUT_TA(0x2000)},
    {sizeof(u32) * DSI_TO_TALLY,            0},
    {sizeof(u32) * DSI_HOST_CONTROL,        DSI_HOST_CONTROL_CRC_RESET | DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC},
    {sizeof(u32) * DSI_CONTROL,             DSI_CONTROL_LANES(3) | DSI_CONTROL_HOST_ENABLE},
    {sizeof(u32) * DSI_POWER_CONTROL,       DSI_POWER_CONTROL_ENABLE},
    {sizeof(u32) * DSI_POWER_CONTROL,       DSI_POWER_CONTROL_ENABLE},
    {sizeof(u32) * DSI_MAX_THRESHOLD,       0x40},
    {sizeof(u32) * DSI_TRIGGER,             0},
    {sizeof(u32) * DSI_TX_CRC,              0},
    {sizeof(u32) * DSI_INIT_SEQ_CONTROL,    0}
};

constexpr RegisterWrite DisplayConfigDsiPhyTimingErista[] = {
    {sizeof(u32) * DSI_PHY_TIMING_0, 0x6070601},
};

constexpr RegisterWrite DisplayConfigDsiPhyTimingMariko[] = {
    {sizeof(u32) * DSI_PHY_TIMING_0, 0x6070603},
};

constexpr DsiSleepOrRegisterWrite DisplayConfigJdiSpecificInit01[] = {
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x439},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x9483FFB9},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xBD15},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x1939},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xAAAAAAD8},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xAAAAAAEB},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xAAEBAAAA},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xAAAAAAAA},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xAAAAAAEB},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xAAEBAAAA},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xAA},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x1BD15},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x2739},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFD8},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x2BD15},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xF39},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFD8},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xFFFFFF},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xBD15},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x6D915},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x439},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0xB9},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x1105},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {DsiSleepOrRegisterWriteKind_Sleep, 0xB4, 0},
    {DsiSleepOrRegisterWriteKind_Write, DSI_WR_DATA, 0x2905},
    {DsiSleepOrRegisterWriteKind_Write, DSI_TRIGGER, DSI_TRIGGER_HOST},
};

constexpr RegisterWrite DisplayConfigPlld02Erista[] = {
    {CLK_RST_CONTROLLER_PLLD_BASE,        0x4810c001},
    {CLK_RST_CONTROLLER_PLLD_MISC1,       0x00000020},
    {CLK_RST_CONTROLLER_PLLD_MISC,        0x002D0AAA},
};

constexpr RegisterWrite DisplayConfigPlld02Mariko[] = {
    {CLK_RST_CONTROLLER_PLLD_BASE,        0x4810c001},
    {CLK_RST_CONTROLLER_PLLD_MISC1,       0x00000000},
    {CLK_RST_CONTROLLER_PLLD_MISC,        0x002DFC00},
};

constexpr RegisterWrite DisplayConfigDsi01Init08[] = {
    {sizeof(u32) * DSI_PAD_CONTROL_1, 0},
};

constexpr RegisterWrite DisplayConfigDsi01Init09[] = {
    {sizeof(u32) * DSI_PHY_TIMING_1,    0x40A0E05},
    {sizeof(u32) * DSI_PHY_TIMING_2,    0x30172},
    {sizeof(u32) * DSI_BTA_TIMING,      0x190A14},
    {sizeof(u32) * DSI_TIMEOUT_0,       DSI_TIMEOUT_LRX(0x2000) | DSI_TIMEOUT_HTX(0xA40)},
    {sizeof(u32) * DSI_TIMEOUT_1,       DSI_TIMEOUT_PR(0x5A2F) | DSI_TIMEOUT_TA(0x2000)},
    {sizeof(u32) * DSI_TO_TALLY,        0},
    {sizeof(u32) * DSI_PKT_SEQ_0_LO,    0x40000208},
    {sizeof(u32) * DSI_PKT_SEQ_2_LO,    0x40000308},
    {sizeof(u32) * DSI_PKT_SEQ_4_LO,    0x40000308},
    {sizeof(u32) * DSI_PKT_SEQ_1_LO,    0x40000308},
    {sizeof(u32) * DSI_PKT_SEQ_3_LO,    0x3F3B2B08},
    {sizeof(u32) * DSI_PKT_SEQ_3_HI,    0x2CC},
    {sizeof(u32) * DSI_PKT_SEQ_5_LO,    0x3F3B2B08},
    {sizeof(u32) * DSI_PKT_SEQ_5_HI,    0x2CC},
    {sizeof(u32) * DSI_PKT_LEN_0_1,     0xCE0000},
    {sizeof(u32) * DSI_PKT_LEN_2_3,     0x87001A2},
    {sizeof(u32) * DSI_PKT_LEN_4_5,     0x190},
    {sizeof(u32) * DSI_PKT_LEN_6_7,     0x190},
    {sizeof(u32) * DSI_HOST_CONTROL,    0},
};

constexpr RegisterWrite DisplayConfigDsi01Init10[] = {
    {sizeof(u32) * DSI_TRIGGER,         0},
    {sizeof(u32) * DSI_CONTROL,         0},
    {sizeof(u32) * DSI_SOL_DELAY,       6},
    {sizeof(u32) * DSI_MAX_THRESHOLD,   0x1E0},
    {sizeof(u32) * DSI_POWER_CONTROL,   DSI_POWER_CONTROL_ENABLE},
    {sizeof(u32) * DSI_CONTROL,         DSI_CONTROL_HS_CLK_CTRL | DSI_CONTROL_FORMAT(3) | DSI_CONTROL_LANES(3) | DSI_CONTROL_VIDEO_ENABLE},
    {sizeof(u32) * DSI_HOST_CONTROL,    DSI_HOST_CONTROL_HS | DSI_HOST_CONTROL_FIFO_SEL| DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC},
    {sizeof(u32) * DSI_CONTROL,         DSI_CONTROL_HS_CLK_CTRL | DSI_CONTROL_FORMAT(3) | DSI_CONTROL_LANES(3) | DSI_CONTROL_VIDEO_ENABLE},
    {sizeof(u32) * DSI_HOST_CONTROL,    DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC},
    {sizeof(u32) * DSI_HOST_CONTROL,    DSI_HOST_CONTROL_HS | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC}
};

constexpr RegisterWrite DisplayConfigDsi01Init11Erista[] = {
    {sizeof(u32) * DSI_PAD_CONTROL_1, 0},
    {sizeof(u32) * DSI_PAD_CONTROL_2, 0},
    {sizeof(u32) * DSI_PAD_CONTROL_3, DSI_PAD_PREEMP_PD_CLK(0x3) | DSI_PAD_PREEMP_PU_CLK(0x3) | DSI_PAD_PREEMP_PD(0x03) | DSI_PAD_PREEMP_PU(0x3)},
    {sizeof(u32) * DSI_PAD_CONTROL_4, 0}
};

constexpr RegisterWrite DisplayConfigDsi01Init11Mariko[] = {
    {sizeof(u32) * DSI_PAD_CONTROL_1,        0},
    {sizeof(u32) * DSI_PAD_CONTROL_2,        0},
    {sizeof(u32) * DSI_PAD_CONTROL_3,        0},
    {sizeof(u32) * DSI_PAD_CONTROL_4,        0x77777},
    {sizeof(u32) * DSI_PAD_CONTROL_5_MARIKO, 0x77777},
    {sizeof(u32) * DSI_PAD_CONTROL_6_MARIKO, DSI_PAD_PREEMP_PD_CLK(0x1) | DSI_PAD_PREEMP_PU_CLK(0x1) | DSI_PAD_PREEMP_PD(0x01) | DSI_PAD_PREEMP_PU(0x1)},
    {sizeof(u32) * DSI_PAD_CONTROL_7_MARIKO, 0},
};

constexpr RegisterWrite DisplayConfigMipiCal01[] = {
    {0x60, 0},
    {0x08, 0xF3F10000},
    {0x58, 1},
    {0x60, 0},
};

constexpr RegisterWrite DisplayConfigMipiCal02Erista[] = {
    {0x60, 0x10010},
    {0x5C, 0x300},
};

constexpr RegisterWrite DisplayConfigMipiCal02Mariko[] = {
    {0x60, 0x10010},
    {0x5C, 0},
};

constexpr RegisterWrite DisplayConfigMipiCal03Erista[] = {
    {0x38, 0x200200},
    {0x3C, 0x200200},
    {0x64, 0x200002},
    {0x68, 0x200002},
    {0x14, 0},
    {0x18, 0},
};

constexpr RegisterWrite DisplayConfigMipiCal03Mariko[] = {
    {0x38, 0x200006},
    {0x3C, 0x200006},
    {0x64, 0x260000},
    {0x68, 0x260000},
    {0x14, 0},
    {0x18, 0},
};

constexpr RegisterWrite DisplayConfigMipiCal04[] = {
    {0x1C, 0},
    {0x20, 0},
    {0x24, 0},
    {0x28, 0},
    {0x40, 0},
    {0x44, 0},
    {0x68, 0},
    {0x70, 0},
    {0x74, 0},
    {0x00, 0x2A000001},
};

constexpr RegisterWrite DisplayConfigDc02[] = {
    {sizeof(u32) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_DV_CONTROL, 0},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    /* Setup default YUV colorspace conversion coefficients */
    {sizeof(u32) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(u32) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(u32) * DC_WIN_CSC_KUR,   0},
    {sizeof(u32) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(u32) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(u32) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(u32) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(u32) * DC_WIN_CSC_KVB,   0},
    /* End of color coefficients */
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_DV_CONTROL, 0},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    /* Setup default YUV colorspace conversion coefficients */
    {sizeof(u32) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(u32) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(u32) * DC_WIN_CSC_KUR,   0},
    {sizeof(u32) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(u32) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(u32) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(u32) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(u32) * DC_WIN_CSC_KVB,   0},
    /* End of color coefficients */
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_DV_CONTROL, 0},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    /* Setup default YUV colorspace conversion coefficients */
    {sizeof(u32) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(u32) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(u32) * DC_WIN_CSC_KUR,   0},
    {sizeof(u32) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(u32) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(u32) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(u32) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(u32) * DC_WIN_CSC_KVB,   0},
    /* End of color coefficients */
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE_888},
    {sizeof(u32) * DC_DISP_DISP_INTERFACE_CONTROL, DISP_DATA_FORMAT_DF1P1C},
    {sizeof(u32) * DC_COM_PIN_OUTPUT_POLARITY(1), 0x1000000},
    {sizeof(u32) * DC_COM_PIN_OUTPUT_POLARITY(3), 0},
    {sizeof(u32) * 0x4E4, 0},
    {sizeof(u32) * DC_COM_CRC_CONTROL, 0},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * 0x716, 0x10000FF},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * 0x716, 0x10000FF},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * 0x716, 0x10000FF},
    {sizeof(u32) * DC_CMD_DISPLAY_COMMAND_OPTION0, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_COMMAND, 0},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
    {sizeof(u32) * DC_CMD_STATE_ACCESS, 0},
    /* Set Display timings */
    {sizeof(u32) * DC_DISP_DISP_TIMING_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_REF_TO_SYNC, (1 << 16)}, // h_ref_to_sync = 0, v_ref_to_sync = 1.
    {sizeof(u32) * DC_DISP_SYNC_WIDTH,  0x10048},
    {sizeof(u32) * DC_DISP_BACK_PORCH,  0x90048},
    {sizeof(u32) * DC_DISP_ACTIVE,      0x50002D0},
    {sizeof(u32) * DC_DISP_FRONT_PORCH, 0xA0088}, // Sources say that this should be above the DC_DISP_ACTIVE cmd.
    /* End of Display timings */
    {sizeof(u32) * DC_DISP_SHIFT_CLOCK_OPTIONS, SC1_H_QUALIFIER_NONE | SC0_H_QUALIFIER_NONE},
    {sizeof(u32) * DC_COM_PIN_OUTPUT_ENABLE(1), 0},
    {sizeof(u32) * DC_DISP_DATA_ENABLE_OPTIONS, DE_SELECT_ACTIVE | DE_CONTROL_NORMAL},
    {sizeof(u32) * DC_DISP_DISP_INTERFACE_CONTROL, DISP_DATA_FORMAT_DF1P1C},
    {sizeof(u32) * DC_DISP_DISP_CLOCK_CONTROL, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_COMMAND_OPTION0, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_COMMAND, DISP_CTRL_MODE_C_DISPLAY},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(u32) * DC_CMD_STATE_ACCESS, READ_MUX | WRITE_MUX},
    {sizeof(u32) * DC_DISP_FRONT_PORCH, 0xA0088},
    {sizeof(u32) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(u32) * DC_CMD_GENERAL_INCR_SYNCPT, 0x301},
    {sizeof(u32) * DC_CMD_GENERAL_INCR_SYNCPT, 0x301},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(u32) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(u32) * DC_DISP_DISP_CLOCK_CONTROL, PIXEL_CLK_DIVIDER_PCD1 | SHIFT_CLK_DIVIDER(4)},
    {sizeof(u32) * DC_DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE_888},
    {sizeof(u32) * DC_CMD_DISPLAY_COMMAND_OPTION0, 0}
};

constexpr u32 DisplayConfigFrameBufferAddress = 0xC0000000;

constexpr RegisterWrite DisplayConfigFrameBuffer[] = {
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT}, //Enable window C.
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT}, //Enable window B.
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT}, //Enable window A.
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE}, //DSI_ENABLE
    {sizeof(u32) * DC_WIN_COLOR_DEPTH, WIN_COLOR_DEPTH_B8G8R8A8}, //T_A8R8G8B8 //NX Default: T_A8B8G8R8, WIN_COLOR_DEPTH_R8G8B8A8
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_WIN_POSITION, 0}, //(0,0)
    {sizeof(u32) * DC_WIN_H_INITIAL_DDA, 0},
    {sizeof(u32) * DC_WIN_V_INITIAL_DDA, 0},
    {sizeof(u32) * DC_WIN_PRESCALED_SIZE, V_PRESCALED_SIZE(1280) | H_PRESCALED_SIZE(2880)}, //Pre-scaled size: 1280x2880 bytes.
    {sizeof(u32) * DC_WIN_DDA_INC, V_DDA_INC(0x1000) | H_DDA_INC(0x1000)},
    {sizeof(u32) * DC_WIN_SIZE, V_SIZE(1280) | H_SIZE(720)}, //Window size: 1280 vertical lines x 720 horizontal pixels.
    {sizeof(u32) * DC_WIN_LINE_STRIDE, 0x6000C00}, //768*2x768*4 (= 0x600 x 0xC00) bytes, see TRM for alignment requirements.
    {sizeof(u32) * DC_WIN_BUFFER_CONTROL, 0},
    {sizeof(u32) * DC_WINBUF_SURFACE_KIND, 0}, //Regular surface.
    {sizeof(u32) * DC_WINBUF_START_ADDR, DisplayConfigFrameBufferAddress}, //Framebuffer address.
    {sizeof(u32) * DC_WINBUF_ADDR_H_OFFSET, 0},
    {sizeof(u32) * DC_WINBUF_ADDR_V_OFFSET, 0},
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE}, //DSI_ENABLE
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE}, //DSI_ENABLE
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(u32) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE}, //DSI_ENABLE
    {sizeof(u32) * DC_WIN_WIN_OPTIONS, WIN_ENABLE}, //Enable window AD.
    {sizeof(u32) * DC_CMD_DISPLAY_COMMAND, DISP_CTRL_MODE_C_DISPLAY}, //DISPLAY_CTRL_MODE: continuous display.
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE}, //General update; window A update.
    {sizeof(u32) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ} //General activation request; window A activation request.
};

constexpr RegisterWrite DisplayConfigDsi01Fini01[] = {
    {sizeof(u32) * DSI_POWER_CONTROL, 0},
    {sizeof(u32) * DSI_PAD_CONTROL_1, 0},
};

constexpr RegisterWrite DisplayConfigDsi01Fini02[] = {
    {sizeof(u32) * DSI_PHY_TIMING_1,      0x40A0E05},
    {sizeof(u32) * DSI_PHY_TIMING_2,      0x30109},
    {sizeof(u32) * DSI_BTA_TIMING,        0x190A14},
    {sizeof(u32) * DSI_TIMEOUT_0,         DSI_TIMEOUT_LRX(0x2000) | DSI_TIMEOUT_HTX(0xFFFF) },
    {sizeof(u32) * DSI_TIMEOUT_1,         DSI_TIMEOUT_PR(0x765) | DSI_TIMEOUT_TA(0x2000)},
    {sizeof(u32) * DSI_TO_TALLY,          0},
    {sizeof(u32) * DSI_HOST_CONTROL,      DSI_HOST_CONTROL_CRC_RESET | DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC},
    {sizeof(u32) * DSI_CONTROL,           DSI_CONTROL_LANES(3) | DSI_CONTROL_HOST_ENABLE},
    {sizeof(u32) * DSI_POWER_CONTROL,     DSI_POWER_CONTROL_ENABLE},
    {sizeof(u32) * DSI_MAX_THRESHOLD,     0x40},
    {sizeof(u32) * DSI_TRIGGER,           0},
    {sizeof(u32) * DSI_TX_CRC,            0},
    {sizeof(u32) * DSI_INIT_SEQ_CONTROL,  0}
};

constexpr RegisterWrite DisplayConfigJdiSpecificFini01[] = {
    {sizeof(u32) * DSI_WR_DATA, 0x439},
    {sizeof(u32) * DSI_WR_DATA, 0x9483FFB9},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
    {sizeof(u32) * DSI_WR_DATA, 0x2139},
    {sizeof(u32) * DSI_WR_DATA, 0x191919D5},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
    {sizeof(u32) * DSI_WR_DATA, 0xB39},
    {sizeof(u32) * DSI_WR_DATA, 0x4F0F41B1},
    {sizeof(u32) * DSI_WR_DATA, 0xF179A433},
    {sizeof(u32) * DSI_WR_DATA, 0x2D81},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
    {sizeof(u32) * DSI_WR_DATA, 0x439},
    {sizeof(u32) * DSI_WR_DATA, 0xB9},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
};

constexpr RegisterWrite DisplayConfigAuoRev1SpecificFini01[] = {
    {sizeof(u32) * DSI_WR_DATA, 0x439},
    {sizeof(u32) * DSI_WR_DATA, 0x9483FFB9},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
    {sizeof(u32) * DSI_WR_DATA, 0x2C39},
    {sizeof(u32) * DSI_WR_DATA, 0x191919D5},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
    {sizeof(u32) * DSI_WR_DATA, 0x2C39},
    {sizeof(u32) * DSI_WR_DATA, 0x191919D6},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_WR_DATA, 0x19191919},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
    {sizeof(u32) * DSI_WR_DATA, 0xB39},
    {sizeof(u32) * DSI_WR_DATA, 0x711148B1},
    {sizeof(u32) * DSI_WR_DATA, 0x71143209},
    {sizeof(u32) * DSI_WR_DATA, 0x114D31},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
    {sizeof(u32) * DSI_WR_DATA, 0x439},
    {sizeof(u32) * DSI_WR_DATA, 0xB9},
    {sizeof(u32) * DSI_TRIGGER, DSI_TRIGGER_HOST},
};
