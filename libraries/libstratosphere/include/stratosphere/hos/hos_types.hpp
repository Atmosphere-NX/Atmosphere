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

    enum Version : u16 {
        Version_Min     = 0,
        Version_1_0_0   = Version_Min,
        Version_2_0_0   = 1,
        Version_3_0_0   = 2,
        Version_4_0_0   = 3,
        Version_5_0_0   = 4,
        Version_6_0_0   = 5,
        Version_7_0_0   = 6,
        Version_8_0_0   = 7,
        Version_8_1_0   = 8,
        Version_9_0_0   = 9,
        Version_9_1_0   = 10,
        Version_10_0_0  = 11,
        Version_Current = Version_10_0_0,
        Version_Max = 32,
    };

}
