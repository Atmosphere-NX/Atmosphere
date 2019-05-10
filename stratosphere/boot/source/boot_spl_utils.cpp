/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include "boot_functions.hpp"

HardwareType Boot::GetHardwareType() {
    u64 out_val = 0;
    if (R_FAILED(splGetConfig(SplConfigItem_HardwareType, &out_val))) {
        std::abort();
    }
    return static_cast<HardwareType>(out_val);
}

bool Boot::IsRecoveryBoot() {
    u64 val = 0;
    if (R_FAILED(splGetConfig(SplConfigItem_IsRecoveryBoot, &val))) {
        std::abort();
    }
    return val != 0;
}

bool Boot::IsMariko() {
    HardwareType hw_type = GetHardwareType();
    switch (hw_type) {
        case HardwareType_Icosa:
        case HardwareType_Copper:
            return false;
        case HardwareType_Hoag:
        case HardwareType_Iowa:
            return true;
        default:
            std::abort();
    }
}