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

namespace ams::gpio {

    enum InterruptMode : u32 {
        InterruptMode_LowLevel    = 0,
        InterruptMode_HighLevel   = 1,
        InterruptMode_RisingEdge  = 2,
        InterruptMode_FallingEdge = 3,
        InterruptMode_AnyEdge     = 4,
    };

    enum Direction : u32 {
        Direction_Input  = 0,
        Direction_Output = 1,
    };

    enum GpioValue : u32 {
        GpioValue_Low  = 0,
        GpioValue_High = 1,
    };

    enum InterruptStatus : u32 {
        InterruptStatus_Inactive = 0,
        InterruptStatus_Active   = 1,
    };

    enum WakePinDebugMode {
        WakePinDebugMode_AutoImmediateWake = 1,
        WakePinDebugMode_NoWake            = 2,
    };

    using WakeBitFlag = util::BitFlagSet<128>;

}
