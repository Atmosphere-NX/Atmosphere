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

#pragma once
#include <vapours.hpp>

namespace ams::hos {

    enum Version : u16 {
        Version_Min = 0,
        Version_100 = Version_Min,
        Version_200 = 1,
        Version_300 = 2,
        Version_400 = 3,
        Version_500 = 4,
        Version_600 = 5,
        Version_700 = 6,
        Version_800 = 7,
        Version_810 = 8,
        Version_900 = 9,
        Version_910 = 10,
        Version_Current = Version_910,
        Version_Max = 32,
    };

}
