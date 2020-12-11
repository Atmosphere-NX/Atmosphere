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

namespace ams::hos {

    enum Version : u32 {
        Version_Min     = ::ams::TargetFirmware_Min,

        Version_1_0_0   = ::ams::TargetFirmware_1_0_0,
        Version_2_0_0   = ::ams::TargetFirmware_2_0_0,
        Version_2_1_0   = ::ams::TargetFirmware_2_1_0,
        Version_2_2_0   = ::ams::TargetFirmware_2_2_0,
        Version_2_3_0   = ::ams::TargetFirmware_2_3_0,
        Version_3_0_0   = ::ams::TargetFirmware_3_0_0,
        Version_3_0_1   = ::ams::TargetFirmware_3_0_1,
        Version_3_0_2   = ::ams::TargetFirmware_3_0_2,
        Version_4_0_0   = ::ams::TargetFirmware_4_0_0,
        Version_4_0_1   = ::ams::TargetFirmware_4_0_1,
        Version_4_1_0   = ::ams::TargetFirmware_4_1_0,
        Version_5_0_0   = ::ams::TargetFirmware_5_0_0,
        Version_5_0_1   = ::ams::TargetFirmware_5_0_1,
        Version_5_0_2   = ::ams::TargetFirmware_5_0_2,
        Version_5_1_0   = ::ams::TargetFirmware_5_1_0,
        Version_6_0_0   = ::ams::TargetFirmware_6_0_0,
        Version_6_0_1   = ::ams::TargetFirmware_6_0_1,
        Version_6_1_0   = ::ams::TargetFirmware_6_1_0,
        Version_6_2_0   = ::ams::TargetFirmware_6_2_0,
        Version_7_0_0   = ::ams::TargetFirmware_7_0_0,
        Version_7_0_1   = ::ams::TargetFirmware_7_0_1,
        Version_8_0_0   = ::ams::TargetFirmware_8_0_0,
        Version_8_0_1   = ::ams::TargetFirmware_8_0_1,
        Version_8_1_0   = ::ams::TargetFirmware_8_1_0,
        Version_8_1_1   = ::ams::TargetFirmware_8_1_1,
        Version_9_0_0   = ::ams::TargetFirmware_9_0_0,
        Version_9_0_1   = ::ams::TargetFirmware_9_0_1,
        Version_9_1_0   = ::ams::TargetFirmware_9_1_0,
        Version_9_2_0   = ::ams::TargetFirmware_9_2_0,
        Version_10_0_0  = ::ams::TargetFirmware_10_0_0,
        Version_10_0_1  = ::ams::TargetFirmware_10_0_1,
        Version_10_0_2  = ::ams::TargetFirmware_10_0_2,
        Version_10_0_3  = ::ams::TargetFirmware_10_0_3,
        Version_10_0_4  = ::ams::TargetFirmware_10_0_4,
        Version_10_1_0  = ::ams::TargetFirmware_10_1_0,
        Version_10_2_0  = ::ams::TargetFirmware_10_2_0,
        Version_11_0_0  = ::ams::TargetFirmware_11_0_0,
        Version_11_0_1  = ::ams::TargetFirmware_11_0_1,

        Version_Current = ::ams::TargetFirmware_Current,

        Version_Max     = ::ams::TargetFirmware_Max,
    };

}
