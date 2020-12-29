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
#include <stratosphere.hpp>
#include "pinmux_pad_index.hpp"
#include "pinmux_board_driver_api.hpp"
#include "pinmux_platform_pads.hpp"

namespace ams::pinmux::driver::board::nintendo::nx {

    namespace {

        uintptr_t g_apb_misc_virtual_address = dd::QueryIoMapping(0x70000000, 0x4000);

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

        enum PinmuxDrivePadMask : u32{
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

        struct PinmuxPadCharacter {
            u32 reg_offset;
            u32 reg_mask;
            u8  safe_func;
            const char *pad_name;
        };

        struct PinmuxDrivePadCharacter {
            u32 reg_offset;
            u32 reg_mask;
            const char *pad_name;
        };

        #include "pinmux_pad_characters.inc"
        #include "pinmux_drive_pad_characters.inc"

        class PinmuxPad {
            private:
                u32 reg_address;
                u32 reg_mask;
                u32 reg_value;
                u8  safe_func;
                const char *pad_name;
            private:
                bool IsValidRegisterAddress() const {
                    return this->reg_address - 0x70003000 <= 0x2C4;
                }

                uintptr_t GetRegisterAddress() const {
                    return g_apb_misc_virtual_address + (this->reg_address - 0x70000000);
                }

                bool UpdateBits(u32 value, u32 offset, u32 mask) {
                    if ((this->reg_mask & mask) != 0) {
                        if ((value & (mask >> offset)) != ((this->reg_value & mask) >> offset)) {
                            this->reg_value = (this->reg_value & ~mask) | ((value << offset) & mask);
                        }
                        return true;
                    } else {
                        return false;
                    }
                }

                u32 ReadReg() const {
                    if (this->IsValidRegisterAddress()) {
                        return reg::Read(this->GetRegisterAddress());
                    } else {
                        return 0;
                    }
                }

                void WriteReg() const {
                    if (this->IsValidRegisterAddress()) {
                        reg::Write(this->GetRegisterAddress(), this->reg_value);
                    }
                }

                bool IsLocked() const {
                    return (this->reg_value & PinmuxPadMask_Lock) != 0;
                }

                void UpdatePm(u8 v) {
                    if (v != 0xFF) {
                        if (v == PinmuxPadPm_Safe) {
                            v = this->safe_func;
                        }
                        this->UpdateBits(v, PinmuxPadBitOffset_Pm, PinmuxPadMask_Pm);
                    }
                }

                void UpdatePupd(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_Pupd, PinmuxPadMask_Pupd);
                    }
                }

                void UpdateTristate(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_Tristate, PinmuxPadMask_Tristate);
                    }
                }

                void UpdateEInput(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_EInput, PinmuxPadMask_EInput);
                    }
                }

                void UpdateEOd(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_EOd, PinmuxPadMask_EOd);
                    }
                }

                void UpdateLock(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_Lock, PinmuxPadMask_Lock);
                    }
                }

                void UpdateIoReset(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_IoReset, PinmuxPadMask_IoReset);
                    }
                }

                void UpdatePark(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_Park, PinmuxPadMask_Park);
                    }
                }

                void UpdateELpdr(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_ELpdr, PinmuxPadMask_ELpdr);
                    }
                }

                void UpdateEHsm(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_EHsm, PinmuxPadMask_EHsm);
                    }
                }

                void UpdateEIoHv(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_EIoHv, PinmuxPadMask_EIoHv);
                    }
                }

                void UpdateESchmt(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_ESchmt, PinmuxPadMask_ESchmt);
                    }
                }

                void UpdatePreemp(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_Preemp, PinmuxPadMask_Preemp);
                    }
                }

                void UpdateDrvType(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxPadBitOffset_DrvType, PinmuxPadMask_DrvType);
                    }
                }
            public:
                constexpr PinmuxPad() : reg_address(), reg_mask(), reg_value(), safe_func(), pad_name() { /* ... */ }

                void UpdatePinmuxPad(u32 config, u32 config_mask) {
                    /* Update register value. */
                    this->reg_value = this->ReadReg();

                    /* Check if we're locked. */
                    if (this->IsLocked()) {
                        return;
                    }

                    /* Update PM. */
                    if ((config_mask & PinmuxOptBitMask_Pm) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Pm);
                        u8 pm = PinmuxPadPm_Safe;
                        if (opt != PinmuxOpt_Gpio) {
                            if (opt == PinmuxOpt_Unused) {
                                this->UpdatePupd(true);
                                this->UpdateTristate(true);
                                this->UpdateEInput(0);
                            } else if (opt <= PinmuxOpt_Unused) {
                                pm = opt >> PinmuxOptBitOffset_Pm;
                            }
                        }
                        this->UpdatePm(pm);
                    }

                    /* Update pupd. */
                    if ((config_mask & PinmuxOptBitMask_Pupd) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Pupd);
                        if (opt == PinmuxOpt_NoPupd || opt == PinmuxOpt_PullDown || opt == PinmuxOpt_PullUp) {
                            this->UpdatePupd(opt >> PinmuxOptBitOffset_Pupd);
                        }
                    }

                    /* Update direction. */
                    if ((config_mask & PinmuxOptBitMask_Dir) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Dir);
                        if (opt == PinmuxOpt_Output) {
                            this->UpdateTristate(false);
                            this->UpdateEInput(false);
                            if ((this->reg_mask & PinmuxPadMask_EOd) != 0) {
                                this->UpdateEOd(false);
                            }
                        } else if (opt == PinmuxOpt_Input) {
                            this->UpdateTristate(true);
                            this->UpdateEInput(true);
                            if ((this->reg_mask & PinmuxPadMask_EOd) != 0) {
                                this->UpdateEOd(false);
                            }
                        } else if (opt == PinmuxOpt_Bidirection) {
                            this->UpdateTristate(false);
                            this->UpdateEInput(true);
                            if ((this->reg_mask & PinmuxPadMask_EOd) != 0) {
                                this->UpdateEOd(false);
                            }
                        } else if (opt == PinmuxOpt_OpenDrain) {
                            this->UpdateTristate(false);
                            this->UpdateEInput(true);
                            this->UpdateEOd(true);
                        }
                    }

                    /* Update Lock. */
                    if ((config_mask & PinmuxOptBitMask_Lock) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Lock);
                        this->UpdateLock(opt != 0);
                    }

                    /* Update IoReset. */
                    if ((config_mask & PinmuxOptBitMask_IoReset) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_IoReset);
                        this->UpdateIoReset(opt != 0);
                    }

                    /* Update Park. */
                    if ((config_mask & PinmuxOptBitMask_Park) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Park);
                        this->UpdatePark(opt != 0);
                    }

                    /* Update Lpdr. */
                    if ((config_mask & PinmuxOptBitMask_Lpdr) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Lpdr);
                        this->UpdateELpdr(opt != 0);
                    }

                    /* Update Hsm. */
                    if ((config_mask & PinmuxOptBitMask_Hsm) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Hsm);
                        this->UpdateEHsm(opt != 0);
                    }

                    /* Update IoHv. */
                    if ((config_mask & PinmuxOptBitMask_IoHv) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_IoHv);
                        this->UpdateEIoHv(opt != 0);
                    }

                    /* Update Schmt. */
                    if ((config_mask & PinmuxOptBitMask_Schmt) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Schmt);
                        this->UpdateESchmt(opt != 0);
                    }

                    /* Update Preemp. */
                    if ((config_mask & PinmuxOptBitMask_Preemp) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_Preemp);
                        this->UpdatePreemp(opt != 0);
                    }

                    /* Update drive type. */
                    if ((config_mask & PinmuxOptBitMask_DrvType) != 0) {
                        const auto opt = (config & PinmuxOptBitMask_DrvType);
                        this->UpdateDrvType(opt >> PinmuxOptBitOffset_DrvType);
                    }

                    /* Write the updated register value. */
                    this->WriteReg();
                }

                void SetCharacter(const PinmuxPadCharacter &character) {
                    this->reg_address = character.reg_offset + 0x70000000;
                    this->reg_mask    = character.reg_mask;
                    this->safe_func   = character.safe_func;
                    this->reg_value   = this->ReadReg();
                    this->pad_name    = character.pad_name;

                    if ((this->reg_mask & this->reg_value & PinmuxPadMask_Park) != 0) {
                        this->reg_value &= ~(PinmuxPadMask_Park);
                    }

                    this->WriteReg();
                }
        };

        class PinmuxDrivePad {
            private:
                u32 reg_address;
                u32 reg_mask;
                u32 reg_value;
                u8  safe_func;
                const char *pad_name;
            private:
                bool IsValidRegisterAddress() const {
                    return this->reg_address - 0x700008E4 <= 0x288;
                }

                uintptr_t GetRegisterAddress() const {
                    return g_apb_misc_virtual_address + (this->reg_address - 0x70000000);
                }

                bool UpdateBits(u32 value, u32 offset, u32 mask) {
                    if ((this->reg_mask & mask) != 0) {
                        if ((value & (mask >> offset)) != ((this->reg_value & mask) >> offset)) {
                            this->reg_value = (this->reg_value & ~mask) | ((value << offset) & mask);
                        }
                        return true;
                    } else {
                        return false;
                    }
                }

                u32 ReadReg() const {
                    if (this->IsValidRegisterAddress()) {
                        return reg::Read(this->GetRegisterAddress());
                    } else {
                        return 0;
                    }
                }

                void WriteReg() const {
                    if (this->IsValidRegisterAddress()) {
                        reg::Write(this->GetRegisterAddress(), this->reg_value);
                    }
                }

                bool IsCzDrvDn() const {
                    return (this->reg_mask & PinmuxDrivePadMask_CzDrvDn) == PinmuxDrivePadMask_CzDrvDn;
                }

                bool IsCzDrvUp() const {
                    return (this->reg_mask & PinmuxDrivePadMask_CzDrvUp) == PinmuxDrivePadMask_CzDrvUp;
                }

                void UpdateDrvDn(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxDrivePadBitOffset_DrvDn, PinmuxDrivePadMask_DrvDn);
                    }
                }

                void UpdateDrvUp(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxDrivePadBitOffset_DrvUp, PinmuxDrivePadMask_DrvUp);
                    }
                }

                void UpdateCzDrvDn(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxDrivePadBitOffset_CzDrvDn, PinmuxDrivePadMask_CzDrvDn);
                    }
                }

                void UpdateCzDrvUp(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxDrivePadBitOffset_CzDrvUp, PinmuxDrivePadMask_CzDrvUp);
                    }
                }

                void UpdateSlwR(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxDrivePadBitOffset_SlwR, PinmuxDrivePadMask_SlwR);
                    }
                }

                void UpdateSlwF(u8 v) {
                    if (v != 0xFF) {
                        this->UpdateBits(v, PinmuxDrivePadBitOffset_SlwF, PinmuxDrivePadMask_SlwF);
                    }
                }
            public:
                constexpr PinmuxDrivePad() : reg_address(), reg_mask(), reg_value(), pad_name() { /* ... */ }

                void UpdatePinmuxDrivePad(u32 config, u32 config_mask) {
                    /* Update register value. */
                    this->reg_value = this->ReadReg();

                    /* Update drvdn. */
                    if ((config_mask & PinmuxDriveOptBitMask_DrvDn) != 0) {
                        if (this->IsCzDrvDn()) {
                            const auto opt = (config & PinmuxDriveOptBitMask_CzDrvDn);
                            this->UpdateCzDrvDn(opt >> PinmuxDriveOptBitOffset_CzDrvDn);
                        } else {
                            const auto opt = (config & PinmuxDriveOptBitMask_DrvDn);
                            this->UpdateDrvDn(opt >> PinmuxDriveOptBitOffset_DrvDn);
                        }
                    }

                    /* Update drvup. */
                    if ((config_mask & PinmuxDriveOptBitMask_DrvUp) != 0) {
                        if (this->IsCzDrvUp()) {
                            const auto opt = (config & PinmuxDriveOptBitMask_CzDrvUp);
                            this->UpdateCzDrvUp(opt >> PinmuxDriveOptBitOffset_CzDrvUp);
                        } else {
                            const auto opt = (config & PinmuxDriveOptBitMask_DrvUp);
                            this->UpdateDrvUp(opt >> PinmuxDriveOptBitOffset_DrvUp);
                        }
                    }

                    /* Update slwr */
                    if ((config_mask & PinmuxDriveOptBitMask_SlwR) != 0) {
                        const auto opt = (config & PinmuxDriveOptBitMask_SlwR);
                        this->UpdateSlwR(opt >> PinmuxDriveOptBitOffset_SlwR);
                    }

                    /* Update slwf */
                    if ((config_mask & PinmuxDriveOptBitMask_SlwR) != 0) {
                        const auto opt = (config & PinmuxDriveOptBitMask_SlwF);
                        this->UpdateSlwF(opt >> PinmuxDriveOptBitOffset_SlwF);
                    }

                    /* Write the updated register value. */
                    this->WriteReg();
                }

                void SetCharacter(const PinmuxDrivePadCharacter &character) {
                    this->reg_address = character.reg_offset + 0x70000000;
                    this->reg_mask    = character.reg_mask;
                    this->reg_value   = this->ReadReg();
                    this->pad_name    = character.pad_name;
                }
        };

        constinit std::array<PinmuxPad, NumPinmuxPadCharacters> g_pinmux_pads{};
        constinit std::array<PinmuxDrivePad, NumPinmuxDrivePadCharacters> g_pinmux_drive_pads{};

    }

    void InitializePlatformPads() {
        /* Initialize all pads. */
        for (size_t i = 0; i < NumPinmuxPadCharacters; ++i) {
            g_pinmux_pads[i].SetCharacter(PinmuxPadCharacters[i]);
        }

        /* Update all drive pads. */
        for (size_t i = 0; i < NumPinmuxDrivePadCharacters; ++i) {
            g_pinmux_drive_pads[i].SetCharacter(PinmuxDrivePadCharacters[i]);
        }
    }

    void UpdateSinglePinmuxPad(const PinmuxPadConfig &config) {
        if (IsInitialized()) {
            g_pinmux_pads[config.index].UpdatePinmuxPad(config.option, config.option_mask);
        }
    }

    void UpdateSinglePinmuxDrivePad(const PinmuxDrivePadConfig &config) {
        if (IsInitialized()) {
            g_pinmux_drive_pads[config.index].UpdatePinmuxDrivePad(config.option, config.option_mask);
        }
    }

}
