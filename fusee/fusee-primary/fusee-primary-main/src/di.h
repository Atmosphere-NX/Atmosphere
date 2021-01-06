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

#ifndef FUSEE_DI_H_
#define FUSEE_DI_H_

#include <stdint.h>
#include <stdbool.h>

#define HOST1X_BASE 0x50000000
#define DI_BASE  0x54200000
#define DSI_BASE 0x54300000
#define VIC_BASE 0x54340000
#define MIPI_CAL_BASE 0x700E3000
#define MAKE_HOST1X_REG(n) MAKE_REG32(HOST1X_BASE + n)
#define MAKE_DI_REG(n) MAKE_REG32(DI_BASE + n * 4)
#define MAKE_DSI_REG(n) MAKE_REG32(DSI_BASE + n * 4)
#define MAKE_MIPI_CAL_REG(n) MAKE_REG32(MIPI_CAL_BASE + n)
#define MAKE_VIC_REG(n) MAKE_REG32(VIC_BASE + n)

/* Clock and reset registers. */
#define CLK_RST_CONTROLLER_CLK_SOURCE_DISP1     0x138
#define CLK_RST_CONTROLLER_PLLD_BASE            0xD0
#define CLK_RST_CONTROLLER_PLLD_MISC1           0xD8
#define CLK_RST_CONTROLLER_PLLD_MISC            0xDC

/* Display registers. */
#define DC_CMD_GENERAL_INCR_SYNCPT 0x00

#define DC_CMD_GENERAL_INCR_SYNCPT_CNTRL 0x01
#define  SYNCPT_CNTRL_NO_STALL   (1 << 8)
#define  SYNCPT_CNTRL_SOFT_RESET (1 << 0)

#define DC_CMD_CONT_SYNCPT_VSYNC 0x28
#define  SYNCPT_VSYNC_ENABLE (1 << 8)

#define DC_CMD_DISPLAY_COMMAND_OPTION0 0x031

#define DC_CMD_DISPLAY_COMMAND 0x32
#define  DISP_CTRL_MODE_STOP       (0 << 5)
#define  DISP_CTRL_MODE_C_DISPLAY  (1 << 5)
#define  DISP_CTRL_MODE_NC_DISPLAY (2 << 5)
#define  DISP_CTRL_MODE_MASK       (3 << 5)

#define DC_CMD_DISPLAY_POWER_CONTROL 0x36
#define  PW0_ENABLE (1 <<  0)
#define  PW1_ENABLE (1 <<  2)
#define  PW2_ENABLE (1 <<  4)
#define  PW3_ENABLE (1 <<  6)
#define  PW4_ENABLE (1 <<  8)
#define  PM0_ENABLE (1 << 16)
#define  PM1_ENABLE (1 << 18)

#define DC_CMD_INT_STATUS 0x37
#define DC_CMD_INT_MASK 0x38
#define DC_CMD_INT_ENABLE 0x39

#define DC_CMD_STATE_ACCESS 0x40
#define  READ_MUX  (1 << 0)
#define  WRITE_MUX (1 << 2)

#define DC_CMD_STATE_CONTROL 0x41
#define  GENERAL_ACT_REQ (1 <<  0)
#define  WIN_A_ACT_REQ   (1 <<  1)
#define  WIN_B_ACT_REQ   (1 <<  2)
#define  WIN_C_ACT_REQ   (1 <<  3)
#define  CURSOR_ACT_REQ  (1 <<  7)
#define  GENERAL_UPDATE  (1 <<  8)
#define  WIN_A_UPDATE    (1 <<  9)
#define  WIN_B_UPDATE    (1 << 10)
#define  WIN_C_UPDATE    (1 << 11)
#define  CURSOR_UPDATE   (1 << 15)
#define  NC_HOST_TRIG    (1 << 24)

#define DC_CMD_DISPLAY_WINDOW_HEADER 0x42
#define  WINDOW_A_SELECT (1 << 4)
#define  WINDOW_B_SELECT (1 << 5)
#define  WINDOW_C_SELECT (1 << 6)

#define DC_CMD_REG_ACT_CONTROL 0x043

#define DC_COM_CRC_CONTROL 0x300
#define DC_COM_PIN_OUTPUT_ENABLE(x) (0x302 + (x))
#define DC_COM_PIN_OUTPUT_POLARITY(x) (0x306 + (x))

#define DC_COM_DSC_TOP_CTL 0x33E

#define DC_DISP_DISP_WIN_OPTIONS 0x402
#define  HDMI_ENABLE     (1 << 30)
#define  DSI_ENABLE      (1 << 29)
#define  SOR1_TIMING_CYA (1 << 27)
#define  SOR1_ENABLE     (1 << 26)
#define  SOR_ENABLE      (1 << 25)
#define  CURSOR_ENABLE   (1 << 16)

#define DC_DISP_DISP_MEM_HIGH_PRIORITY 0x403
#define DC_DISP_DISP_MEM_HIGH_PRIORITY_TIMER 0x404
#define DC_DISP_DISP_TIMING_OPTIONS 0x405
#define DC_DISP_REF_TO_SYNC 0x406
#define DC_DISP_SYNC_WIDTH 0x407
#define DC_DISP_BACK_PORCH 0x408
#define DC_DISP_ACTIVE 0x409
#define DC_DISP_FRONT_PORCH 0x40A

#define DC_DISP_DISP_CLOCK_CONTROL 0x42E
#define  PIXEL_CLK_DIVIDER_PCD1  (0 << 8)
#define  PIXEL_CLK_DIVIDER_PCD1H (1 << 8)
#define  PIXEL_CLK_DIVIDER_PCD2  (2 << 8)
#define  PIXEL_CLK_DIVIDER_PCD3  (3 << 8)
#define  PIXEL_CLK_DIVIDER_PCD4  (4 << 8)
#define  PIXEL_CLK_DIVIDER_PCD6  (5 << 8)
#define  PIXEL_CLK_DIVIDER_PCD8  (6 << 8)
#define  PIXEL_CLK_DIVIDER_PCD9  (7 << 8)
#define  PIXEL_CLK_DIVIDER_PCD12 (8 << 8)
#define  PIXEL_CLK_DIVIDER_PCD16 (9 << 8)
#define  PIXEL_CLK_DIVIDER_PCD18 (10 << 8)
#define  PIXEL_CLK_DIVIDER_PCD24 (11 << 8)
#define  PIXEL_CLK_DIVIDER_PCD13 (12 << 8)
#define  SHIFT_CLK_DIVIDER(x)    ((x) & 0xff)

#define DC_DISP_DISP_INTERFACE_CONTROL 0x42F
#define  DISP_DATA_FORMAT_DF1P1C    (0 << 0)
#define  DISP_DATA_FORMAT_DF1P2C24B (1 << 0)
#define  DISP_DATA_FORMAT_DF1P2C18B (2 << 0)
#define  DISP_DATA_FORMAT_DF1P2C16B (3 << 0)
#define  DISP_DATA_FORMAT_DF2S      (4 << 0)
#define  DISP_DATA_FORMAT_DF3S      (5 << 0)
#define  DISP_DATA_FORMAT_DFSPI     (6 << 0)
#define  DISP_DATA_FORMAT_DF1P3C24B (7 << 0)
#define  DISP_DATA_FORMAT_DF1P3C18B (8 << 0)
#define  DISP_ALIGNMENT_MSB         (0 << 8)
#define  DISP_ALIGNMENT_LSB         (1 << 8)
#define  DISP_ORDER_RED_BLUE        (0 << 9)
#define  DISP_ORDER_BLUE_RED        (1 << 9)

#define DC_DISP_DISP_COLOR_CONTROL 0x430
#define  DITHER_CONTROL_MASK    (3 << 8)
#define  DITHER_CONTROL_DISABLE (0 << 8)
#define  DITHER_CONTROL_ORDERED (2 << 8)
#define  DITHER_CONTROL_ERRDIFF (3 << 8)
#define  BASE_COLOR_SIZE_MASK   (0xf << 0)
#define  BASE_COLOR_SIZE_666    (0 << 0)
#define  BASE_COLOR_SIZE_111    (1 << 0)
#define  BASE_COLOR_SIZE_222    (2 << 0)
#define  BASE_COLOR_SIZE_333    (3 << 0)
#define  BASE_COLOR_SIZE_444    (4 << 0)
#define  BASE_COLOR_SIZE_555    (5 << 0)
#define  BASE_COLOR_SIZE_565    (6 << 0)
#define  BASE_COLOR_SIZE_332    (7 << 0)
#define  BASE_COLOR_SIZE_888    (8 << 0)

#define DC_DISP_SHIFT_CLOCK_OPTIONS 0x431
#define  SC1_H_QUALIFIER_NONE   (1 << 16)
#define  SC0_H_QUALIFIER_NONE   (1 <<  0)

#define DC_DISP_DATA_ENABLE_OPTIONS 0x432
#define  DE_SELECT_ACTIVE_BLANK  (0 << 0)
#define  DE_SELECT_ACTIVE        (1 << 0)
#define  DE_SELECT_ACTIVE_IS     (2 << 0)
#define  DE_CONTROL_ONECLK       (0 << 2)
#define  DE_CONTROL_NORMAL       (1 << 2)
#define  DE_CONTROL_EARLY_EXT    (2 << 2)
#define  DE_CONTROL_EARLY        (3 << 2)
#define  DE_CONTROL_ACTIVE_BLANK (4 << 2)

#define DC_DISP_DC_MCCIF_FIFOCTRL 0x480
#define DC_DISP_BLEND_BACKGROUND_COLOR 0x4E4

#define DC_WIN_CSC_YOF 0x611
#define DC_WIN_CSC_KYRGB 0x612
#define DC_WIN_CSC_KUR 0x613
#define DC_WIN_CSC_KVR 0x614
#define DC_WIN_CSC_KUG 0x615
#define DC_WIN_CSC_KVG 0x616
#define DC_WIN_CSC_KUB 0x617
#define DC_WIN_CSC_KVB 0x618
#define DC_WIN_AD_WIN_OPTIONS 0xB80
#define DC_WIN_BD_WIN_OPTIONS 0xD80
#define DC_WIN_CD_WIN_OPTIONS 0xF80

/* The following registers are A/B/C shadows of the 0xB80/0xD80/0xF80 registers (see DISPLAY_WINDOW_HEADER). */
#define DC_WIN_WIN_OPTIONS 0x700
#define  H_DIRECTION  (1 <<  0)
#define  V_DIRECTION  (1 <<  2)
#define  COLOR_EXPAND (1 <<  6)
#define  CSC_ENABLE   (1 << 18)
#define  WIN_ENABLE   (1 << 30)

#define DC_WIN_COLOR_DEPTH 0x703
#define  WIN_COLOR_DEPTH_P1             0x0
#define  WIN_COLOR_DEPTH_P2             0x1
#define  WIN_COLOR_DEPTH_P4             0x2
#define  WIN_COLOR_DEPTH_P8             0x3
#define  WIN_COLOR_DEPTH_B4G4R4A4       0x4
#define  WIN_COLOR_DEPTH_B5G5R5A        0x5
#define  WIN_COLOR_DEPTH_B5G6R5         0x6
#define  WIN_COLOR_DEPTH_AB5G5R5        0x7
#define  WIN_COLOR_DEPTH_B8G8R8A8       0xC
#define  WIN_COLOR_DEPTH_R8G8B8A8       0xD
#define  WIN_COLOR_DEPTH_B6x2G6x2R6x2A8 0xE
#define  WIN_COLOR_DEPTH_R6x2G6x2B6x2A8 0xF
#define  WIN_COLOR_DEPTH_YCbCr422       0x10
#define  WIN_COLOR_DEPTH_YUV422         0x11
#define  WIN_COLOR_DEPTH_YCbCr420P      0x12
#define  WIN_COLOR_DEPTH_YUV420P        0x13
#define  WIN_COLOR_DEPTH_YCbCr422P      0x14
#define  WIN_COLOR_DEPTH_YUV422P        0x15
#define  WIN_COLOR_DEPTH_YCbCr422R      0x16
#define  WIN_COLOR_DEPTH_YUV422R        0x17
#define  WIN_COLOR_DEPTH_YCbCr422RA     0x18
#define  WIN_COLOR_DEPTH_YUV422RA       0x19

#define DC_WIN_BUFFER_CONTROL 0x702
#define DC_WIN_POSITION 0x704

#define DC_WIN_SIZE 0x705
#define  H_SIZE(x) (((x) & 0x1fff) <<  0)
#define  V_SIZE(x) (((x) & 0x1fff) << 16)

#define DC_WIN_PRESCALED_SIZE 0x706
#define  H_PRESCALED_SIZE(x) (((x) & 0x7fff) <<  0)
#define  V_PRESCALED_SIZE(x) (((x) & 0x1fff) << 16)

#define DC_WIN_H_INITIAL_DDA 0x707
#define DC_WIN_V_INITIAL_DDA 0x708

#define DC_WIN_DDA_INC 0x709
#define  H_DDA_INC(x) (((x) & 0xffff) <<  0)
#define  V_DDA_INC(x) (((x) & 0xffff) << 16)

#define DC_WIN_LINE_STRIDE 0x70A
#define DC_WIN_DV_CONTROL 0x70E
#define DC_WINBUF_BLEND_LAYER_CONTROL 0x716

/* The following registers are A/B/C shadows of the 0xBC0/0xDC0/0xFC0 registers (see DISPLAY_WINDOW_HEADER). */
#define DC_WINBUF_START_ADDR 0x800
#define DC_WINBUF_ADDR_H_OFFSET 0x806
#define DC_WINBUF_ADDR_V_OFFSET 0x808
#define DC_WINBUF_SURFACE_KIND 0x80B

/* Display serial interface registers. */
#define DSI_INCR_SYNCPT_CNTRL 0x1

#define DSI_RD_DATA 0x9
#define DSI_WR_DATA 0xA

#define DSI_POWER_CONTROL 0xB
#define  DSI_POWER_CONTROL_ENABLE 1

#define DSI_INT_ENABLE 0xC
#define DSI_INT_STATUS 0xD
#define DSI_INT_MASK 0xE

#define DSI_HOST_CONTROL 0xF
#define  DSI_HOST_CONTROL_FIFO_RESET   (1 << 21)
#define  DSI_HOST_CONTROL_CRC_RESET    (1 << 20)
#define  DSI_HOST_CONTROL_TX_TRIG_SOL  (0 << 12)
#define  DSI_HOST_CONTROL_TX_TRIG_FIFO (1 << 12)
#define  DSI_HOST_CONTROL_TX_TRIG_HOST (2 << 12)
#define  DSI_HOST_CONTROL_RAW          (1 << 6)
#define  DSI_HOST_CONTROL_HS           (1 << 5)
#define  DSI_HOST_CONTROL_FIFO_SEL     (1 << 4)
#define  DSI_HOST_CONTROL_IMM_BTA      (1 << 3)
#define  DSI_HOST_CONTROL_PKT_BTA      (1 << 2)
#define  DSI_HOST_CONTROL_CS           (1 << 1)
#define  DSI_HOST_CONTROL_ECC          (1 << 0)

#define DSI_CONTROL 0x10
#define  DSI_CONTROL_HS_CLK_CTRL  (1 << 20)
#define  DSI_CONTROL_CHANNEL(c)   (((c) & 0x3) << 16)
#define  DSI_CONTROL_FORMAT(f)    (((f) & 0x3) << 12)
#define  DSI_CONTROL_TX_TRIG(x)   (((x) & 0x3) <<  8)
#define  DSI_CONTROL_LANES(n)     (((n) & 0x3) <<  4)
#define  DSI_CONTROL_DCS_ENABLE   (1 << 3)
#define  DSI_CONTROL_SOURCE(s)    (((s) & 0x1) <<  2)
#define  DSI_CONTROL_VIDEO_ENABLE (1 << 1)
#define  DSI_CONTROL_HOST_ENABLE  (1 << 0)

#define DSI_SOL_DELAY 0x11
#define DSI_MAX_THRESHOLD 0x12

#define DSI_TRIGGER 0x13
#define  DSI_TRIGGER_HOST  (1 << 1)
#define  DSI_TRIGGER_VIDEO (1 << 0)

#define DSI_TX_CRC 0x14
#define DSI_STATUS 0x15
#define DSI_INIT_SEQ_CONTROL 0x1A
#define DSI_INIT_SEQ_DATA_0 0x1B
#define DSI_INIT_SEQ_DATA_1 0x1C
#define DSI_INIT_SEQ_DATA_2 0x1D
#define DSI_INIT_SEQ_DATA_3 0x1E
#define DSI_PKT_SEQ_0_LO 0x23
#define DSI_PKT_SEQ_0_HI 0x24
#define DSI_PKT_SEQ_1_LO 0x25
#define DSI_PKT_SEQ_1_HI 0x26
#define DSI_PKT_SEQ_2_LO 0x27
#define DSI_PKT_SEQ_2_HI 0x28
#define DSI_PKT_SEQ_3_LO 0x29
#define DSI_PKT_SEQ_3_HI 0x2A
#define DSI_PKT_SEQ_4_LO 0x2B
#define DSI_PKT_SEQ_4_HI 0x2C
#define DSI_PKT_SEQ_5_LO 0x2D
#define DSI_PKT_SEQ_5_HI 0x2E
#define DSI_DCS_CMDS 0x33
#define DSI_PKT_LEN_0_1 0x34
#define DSI_PKT_LEN_2_3 0x35
#define DSI_PKT_LEN_4_5 0x36
#define DSI_PKT_LEN_6_7 0x37
#define DSI_PHY_TIMING_0 0x3C
#define DSI_PHY_TIMING_1 0x3D
#define DSI_PHY_TIMING_2 0x3E
#define DSI_BTA_TIMING 0x3F

#define DSI_TIMEOUT_0 0x44
#define  DSI_TIMEOUT_LRX(x) (((x) & 0xffff) << 16)
#define  DSI_TIMEOUT_HTX(x) (((x) & 0xffff) <<  0)

#define DSI_TIMEOUT_1 0x45
#define  DSI_TIMEOUT_PR(x) (((x) & 0xffff) << 16)
#define  DSI_TIMEOUT_TA(x) (((x) & 0xffff) <<  0)

#define DSI_TO_TALLY 0x46

#define DSI_PAD_CONTROL_0 0x4B
#define  DSI_PAD_CONTROL_VS1_PULLDN_CLK (1 << 24)
#define  DSI_PAD_CONTROL_VS1_PULLDN(x)  (((x) & 0xf) << 16)
#define  DSI_PAD_CONTROL_VS1_PDIO_CLK   (1 <<  8)
#define  DSI_PAD_CONTROL_VS1_PDIO(x)    (((x) & 0xf) <<  0)

#define DSI_PAD_CONTROL_CD 0x4C
#define DSI_VIDEO_MODE_CONTROL 0x4E

#define DSI_PAD_CONTROL_1 0x4F
#define DSI_PAD_CONTROL_2 0x50

#define DSI_PAD_CONTROL_3 0x51
#define  DSI_PAD_PREEMP_PD_CLK(x) (((x) & 0x3) << 12)
#define  DSI_PAD_PREEMP_PU_CLK(x) (((x) & 0x3) << 8)
#define  DSI_PAD_PREEMP_PD(x)     (((x) & 0x3) << 4)
#define  DSI_PAD_PREEMP_PU(x)     (((x) & 0x3) << 0)

#define DSI_PAD_CONTROL_4 0x52
#define DSI_PAD_CONTROL_5_MARIKO 0x53
#define DSI_PAD_CONTROL_6_MARIKO 0x54
#define DSI_PAD_CONTROL_7_MARIKO 0x55
#define DSI_INIT_SEQ_DATA_15 0x5F
#define DSI_INIT_SEQ_DATA_15_MARIKO 0x62

/* MIPI calibration registers. */
#define MIPI_CAL_MIPI_CAL_CTRL 0x0
#define MIPI_CAL_MIPI_CAL_AUTOCAL_CTRL0 0x4
#define MIPI_CAL_CIL_MIPI_CAL_STATUS 0x8
#define MIPI_CAL_CIL_MIPI_CAL_STATUS_2 0xC
#define MIPI_CAL_CILA_MIPI_CAL_CONFIG 0x14
#define MIPI_CAL_CILB_MIPI_CAL_CONFIG 0x18
#define MIPI_CAL_CILC_MIPI_CAL_CONFIG 0x1C
#define MIPI_CAL_CILD_MIPI_CAL_CONFIG 0x20
#define MIPI_CAL_CILE_MIPI_CAL_CONFIG 0x24
#define MIPI_CAL_CILF_MIPI_CAL_CONFIG 0x28
#define MIPI_CAL_DSIA_MIPI_CAL_CONFIG 0x38
#define MIPI_CAL_DSIB_MIPI_CAL_CONFIG 0x3C
#define MIPI_CAL_DSIC_MIPI_CAL_CONFIG 0x40
#define MIPI_CAL_DSID_MIPI_CAL_CONFIG 0x44
#define MIPI_CAL_MIPI_BIAS_PAD_CFG0 0x58
#define MIPI_CAL_MIPI_BIAS_PAD_CFG1 0x5C
#define MIPI_CAL_MIPI_BIAS_PAD_CFG2 0x60
#define MIPI_CAL_DSIA_MIPI_CAL_CONFIG_2 0x64
#define MIPI_CAL_DSIB_MIPI_CAL_CONFIG_2 0x68
#define MIPI_CAL_DSIC_MIPI_CAL_CONFIG_2 0x70
#define MIPI_CAL_DSID_MIPI_CAL_CONFIG_2 0x74

void display_init(void);
void display_end(void);

/* Switches screen backlight ON/OFF. */
void display_backlight(bool enable);

/* Show one single color on the display. */
void display_color_screen(uint32_t color);

/* Init display in full 1280x720 resolution (B8G8R8A8, line stride 768, framebuffer size = 1280*768*4 bytes). */
uint32_t *display_init_framebuffer(void *address);

#endif
