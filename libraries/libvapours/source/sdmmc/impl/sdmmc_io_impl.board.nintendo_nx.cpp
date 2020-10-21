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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "sdmmc_sdmmc_controller.board.nintendo_nx.hpp"
#include "sdmmc_io_impl.board.nintendo_nx.hpp"

namespace ams::sdmmc::impl {

    Result SetSdCardVoltageEnabled(bool en) {
        /* TODO */
        AMS_UNUSED(en);
        AMS_ABORT();
    }

    Result SetSdCardVoltageValue(u32 micro_volts) {
        /* TODO */
        AMS_UNUSED(micro_volts);
        AMS_ABORT();
    }

    namespace gpio_impl {

        namespace {

            constexpr inline dd::PhysicalAddress GpioRegistersPhysicalAddress = 0x6000d000;
            constexpr inline size_t GpioRegistersSize = 4_KB;

            enum GpioPadPort {
                GpioPadPort_A  =  0,
                GpioPadPort_B  =  1,
                GpioPadPort_C  =  2,
                GpioPadPort_D  =  3,
                GpioPadPort_E  =  4,
                GpioPadPort_F  =  5,
                GpioPadPort_G  =  6,
                GpioPadPort_H  =  7,
                GpioPadPort_I  =  8,
                GpioPadPort_J  =  9,
                GpioPadPort_K  = 10,
                GpioPadPort_L  = 11,
                GpioPadPort_M  = 12,
                GpioPadPort_N  = 13,
                GpioPadPort_O  = 14,
                GpioPadPort_P  = 15,
                GpioPadPort_Q  = 16,
                GpioPadPort_R  = 17,
                GpioPadPort_S  = 18,
                GpioPadPort_T  = 19,
                GpioPadPort_U  = 20,
                GpioPadPort_V  = 21,
                GpioPadPort_W  = 22,
                GpioPadPort_X  = 23,
                GpioPadPort_Y  = 24,
                GpioPadPort_Z  = 25,
                GpioPadPort_AA = 26,
                GpioPadPort_BB = 27,
                GpioPadPort_CC = 28,
                GpioPadPort_DD = 29,
                GpioPadPort_EE = 30,
                GpioPadPort_FF = 31,
            };

            consteval unsigned int GetInternalGpioPadNumber(GpioPadPort port, unsigned int which) {
                AMS_ASSUME(which < 8);

                return (static_cast<unsigned int>(port) * 8) + which;
            }

            enum InternalGpioPadNumber {
                InternalGpioPadNumber_E4 = GetInternalGpioPadNumber(GpioPadPort_E, 4),
                InternalGpioPadNumber_M0 = GetInternalGpioPadNumber(GpioPadPort_M, 0),
            };

            constexpr int ConvertInternalGpioPadNumberToController(InternalGpioPadNumber number) {
                return (number >> 5);
            }

            constexpr int ConvertInternalGpioPadNumberToPort(InternalGpioPadNumber number) {
                return (number >> 3);
            }

            constexpr int ConvertInternalGpioPadNumberToBitIndex(InternalGpioPadNumber number) {
                return (number & 7);
            }

            constexpr int ConvertPortNumberToOffset(int port_number) {
                return (port_number & 3);
            }

            struct PadNameToInternalPadNumberEntry {
                GpioPadName pad_name;
                InternalGpioPadNumber internal_number;
            };

            constexpr inline const PadNameToInternalPadNumberEntry PadNameToInternalPadNumberTable[] = {
                { GpioPadName_PowSdEn, InternalGpioPadNumber_E4 },
            };

            constexpr InternalGpioPadNumber ConvertPadNameToInternalPadNumber(GpioPadName pad) {
                const PadNameToInternalPadNumberEntry *target = nullptr;
                for (const auto &entry : PadNameToInternalPadNumberTable) {
                    if (entry.pad_name == pad) {
                        target = std::addressof(entry);
                        break;
                    }
                }

                AMS_ABORT_UNLESS(target != nullptr);
                return target->internal_number;
            }

            enum GpioRegisterType {
                GpioRegisterType_GPIO_CNF      = 0,
                GpioRegisterType_GPIO_OE       = 1,
                GpioRegisterType_GPIO_OUT      = 2,
                GpioRegisterType_GPIO_IN       = 3,
                GpioRegisterType_GPIO_INT_STA  = 4,
                GpioRegisterType_GPIO_INT_ENB  = 5,
                GpioRegisterType_GPIO_INT_LVL  = 6,
                GpioRegisterType_GPIO_INT_CLR  = 7,
                GpioRegisterType_GPIO_DB_CTRL  = 8,
                GpioRegisterType_GPIO_DB_CNT   = 9,
            };

            constexpr inline uintptr_t MaskedWriteAddressOffset = 0x80;
            constexpr inline int MaskedWriteBitOffset = 8;

            constexpr uintptr_t GetGpioRegisterAddress(uintptr_t gpio_address, GpioRegisterType reg_type, InternalGpioPadNumber pad_number) {
                const auto controller = ConvertInternalGpioPadNumberToController(pad_number);
                const auto port       = ConvertInternalGpioPadNumberToPort(pad_number);
                const auto offset     = ConvertPortNumberToOffset(port);

                switch (reg_type) {
                    default:
                        return gpio_address + (0x100 * controller) + (0x10 * reg_type) + (0x4 * offset);
                    case GpioRegisterType_GPIO_DB_CTRL:
                        return gpio_address + (0x100 * controller) + (0x10 * GpioRegisterType_GPIO_IN) + (0x4 * offset);
                    case GpioRegisterType_GPIO_DB_CNT:
                        return gpio_address + (0x100 * controller) + MaskedWriteAddressOffset + (0x10 * GpioRegisterType_GPIO_INT_CLR) + (0x4 * offset);
                }
            }

            void SetMaskedBit(uintptr_t pad_address, int index, int value) {
                const uintptr_t mask_address = pad_address + MaskedWriteAddressOffset;

                reg::Write(mask_address, (1u << (MaskedWriteBitOffset + index)) | (static_cast<unsigned int>(value) << index));
            }

            void SetMaskedBits(uintptr_t pad_address, unsigned int mask, unsigned int value) {
                const uintptr_t mask_address = pad_address + MaskedWriteAddressOffset;

                reg::Write(mask_address, (mask << MaskedWriteBitOffset) | (value));
            }

        }

        void OpenSession(GpioPadName pad) {
            /* Convert the pad to an internal number. */
            const auto pad_number = ConvertPadNameToInternalPadNumber(pad);

            /* Get the gpio registers address. */
            const uintptr_t gpio_address = dd::QueryIoMapping(GpioRegistersPhysicalAddress, GpioRegistersSize);

            /* Configure the pad as GPIO by setting the appropriate bit in CNF. */
            const uintptr_t pad_address = GetGpioRegisterAddress(gpio_address, GpioRegisterType_GPIO_CNF, pad_number);
            const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);
            SetMaskedBit(pad_address, pad_index, 1);

            /* Read the pad address to make sure our configuration takes. */
            reg::Read(pad_address);
        }

        void CloseSession(GpioPadName pad) {
            /* Nothing needs to be done here, as the only thing official code does is unbind the interrupt event. */
            AMS_UNUSED(pad);
        }

        void SetDirection(GpioPadName pad, Direction direction) {
            /* Convert the pad to an internal number. */
            const auto pad_number = ConvertPadNameToInternalPadNumber(pad);

            /* Get the gpio registers address. */
            const uintptr_t gpio_address = dd::QueryIoMapping(GpioRegistersPhysicalAddress, GpioRegistersSize);

            /* Configure the pad direction modifying the appropriate bit in OE. */
            const uintptr_t pad_address = GetGpioRegisterAddress(gpio_address, GpioRegisterType_GPIO_OE, pad_number);
            const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);
            SetMaskedBit(pad_address, pad_index, direction);

            /* Read the pad address to make sure our configuration takes. */
            reg::Read(pad_address);
        }

        void SetValue(GpioPadName pad, GpioValue value) {
            /* Convert the pad to an internal number. */
            const auto pad_number = ConvertPadNameToInternalPadNumber(pad);

            /* Get the gpio registers address. */
            const uintptr_t gpio_address = dd::QueryIoMapping(GpioRegistersPhysicalAddress, GpioRegistersSize);

            /* Configure the pad value modifying the appropriate bit in OUT. */
            const uintptr_t pad_address = GetGpioRegisterAddress(gpio_address, GpioRegisterType_GPIO_OUT, pad_number);
            const uintptr_t pad_index   = ConvertInternalGpioPadNumberToBitIndex(pad_number);
            SetMaskedBit(pad_address, pad_index, value);

            /* Read the pad address to make sure our configuration takes. */
            reg::Read(pad_address);
        }

    }

    namespace pinmux_impl {

        namespace {

            constexpr auto Sdmmc1ClkCmdDat03PadNumber = gpio_impl::InternalGpioPadNumber_M0;

            constexpr unsigned int Sdmmc1ClkCmdDat03PadMask     = 0x3F;

            constexpr unsigned int Sdmmc1ClkCmdDat03PadCnfGpio  = 0x3F;
            constexpr unsigned int Sdmmc1ClkCmdDat03PadCnfSfio  = 0x00;

            constexpr unsigned int Sdmmc1ClkCmdDat03PadOutHigh  = 0x3F;
            constexpr unsigned int Sdmmc1ClkCmdDat03PadOutLow   = 0x00;

            constexpr unsigned int Sdmmc1ClkCmdDat03PadOeOutput = 0x3F;
            constexpr unsigned int Sdmmc1ClkCmdDat03PadOeInput  = 0x00;

            struct PinmuxDefinition {
                u32 reg_offset;
                u32 mask_val;
                u32 pm_val;
            };

            /* NOTE: We only use the SDMMC1 pins, which are conveniently the first few... */
            constexpr const PinmuxDefinition PinmuxDefinitionMap[] = {
                {0x00003000, 0x72FF, 0x01},   /* Sdmmc1Clk */
                {0x00003004, 0x72FF, 0x02},   /* Sdmmc1Cmd */
                {0x00003008, 0x72FF, 0x02},   /* Sdmmc1Dat3 */
                {0x0000300C, 0x72FF, 0x02},   /* Sdmmc1Dat2 */
                {0x00003010, 0x72FF, 0x02},   /* Sdmmc1Dat1 */
                {0x00003014, 0x72FF, 0x01},   /* Sdmmc1Dat0 */
            };

            enum PinmuxPadIndex {
                PinmuxPadIndex_Sdmmc1Clk  = 0,
                PinmuxPadIndex_Sdmmc1Cmd  = 1,
                PinmuxPadIndex_Sdmmc1Dat3 = 2,
                PinmuxPadIndex_Sdmmc1Dat2 = 3,
                PinmuxPadIndex_Sdmmc1Dat1 = 4,
                PinmuxPadIndex_Sdmmc1Dat0 = 5,

                PinmuxPadIndex_Count,
            };

            static_assert(util::size(PinmuxDefinitionMap) == PinmuxPadIndex_Count);

            consteval const PinmuxDefinition GetDefinition(PinmuxPadIndex pad_index) {
                AMS_ABORT_UNLESS(pad_index < PinmuxPadIndex_Count);

                return PinmuxDefinitionMap[pad_index];
            }

            template<PinmuxPadIndex PadIndex, u32 PinmuxConfigVal, u32 PinmuxConfigMaskVal>
            ALWAYS_INLINE u32 UpdatePinmuxPad(uintptr_t pinmux_base_vaddr) {
                constexpr const PinmuxDefinition Definition = GetDefinition(PadIndex);

                /* Fetch this PINMUX's register offset */
                constexpr u32 PinmuxRegOffset = Definition.reg_offset;

                /* Fetch this PINMUX's mask value */
                constexpr u32 PinmuxMaskVal = Definition.mask_val;

                /* Get current register ptr. */
                const uintptr_t pinmux_reg = pinmux_base_vaddr + PinmuxRegOffset;

                /* Read from the PINMUX register */
                u32 pinmux_val = reg::Read(pinmux_reg);

                /* This PINMUX register is locked */
                AMS_ABORT_UNLESS((pinmux_val & 0x80) == 0);

                constexpr u32 PmVal = (PinmuxConfigVal & 0x07);

                /* Adjust PM */
                if constexpr (PinmuxConfigMaskVal & 0x07) {
                    /* Apply additional changes first */
                    if constexpr (PmVal == 0x05) {
                        /* This pin supports PUPD change */
                        if constexpr (PinmuxMaskVal & 0x0C) {
                            /* Change PUPD */
                            if ((pinmux_val & 0x0C) != 0x04) {
                                pinmux_val &= 0xFFFFFFF3;
                                pinmux_val |= 0x04;
                            }
                        }

                        /* This pin supports Tristate change */
                        if constexpr (PinmuxMaskVal & 0x10) {
                            /* Change Tristate */
                            if (!(pinmux_val & 0x10)) {
                                pinmux_val |= 0x10;
                            }
                        }

                        /* This pin supports EInput change */
                        if constexpr (PinmuxMaskVal & 0x40) {
                            /* Change EInput */
                            if (pinmux_val & 0x40) {
                                pinmux_val &= 0xFFFFFFBF;
                            }
                        }
                    }

                    /* Translate PM value if necessary */
                    constexpr u32 TranslatedPmVal = (PmVal == 0x04 || PmVal == 0x05 || PmVal >= 0x06) ? Definition.pm_val : PmVal;

                    /* This pin supports PM change */
                    if constexpr (PinmuxMaskVal & 0x03) {
                        /* Change PM */
                        if ((pinmux_val & 0x03) != (TranslatedPmVal & 0x03)) {
                            pinmux_val &= 0xFFFFFFFC;
                            pinmux_val |= (TranslatedPmVal & 0x03);
                        }
                    }
                }

                constexpr u32 PupdConfigVal = (PinmuxConfigVal & 0x18);

                /* Adjust PUPD */
                if constexpr (PinmuxConfigMaskVal & 0x18) {
                    if constexpr (PupdConfigVal < 0x11) {
                        /* This pin supports PUPD change */
                        if constexpr (PinmuxMaskVal & 0x0C) {
                            /* Change PUPD */
                            if (((pinmux_val >> 0x02) & 0x03) != (PupdConfigVal >> 0x03)) {
                                pinmux_val &= 0xFFFFFFF3;
                                pinmux_val |= (PupdConfigVal >> 0x01);
                            }
                        }
                    }
                }

                constexpr u32 EodConfigVal = (PinmuxConfigVal & 0x60);

                /* Adjust EOd field */
                if constexpr (PinmuxConfigMaskVal & 0x60) {
                    if constexpr (EodConfigVal == 0x20) {
                        /* This pin supports Tristate change */
                        if constexpr (PinmuxMaskVal & 0x10) {
                            /* Change Tristate */
                            if (!(pinmux_val & 0x10)) {
                                pinmux_val |= 0x10;
                            }
                        }

                        /* This pin supports EInput change */
                        if constexpr (PinmuxMaskVal & 0x40) {
                            /* Change EInput */
                            if (!(pinmux_val & 0x40)) {
                                pinmux_val |= 0x40;
                            }
                        }

                        /* This pin supports EOd change */
                        if constexpr (PinmuxMaskVal & 0x800) {
                            /* Change EOd */
                            if (pinmux_val & 0x800) {
                                pinmux_val &= 0xFFFFF7FF;
                            }
                        }
                    } else if constexpr (EodConfigVal == 0x40) {
                        /* This pin supports Tristate change */
                        if constexpr (PinmuxMaskVal & 0x10) {
                            /* Change Tristate */
                            if (pinmux_val & 0x10) {
                                pinmux_val &= 0xFFFFFFEF;
                            }
                        }

                        /* This pin supports EInput change */
                        if constexpr (PinmuxMaskVal & 0x40) {
                            /* Change EInput */
                            if (!(pinmux_val & 0x40)) {
                                pinmux_val |= 0x40;
                            }
                        }

                        /* This pin supports EOd change */
                        if constexpr (PinmuxMaskVal & 0x800) {
                            /* Change EOd */
                            if (pinmux_val & 0x800) {
                                pinmux_val &= 0xFFFFF7FF;
                            }
                        }
                    } else if constexpr (EodConfigVal == 0x60) {
                        /* This pin supports Tristate change */
                        if constexpr (PinmuxMaskVal & 0x10) {
                            /* Change Tristate */
                            if (pinmux_val & 0x10) {
                                pinmux_val &= 0xFFFFFFEF;
                            }
                        }

                        /* This pin supports EInput change */
                        if constexpr (PinmuxMaskVal & 0x40) {
                            /* Change EInput */
                            if (!(pinmux_val & 0x40)) {
                                pinmux_val |= 0x40;
                            }
                        }

                        /* This pin supports EOd change */
                        if constexpr (PinmuxMaskVal & 0x800) {
                            /* Change EOd */
                            if (!(pinmux_val & 0x800)) {
                                pinmux_val |= 0x800;
                            }
                        }
                    } else {
                        /* This pin supports Tristate change */
                        if constexpr (PinmuxMaskVal & 0x10) {
                            /* Change Tristate */
                            if (pinmux_val & 0x10) {
                                pinmux_val &= 0xFFFFFFEF;
                            }
                        }

                        /* This pin supports EInput change */
                        if constexpr (PinmuxMaskVal & 0x40) {
                            /* Change EInput */
                            if (pinmux_val & 0x40) {
                                pinmux_val &= 0xFFFFFFBF;
                            }
                        }

                        /* This pin supports EOd change */
                        if constexpr (PinmuxMaskVal & 0x800) {
                            /* Change EOd */
                            if (pinmux_val & 0x800) {
                                pinmux_val &= 0xFFFFF7FF;
                            }
                        }
                    }
                }

                constexpr u32 LockConfigVal = (PinmuxConfigVal & 0x80);

                /* Adjust Lock */
                if constexpr (PinmuxConfigMaskVal & 0x80) {
                    /* This pin supports Lock change */
                    if constexpr (PinmuxMaskVal & 0x80) {
                        /* Change Lock */
                        if ((pinmux_val ^ PinmuxConfigVal) & 0x80) {
                            pinmux_val &= 0xFFFFFF7F;
                            pinmux_val |= LockConfigVal;
                        }
                    }
                }

                constexpr u32 IoResetConfigVal = (((PinmuxConfigVal >> 0x08) & 0x1) << 0x10);

                /* Adjust IoReset */
                if constexpr (PinmuxConfigMaskVal & 0x100) {
                    /* This pin supports IoReset change */
                    if constexpr (PinmuxMaskVal & 0x10000) {
                        /* Change IoReset */
                        if (((pinmux_val >> 0x10) ^ (PinmuxConfigVal >> 0x08)) & 0x01) {
                            pinmux_val &= 0xFFFEFFFF;
                            pinmux_val |= IoResetConfigVal;
                        }
                    }
                }

                constexpr u32 ParkConfigVal = (((PinmuxConfigVal >> 0x0A) & 0x1) << 0x5);

                /* Adjust Park */
                if constexpr (PinmuxConfigMaskVal & 0x400) {
                    /* This pin supports Park change */
                    if constexpr (PinmuxMaskVal & 0x20) {
                        /* Change Park */
                        if (((pinmux_val >> 0x05) ^ (PinmuxConfigVal >> 0x0A)) & 0x01) {
                            pinmux_val &= 0xFFFFFFDF;
                            pinmux_val |= ParkConfigVal;
                        }
                    }
                }

                constexpr u32 ElpdrConfigVal = (((PinmuxConfigVal >> 0x0B) & 0x1) << 0x08);

                /* Adjust ELpdr */
                if constexpr (PinmuxConfigMaskVal & 0x800) {
                    /* This pin supports ELpdr change */
                    if constexpr (PinmuxMaskVal & 0x100) {
                        /* Change ELpdr */
                        if (((pinmux_val >> 0x08) ^ (PinmuxConfigVal >> 0x0B)) & 0x01) {
                            pinmux_val &= 0xFFFFFEFF;
                            pinmux_val |= ElpdrConfigVal;
                        }
                    }
                }

                constexpr u32 EhsmConfigVal = (((PinmuxConfigVal >> 0x0C) & 0x1) << 0x09);

                /* Adjust EHsm */
                if constexpr (PinmuxConfigMaskVal & 0x1000) {
                    /* This pin supports EHsm change */
                    if constexpr (PinmuxMaskVal & 0x200) {
                        /* Change EHsm */
                        if (((pinmux_val >> 0x09) ^ (PinmuxConfigVal >> 0x0C)) & 0x01) {
                            pinmux_val &= 0xFFFFFDFF;
                            pinmux_val |= EhsmConfigVal;
                        }
                    }
                }

                constexpr u32 EIoHvConfigVal = (((PinmuxConfigVal >> 0x09) & 0x1) << 0x0A);

                /* Adjust EIoHv */
                if constexpr (PinmuxConfigMaskVal & 0x200) {
                    /* This pin supports EIoHv change */
                    if constexpr (PinmuxMaskVal & 0x400) {
                        /* Change EIoHv */
                        if (((pinmux_val >> 0x0A) ^ (PinmuxConfigVal >> 0x09)) & 0x01) {
                            pinmux_val &= 0xFFFFFBFF;
                            pinmux_val |= EIoHvConfigVal;
                        }
                    }
                }

                constexpr u32 EschmtConfigVal = (((PinmuxConfigVal >> 0x0D) & 0x1) << 0x0C);

                /* Adjust ESchmt */
                if constexpr (PinmuxConfigMaskVal & 0x2000) {
                    /* This pin supports ESchmt change */
                    if constexpr (PinmuxMaskVal & 0x1000) {
                        /* Change ESchmt */
                        if (((pinmux_val >> 0x0C) ^ (PinmuxConfigVal >> 0x0D)) & 0x01) {
                            pinmux_val &= 0xFFFFEFFF;
                            pinmux_val |= EschmtConfigVal;
                        }
                    }
                }

                constexpr u32 PreempConfigVal = (((PinmuxConfigVal >> 0x10) & 0x1) << 0xF);

                /* Adjust Preemp */
                if constexpr (PinmuxConfigMaskVal & 0x10000) {
                    /* This pin supports Preemp change */
                    if constexpr (PinmuxMaskVal & 0x8000) {
                        /* Change Preemp */
                        if (((pinmux_val >> 0x0F) ^ (PinmuxConfigVal >> 0x10)) & 0x01) {
                            pinmux_val &= 0xFFFF7FFF;
                            pinmux_val |= PreempConfigVal;
                        }
                    }
                }

                constexpr u32 DrvTypeConfigVal = (((PinmuxConfigVal >> 0x0E) & 0x3) << 0xD);

                /* Adjust DrvType */
                if constexpr (PinmuxConfigMaskVal & 0xC000) {
                    /* This pin supports DrvType change */
                    if constexpr (PinmuxMaskVal & 0x6000) {
                        /* Change DrvType */
                        if (((pinmux_val >> 0x0D) ^ (PinmuxConfigVal >> 0x0E)) & 0x03) {
                            pinmux_val &= 0xFFFF9FFF;
                            pinmux_val |= DrvTypeConfigVal;
                        }
                    }
                }

                /* Write to the appropriate PINMUX register */
                reg::Write(pinmux_reg, pinmux_val);

                /* Do a dummy read from the PINMUX register */
                pinmux_val = reg::Read(pinmux_reg);

                return pinmux_val;
            }

        }

        void SetPinAssignment(PinAssignment assignment) {
            /* Get the apb registers address. */
            const uintptr_t apb_address = dd::QueryIoMapping(ApbMiscRegistersPhysicalAddress, ApbMiscRegistersSize);
            AMS_UNUSED(apb_address);

            /* Set the pin assignment. */
            switch (assignment) {
                case PinAssignment_Sdmmc1OutputHigh:
                    {
                        /* Clear Sdmmc1Clk pulldown. */
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Clk, 0, 0x18>(apb_address);

                        /* Get the gpio registers address. */
                        const uintptr_t gpio_address = dd::QueryIoMapping(gpio_impl::GpioRegistersPhysicalAddress, gpio_impl::GpioRegistersSize);

                        /* Configure GPIO M0-5 (SDMMC1 CLK + CMD + DAT0/1/2/3) as gpio. */
                        const uintptr_t cnf_address = gpio_impl::GetGpioRegisterAddress(gpio_address, gpio_impl::GpioRegisterType_GPIO_CNF, Sdmmc1ClkCmdDat03PadNumber);
                        gpio_impl::SetMaskedBits(cnf_address, Sdmmc1ClkCmdDat03PadMask, Sdmmc1ClkCmdDat03PadCnfGpio);

                        /* Configure GPIO M0-5 (SDMMC1 CLK + CMD + DAT0/1/2/3) as high. */
                        const uintptr_t out_address = gpio_impl::GetGpioRegisterAddress(gpio_address, gpio_impl::GpioRegisterType_GPIO_OUT, Sdmmc1ClkCmdDat03PadNumber);
                        gpio_impl::SetMaskedBits(out_address, Sdmmc1ClkCmdDat03PadMask, Sdmmc1ClkCmdDat03PadOutHigh);

                        /* Configure GPIO M0-5 (SDMMC1 CLK + CMD + DAT0/1/2/3) as output. */
                        const uintptr_t oe_address = gpio_impl::GetGpioRegisterAddress(gpio_address, gpio_impl::GpioRegisterType_GPIO_OE, Sdmmc1ClkCmdDat03PadNumber);
                        gpio_impl::SetMaskedBits(oe_address, Sdmmc1ClkCmdDat03PadMask, Sdmmc1ClkCmdDat03PadOeOutput);

                        /* Read to be sure that our configuration takes. */
                        reg::Read(oe_address);
                    }
                    break;
                case PinAssignment_Sdmmc1ResetState:
                    {
                        /* Get the gpio registers address. */
                        const uintptr_t gpio_address = dd::QueryIoMapping(gpio_impl::GpioRegistersPhysicalAddress, gpio_impl::GpioRegistersSize);

                        /* Configure GPIO M0-5 (SDMMC1 CLK + CMD + DAT0/1/2/3) as sfio. */
                        const uintptr_t cnf_address = gpio_impl::GetGpioRegisterAddress(gpio_address, gpio_impl::GpioRegisterType_GPIO_CNF, Sdmmc1ClkCmdDat03PadNumber);
                        gpio_impl::SetMaskedBits(cnf_address, Sdmmc1ClkCmdDat03PadMask, Sdmmc1ClkCmdDat03PadCnfSfio);

                        /* Configure GPIO M0-5 (SDMMC1 CLK + CMD + DAT0/1/2/3) as low. */
                        const uintptr_t out_address = gpio_impl::GetGpioRegisterAddress(gpio_address, gpio_impl::GpioRegisterType_GPIO_OUT, Sdmmc1ClkCmdDat03PadNumber);
                        gpio_impl::SetMaskedBits(out_address, Sdmmc1ClkCmdDat03PadMask, Sdmmc1ClkCmdDat03PadOutLow);

                        /* Configure GPIO M0-5 (SDMMC1 CLK + CMD + DAT0/1/2/3) as input. */
                        const uintptr_t oe_address = gpio_impl::GetGpioRegisterAddress(gpio_address, gpio_impl::GpioRegisterType_GPIO_OE, Sdmmc1ClkCmdDat03PadNumber);
                        gpio_impl::SetMaskedBits(oe_address, Sdmmc1ClkCmdDat03PadMask, Sdmmc1ClkCmdDat03PadOeInput);

                        /* Read to be sure that our configuration takes. */
                        reg::Read(oe_address);

                        /* Set Sdmmc1Clk pulldown. */
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Clk, 0x8, 0x18>(apb_address);
                    }
                    break;
                case PinAssignment_Sdmmc1SchmtEnable:
                    {
                        /* Set Schmitt enable for all pins in the group. */
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Clk,  0x2000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Cmd,  0x2000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Dat3, 0x2000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Dat2, 0x2000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Dat1, 0x2000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Dat0, 0x2000, 0x2000>(apb_address);
                    }
                    break;
                case PinAssignment_Sdmmc1SchmtDisable:
                    {
                        /* Set Schmitt disable for all pins in the group. */
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Clk,  0x0000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Cmd,  0x0000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Dat3, 0x0000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Dat2, 0x0000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Dat1, 0x0000, 0x2000>(apb_address);
                        UpdatePinmuxPad<PinmuxPadIndex_Sdmmc1Dat0, 0x0000, 0x2000>(apb_address);
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

    }

}
