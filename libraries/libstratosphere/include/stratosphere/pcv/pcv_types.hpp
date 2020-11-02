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
#pragma once
#include <vapours.hpp>

namespace ams::pcv {

    using ClockHz   = u32;
    using MicroVolt = s32;
    using MilliC    = s32;

    /* TODO: Device codes? */
    enum Module {
        Module_Cpu             =  0,
        Module_Gpu             =  1,
        Module_I2s1            =  2,
        Module_I2s2            =  3,
        Module_I2s3            =  4,
        Module_Pwm             =  5,
        Module_I2c1            =  6,
        Module_I2c2            =  7,
        Module_I2c3            =  8,
        Module_I2c4            =  9,
        Module_I2c5            = 10,
        Module_I2c6            = 11,
        Module_Spi1            = 12,
        Module_Spi2            = 13,
        Module_Spi3            = 14,
        Module_Spi4            = 15,
        Module_Disp1           = 16,
        Module_Disp2           = 17,
        Module_Isp             = 18,
        Module_Vi              = 19,
        Module_Sdmmc1          = 20,
        Module_Sdmmc2          = 21,
        Module_Sdmmc3          = 22,
        Module_Sdmmc4          = 23,
        Module_Owr             = 24,
        Module_Csite           = 25,
        Module_Tsec            = 26,
        Module_Mselect         = 27,
        Module_Hda2codec2x     = 28,
        Module_Actmon          = 29,
        Module_I2cSlow         = 30,
        Module_Sor1            = 31,
        Module_Sata            = 32,
        Module_Hda             = 33,
        Module_XusbCoreHostSrc = 34,
        Module_XusbFalconSrc   = 35,
        Module_XusbFsSrc       = 36,
        Module_XusbCoreDevSrc  = 37,
        Module_XusbSsSrc       = 38,
        Module_UartA           = 39,
        Module_UartB           = 40,
        Module_UartC           = 41,
        Module_UartD           = 42,
        Module_Host1x          = 43,
        Module_Entropy         = 44,
        Module_SocTherm        = 45,
        Module_Vic             = 46,
        Module_Nvenc           = 47,
        Module_Nvjpg           = 48,
        Module_Nvdec           = 49,
        Module_Qspi            = 50,
        Module_ViI2c           = 51,
        Module_Tsecb           = 52,
        Module_Ape             = 53,
        Module_AudioDsp        = 54,
        Module_AudioUart       = 55,
        Module_Emc             = 56,
        Module_Plle            = 57,
        Module_PlleHwSeq       = 58,
        Module_Dsi             = 59,
        Module_Maud            = 60,
        Module_Dpaux1          = 61,
        Module_MipiCal         = 62,
        Module_UartFstMipiCal  = 63,
        Module_Osc             = 64,
        Module_SysBus          = 65,
        Module_SorSafe         = 66,
        Module_XusbSs          = 67,
        Module_XusbHost        = 68,
        Module_XusbDevice      = 69,
        Module_Extperiph1      = 70,
        Module_Ahub            = 71,
        Module_Hda2hdmicodec   = 72,
        Module_Gpuaux          = 73,
        Module_UsbD            = 74,
        Module_Usb2            = 75,
        Module_Pcie            = 76,
        Module_Afi             = 77,
        Module_PciExClk        = 78,
        Module_PExUsbPhy       = 79,
        Module_XUsbPadCtl      = 80,
        Module_Apbdma          = 81,
        Module_Usb2TrkClk      = 82,
        Module_XUsbIoPll       = 83,
        Module_XUsbIoPllHwSeq  = 84,
        Module_Cec             = 85,
        Module_Extperiph2      = 86,
    };

}
