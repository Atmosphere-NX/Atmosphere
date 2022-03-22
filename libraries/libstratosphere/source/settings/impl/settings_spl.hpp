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

namespace ams::settings::impl {

    enum SplHardwareType : u8 {
        SplHardwareType_None        = 0x0,
        SplHardwareType_Icosa       = 0x1,
        SplHardwareType_IcosaMariko = 0x2,
        SplHardwareType_Copper      = 0x3,
        SplHardwareType_Hoag        = 0x4,
        SplHardwareType_Calcio      = 0x5,
        SplHardwareType_Aula        = 0x6,
    };

    bool IsSplDevelopment();
    SplHardwareType GetSplHardwareType();
    bool IsSplRetailInteractiveDisplayStateEnabled();
    u64 GetSplDeviceIdLow();

}
