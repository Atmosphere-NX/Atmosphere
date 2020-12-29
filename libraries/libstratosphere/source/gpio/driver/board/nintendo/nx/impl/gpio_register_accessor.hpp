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
#include <stratosphere.hpp>

#include "gpio_tegra_pad.hpp"

namespace ams::gpio::driver::board::nintendo_nx::impl {

    constexpr inline dd::PhysicalAddress GpioRegistersPhysicalAddress = 0x6000D000;
    constexpr inline size_t GpioRegistersSize                         = 4_KB;

    constexpr inline int GpioPerControllerBitWidth = 5;
    constexpr inline int GpioControllerBitWidth    = 3;

    constexpr inline int PortPerControllerBitWidth = 2;
    constexpr inline int PortPerController         = (1 << PortPerControllerBitWidth);

    constexpr inline int GpioPerPortBitWidth       = 3;
    constexpr inline int GpioPerPort               = (1 << GpioPerPortBitWidth);

    static_assert(PortPerControllerBitWidth + GpioPerPortBitWidth == GpioPerControllerBitWidth);
    static_assert(PortPerController * GpioPerPort == (1 << GpioPerControllerBitWidth));

    constexpr int ConvertInternalGpioPadNumberToController(InternalGpioPadNumber number) {
        return (number >> GpioPerControllerBitWidth);
    }

    constexpr int ConvertInternalGpioPadNumberToPort(InternalGpioPadNumber number) {
        return (number >> GpioControllerBitWidth);
    }

    constexpr InternalGpioPadNumber ConvertPortToInternalGpioPadNumber(int port) {
        return static_cast<InternalGpioPadNumber>(port << GpioControllerBitWidth);
    }

    constexpr int ConvertInternalGpioPadNumberToBitIndex(InternalGpioPadNumber number) {
        return (number & (GpioPerPort - 1));
    }

    constexpr int ConvertPortNumberToOffset(int port_number) {
        return (port_number & (PortPerController - 1));
    }

    enum GpioController {
        GpioController_1     = 0,
        GpioController_2     = 1,
        GpioController_3     = 2,
        GpioController_4     = 3,
        GpioController_5     = 4,
        GpioController_6     = 5,
        GpioController_7     = 6,
        GpioController_8     = 7,
        GpioController_Count = 8,
    };
    static_assert(GpioController_Count == static_cast<GpioController>(1 << GpioControllerBitWidth));

    constexpr inline os::InterruptName InterruptNameTable[GpioController_Count] = {
         64, /* GpioController_1 */
         65, /* GpioController_2 */
         66, /* GpioController_3 */
         67, /* GpioController_4 */
         87, /* GpioController_5 */
        119, /* GpioController_6 */
        121, /* GpioController_7 */
        157, /* GpioController_8 */
    };

    enum InternalInterruptMode {
        InternalInterruptMode_LowLevel    = 0x000000,
        InternalInterruptMode_HighLevel   = 0x000001,
        InternalInterruptMode_RisingEdge  = 0x000101,
        InternalInterruptMode_FallingEdge = 0x000100,
        InternalInterruptMode_AnyEdge     = 0x010100,

        InternalInterruptMode_Mask        = 0x010101,
    };

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

    constexpr inline uintptr_t GetGpioRegisterAddress(uintptr_t gpio_address, GpioRegisterType reg_type, InternalGpioPadNumber pad_number) {
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

    inline void SetMaskedBit(uintptr_t pad_address, int index, int value) {
        const uintptr_t mask_address = pad_address + MaskedWriteAddressOffset;

        reg::Write(mask_address, (1u << (MaskedWriteBitOffset + index)) | (static_cast<unsigned int>(value) << index));
    }

    inline void SetMaskedBits(uintptr_t pad_address, unsigned int mask, unsigned int value) {
        const uintptr_t mask_address = pad_address + MaskedWriteAddressOffset;

        reg::Write(mask_address, (mask << MaskedWriteBitOffset) | (value));
    }

}
