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

namespace ams::i2c {

    enum TransactionOption : u32 {
        TransactionOption_StartCondition = (1u <<  0),
        TransactionOption_StopCondition  = (1u <<  1),
        TransactionOption_MaxBits        = (1u << 30),
    };

    enum AddressingMode : u32 {
        AddressingMode_SevenBit = 0,
    };

    enum SpeedMode : u32 {
        SpeedMode_Standard  = 100000,
        SpeedMode_Fast      = 400000,
        SpeedMode_FastPlus  = 1000000,
        SpeedMode_HighSpeed = 3400000,
    };

    using I2cCommand = u8;

}
