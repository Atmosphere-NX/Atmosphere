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
#include <exosphere/br.hpp>
#include <exosphere/pmic.hpp>

namespace ams::fuse {

    enum HardwareType {
        HardwareType_Icosa  = 0,
        HardwareType_Copper = 1,
        HardwareType_Hoag   = 2,
        HardwareType_Iowa   = 3,
        HardwareType_Calcio = 4,
        HardwareType_Five   = 5,

        HardwareType_Undefined = 0xF,
    };

    enum HardwareState {
        HardwareState_Development = 0,
        HardwareState_Production  = 1,
        HardwareState_Undefined   = 2,
    };

    void SetRegisterAddress(uintptr_t address);
    void SetWriteSecureOnly();
    void Lockout();

    u32 GetOdmWord(int index);

    HardwareType    GetHardwareType();
    HardwareState   GetHardwareState();
    pmic::Regulator GetRegulator();
    void GetEcid(br::BootEcid *out);

}