/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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

typedef struct {
    uint32_t offset;
    uint32_t value;
} register_write_t;

typedef struct {
    uint16_t kind;
    uint16_t offset;
    uint32_t value;
} dsi_sleep_or_register_write_t;

static const uint32_t display_config_frame_buffer_address = 0xC0000000;

static const register_write_t display_config_plld_01_erista[4] = {
    {CLK_RST_CONTROLLER_CLK_SOURCE_DISP1, 0x40000000},
    {CLK_RST_CONTROLLER_PLLD_BASE,        0x4830A001},
    {CLK_RST_CONTROLLER_PLLD_MISC1,       0x00000020},
    {CLK_RST_CONTROLLER_PLLD_MISC,        0x002D0AAA},
};

static const register_write_t display_config_plld_01_mariko[4] = {
    {CLK_RST_CONTROLLER_CLK_SOURCE_DISP1, 0x40000000},
    {CLK_RST_CONTROLLER_PLLD_BASE,        0x4830A001},
    {CLK_RST_CONTROLLER_PLLD_MISC1,       0x00000000},
    {CLK_RST_CONTROLLER_PLLD_MISC,        0x002DFC00},
};

static const register_write_t display_config_dc_01[94] = {
    {sizeof(uint32_t) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_REG_ACT_CONTROL, 0x54},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_DISP_DC_MCCIF_FIFOCTRL, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_MEM_HIGH_PRIORITY, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_MEM_HIGH_PRIORITY_TIMER, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_POWER_CONTROL, PW0_ENABLE | PW1_ENABLE | PW2_ENABLE | PW3_ENABLE | PW4_ENABLE | PM0_ENABLE | PM1_ENABLE},
    {sizeof(uint32_t) * DC_CMD_GENERAL_INCR_SYNCPT_CNTRL, SYNCPT_CNTRL_NO_STALL},
    {sizeof(uint32_t) * DC_CMD_CONT_SYNCPT_VSYNC, SYNCPT_VSYNC_ENABLE | 0x9},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_DV_CONTROL, 0},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(uint32_t) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(uint32_t) * DC_WIN_CSC_KUR,   0},
    {sizeof(uint32_t) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(uint32_t) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(uint32_t) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(uint32_t) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(uint32_t) * DC_WIN_CSC_KVB,   0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_DV_CONTROL, 0},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(uint32_t) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(uint32_t) * DC_WIN_CSC_KUR,   0},
    {sizeof(uint32_t) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(uint32_t) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(uint32_t) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(uint32_t) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(uint32_t) * DC_WIN_CSC_KVB,   0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_DV_CONTROL, 0},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(uint32_t) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(uint32_t) * DC_WIN_CSC_KUR,   0},
    {sizeof(uint32_t) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(uint32_t) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(uint32_t) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(uint32_t) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(uint32_t) * DC_WIN_CSC_KVB,   0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE_888},
    {sizeof(uint32_t) * DC_DISP_DISP_INTERFACE_CONTROL, DISP_DATA_FORMAT_DF1P1C},
    {sizeof(uint32_t) * DC_COM_PIN_OUTPUT_POLARITY(1), 0x1000000},
    {sizeof(uint32_t) * DC_COM_PIN_OUTPUT_POLARITY(3), 0},
    {sizeof(uint32_t) * DC_DISP_BLEND_BACKGROUND_COLOR, 0},
    {sizeof(uint32_t) * DC_COM_CRC_CONTROL, 0},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WINBUF_BLEND_LAYER_CONTROL, 0x10000FF},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WINBUF_BLEND_LAYER_CONTROL, 0x10000FF},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WINBUF_BLEND_LAYER_CONTROL, 0x10000FF},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND_OPTION0, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND, 0},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
};

static const register_write_t display_config_dsi_01_init_01[8] = {
    {sizeof(uint32_t) * DSI_WR_DATA,         0x0},
    {sizeof(uint32_t) * DSI_INT_ENABLE,      0x0},
    {sizeof(uint32_t) * DSI_INT_STATUS,      0x0},
    {sizeof(uint32_t) * DSI_INT_MASK,        0x0},
    {sizeof(uint32_t) * DSI_INIT_SEQ_DATA_0, 0x0},
    {sizeof(uint32_t) * DSI_INIT_SEQ_DATA_1, 0x0},
    {sizeof(uint32_t) * DSI_INIT_SEQ_DATA_2, 0x0},
    {sizeof(uint32_t) * DSI_INIT_SEQ_DATA_3, 0x0},
};

static const register_write_t display_config_dsi_01_init_02_erista[1] = {
    {sizeof(uint32_t) * DSI_INIT_SEQ_DATA_15, 0x0},
};

static const register_write_t display_config_dsi_01_init_02_mariko[1] = {
    {sizeof(uint32_t) * DSI_INIT_SEQ_DATA_15_MARIKO, 0x0},
};

static const register_write_t display_config_dsi_01_init_03[14] = {
    {sizeof(uint32_t) * DSI_DCS_CMDS,     0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_0_LO, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_1_LO, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_2_LO, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_3_LO, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_4_LO, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_5_LO, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_0_HI, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_1_HI, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_2_HI, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_3_HI, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_4_HI, 0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_5_HI, 0},
    {sizeof(uint32_t) * DSI_CONTROL,      0},
};

static const register_write_t display_config_dsi_01_init_04_erista[0] = {
    /* No register writes. */
};

static const register_write_t display_config_dsi_01_init_04_mariko[7] = {
    {sizeof(uint32_t) * DSI_PAD_CONTROL_1,        0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_2,        0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_3,        0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_4,        0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_5_MARIKO, 0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_6_MARIKO, 0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_7_MARIKO, 0},
};

static const register_write_t display_config_dsi_01_init_05[10] = {
    {sizeof(uint32_t) * DSI_PAD_CONTROL_CD,   0},
    {sizeof(uint32_t) * DSI_SOL_DELAY,        0x18},
    {sizeof(uint32_t) * DSI_MAX_THRESHOLD,    0x1E0},
    {sizeof(uint32_t) * DSI_TRIGGER,          0},
    {sizeof(uint32_t) * DSI_INIT_SEQ_CONTROL, 0},
    {sizeof(uint32_t) * DSI_PKT_LEN_0_1,      0},
    {sizeof(uint32_t) * DSI_PKT_LEN_2_3,      0},
    {sizeof(uint32_t) * DSI_PKT_LEN_4_5,      0},
    {sizeof(uint32_t) * DSI_PKT_LEN_6_7,      0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_1,    0},
};

static const register_write_t display_config_dsi_01_init_06[12] = {
    {sizeof(uint32_t) * DSI_PHY_TIMING_1,  0x40A0E05},
    {sizeof(uint32_t) * DSI_PHY_TIMING_2,  0x30109},
    {sizeof(uint32_t) * DSI_BTA_TIMING,    0x190A14},
    {sizeof(uint32_t) * DSI_TIMEOUT_0,     DSI_TIMEOUT_LRX(0x2000) | DSI_TIMEOUT_HTX(0xFFFF)},
    {sizeof(uint32_t) * DSI_TIMEOUT_1,     DSI_TIMEOUT_PR(0x765) | DSI_TIMEOUT_TA(0x2000)},
    {sizeof(uint32_t) * DSI_TO_TALLY,      0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_0, DSI_PAD_CONTROL_VS1_PULLDN(0) | DSI_PAD_CONTROL_VS1_PDIO(0)},
    {sizeof(uint32_t) * DSI_POWER_CONTROL, DSI_POWER_CONTROL_ENABLE},
    {sizeof(uint32_t) * DSI_POWER_CONTROL, DSI_POWER_CONTROL_ENABLE},
    {sizeof(uint32_t) * DSI_POWER_CONTROL, 0},
    {sizeof(uint32_t) * DSI_POWER_CONTROL, 0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_1, 0},
};

static const register_write_t display_config_dsi_01_init_07[14] = {
    {sizeof(uint32_t) * DSI_PHY_TIMING_1,        0x40A0E05},
    {sizeof(uint32_t) * DSI_PHY_TIMING_2,        0x30118},
    {sizeof(uint32_t) * DSI_BTA_TIMING,          0x190A14},
    {sizeof(uint32_t) * DSI_TIMEOUT_0,           DSI_TIMEOUT_LRX(0x2000) | DSI_TIMEOUT_HTX(0xFFFF)},
    {sizeof(uint32_t) * DSI_TIMEOUT_1,           DSI_TIMEOUT_PR(0x1343) | DSI_TIMEOUT_TA(0x2000)},
    {sizeof(uint32_t) * DSI_TO_TALLY,            0},
    {sizeof(uint32_t) * DSI_HOST_CONTROL,        DSI_HOST_CONTROL_CRC_RESET | DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC},
    {sizeof(uint32_t) * DSI_CONTROL,             DSI_CONTROL_LANES(3) | DSI_CONTROL_HOST_ENABLE},
    {sizeof(uint32_t) * DSI_POWER_CONTROL,       DSI_POWER_CONTROL_ENABLE},
    {sizeof(uint32_t) * DSI_POWER_CONTROL,       DSI_POWER_CONTROL_ENABLE},
    {sizeof(uint32_t) * DSI_MAX_THRESHOLD,       0x40},
    {sizeof(uint32_t) * DSI_TRIGGER,             0},
    {sizeof(uint32_t) * DSI_TX_CRC,              0},
    {sizeof(uint32_t) * DSI_INIT_SEQ_CONTROL,    0},
};

static const register_write_t display_config_dsi_phy_timing_erista[1] = {
    {sizeof(uint32_t) * DSI_PHY_TIMING_0, 0x6070601},
};

static const register_write_t display_config_dsi_phy_timing_mariko[1] = {
    {sizeof(uint32_t) * DSI_PHY_TIMING_0, 0x6070603},
};

static const dsi_sleep_or_register_write_t display_config_jdi_specific_init_01[48] = {
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0x9483FFB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0xBD15},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x1939},
    {0, DSI_WR_DATA, 0xAAAAAAD8},
    {0, DSI_WR_DATA, 0xAAAAAAEB},
    {0, DSI_WR_DATA, 0xAAEBAAAA},
    {0, DSI_WR_DATA, 0xAAAAAAAA},
    {0, DSI_WR_DATA, 0xAAAAAAEB},
    {0, DSI_WR_DATA, 0xAAEBAAAA},
    {0, DSI_WR_DATA, 0xAA},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x1BD15},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x2739},
    {0, DSI_WR_DATA, 0xFFFFFFD8},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFF},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x2BD15},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0xF39},
    {0, DSI_WR_DATA, 0xFFFFFFD8},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFFFF},
    {0, DSI_WR_DATA, 0xFFFFFF},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0xBD15},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x6D915},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0xB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x1105},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0xB4, 0},
    {0, DSI_WR_DATA, 0x2905},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
};

static const dsi_sleep_or_register_write_t display_config_innolux_nx_abca2_specific_init_01[14] = {
    {0, DSI_WR_DATA, 0x1105},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0xB4, 0},
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0x9483FFB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
    {0, DSI_WR_DATA, 0x739},
    {0, DSI_WR_DATA, 0x751548B1},
    {0, DSI_WR_DATA, 0x143209},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
    {0, DSI_WR_DATA, 0x2905},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
};

static const dsi_sleep_or_register_write_t display_config_auo_nx_abca2_specific_init_01[14] = {
    {0, DSI_WR_DATA, 0x1105},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0xB4, 0},
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0x9483FFB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
    {0, DSI_WR_DATA, 0x739},
    {0, DSI_WR_DATA, 0x711148B1},
    {0, DSI_WR_DATA, 0x143209},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
    {0, DSI_WR_DATA, 0x2905},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
};

static const dsi_sleep_or_register_write_t display_config_innolux_auo_40_nx_abcc_specific_init_01[5] = {
    {0, DSI_WR_DATA, 0x1105},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x78, 0},
    {0, DSI_WR_DATA, 0x2905},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
};

static const dsi_sleep_or_register_write_t display_config_50_nx_abcd_specific_init_01[13] = {
    {0, DSI_WR_DATA, 0x1105},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0xB4, 0},
    {0, DSI_WR_DATA, 0xA015},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x205315},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x339},
    {0, DSI_WR_DATA, 0xFF0751},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
    {0, DSI_WR_DATA, 0x2905},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
};

static const register_write_t display_config_plld_02_erista[3] = {
    {CLK_RST_CONTROLLER_PLLD_BASE,        0x4810c001},
    {CLK_RST_CONTROLLER_PLLD_MISC1,       0x00000020},
    {CLK_RST_CONTROLLER_PLLD_MISC,        0x002D0AAA},
};

static const register_write_t display_config_plld_02_mariko[3] = {
    {CLK_RST_CONTROLLER_PLLD_BASE,        0x4810c001},
    {CLK_RST_CONTROLLER_PLLD_MISC1,       0x00000000},
    {CLK_RST_CONTROLLER_PLLD_MISC,        0x002DFC00},
};

static const register_write_t display_config_dsi_01_init_08[1] = {
    {sizeof(uint32_t) * DSI_PAD_CONTROL_1, 0},
};

static const register_write_t display_config_dsi_01_init_09[19] = {
    {sizeof(uint32_t) * DSI_PHY_TIMING_1,    0x40A0E05},
    {sizeof(uint32_t) * DSI_PHY_TIMING_2,    0x30172},
    {sizeof(uint32_t) * DSI_BTA_TIMING,      0x190A14},
    {sizeof(uint32_t) * DSI_TIMEOUT_0,       DSI_TIMEOUT_LRX(0x2000) | DSI_TIMEOUT_HTX(0xA40)},
    {sizeof(uint32_t) * DSI_TIMEOUT_1,       DSI_TIMEOUT_PR(0x5A2F) | DSI_TIMEOUT_TA(0x2000)},
    {sizeof(uint32_t) * DSI_TO_TALLY,        0},
    {sizeof(uint32_t) * DSI_PKT_SEQ_0_LO,    0x40000208},
    {sizeof(uint32_t) * DSI_PKT_SEQ_2_LO,    0x40000308},
    {sizeof(uint32_t) * DSI_PKT_SEQ_4_LO,    0x40000308},
    {sizeof(uint32_t) * DSI_PKT_SEQ_1_LO,    0x40000308},
    {sizeof(uint32_t) * DSI_PKT_SEQ_3_LO,    0x3F3B2B08},
    {sizeof(uint32_t) * DSI_PKT_SEQ_3_HI,    0x2CC},
    {sizeof(uint32_t) * DSI_PKT_SEQ_5_LO,    0x3F3B2B08},
    {sizeof(uint32_t) * DSI_PKT_SEQ_5_HI,    0x2CC},
    {sizeof(uint32_t) * DSI_PKT_LEN_0_1,     0xCE0000},
    {sizeof(uint32_t) * DSI_PKT_LEN_2_3,     0x87001A2},
    {sizeof(uint32_t) * DSI_PKT_LEN_4_5,     0x190},
    {sizeof(uint32_t) * DSI_PKT_LEN_6_7,     0x190},
    {sizeof(uint32_t) * DSI_HOST_CONTROL,    0},
};

static const register_write_t display_config_dsi_01_init_10[10] = {
    {sizeof(uint32_t) * DSI_TRIGGER,         0},
    {sizeof(uint32_t) * DSI_CONTROL,         0},
    {sizeof(uint32_t) * DSI_SOL_DELAY,       6},
    {sizeof(uint32_t) * DSI_MAX_THRESHOLD,   0x1E0},
    {sizeof(uint32_t) * DSI_POWER_CONTROL,   DSI_POWER_CONTROL_ENABLE},
    {sizeof(uint32_t) * DSI_CONTROL,         DSI_CONTROL_HS_CLK_CTRL | DSI_CONTROL_FORMAT(3) | DSI_CONTROL_LANES(3) | DSI_CONTROL_VIDEO_ENABLE},
    {sizeof(uint32_t) * DSI_HOST_CONTROL,    DSI_HOST_CONTROL_HS | DSI_HOST_CONTROL_FIFO_SEL| DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC},
    {sizeof(uint32_t) * DSI_CONTROL,         DSI_CONTROL_HS_CLK_CTRL | DSI_CONTROL_FORMAT(3) | DSI_CONTROL_LANES(3) | DSI_CONTROL_VIDEO_ENABLE},
    {sizeof(uint32_t) * DSI_HOST_CONTROL,    DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC},
    {sizeof(uint32_t) * DSI_HOST_CONTROL,    DSI_HOST_CONTROL_HS | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC}
};

static const register_write_t display_config_dsi_01_init_11_erista[4] = {
    {sizeof(uint32_t) * DSI_PAD_CONTROL_1, 0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_2, 0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_3, DSI_PAD_PREEMP_PD_CLK(0x3) | DSI_PAD_PREEMP_PU_CLK(0x3) | DSI_PAD_PREEMP_PD(0x03) | DSI_PAD_PREEMP_PU(0x3)},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_4, 0}
};

static const register_write_t display_config_dsi_01_init_11_mariko[7] = {
    {sizeof(uint32_t) * DSI_PAD_CONTROL_1,        0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_2,        0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_3,        0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_4,        0x77777},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_5_MARIKO, 0x77777},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_6_MARIKO, DSI_PAD_PREEMP_PD_CLK(0x1) | DSI_PAD_PREEMP_PU_CLK(0x1) | DSI_PAD_PREEMP_PD(0x01) | DSI_PAD_PREEMP_PU(0x1)},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_7_MARIKO, 0},
};

static const register_write_t display_config_mipi_cal_01[4] = {
    {MIPI_CAL_MIPI_BIAS_PAD_CFG2, 0},
    {MIPI_CAL_CIL_MIPI_CAL_STATUS, 0xF3F10000},
    {MIPI_CAL_MIPI_BIAS_PAD_CFG0, 1},
    {MIPI_CAL_MIPI_BIAS_PAD_CFG2, 0},
};

static const register_write_t display_config_mipi_cal_02_erista[2] = {
    {MIPI_CAL_MIPI_BIAS_PAD_CFG2, 0x10010},
    {MIPI_CAL_MIPI_BIAS_PAD_CFG1, 0x300},
};

static const register_write_t display_config_mipi_cal_02_mariko[2] = {
    {MIPI_CAL_MIPI_BIAS_PAD_CFG2, 0x10010},
    {MIPI_CAL_MIPI_BIAS_PAD_CFG1, 0},
};

static const register_write_t display_config_mipi_cal_03_erista[6] = {
    {MIPI_CAL_DSIA_MIPI_CAL_CONFIG, 0x200200},
    {MIPI_CAL_DSIB_MIPI_CAL_CONFIG, 0x200200},
    {MIPI_CAL_DSIA_MIPI_CAL_CONFIG_2, 0x200002},
    {MIPI_CAL_DSIB_MIPI_CAL_CONFIG_2, 0x200002},
    {MIPI_CAL_CILA_MIPI_CAL_CONFIG, 0},
    {MIPI_CAL_CILB_MIPI_CAL_CONFIG, 0},
};

static const register_write_t display_config_mipi_cal_03_mariko[6] = {
    {MIPI_CAL_DSIA_MIPI_CAL_CONFIG, 0x200006},
    {MIPI_CAL_DSIB_MIPI_CAL_CONFIG, 0x200006},
    {MIPI_CAL_DSIA_MIPI_CAL_CONFIG_2, 0x260000},
    {MIPI_CAL_DSIB_MIPI_CAL_CONFIG_2, 0x260000},
    {MIPI_CAL_CILA_MIPI_CAL_CONFIG, 0},
    {MIPI_CAL_CILB_MIPI_CAL_CONFIG, 0},
};

static const register_write_t display_config_mipi_cal_04[10] = {
    {MIPI_CAL_CILC_MIPI_CAL_CONFIG, 0},
    {MIPI_CAL_CILD_MIPI_CAL_CONFIG, 0},
    {MIPI_CAL_CILE_MIPI_CAL_CONFIG, 0},
    {MIPI_CAL_CILF_MIPI_CAL_CONFIG, 0},
    {MIPI_CAL_DSIC_MIPI_CAL_CONFIG, 0},
    {MIPI_CAL_DSID_MIPI_CAL_CONFIG, 0},
    {MIPI_CAL_DSIB_MIPI_CAL_CONFIG_2, 0},
    {MIPI_CAL_DSIC_MIPI_CAL_CONFIG_2, 0},
    {MIPI_CAL_DSID_MIPI_CAL_CONFIG_2, 0},
    {MIPI_CAL_MIPI_CAL_CTRL, 0x2A000001},
};

static const register_write_t display_config_dc_02[113] = {
    {sizeof(uint32_t) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_DV_CONTROL, 0},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(uint32_t) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(uint32_t) * DC_WIN_CSC_KUR,   0},
    {sizeof(uint32_t) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(uint32_t) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(uint32_t) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(uint32_t) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(uint32_t) * DC_WIN_CSC_KVB,   0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_DV_CONTROL, 0},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(uint32_t) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(uint32_t) * DC_WIN_CSC_KUR,   0},
    {sizeof(uint32_t) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(uint32_t) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(uint32_t) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(uint32_t) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(uint32_t) * DC_WIN_CSC_KVB,   0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_DV_CONTROL, 0},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_CSC_YOF,   0xF0},
    {sizeof(uint32_t) * DC_WIN_CSC_KYRGB, 0x12A},
    {sizeof(uint32_t) * DC_WIN_CSC_KUR,   0},
    {sizeof(uint32_t) * DC_WIN_CSC_KVR,   0x198},
    {sizeof(uint32_t) * DC_WIN_CSC_KUG,   0x39B},
    {sizeof(uint32_t) * DC_WIN_CSC_KVG,   0x32F},
    {sizeof(uint32_t) * DC_WIN_CSC_KUB,   0x204},
    {sizeof(uint32_t) * DC_WIN_CSC_KVB,   0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE_888},
    {sizeof(uint32_t) * DC_DISP_DISP_INTERFACE_CONTROL, DISP_DATA_FORMAT_DF1P1C},
    {sizeof(uint32_t) * DC_COM_PIN_OUTPUT_POLARITY(1), 0x1000000},
    {sizeof(uint32_t) * DC_COM_PIN_OUTPUT_POLARITY(3), 0},
    {sizeof(uint32_t) * DC_DISP_BLEND_BACKGROUND_COLOR, 0},
    {sizeof(uint32_t) * DC_COM_CRC_CONTROL, 0},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WINBUF_BLEND_LAYER_CONTROL, 0x10000FF},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WINBUF_BLEND_LAYER_CONTROL, 0x10000FF},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WINBUF_BLEND_LAYER_CONTROL, 0x10000FF},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND_OPTION0, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND, 0},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE | WIN_B_UPDATE | WIN_C_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ | WIN_B_ACT_REQ | WIN_C_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_TIMING_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_REF_TO_SYNC, (1 << 16)},
    {sizeof(uint32_t) * DC_DISP_SYNC_WIDTH,  0x10048},
    {sizeof(uint32_t) * DC_DISP_BACK_PORCH,  0x90048},
    {sizeof(uint32_t) * DC_DISP_ACTIVE,      0x50002D0},
    {sizeof(uint32_t) * DC_DISP_FRONT_PORCH, 0xA0088},
    {sizeof(uint32_t) * DC_DISP_SHIFT_CLOCK_OPTIONS, SC1_H_QUALIFIER_NONE | SC0_H_QUALIFIER_NONE},
    {sizeof(uint32_t) * DC_COM_PIN_OUTPUT_ENABLE(1), 0},
    {sizeof(uint32_t) * DC_DISP_DATA_ENABLE_OPTIONS, DE_SELECT_ACTIVE | DE_CONTROL_NORMAL},
    {sizeof(uint32_t) * DC_DISP_DISP_INTERFACE_CONTROL, DISP_DATA_FORMAT_DF1P1C},
    {sizeof(uint32_t) * DC_DISP_DISP_CLOCK_CONTROL, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND_OPTION0, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND, DISP_CTRL_MODE_C_DISPLAY},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_STATE_ACCESS, READ_MUX | WRITE_MUX},
    {sizeof(uint32_t) * DC_DISP_FRONT_PORCH, 0xA0088},
    {sizeof(uint32_t) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_GENERAL_INCR_SYNCPT, 0x301},
    {sizeof(uint32_t) * DC_CMD_GENERAL_INCR_SYNCPT, 0x301},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_STATE_ACCESS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_CLOCK_CONTROL, PIXEL_CLK_DIVIDER_PCD1 | SHIFT_CLK_DIVIDER(4)},
    {sizeof(uint32_t) * DC_DISP_DISP_COLOR_CONTROL, BASE_COLOR_SIZE_888},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND_OPTION0, 0},
};

static const register_write_t display_config_frame_buffer[32] = {
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE},
    {sizeof(uint32_t) * DC_WIN_COLOR_DEPTH, WIN_COLOR_DEPTH_B8G8R8A8},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_WIN_POSITION, 0},
    {sizeof(uint32_t) * DC_WIN_H_INITIAL_DDA, 0},
    {sizeof(uint32_t) * DC_WIN_V_INITIAL_DDA, 0},
    {sizeof(uint32_t) * DC_WIN_PRESCALED_SIZE, V_PRESCALED_SIZE(1280) | H_PRESCALED_SIZE(2880)},
    {sizeof(uint32_t) * DC_WIN_DDA_INC, V_DDA_INC(0x1000) | H_DDA_INC(0x1000)},
    {sizeof(uint32_t) * DC_WIN_SIZE, V_SIZE(1280) | H_SIZE(720)},
    {sizeof(uint32_t) * DC_WIN_LINE_STRIDE, 0x6000C00},
    {sizeof(uint32_t) * DC_WIN_BUFFER_CONTROL, 0},
    {sizeof(uint32_t) * DC_WINBUF_SURFACE_KIND, 0},
    {sizeof(uint32_t) * DC_WINBUF_START_ADDR, display_config_frame_buffer_address},
    {sizeof(uint32_t) * DC_WINBUF_ADDR_H_OFFSET, 0},
    {sizeof(uint32_t) * DC_WINBUF_ADDR_V_OFFSET, 0},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, WIN_ENABLE},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND, DISP_CTRL_MODE_C_DISPLAY},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_UPDATE | WIN_A_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL, GENERAL_ACT_REQ | WIN_A_ACT_REQ},
};

static const register_write_t display_config_solid_color[8] = {
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_A_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_B_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_WINDOW_HEADER, WINDOW_C_SELECT},
    {sizeof(uint32_t) * DC_WIN_WIN_OPTIONS, 0},
    {sizeof(uint32_t) * DC_DISP_DISP_WIN_OPTIONS, DSI_ENABLE},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND, DISP_CTRL_MODE_C_DISPLAY},
};

static const register_write_t display_config_dc_01_fini_01[13] = {
    {sizeof(uint32_t) * DC_DISP_FRONT_PORCH,        0xA0088},
    {sizeof(uint32_t) * DC_CMD_INT_MASK,            0},
    {sizeof(uint32_t) * DC_CMD_STATE_ACCESS,        0},
    {sizeof(uint32_t) * DC_CMD_INT_ENABLE,          0},
    {sizeof(uint32_t) * DC_CMD_CONT_SYNCPT_VSYNC,   0},
    {sizeof(uint32_t) * DC_CMD_DISPLAY_COMMAND,     DISP_CTRL_MODE_STOP},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL,       GENERAL_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL,       GENERAL_ACT_REQ},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL,       GENERAL_UPDATE},
    {sizeof(uint32_t) * DC_CMD_GENERAL_INCR_SYNCPT, 0x301},
    {sizeof(uint32_t) * DC_CMD_GENERAL_INCR_SYNCPT, 0x301},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL,       GENERAL_UPDATE},
    {sizeof(uint32_t) * DC_CMD_STATE_CONTROL,       GENERAL_ACT_REQ},
};

static const register_write_t display_config_dsi_01_fini_01[2] = {
    {sizeof(uint32_t) * DSI_POWER_CONTROL, 0},
    {sizeof(uint32_t) * DSI_PAD_CONTROL_1, 0},
};

static const register_write_t display_config_dsi_01_fini_02[13] = {
    {sizeof(uint32_t) * DSI_PHY_TIMING_1,      0x40A0E05},
    {sizeof(uint32_t) * DSI_PHY_TIMING_2,      0x30109},
    {sizeof(uint32_t) * DSI_BTA_TIMING,        0x190A14},
    {sizeof(uint32_t) * DSI_TIMEOUT_0,         DSI_TIMEOUT_LRX(0x2000) | DSI_TIMEOUT_HTX(0xFFFF) },
    {sizeof(uint32_t) * DSI_TIMEOUT_1,         DSI_TIMEOUT_PR(0x765) | DSI_TIMEOUT_TA(0x2000)},
    {sizeof(uint32_t) * DSI_TO_TALLY,          0},
    {sizeof(uint32_t) * DSI_HOST_CONTROL,      DSI_HOST_CONTROL_CRC_RESET | DSI_HOST_CONTROL_TX_TRIG_HOST | DSI_HOST_CONTROL_CS | DSI_HOST_CONTROL_ECC},
    {sizeof(uint32_t) * DSI_CONTROL,           DSI_CONTROL_LANES(3) | DSI_CONTROL_HOST_ENABLE},
    {sizeof(uint32_t) * DSI_POWER_CONTROL,     DSI_POWER_CONTROL_ENABLE},
    {sizeof(uint32_t) * DSI_MAX_THRESHOLD,     0x40},
    {sizeof(uint32_t) * DSI_TRIGGER,           0},
    {sizeof(uint32_t) * DSI_TX_CRC,            0},
    {sizeof(uint32_t) * DSI_INIT_SEQ_CONTROL,  0}
};

static const dsi_sleep_or_register_write_t display_config_jdi_specific_fini_01[22] = {
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0x9483FFB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x2139},
    {0, DSI_WR_DATA, 0x191919D5},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0xB39},
    {0, DSI_WR_DATA, 0x4F0F41B1},
    {0, DSI_WR_DATA, 0xF179A433},
    {0, DSI_WR_DATA, 0x2D81},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0xB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
};

static const dsi_sleep_or_register_write_t display_config_auo_nx_abca2_specific_fini_01[38] = {
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0x9483FFB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x2C39},
    {0, DSI_WR_DATA, 0x191919D5},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x2C39},
    {0, DSI_WR_DATA, 0x191919D6},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_WR_DATA, 0x19191919},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0xB39},
    {0, DSI_WR_DATA, 0x711148B1},
    {0, DSI_WR_DATA, 0x71143209},
    {0, DSI_WR_DATA, 0x114D31},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0xB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
};

static const dsi_sleep_or_register_write_t display_config_innolux_nx_abcc_specific_fini_01[10] = {
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0x9483FFB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
    {0, DSI_WR_DATA, 0xB39},
    {0, DSI_WR_DATA, 0x751548B1},
    {0, DSI_WR_DATA, 0x71143209},
    {0, DSI_WR_DATA, 0x115631},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
};

static const dsi_sleep_or_register_write_t display_config_auo_nx_abcc_specific_fini_01[10] = {
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0x9483FFB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
    {0, DSI_WR_DATA, 0xB39},
    {0, DSI_WR_DATA, 0x711148B1},
    {0, DSI_WR_DATA, 0x71143209},
    {0, DSI_WR_DATA, 0x114D31},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
};

static const dsi_sleep_or_register_write_t display_config_40_nx_abcc_specific_fini_01[10] = {
    {0, DSI_WR_DATA, 0x439},
    {0, DSI_WR_DATA, 0x9483FFB9},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
    {0, DSI_WR_DATA, 0xB39},
    {0, DSI_WR_DATA, 0x731348B1},
    {0, DSI_WR_DATA, 0x71243209},
    {0, DSI_WR_DATA, 0x4C31},
    {0, DSI_TRIGGER, DSI_TRIGGER_HOST},
    {1, 0x5, 0},
};