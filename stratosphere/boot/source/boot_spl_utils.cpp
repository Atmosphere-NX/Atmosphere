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

#include "boot_spl_utils.hpp"

namespace sts::spl {

    spl::HardwareType GetHardwareType() {
        u64 out_val = 0;
        R_ASSERT(splGetConfig(SplConfigItem_HardwareType, &out_val));
        return static_cast<spl::HardwareType>(out_val);
    }

    bool IsRecoveryBoot() {
        u64 val = 0;
        R_ASSERT(splGetConfig(SplConfigItem_IsRecoveryBoot, &val));
        return val != 0;
    }

    bool IsMariko() {
        const auto hw_type = GetHardwareType();
        switch (hw_type) {
            case spl::HardwareType::Icosa:
            case spl::HardwareType::Copper:
                return false;
            case spl::HardwareType::Hoag:
            case spl::HardwareType::Iowa:
                return true;
            default:
                std::abort();
        }
    }

}
