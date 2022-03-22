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
#include <vapours.hpp>
#include <stratosphere/settings/settings_types.hpp>

namespace ams::settings::system {

    enum RegionCode {
        RegionCode_Japan               = 0,
        RegionCode_Usa                 = 1,
        RegionCode_Europe              = 2,
        RegionCode_Australia           = 3,
        RegionCode_HongKongTaiwanKorea = 4,
        RegionCode_China               = 5,
    };

    void GetRegionCode(RegionCode *out);

}
