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
#include "sdmmc_io_impl.board.nintendo_nx.hpp"

namespace ams::sdmmc::impl {

    Result SetSdCardVoltageEnabled(bool en) {
        /* TODO */
    }

    Result SetSdCardVoltageValue(u32 micro_volts) {
        /* TODO */
    }

    namespace pinmux_impl {

        void SetPinAssignment(PinAssignment assignment) {
            /* TODO */
        }

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
                for (const auto &entry : PadNameToInternalPadNumberTable) {
                    if (entry.pad_name == pad) {
                        return entry.internal_number;
                    }
                }

                AMS_ASSUME(false);
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

            void SetMaskedBit(uintptr_t gpio_address, int index, int value) {
                const uintptr_t mask_address = gpio_address + MaskedWriteAddressOffset;

                reg::Write(mask_address, (1u << (MaskedWriteBitOffset + index)) | (static_cast<unsigned int>(value) << index));
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

}
