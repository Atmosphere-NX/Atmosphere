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
#include <exosphere.hpp>

namespace ams::charger {

    namespace {

        /* https://www.ti.com/lit/ds/symlink/bq24193.pdf */
        constexpr inline int I2cAddressBq24193   = 0x6B;

        constexpr inline int Bq24193RegisterInputSourceControl = 0x00;

        /* 8.5.1.1 EN_HIZ */
        enum EnHiZ : u8 {
            EnHiZ_Disable = (0u << 7),
            EnHiZ_Enable  = (1u << 7),

            EnHiZ_Mask    = (1u << 7),
        };

    }

    bool IsHiZMode() {
        return (i2c::QueryByte(i2c::Port_1, I2cAddressBq24193, Bq24193RegisterInputSourceControl) & EnHiZ_Mask) == EnHiZ_Enable;
    }

    void EnterHiZMode() {
        u8 ctrl = i2c::QueryByte(i2c::Port_1, I2cAddressBq24193, Bq24193RegisterInputSourceControl);
        ctrl &= ~EnHiZ_Mask;
        ctrl |= EnHiZ_Enable;
        i2c::SendByte(i2c::Port_1, I2cAddressBq24193, Bq24193RegisterInputSourceControl, ctrl);
    }

    void ExitHiZMode() {
        u8 ctrl = i2c::QueryByte(i2c::Port_1, I2cAddressBq24193, Bq24193RegisterInputSourceControl);
        ctrl &= ~EnHiZ_Mask;
        ctrl |= EnHiZ_Disable;
        i2c::SendByte(i2c::Port_1, I2cAddressBq24193, Bq24193RegisterInputSourceControl, ctrl);
    }

}
