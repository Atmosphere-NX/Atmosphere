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

struct Definition {
    u32 reg_offset;
    u32 mask_val;
    u32 pm_val;
};

struct DrivePadDefinition {
    u32 reg_offset;
    u32 mask_val;
};

constexpr Definition Map[] = {
    {0x00003000, 0x72FF, 0x01},   /* Sdmmc1Clk */
    {0x00003004, 0x72FF, 0x02},   /* Sdmmc1Cmd */
    {0x00003008, 0x72FF, 0x02},   /* Sdmmc1Dat3 */
    {0x0000300C, 0x72FF, 0x02},   /* Sdmmc1Dat2 */
    {0x00003010, 0x72FF, 0x02},   /* Sdmmc1Dat1 */
    {0x00003014, 0x72FF, 0x01},   /* Sdmmc1Dat0 */
    {0x0000301C, 0x72FF, 0x01},   /* Sdmmc3Clk */
    {0x00003020, 0x72FF, 0x01},   /* Sdmmc3Cmd */
    {0x00003024, 0x72FF, 0x01},   /* Sdmmc3Dat0 */
    {0x00003028, 0x72FF, 0x01},   /* Sdmmc3Dat1 */
    {0x0000302C, 0x72FF, 0x01},   /* Sdmmc3Dat2 */
    {0x00003030, 0x72FF, 0x01},   /* Sdmmc3Dat3 */
    {0x00003038, 0x1DFF, 0x01},   /* PexL0RstN */
    {0x0000303C, 0x1DFF, 0x01},   /* PexL0ClkreqN */
    {0x00003040, 0x1DFF, 0x01},   /* PexWakeN */
    {0x00003044, 0x1DFF, 0x01},   /* PexL1RstN */
    {0x00003048, 0x1DFF, 0x01},   /* PexL1ClkreqN */
    {0x0000304C, 0x19FF, 0x01},   /* SataLedActive */
    {0x00003050, 0x1F2FF, 0x01},  /* Spi1Mosi */
    {0x00003054, 0x1F2FF, 0x01},  /* Spi1Miso */
    {0x00003058, 0x1F2FF, 0x01},  /* Spi1Sck */
    {0x0000305C, 0x1F2FF, 0x01},  /* Spi1Cs0 */
    {0x00003060, 0x1F2FF, 0x01},  /* Spi1Cs1 */
    {0x00003064, 0x72FF, 0x02},   /* Spi2Mosi */
    {0x00003068, 0x72FF, 0x02},   /* Spi2Miso */
    {0x0000306C, 0x72FF, 0x02},   /* Spi2Sck */
    {0x00003070, 0x72FF, 0x02},   /* Spi2Cs0 */
    {0x00003074, 0x72FF, 0x01},   /* Spi2Cs1 */
    {0x00003078, 0x1F2FF, 0x01},  /* Spi4Mosi */
    {0x0000307C, 0x1F2FF, 0x01},  /* Spi4Miso */
    {0x00003080, 0x1F2FF, 0x01},  /* Spi4Sck */
    {0x00003084, 0x1F2FF, 0x01},  /* Spi4Cs0 */
    {0x00003088, 0x72FF, 0x01},   /* QspiSck */
    {0x0000308C, 0x72FF, 0x01},   /* QspiCsN */
    {0x00003090, 0x72FF, 0x01},   /* QspiIo0 */
    {0x00003094, 0x72FF, 0x01},   /* QspiIo1 */
    {0x00003098, 0x72FF, 0x01},   /* QspiIo2 */
    {0x0000309C, 0x72FF, 0x01},   /* QspiIo3 */
    {0x000030A4, 0x19FF, 0x02},   /* Dmic1Clk */
    {0x000030A8, 0x19FF, 0x02},   /* Dmic1Dat */
    {0x000030AC, 0x19FF, 0x02},   /* Dmic2Clk */
    {0x000030B0, 0x19FF, 0x02},   /* Dmic2Dat */
    {0x000030B4, 0x19FF, 0x02},   /* Dmic3Clk */
    {0x000030B8, 0x19FF, 0x02},   /* Dmic3Dat */
    {0x000030BC, 0x1DFF, 0x01},   /* Gen1I2cScl */
    {0x000030C0, 0x1DFF, 0x01},   /* Gen1I2cSda */
    {0x000030C4, 0x1DFF, 0x01},   /* Gen2I2cScl */
    {0x000030C8, 0x1DFF, 0x01},   /* Gen2I2cSda */
    {0x000030CC, 0x1DFF, 0x01},   /* Gen3I2cScl */
    {0x000030D0, 0x1DFF, 0x01},   /* Gen3I2cSda */
    {0x000030D4, 0x1DFF, 0x02},   /* CamI2cScl */
    {0x000030D8, 0x1DFF, 0x02},   /* CamI2cSda */
    {0x000030DC, 0x1DFF, 0x01},   /* PwrI2cScl */
    {0x000030E0, 0x1DFF, 0x01},   /* PwrI2cSda */
    {0x000030E4, 0x19FF, 0x01},   /* Uart1Tx */
    {0x000030E8, 0x19FF, 0x01},   /* Uart1Rx */
    {0x000030EC, 0x19FF, 0x01},   /* Uart1Rts */
    {0x000030F0, 0x19FF, 0x01},   /* Uart1Cts */
    {0x000030F4, 0x19FF, 0x00},   /* Uart2Tx */
    {0x000030F8, 0x19FF, 0x00},   /* Uart2Rx */
    {0x000030FC, 0x19FF, 0x02},   /* Uart2Rts */
    {0x00003100, 0x19FF, 0x02},   /* Uart2Cts */
    {0x00003104, 0x19FF, 0x02},   /* Uart3Tx */
    {0x00003108, 0x19FF, 0x02},   /* Uart3Rx */
    {0x0000310C, 0x19FF, 0x02},   /* Uart3Rts */
    {0x00003110, 0x19FF, 0x02},   /* Uart3Cts */
    {0x00003114, 0x19FF, 0x02},   /* Uart4Tx */
    {0x00003118, 0x19FF, 0x02},   /* Uart4Rx */
    {0x0000311C, 0x19FF, 0x02},   /* Uart4Rts */
    {0x00003120, 0x19FF, 0x02},   /* Uart4Cts */
    {0x00003124, 0x72FF, 0x01},   /* Dap1Fs */
    {0x00003128, 0x72FF, 0x01},   /* Dap1Din */
    {0x0000312C, 0x72FF, 0x01},   /* Dap1Dout */
    {0x00003130, 0x72FF, 0x01},   /* Dap1Sclk */
    {0x00003134, 0x72FF, 0x01},   /* Dap2Fs */
    {0x00003138, 0x72FF, 0x01},   /* Dap2Din */
    {0x0000313C, 0x72FF, 0x01},   /* Dap2Dout */
    {0x00003140, 0x72FF, 0x01},   /* Dap2Sclk */
    {0x00003144, 0x72FF, 0x01},   /* Dap4Fs */
    {0x00003148, 0x72FF, 0x01},   /* Dap4Din */
    {0x0000314C, 0x72FF, 0x01},   /* Dap4Dout */
    {0x00003150, 0x72FF, 0x01},   /* Dap4Sclk */
    {0x00003154, 0x72FF, 0x01},   /* Cam1Mclk */
    {0x00003158, 0x72FF, 0x01},   /* Cam2Mclk */
    {0x0000315C, 0x72FF, 0x01},   /* JtagRtck */
    {0x00003160, 0x118C, 0xFF},   /* Clk32kIn */
    {0x00003164, 0x72FF, 0x02},   /* Clk32kOut */
    {0x00003168, 0x1DFF, 0x01},   /* BattBcl */
    {0x0000316C, 0x11CC, 0xFF},   /* ClkReq */
    {0x00003170, 0x11CC, 0xFF},   /* CpuPwrReq */
    {0x00003174, 0x11CC, 0xFF},   /* PwrIntN */
    {0x00003178, 0x11CC, 0xFF},   /* Shutdown */
    {0x0000317C, 0x11CC, 0xFF},   /* CorePwrReq */
    {0x00003180, 0x19FF, 0x01},   /* AudMclk */
    {0x00003184, 0x19FF, 0x00},   /* DvfsPwm */
    {0x00003188, 0x19FF, 0x00},   /* DvfsClk */
    {0x0000318C, 0x19FF, 0x00},   /* GpioX1Aud */
    {0x00003190, 0x19FF, 0x00},   /* GpioX3Aud */
    {0x00003194, 0x1DFF, 0x00},   /* GpioPcc7 */
    {0x00003198, 0x1DFF, 0x01},   /* HdmiCec */
    {0x0000319C, 0x1DFF, 0x01},   /* HdmiIntDpHpd */
    {0x000031A0, 0x19FF, 0x01},   /* SpdifOut */
    {0x000031A4, 0x19FF, 0x01},   /* SpdifIn */
    {0x000031A8, 0x1DFF, 0x01},   /* UsbVbusEn0 */
    {0x000031AC, 0x1DFF, 0x01},   /* UsbVbusEn1 */
    {0x000031B0, 0x19FF, 0x01},   /* DpHpd0 */
    {0x000031B4, 0x19FF, 0x00},   /* WifiEn */
    {0x000031B8, 0x19FF, 0x00},   /* WifiRst */
    {0x000031BC, 0x19FF, 0x00},   /* WifiWakeAp */
    {0x000031C0, 0x19FF, 0x00},   /* ApWakeBt */
    {0x000031C4, 0x19FF, 0x00},   /* BtRst */
    {0x000031C8, 0x19FF, 0x00},   /* BtWakeAp */
    {0x000031CC, 0x19FF, 0x00},   /* ApWakeNfc */
    {0x000031D0, 0x19FF, 0x00},   /* NfcEn */
    {0x000031D4, 0x19FF, 0x00},   /* NfcInt */
    {0x000031D8, 0x19FF, 0x00},   /* GpsEn */
    {0x000031DC, 0x19FF, 0x00},   /* GpsRst */
    {0x000031E0, 0x19FF, 0x01},   /* CamRst */
    {0x000031E4, 0x19FF, 0x02},   /* CamAfEn */
    {0x000031E8, 0x19FF, 0x02},   /* CamFlashEn */
    {0x000031EC, 0x19FF, 0x01},   /* Cam1Pwdn */
    {0x000031F0, 0x19FF, 0x01},   /* Cam2Pwdn */
    {0x000031F4, 0x19FF, 0x01},   /* Cam1Strobe */
    {0x000031F8, 0x19FF, 0x01},   /* LcdTe */
    {0x000031FC, 0x19FF, 0x03},   /* LcdBlPwm */
    {0x00003200, 0x19FF, 0x00},   /* LcdBlEn */
    {0x00003204, 0x19FF, 0x00},   /* LcdRst */
    {0x00003208, 0x19FF, 0x01},   /* LcdGpio1 */
    {0x0000320C, 0x19FF, 0x02},   /* LcdGpio2 */
    {0x00003210, 0x19FF, 0x00},   /* ApReady */
    {0x00003214, 0x19FF, 0x00},   /* TouchRst */
    {0x00003218, 0x19FF, 0x01},   /* TouchClk */
    {0x0000321C, 0x19FF, 0x00},   /* ModemWakeAp */
    {0x00003220, 0x19FF, 0x00},   /* TouchInt */
    {0x00003224, 0x19FF, 0x00},   /* MotionInt */
    {0x00003228, 0x19FF, 0x00},   /* AlsProxInt */
    {0x0000322C, 0x19FF, 0x00},   /* TempAlert */
    {0x00003230, 0x19FF, 0x00},   /* ButtonPowerOn */
    {0x00003234, 0x19FF, 0x00},   /* ButtonVolUp */
    {0x00003238, 0x19FF, 0x00},   /* ButtonVolDown */
    {0x0000323C, 0x19FF, 0x00},   /* ButtonSlideSw */
    {0x00003240, 0x19FF, 0x00},   /* ButtonHome */
    {0x00003244, 0x19FF, 0x01},   /* GpioPa6 */
    {0x00003248, 0x19FF, 0x00},   /* GpioPe6 */
    {0x0000324C, 0x19FF, 0x00},   /* GpioPe7 */
    {0x00003250, 0x19FF, 0x00},   /* GpioPh6 */
    {0x00003254, 0x72FF, 0x02},   /* GpioPk0 */
    {0x00003258, 0x72FF, 0x02},   /* GpioPk1 */
    {0x0000325C, 0x72FF, 0x02},   /* GpioPk2 */
    {0x00003260, 0x72FF, 0x02},   /* GpioPk3 */
    {0x00003264, 0x72FF, 0x01},   /* GpioPk4 */
    {0x00003268, 0x72FF, 0x01},   /* GpioPk5 */
    {0x0000326C, 0x72FF, 0x01},   /* GpioPk6 */
    {0x00003270, 0x72FF, 0x01},   /* GpioPk7 */
    {0x00003274, 0x72FF, 0x00},   /* GpioPl0 */
    {0x00003278, 0x72FF, 0x01},   /* GpioPl1 */
    {0x0000327C, 0x72FF, 0x01},   /* GpioPz0 */
    {0x00003280, 0x72FF, 0x02},   /* GpioPz1 */
    {0x00003284, 0x72FF, 0x02},   /* GpioPz2 */
    {0x00003288, 0x72FF, 0x01},   /* GpioPz3 */
    {0x0000328C, 0x72FF, 0x01},   /* GpioPz4 */
    {0x00003290, 0x72FF, 0x01},   /* GpioPz5 */

    /* 5.0.0+ only */
    {0x00003294, 0x1F2FF, 0x02},   /* Sdmmc2Dat0 */
    {0x00003298, 0x1F2FF, 0x02},   /* Sdmmc2Dat1 */
    {0x0000329C, 0x1F2FF, 0x02},   /* Sdmmc2Dat2 */
    {0x000032A0, 0x1F2FF, 0x02},   /* Sdmmc2Dat3 */
    {0x000032A4, 0x1F2FF, 0x02},   /* Sdmmc2Dat4 */
    {0x000032A8, 0x1F2FF, 0x02},   /* Sdmmc2Dat5 */
    {0x000032AC, 0x1F2FF, 0x02},   /* Sdmmc2Dat6 */
    {0x000032B0, 0x1F2FF, 0x02},   /* Sdmmc2Dat7 */
    {0x000032B4, 0x1F2FF, 0x02},   /* Sdmmc2Clk */
    {0x000032B8, 0x1F2FF, 0x00},   /* Sdmmc2Clkb */
    {0x000032BC, 0x1F2FF, 0x02},   /* Sdmmc2Cmd */
    {0x000032C0, 0x1F2FF, 0x00},   /* Sdmmc2Dqs */
    {0x000032C4, 0x1F2FF, 0x00},   /* Sdmmc2Dqsb */
};

constexpr u32 PadNameMax = util::size(Map);

constexpr DrivePadDefinition DrivePadMap[] = {
    {0x000008E4, 0x01F1F000},   /* AlsProxInt */
    {0x000008E8, 0x01F1F000},   /* ApReady */
    {0x000008EC, 0x01F1F000},   /* ApWakeBt */
    {0x000008F0, 0x01F1F000},   /* ApWakeNfc */
    {0x000008F4, 0x01F1F000},   /* AudMclk */
    {0x000008F8, 0x01F1F000},   /* BattBcl */
    {0x000008FC, 0x01F1F000},   /* BtRst */
    {0x00000900, 0x01F1F000},   /* BtWakeAp */
    {0x00000904, 0x01F1F000},   /* ButtonHome */
    {0x00000908, 0x01F1F000},   /* ButtonPowerOn */
    {0x0000090C, 0x01F1F000},   /* ButtonSlideSw */
    {0x00000910, 0x01F1F000},   /* ButtonVolDown */
    {0x00000914, 0x01F1F000},   /* ButtonVolUp */
    {0x00000918, 0x01F1F000},   /* Cam1Mclk */
    {0x0000091C, 0x01F1F000},   /* Cam1Pwdn */
    {0x00000920, 0x01F1F000},   /* Cam1Strobe */
    {0x00000924, 0x01F1F000},   /* Cam2Mclk */
    {0x00000928, 0x01F1F000},   /* Cam2Pwdn */
    {0x0000092C, 0x01F1F000},   /* CamAfEn */
    {0x00000930, 0x01F1F000},   /* CamFlashEn */
    {0x00000934, 0x01F1F000},   /* CamI2cScl */
    {0x00000938, 0x01F1F000},   /* CamI2cSda */
    {0x0000093C, 0x01F1F000},   /* CamRst */
    {0x00000940, 0x01F1F000},   /* Clk32kIn */
    {0x00000944, 0x01F1F000},   /* Clk32kOut */
    {0x00000948, 0x01F1F000},   /* ClkReq */
    {0x0000094C, 0x01F1F000},   /* CorePwrReq */
    {0x00000950, 0x01F1F000},   /* CpuPwrReq */
    {0x00000954, 0xF0000000},   /* Dap1Din */
    {0x00000958, 0xF0000000},   /* Dap1Dout */
    {0x0000095C, 0xF0000000},   /* Dap1Fs */
    {0x00000960, 0xF0000000},   /* Dap1Sclk */
    {0x00000964, 0xF0000000},   /* Dap2Din */
    {0x00000968, 0xF0000000},   /* Dap2Dout */
    {0x0000096C, 0xF0000000},   /* Dap2Fs */
    {0x00000970, 0xF0000000},   /* Dap2Sclk */
    {0x00000974, 0x01F1F000},   /* Dap4Din */
    {0x00000978, 0x01F1F000},   /* Dap4Dout */
    {0x0000097C, 0x01F1F000},   /* Dap4Fs */
    {0x00000980, 0x01F1F000},   /* Dap4Sclk */
    {0x00000984, 0x01F1F000},   /* Dmic1Clk */
    {0x00000988, 0x01F1F000},   /* Dmic1Dat */
    {0x0000098C, 0x01F1F000},   /* Dmic2Clk */
    {0x00000990, 0x01F1F000},   /* Dmic2Dat */
    {0x00000994, 0x01F1F000},   /* Dmic3Clk */
    {0x00000998, 0x01F1F000},   /* Dmic3Dat */
    {0x0000099C, 0x01F1F000},   /* DpHpd */
    {0x000009A0, 0x01F1F000},   /* DvfsClk */
    {0x000009A4, 0x01F1F000},   /* DvfsPwm */
    {0x000009A8, 0x01F1F000},   /* Gen1I2cScl */
    {0x000009AC, 0x01F1F000},   /* Gen1I2cSda */
    {0x000009B0, 0x01F1F000},   /* Gen2I2cScl */
    {0x000009B4, 0x01F1F000},   /* Gen2I2cSda */
    {0x000009B8, 0x01F1F000},   /* Gen3I2cScl */
    {0x000009BC, 0x01F1F000},   /* Gen3I2cSda */
    {0x000009C0, 0x01F1F000},   /* GpioPa6 */
    {0x000009C4, 0x01F1F000},   /* GpioPcc7 */
    {0x000009C8, 0x01F1F000},   /* GpioPe6 */
    {0x000009CC, 0x01F1F000},   /* GpioPe7 */
    {0x000009D0, 0x01F1F000},   /* GpioPh6 */
    {0x000009D4, 0xF0000000},   /* GpioPk0 */
    {0x000009D8, 0xF0000000},   /* GpioPk1 */
    {0x000009DC, 0xF0000000},   /* GpioPk2 */
    {0x000009E0, 0xF0000000},   /* GpioPk3 */
    {0x000009E4, 0xF0000000},   /* GpioPk4 */
    {0x000009E8, 0xF0000000},   /* GpioPk5 */
    {0x000009EC, 0xF0000000},   /* GpioPk6 */
    {0x000009F0, 0xF0000000},   /* GpioPk7 */
    {0x000009F4, 0xF0000000},   /* GpioPl0 */
    {0x000009F8, 0xF0000000},   /* GpioPl1 */
    {0x000009FC, 0x07F7F000},   /* GpioPz0 */
    {0x00000A00, 0x07F7F000},   /* GpioPz1 */
    {0x00000A04, 0x07F7F000},   /* GpioPz2 */
    {0x00000A08, 0x07F7F000},   /* GpioPz3 */
    {0x00000A0C, 0x07F7F000},   /* GpioPz4 */
    {0x00000A10, 0x07F7F000},   /* GpioPz5 */
    {0x00000A14, 0x01F1F000},   /* GpioX1Aud */
    {0x00000A18, 0x01F1F000},   /* GpioX3Aud */
    {0x00000A1C, 0x01F1F000},   /* GpsEn */
    {0x00000A20, 0x01F1F000},   /* GpsRst */
    {0x00000A24, 0x01F1F000},   /* HdmiCec */
    {0x00000A28, 0x01F1F000},   /* HdmiIntDpHpd */
    {0x00000A2C, 0x01F1F000},   /* JtagRtck */
    {0x00000A30, 0x01F1F000},   /* LcdBlEn */
    {0x00000A34, 0x01F1F000},   /* LcdBlPwm */
    {0x00000A38, 0x01F1F000},   /* LcdGpio1 */
    {0x00000A3C, 0x01F1F000},   /* LcdGpio2 */
    {0x00000A40, 0x01F1F000},   /* LcdRst */
    {0x00000A44, 0x01F1F000},   /* LcdTe */
    {0x00000A48, 0x01F1F000},   /* ModemWakeAp */
    {0x00000A4C, 0x01F1F000},   /* MotionInt */
    {0x00000A50, 0x01F1F000},   /* NfcEn */
    {0x00000A54, 0x01F1F000},   /* NfcInt */
    {0x00000A58, 0x01F1F000},   /* PexL0ClkReqN */
    {0x00000A5C, 0x01F1F000},   /* PexL0RstN */
    {0x00000A60, 0x01F1F000},   /* PexL1ClkreqN */
    {0x00000A64, 0x01F1F000},   /* PexL1RstN */
    {0x00000A68, 0x01F1F000},   /* PexWakeN */
    {0x00000A6C, 0x01F1F000},   /* PwrI2cScl */
    {0x00000A70, 0x01F1F000},   /* PwrI2cSda */
    {0x00000A74, 0x01F1F000},   /* PwrIntN */
    {0x00000A78, 0x07F7F000},   /* QspiComp */
    {0x00000A90, 0xF0000000},   /* QspiSck */
    {0x00000A94, 0x01F1F000},   /* SataLedActive */
    {0x00000A98, 0xF7F7F000},   /* Sdmmc1Pad */
    {0x00000AB0, 0xF7F7F000},   /* Sdmmc3Pad */
    {0x00000AC8, 0x01F1F000},   /* Shutdown */
    {0x00000ACC, 0x01F1F000},   /* SpdifIn */
    {0x00000AD0, 0x01F1F000},   /* SpdifOut */
    {0x00000AD4, 0xF0000000},   /* Spi1Cs0 */
    {0x00000AD8, 0xF0000000},   /* Spi1Cs1 */
    {0x00000ADC, 0xF0000000},   /* Spi1Miso */
    {0x00000AE0, 0xF0000000},   /* Spi1Mosi */
    {0x00000AE4, 0xF0000000},   /* Spi1Sck */
    {0x00000AE8, 0xF0000000},   /* Spi2Cs0 */
    {0x00000AEC, 0xF0000000},   /* Spi2Cs1 */
    {0x00000AF0, 0xF0000000},   /* Spi2Miso */
    {0x00000AF4, 0xF0000000},   /* Spi2Mosi */
    {0x00000AF8, 0xF0000000},   /* Spi2Sck */
    {0x00000AFC, 0xF0000000},   /* Spi4Cs0 */
    {0x00000B00, 0xF0000000},   /* Spi4Miso */
    {0x00000B04, 0xF0000000},   /* Spi4Mosi */
    {0x00000B08, 0xF0000000},   /* Spi4Sck */
    {0x00000B0C, 0x01F1F000},   /* TempAlert */
    {0x00000B10, 0x01F1F000},   /* TouchClk */
    {0x00000B14, 0x01F1F000},   /* TouchInt */
    {0x00000B18, 0x01F1F000},   /* TouchRst */
    {0x00000B1C, 0x01F1F000},   /* Uart1Cts */
    {0x00000B20, 0x01F1F000},   /* Uart1Rts */
    {0x00000B24, 0x01F1F000},   /* Uart1Rx */
    {0x00000B28, 0x01F1F000},   /* Uart1Tx */
    {0x00000B2C, 0x01F1F000},   /* Uart2Cts */
    {0x00000B30, 0x01F1F000},   /* Uart2Rts */
    {0x00000B34, 0x01F1F000},   /* Uart2Rx */
    {0x00000B38, 0x01F1F000},   /* Uart2Tx */
    {0x00000B3C, 0x01F1F000},   /* Uart3Cts */
    {0x00000B40, 0x01F1F000},   /* Uart3Rts */
    {0x00000B44, 0x01F1F000},   /* Uart3Rx */
    {0x00000B48, 0x01F1F000},   /* Uart3Tx */
    {0x00000B4C, 0x01F1F000},   /* Uart4Cts */
    {0x00000B50, 0x01F1F000},   /* Uart4Rts */
    {0x00000B54, 0x01F1F000},   /* Uart4Rx */
    {0x00000B58, 0x01F1F000},   /* Uart4Tx */
    {0x00000B5C, 0x01F1F000},   /* UsbVbusEn0 */
    {0x00000B60, 0x01F1F000},   /* UsbVbusEn1 */
    {0x00000B64, 0x01F1F000},   /* WifiEn */
    {0x00000B68, 0x01F1F000},   /* WifiRst */
    {0x00000B6C, 0x01F1F000},   /* WifiWakeAp */
};

constexpr u32 DrivePadNameMax = util::size(DrivePadMap);
