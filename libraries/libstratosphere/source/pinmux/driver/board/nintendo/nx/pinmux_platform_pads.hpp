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
#pragma once
#include <stratosphere.hpp>

namespace ams::pinmux::driver::board::nintendo::nx {

    struct PinmuxPadConfig {
        u32 index;
        u32 option;
        u32 option_mask;
    };

    struct PinmuxDrivePadConfig {
        u32 index;
        u32 option;
        u32 option_mask;
    };

    enum PinmuxPadMask : u32 {
        PinmuxPadMask_Pm        = 0x3,
        PinmuxPadMask_Pupd      = 0xC,
        PinmuxPadMask_Tristate  = 0x10,
        PinmuxPadMask_Park      = 0x20,
        PinmuxPadMask_EInput    = 0x40,
        PinmuxPadMask_Lock      = 0x80,
        PinmuxPadMask_ELpdr     = 0x100,
        PinmuxPadMask_EHsm      = 0x200,
        PinmuxPadMask_EIoHv     = 0x400,
        PinmuxPadMask_EOd       = 0x800,
        PinmuxPadMask_ESchmt    = 0x1000,
        PinmuxPadMask_DrvType   = 0x6000,
        PinmuxPadMask_Preemp    = 0x8000,
        PinmuxPadMask_IoReset   = 0x10000,
    };

    enum PinmuxPadBitOffset : u32 {
        PinmuxPadBitOffset_Pm       = 0x0,
        PinmuxPadBitOffset_Pupd     = 0x2,
        PinmuxPadBitOffset_Tristate = 0x4,
        PinmuxPadBitOffset_Park     = 0x5,
        PinmuxPadBitOffset_EInput   = 0x6,
        PinmuxPadBitOffset_Lock     = 0x7,
        PinmuxPadBitOffset_ELpdr    = 0x8,
        PinmuxPadBitOffset_EHsm     = 0x9,
        PinmuxPadBitOffset_EIoHv    = 0xA,
        PinmuxPadBitOffset_EOd      = 0xB,
        PinmuxPadBitOffset_ESchmt   = 0xC,
        PinmuxPadBitOffset_DrvType  = 0xD,
        PinmuxPadBitOffset_Preemp   = 0xF,
        PinmuxPadBitOffset_IoReset  = 0x10,
    };

    enum PinmuxOptBitMask : u32 {
        PinmuxOptBitMask_Pm         = 0x7,
        PinmuxOptBitMask_Pupd       = 0x18,
        PinmuxOptBitMask_Dir        = 0x60,
        PinmuxOptBitMask_Lock       = 0x80,
        PinmuxOptBitMask_IoReset    = 0x100,
        PinmuxOptBitMask_IoHv       = 0x200,
        PinmuxOptBitMask_Park       = 0x400,
        PinmuxOptBitMask_Lpdr       = 0x800,
        PinmuxOptBitMask_Hsm        = 0x1000,
        PinmuxOptBitMask_Schmt      = 0x2000,
        PinmuxOptBitMask_DrvType    = 0xC000,
        PinmuxOptBitMask_Preemp     = 0x10000,
    };

    enum PinmuxOptBitOffset {
        PinmuxOptBitOffset_Pm       = 0x0,
        PinmuxOptBitOffset_Pupd     = 0x3,
        PinmuxOptBitOffset_Dir      = 0x5,
        PinmuxOptBitOffset_Lock     = 0x7,
        PinmuxOptBitOffset_IoReset  = 0x8,
        PinmuxOptBitOffset_IoHv     = 0x9,
        PinmuxOptBitOffset_Park     = 0xA,
        PinmuxOptBitOffset_Lpdr     = 0xB,
        PinmuxOptBitOffset_Hsm      = 0xC,
        PinmuxOptBitOffset_Schmt    = 0xD,
        PinmuxOptBitOffset_DrvType  = 0xE,
        PinmuxOptBitOffset_Preemp   = 0x10,
    };

    enum PinmuxDrivePadMask : u32 {
        PinmuxDrivePadMask_DrvDn   = 0x0001F000,
        PinmuxDrivePadMask_DrvUp   = 0x01F00000,
        PinmuxDrivePadMask_CzDrvDn = 0x0007F000,
        PinmuxDrivePadMask_CzDrvUp = 0x07F00000,
        PinmuxDrivePadMask_SlwR    = 0x30000000,
        PinmuxDrivePadMask_SlwF    = 0xC0000000,
    };

    enum PinmuxDrivePadBitOffset : u32 {
        PinmuxDrivePadBitOffset_DrvDn   = 12,
        PinmuxDrivePadBitOffset_DrvUp   = 20,
        PinmuxDrivePadBitOffset_CzDrvDn = 12,
        PinmuxDrivePadBitOffset_CzDrvUp = 20,
        PinmuxDrivePadBitOffset_SlwR    = 28,
        PinmuxDrivePadBitOffset_SlwF    = 30,
    };

    enum PinmuxDriveOptBitMask : u32 {
        PinmuxDriveOptBitMask_DrvDn   = 0x0001F000,
        PinmuxDriveOptBitMask_DrvUp   = 0x01F00000,
        PinmuxDriveOptBitMask_CzDrvDn = 0x0007F000,
        PinmuxDriveOptBitMask_CzDrvUp = 0x07F00000,
        PinmuxDriveOptBitMask_SlwR    = 0x30000000,
        PinmuxDriveOptBitMask_SlwF    = 0xC0000000,
    };

    enum PinmuxDriveOptBitOffset : u32 {
        PinmuxDriveOptBitOffset_DrvDn   = 12,
        PinmuxDriveOptBitOffset_DrvUp   = 20,
        PinmuxDriveOptBitOffset_CzDrvDn = 12,
        PinmuxDriveOptBitOffset_CzDrvUp = 20,
        PinmuxDriveOptBitOffset_SlwR    = 28,
        PinmuxDriveOptBitOffset_SlwF    = 30,
    };

    enum PinmuxOpt : u32 {
        /* Pm */
        PinmuxOpt_Gpio   = 0x4,
        PinmuxOpt_Unused = 0x5,

        /* Pupd */
        PinmuxOpt_NoPupd    = 0x0,
        PinmuxOpt_PullDown  = 0x8,
        PinmuxOpt_PullUp    = 0x10,

        /* Dir */
        PinmuxOpt_Output        = 0x0,
        PinmuxOpt_Input         = 0x20,
        PinmuxOpt_Bidirection   = 0x40,
        PinmuxOpt_OpenDrain     = 0x60,

        /* Lock */
        PinmuxOpt_Unlock    = 0x0,
        PinmuxOpt_Lock      = 0x80,

        /* IoReset */
        PinmuxOpt_DisableIoReset    = 0x0,
        PinmuxOpt_EnableIoReset     = 0x100,

        /* IoHv */
        PinmuxOpt_NormalVoltage = 0x0,
        PinmuxOpt_HighVoltage   = 0x200,

        /* Park */
        PinmuxOpt_ResetOnLowPower   = 0x0,
        PinmuxOpt_ParkOnLowPower    = 0x400,

        /* Lpdr */
        PinmuxOpt_DisableBaseDriver = 0x0,
        PinmuxOpt_EnableBaseDriver  = 0x800,

        /* Hsm */
        PinmuxOpt_DisableHighSpeedMode = 0x0,
        PinmuxOpt_EnableHighSpeedMode   = 0x1000,

        /* Schmt */
        PinmuxOpt_CmosMode          = 0x0,
        PinmuxOpt_SchmittTrigger    = 0x2000,

        /* DrvType */
        PinmuxOpt_DrvType1X = 0x0,
        PinmuxOpt_DrvType2X = 0x4000,
        PinmuxOpt_DrvType3X = 0x8000,
        PinmuxOpt_DrvType4X = 0xC000,

        /* Preemp */
        PinmuxOpt_DisablePreemp = 0x0,
        PinmuxOpt_EnablePreemp  = 0x10000,
    };

    enum PinmuxPadPm : u32 {
        PinmuxPadPm_Default = 0xFFFFFFFF,
        PinmuxPadPm_Pm0     = 0x0,
        PinmuxPadPm_Pm1     = 0x1,
        PinmuxPadPm_Pm2     = 0x2,
        PinmuxPadPm_Pm3     = 0x3,
        PinmuxPadPm_Safe    = 0x4,
    };

    void InitializePlatformPads();

    void UpdateSinglePinmuxPad(const PinmuxPadConfig &config);
    void UpdateSinglePinmuxDrivePad(const PinmuxDrivePadConfig &config);

}
